/*
 * tex_pool.c -- reuse libplacebo textures from frame to frame
 * Copyright (C) 2026 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "tex_pool.h"

#include "gpu_context.h"
#include "idle_trim.h"

#include <framework/mlt_log.h>

#include <stdint.h>
#include <stdlib.h>

/* Idle textures beyond the byte limit are destroyed, oldest first. Ones idle for
   longer than placebo_idle_ms() are destroyed by the idle thread. */
#define MAX_IDLE_BYTES ((size_t) 768 << 20)

/* Marks a pool_entry, so a texture whose user data is some other pointer, which
   the contract forbids, is more likely to be recognized than read as an entry.
   Cleared when an entry is freed. It narrows the risk; it can't make a stray
   user data pointer safe to read. */
#define ENTRY_MAGIC UINT64_C(0x706c6163506f6f6c)

/* One per texture the pool created. The texture's user data points back at it,
   so a texture finds its entry without a search and the pool can tell its own
   textures from any other. Nothing else may set user data on a texture. */
typedef struct
{
    uint64_t magic;
    pl_tex tex;
    struct pl_tex_params key; /* as requested, not as libplacebo adjusted it */
    size_t size;
    int64_t used; /* placebo_now_ms() when it was last acquired or returned */
    int busy;
    int orphan; /* the pool let go of it while it was in use */
    VkSemaphore sem;
    uint64_t sem_value;
} pool_entry;

/* Guarded by the render lock. */
static pool_entry **s_entries;
static int s_count;
static int s_capacity;

static int same_params(const struct pl_tex_params *a, const struct pl_tex_params *b)
{
    return a->w == b->w && a->h == b->h && a->d == b->d && a->format == b->format
           && a->sampleable == b->sampleable && a->renderable == b->renderable
           && a->storable == b->storable && a->blit_src == b->blit_src && a->blit_dst == b->blit_dst
           && a->host_writable == b->host_writable && a->host_readable == b->host_readable;
}

/* Whether the parameters ask for more than a reusable texture can give: a
   texture created with data or a handle is not interchangeable with an idle one. */
static int needs_own_texture(const struct pl_tex_params *params)
{
    return params->initial_data || params->import_handle || params->export_handle;
}

static size_t texture_size(const struct pl_tex_params *params)
{
    size_t depth = params->d ? params->d : 1;
    size_t height = params->h ? params->h : 1;
    return (size_t) params->w * height * depth * (params->format ? params->format->texel_size : 4);
}

/* The entry of a texture this pool created, or NULL for any other texture. */
static pool_entry *entry_of(pl_tex tex)
{
    pool_entry *entry = tex ? tex->params.user_data : NULL;

    return entry && entry->magic == ENTRY_MAGIC && entry->tex == tex ? entry : NULL;
}

static void free_entry(pool_entry *entry)
{
    entry->magic = 0;
    free(entry);
}

/* Make room in the table for one more entry. */
static int reserve_entry(void)
{
    if (s_count < s_capacity)
        return 1;

    int capacity = s_capacity ? s_capacity * 2 : 32;
    pool_entry **grown = realloc(s_entries, capacity * sizeof(*grown));
    if (!grown)
        return 0;
    s_entries = grown;
    s_capacity = capacity;
    return 1;
}

/* The semaphore is destroyed after the texture, as it was before pooling. */
static void remove_entry(pl_gpu gpu, int index)
{
    pool_entry *entry = s_entries[index];

    pl_tex_destroy(gpu, &entry->tex);
    if (entry->sem)
        pl_vulkan_sem_destroy(gpu, &entry->sem);
    free_entry(entry);
    s_entries[index] = s_entries[--s_count];
}

/* Destroy the idle textures that have not been used for too long. */
static int expire(pl_gpu gpu)
{
    int64_t now = placebo_now_ms();
    int released = 0;

    for (int i = 0; i < s_count;) {
        if (!s_entries[i]->busy && now - s_entries[i]->used > placebo_idle_ms()) {
            remove_entry(gpu, i);
            released++;
        } else {
            i++;
        }
    }
    return released;
}

/* Destroy the oldest idle textures while they hold more than MAX_IDLE_BYTES. */
static int enforce_cap(pl_gpu gpu)
{
    size_t idle_bytes = 0;
    int released = 0;

    for (int i = 0; i < s_count; i++) {
        if (!s_entries[i]->busy)
            idle_bytes += s_entries[i]->size;
    }
    while (idle_bytes > MAX_IDLE_BYTES) {
        int oldest = -1;
        for (int i = 0; i < s_count; i++) {
            if (!s_entries[i]->busy && (oldest < 0 || s_entries[i]->used < s_entries[oldest]->used))
                oldest = i;
        }
        if (oldest < 0)
            break;
        idle_bytes -= s_entries[oldest]->size;
        remove_entry(gpu, oldest);
        released++;
    }
    return released;
}

/* Run by the idle thread. */
static int trim(pl_gpu gpu)
{
    return expire(gpu) + enforce_cap(gpu);
}

pl_tex placebo_tex_acquire(pl_gpu gpu, const struct pl_tex_params *params)
{
    if (needs_own_texture(params))
        return pl_tex_create(gpu, params); /* destroyed on recycle, like any unpooled texture */

    for (int i = 0; i < s_count; i++) {
        pool_entry *entry = s_entries[i];
        if (!entry->busy && same_params(&entry->key, params)) {
            entry->busy = 1;
            entry->used = placebo_now_ms();
            return entry->tex;
        }
    }

    pool_entry *entry = reserve_entry() ? calloc(1, sizeof(*entry)) : NULL;
    if (!entry) /* an unpooled texture still works, it is destroyed on recycle */
        return pl_tex_create(gpu, params);

    struct pl_tex_params created = *params;
    created.user_data = entry;
    pl_tex tex = pl_tex_create(gpu, &created);
    if (!tex) {
        free_entry(entry);
        return NULL;
    }
    entry->magic = ENTRY_MAGIC;
    entry->tex = tex;
    entry->key = *params;
    entry->key.user_data = NULL;
    entry->size = texture_size(params);
    entry->used = placebo_now_ms();
    entry->busy = 1;
    s_entries[s_count++] = entry;
    placebo_idle_trim_start(trim);
    return tex;
}

pl_tex placebo_tex_acquire_work(pl_gpu gpu, int w, int h, pl_fmt fmt, int use)
{
    return placebo_tex_acquire(gpu,
                               pl_tex_params(.w = w,
                                             .h = h,
                                             .format = fmt,
                                             .sampleable = true,
                                             .renderable = true,
                                             .storable = !!(use & PLACEBO_TEX_STORABLE),
                                             .host_readable = !!(use & PLACEBO_TEX_HOST_READABLE),
                                             .blit_dst = !!(use & PLACEBO_TEX_BLIT_DST), ));
}

/* The transfer destination usage is always set, so every output shares one
   class of idle texture whether or not its filter clears it. */
pl_tex placebo_tex_acquire_target(pl_gpu gpu, int w, int h, pl_fmt fmt)
{
    return placebo_tex_acquire_work(gpu, w, h, fmt, PLACEBO_TEX_HOST_READABLE | PLACEBO_TEX_BLIT_DST);
}

void placebo_tex_recycle(pl_gpu gpu, pl_tex *tex)
{
    pool_entry *entry = entry_of(tex ? *tex : NULL);

    if (!tex || !*tex)
        return;
    if (entry && entry->orphan) {
        /* The GPU it belonged to is gone, and with it the need for the entry. */
        free_entry(entry);
        *tex = NULL;
        return;
    }
    if (!gpu)
        return;
    if (!entry) {
        pl_tex_destroy(gpu, tex);
        return;
    }
    entry->busy = 0;
    entry->used = placebo_now_ms();
    *tex = NULL;
    enforce_cap(gpu);
}

/* A timeline semaphore's current value, read through functions resolved once per
   device. Returns 0 when they can't be resolved, so the recorded value stands.
   The caller holds the render lock, which guards the statics. */
static uint64_t completed_value(pl_gpu gpu, VkSemaphore sem)
{
    static VkDevice s_device;
    static PFN_vkGetSemaphoreCounterValue s_get_value;
    pl_vulkan vk = pl_vulkan_get(gpu);
    uint64_t value = 0;

    if (!vk || !vk->get_proc_addr)
        return 0;
    if (vk->device != s_device) {
        PFN_vkGetDeviceProcAddr get_device
            = (PFN_vkGetDeviceProcAddr) vk->get_proc_addr(vk->instance, "vkGetDeviceProcAddr");

        s_get_value = get_device ? (PFN_vkGetSemaphoreCounterValue)
                              get_device(vk->device, "vkGetSemaphoreCounterValue")
                                 : NULL;
        s_device = vk->device;
        if (!s_get_value)
            mlt_log_warning(NULL, "[placebo] vkGetSemaphoreCounterValue is unavailable\n");
    }
    if (s_get_value)
        s_get_value(vk->device, sem, &value);
    return value;
}

VkSemaphore placebo_tex_semaphore(pl_gpu gpu, pl_tex tex, uint64_t *value)
{
    pool_entry *entry = entry_of(tex);

    if (!entry || entry->orphan) {
        mlt_log_error(NULL, "[placebo] Only a texture from the pool can be shared\n");
        return VK_NULL_HANDLE;
    }
    if (!entry->sem) {
        entry->sem = pl_vulkan_sem_create(gpu,
                                          pl_vulkan_sem_params(.type = VK_SEMAPHORE_TYPE_TIMELINE,
                                                               .initial_value = 0, ));
        entry->sem_value = 0;
    } else {
        uint64_t completed = completed_value(gpu, entry->sem);
        if (completed > entry->sem_value)
            entry->sem_value = completed;
    }
    if (value)
        *value = entry->sem_value;
    return entry->sem;
}

void placebo_tex_set_semaphore_value(pl_tex tex, uint64_t value)
{
    pool_entry *entry = entry_of(tex);

    if (entry)
        entry->sem_value = value;
}

void placebo_tex_pool_drain(pl_gpu gpu)
{
    while (s_count > 0) {
        if (s_entries[s_count - 1]->busy) {
            /* Its owner destroys the texture. The entry stays valid for that. */
            s_entries[s_count - 1]->orphan = 1;
            s_count--;
        } else {
            remove_entry(gpu, s_count - 1);
        }
    }
    free(s_entries);
    s_entries = NULL;
    s_capacity = 0;
}
