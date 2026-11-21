/*
 * display_export.c -- render an mlt_image_vulkan into an exported image
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

#include "display_export.h"

#include <libplacebo/config.h>

/* The exported picture is scRGB, which libplacebo has since API 362. An older
   one has no transfer that keeps both 1.0 at 80 nits and the HDR range, so it
   publishes no export and the display uses its own path. */
#if !defined(PL_HAVE_VULKAN) || PL_API_VER < 362

const mlt_vulkan_export_api *placebo_display_export_api(void)
{
    return NULL;
}

void placebo_display_export_close(void) {}

#else

#ifdef _WIN32
/* windows.h before vulkan_win32.h: that header uses HANDLE. */
#include <windows.h>
#endif

#include "gpu_context.h"
#include "vulkan_frame.h"
#include "vulkan_publish.h"

#include <framework/mlt_log.h>

#include <libplacebo/renderer.h>
#include <libplacebo/shaders/colorspace.h>
#include <libplacebo/tone_mapping.h>
#include <libplacebo/vulkan.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#else
#include <unistd.h>
#endif

/* The placebo VkImage is not the one an importing window samples. Each slot is
   an RGBA16F image on the placebo device, created exclusive to the graphics
   queue and exported. pl_render_image writes linear scRGB into it. hold_ex then
   releases ownership to VK_QUEUE_FAMILY_EXTERNAL and signals one internal
   binary semaphore. A second submit fans that out to each attached side's
   ready semaphore, because hold can signal only one. The next use of the slot
   waits the done semaphores the sides signaled, then release_ex acquires
   the image back. Nothing here waits on the CPU. A slot whose ready has not
   been seen, or whose last submit is still queued, is left alone. */

static const VkImageUsageFlags k_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                         | VK_IMAGE_USAGE_SAMPLED_BIT
                                         | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                         | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

#ifdef _WIN32
static const enum pl_handle_type k_export = PL_HANDLE_WIN32;
static const VkExternalMemoryHandleTypeFlagBits k_handle_bit
    = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
static const enum pl_handle_type k_export = PL_HANDLE_FD;
static const VkExternalMemoryHandleTypeFlagBits k_handle_bit
    = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

struct exp_sem
{
    VkSemaphore sem;
    union pl_handle handle;
    int is_export;
};

struct exp_side
{
    struct exp_sem ready;
    struct exp_sem done;
    int armed;
};

struct exp_slot
{
    VkImage image;
    VkDeviceMemory memory;
    union pl_handle memory_handle;
    uint64_t memory_size;
    int has_memory;
    pl_tex tex;
    VkFence fence;
    int width;
    int height;
    int held;
    int exported;
    int generation;
    struct exp_side side[MLT_VULKAN_EXPORT_SIDES];
    struct exp_sem internal_ready;
    struct exp_sem internal_done;
};

struct vk_procs
{
    VkDevice device;
    PFN_vkCreateImage create_image;
    PFN_vkDestroyImage destroy_image;
    PFN_vkGetImageMemoryRequirements image_reqs;
    PFN_vkAllocateMemory alloc_memory;
    PFN_vkFreeMemory free_memory;
    PFN_vkBindImageMemory bind_image;
    PFN_vkCreateFence create_fence;
    PFN_vkDestroyFence destroy_fence;
    PFN_vkResetFences reset_fences;
    PFN_vkGetFenceStatus fence_status;
    PFN_vkQueueSubmit queue_submit;
    VkQueue graphics_queue;
    PFN_vkGetPhysicalDeviceMemoryProperties mem_props;
#ifdef _WIN32
    PFN_vkGetMemoryWin32HandleKHR get_win32;
#else
    PFN_vkGetMemoryFdKHR get_fd;
#endif
};

static struct vk_procs s_procs;
static struct exp_slot s_slots[MLT_VULKAN_EXPORT_SLOTS];
static pl_gpu s_gpu;
static int s_next;
static int s_have_desc;
static mlt_vulkan_export_desc s_last;

/* What the importers have reported. The importers' threads change it while
   the render thread reads it, so it has a lock of its own and does not wait on
   the render lock, which is held across GPU work. */
static pthread_mutex_t s_sides_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct
{
    int attached;
    int seen[MLT_VULKAN_EXPORT_SLOTS];
} s_sides[MLT_VULKAN_EXPORT_SIDES];

/* The side state and the caller's parameters, taken once per render so that
   every decision in it sees the same picture of the importers. */
struct exp_request
{
    struct
    {
        int attached;
        int seen[MLT_VULKAN_EXPORT_SLOTS];
    } side[MLT_VULKAN_EXPORT_SIDES];
    mlt_vulkan_export_params params;
};

/* What happened when a slot was asked to render. A busy slot is not an error:
   an importer has not finished with it, or its last submit is still queued. */
enum slot_result {
    SLOT_RENDERED,
    SLOT_BUSY,
    SLOT_FAILED,
};

/* Every way an export can fail, with the text logged for it. */
#define EXPORT_ERRORS(X) \
    X(NO_VISIBLE_PIXELS, "Vulkan frame has no visible pixels") \
    X(DEVICE_NOT_CURRENT, "Vulkan device is not current") \
    X(PROCS_MISSING, "Vulkan export procedures are missing") \
    X(NO_SEMAPHORE_EXPORT, "this Vulkan device cannot export semaphores") \
    X(NO_RENDERER, "libplacebo renderer is missing") \
    X(CREATE_IMAGE, "vkCreateImage for the exported picture failed") \
    X(NO_MEMORY_TYPE, "no memory type for the exported picture") \
    X(ALLOCATE_MEMORY, "vkAllocateMemory for the exported picture failed") \
    X(BIND_MEMORY, "vkBindImageMemory for the exported picture failed") \
    X(MEMORY_HANDLE_WIN32, "vkGetMemoryWin32HandleKHR failed") \
    X(MEMORY_HANDLE_FD, "vkGetMemoryFdKHR failed") \
    X(WRAP_IMAGE, "pl_vulkan_wrap of the exported picture failed") \
    X(EXPORT_SEMAPHORES, "binary semaphore export failed") \
    X(CREATE_FENCE, "vkCreateFence failed") \
    X(COMBINE_SEMAPHORES, "previous preview semaphores could not be combined") \
    X(HOLD_IMAGE, "pl_vulkan_hold_ex failed") \
    X(SIGNAL_READY, "ready semaphore submit failed") \
    X(RESIZE_FENCE, "resize fence failed") \
    X(MAP_FRAME, "Vulkan frame map failed") \
    X(RENDER_IMAGE, "pl_render_image into the exported picture failed")

enum export_error {
#define X(id, text) ERR_##id,
    EXPORT_ERRORS(X)
#undef X
        ERR_COUNT
};

static const char *const k_error_text[ERR_COUNT] = {
#define X(id, text) text,
    EXPORT_ERRORS(X)
#undef X
};

/* Each error is logged once, until a render succeeds. A failure that repeats
   every frame does not flood the log, errors that alternate do not either, and
   one that comes back after a good picture is reported again. */
static unsigned s_logged_errors;

static int fail(enum export_error error)
{
    if (!(s_logged_errors & (1u << error))) {
        s_logged_errors |= 1u << error;
        mlt_log_warning(NULL, "[placebo] Vulkan display export: %s\n", k_error_text[error]);
    }
    return 1;
}

static int load_procs(pl_vulkan vk)
{
    PFN_vkGetInstanceProcAddr get;

    if (!vk || !vk->device || !vk->get_proc_addr)
        return 0;
    if (s_procs.device == vk->device && s_procs.create_image)
        return 1;
    memset(&s_procs, 0, sizeof(s_procs));
    get = vk->get_proc_addr;
#define LOAD(field, type, name) \
    s_procs.field = (type) get(vk->instance, name); \
    if (!s_procs.field) \
        goto missing;
    LOAD(create_image, PFN_vkCreateImage, "vkCreateImage")
    LOAD(destroy_image, PFN_vkDestroyImage, "vkDestroyImage")
    LOAD(image_reqs, PFN_vkGetImageMemoryRequirements, "vkGetImageMemoryRequirements")
    LOAD(alloc_memory, PFN_vkAllocateMemory, "vkAllocateMemory")
    LOAD(free_memory, PFN_vkFreeMemory, "vkFreeMemory")
    LOAD(bind_image, PFN_vkBindImageMemory, "vkBindImageMemory")
    LOAD(create_fence, PFN_vkCreateFence, "vkCreateFence")
    LOAD(destroy_fence, PFN_vkDestroyFence, "vkDestroyFence")
    LOAD(reset_fences, PFN_vkResetFences, "vkResetFences")
    LOAD(fence_status, PFN_vkGetFenceStatus, "vkGetFenceStatus")
    LOAD(queue_submit, PFN_vkQueueSubmit, "vkQueueSubmit")
    LOAD(mem_props, PFN_vkGetPhysicalDeviceMemoryProperties, "vkGetPhysicalDeviceMemoryProperties")
#ifdef _WIN32
    LOAD(get_win32, PFN_vkGetMemoryWin32HandleKHR, "vkGetMemoryWin32HandleKHR")
#else
    LOAD(get_fd, PFN_vkGetMemoryFdKHR, "vkGetMemoryFdKHR")
#endif
#undef LOAD
    s_procs.graphics_queue = placebo_vulkan_get_queue(vk, vk->queue_graphics.index, 0);
    s_procs.device = vk->device;
    return 1;
missing:
    memset(&s_procs, 0, sizeof(s_procs));
    return 0;
}

static int submit(pl_vulkan vk,
                  const VkSemaphore *waits,
                  int nwaits,
                  const VkSemaphore *signals,
                  int nsignals,
                  VkFence fence)
{
    VkPipelineStageFlags stages[MLT_VULKAN_EXPORT_SIDES];
    VkSubmitInfo info;
    VkResult result;
    int i;
    uint32_t family;

    if (!vk || !vk->lock_queue || !vk->unlock_queue || nwaits < 0
        || nwaits > MLT_VULKAN_EXPORT_SIDES || nsignals < 0 || nsignals > MLT_VULKAN_EXPORT_SIDES
        || !s_procs.queue_submit || !s_procs.graphics_queue)
        return 0;
    family = vk->queue_graphics.index;
    for (i = 0; i < nwaits; i++)
        stages[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    memset(&info, 0, sizeof(info));
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = (uint32_t) nwaits;
    info.pWaitSemaphores = nwaits ? waits : NULL;
    info.pWaitDstStageMask = nwaits ? stages : NULL;
    info.signalSemaphoreCount = (uint32_t) nsignals;
    info.pSignalSemaphores = nsignals ? signals : NULL;

    vk->lock_queue(vk, family, 0);
    result = s_procs.queue_submit(s_procs.graphics_queue, 1, &info, fence);
    vk->unlock_queue(vk, family, 0);
    return result == VK_SUCCESS;
}

static void close_export_handle(union pl_handle *handle, int is_export)
{
    if (!is_export)
        return;
#ifdef _WIN32
    if (handle->handle)
        CloseHandle((HANDLE) handle->handle);
#else
    if (handle->fd >= 0)
        close(handle->fd);
#endif
    memset(handle, 0, sizeof(*handle));
}

static void destroy_sem(pl_gpu gpu, struct exp_sem *sem)
{
    if (!sem)
        return;
    close_export_handle(&sem->handle, sem->is_export);
    sem->is_export = 0;
    if (sem->sem && gpu)
        pl_vulkan_sem_destroy(gpu, &sem->sem);
    sem->sem = VK_NULL_HANDLE;
}

static int create_sem(pl_gpu gpu, struct exp_sem *sem, int export)
{
    memset(sem, 0, sizeof(*sem));
#ifndef _WIN32
    sem->handle.fd = -1;
#endif
    if (export) {
        sem->sem = pl_vulkan_sem_create(gpu,
                                        pl_vulkan_sem_params(.type = VK_SEMAPHORE_TYPE_BINARY,
                                                             .export_handle = k_export,
                                                             .out_handle = &sem->handle, ));
        sem->is_export = sem->sem != VK_NULL_HANDLE;
    } else {
        sem->sem = pl_vulkan_sem_create(gpu,
                                        pl_vulkan_sem_params(.type = VK_SEMAPHORE_TYPE_BINARY, ));
    }
    if (!sem->sem)
        return 0;
#ifdef _WIN32
    if (export && !sem->handle.handle) {
        pl_vulkan_sem_destroy(gpu, &sem->sem);
        sem->is_export = 0;
        return 0;
    }
#else
    if (export && sem->handle.fd < 0) {
        pl_vulkan_sem_destroy(gpu, &sem->sem);
        sem->is_export = 0;
        return 0;
    }
#endif
    return 1;
}

static void destroy_image(pl_gpu gpu, pl_vulkan vk, struct exp_slot *slot)
{
    if (slot->tex && gpu)
        pl_tex_destroy(gpu, &slot->tex);
    slot->tex = NULL;
    slot->held = 0;
    slot->exported = 0;
    if (slot->image && s_procs.destroy_image && vk)
        s_procs.destroy_image(vk->device, slot->image, NULL);
    slot->image = VK_NULL_HANDLE;
    if (slot->memory && s_procs.free_memory && vk)
        s_procs.free_memory(vk->device, slot->memory, NULL);
    slot->memory = VK_NULL_HANDLE;
    if (slot->has_memory)
        close_export_handle(&slot->memory_handle, 1);
    slot->has_memory = 0;
    slot->memory_size = 0;
    slot->width = 0;
    slot->height = 0;
}

static int memory_type_index(pl_vulkan vk, uint32_t bits)
{
    VkPhysicalDeviceMemoryProperties props;
    uint32_t i;

    s_procs.mem_props(vk->phys_device, &props);
    for (i = 0; i < props.memoryTypeCount; i++) {
        if ((bits & (1u << i))
            && (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            return (int) i;
    }
    for (i = 0; i < props.memoryTypeCount; i++) {
        if (bits & (1u << i))
            return (int) i;
    }
    return -1;
}

static int create_image(pl_gpu gpu, pl_vulkan vk, struct exp_slot *slot, int width, int height)
{
    VkExternalMemoryImageCreateInfo external;
    VkImageCreateInfo image_info;
    VkMemoryRequirements reqs;
    VkMemoryDedicatedAllocateInfo dedicated;
    VkExportMemoryAllocateInfo export_info;
    VkMemoryAllocateInfo alloc;
    VkResult result;
    int type;

    memset(&external, 0, sizeof(external));
    external.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    external.handleTypes = k_handle_bit;
    memset(&image_info, 0, sizeof(image_info));
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = &external;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    image_info.extent.width = (uint32_t) width;
    image_info.extent.height = (uint32_t) height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = k_usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    result = s_procs.create_image(vk->device, &image_info, NULL, &slot->image);
    if (result != VK_SUCCESS)
        return fail(ERR_CREATE_IMAGE);

    s_procs.image_reqs(vk->device, slot->image, &reqs);
    type = memory_type_index(vk, reqs.memoryTypeBits);
    if (type < 0) {
        destroy_image(gpu, vk, slot);
        return fail(ERR_NO_MEMORY_TYPE);
    }
    memset(&dedicated, 0, sizeof(dedicated));
    dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    dedicated.image = slot->image;
    memset(&export_info, 0, sizeof(export_info));
    export_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    export_info.pNext = &dedicated;
    export_info.handleTypes = k_handle_bit;
    memset(&alloc, 0, sizeof(alloc));
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.pNext = &export_info;
    alloc.allocationSize = reqs.size;
    slot->memory_size = reqs.size;
    alloc.memoryTypeIndex = (uint32_t) type;
    result = s_procs.alloc_memory(vk->device, &alloc, NULL, &slot->memory);
    if (result != VK_SUCCESS) {
        destroy_image(gpu, vk, slot);
        return fail(ERR_ALLOCATE_MEMORY);
    }
    if (s_procs.bind_image(vk->device, slot->image, slot->memory, 0) != VK_SUCCESS) {
        destroy_image(gpu, vk, slot);
        return fail(ERR_BIND_MEMORY);
    }
#ifdef _WIN32
    {
        VkMemoryGetWin32HandleInfoKHR get;
        HANDLE handle = NULL;
        memset(&get, 0, sizeof(get));
        get.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        get.memory = slot->memory;
        get.handleType = k_handle_bit;
        if (s_procs.get_win32(vk->device, &get, &handle) != VK_SUCCESS || !handle) {
            destroy_image(gpu, vk, slot);
            return fail(ERR_MEMORY_HANDLE_WIN32);
        }
        slot->memory_handle.handle = handle;
    }
#else
    {
        VkMemoryGetFdInfoKHR get;
        int fd = -1;
        memset(&get, 0, sizeof(get));
        get.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        get.memory = slot->memory;
        get.handleType = k_handle_bit;
        if (s_procs.get_fd(vk->device, &get, &fd) != VK_SUCCESS || fd < 0) {
            destroy_image(gpu, vk, slot);
            return fail(ERR_MEMORY_HANDLE_FD);
        }
        slot->memory_handle.fd = fd;
    }
#endif
    slot->has_memory = 1;
    slot->tex = pl_vulkan_wrap(gpu,
                               pl_vulkan_wrap_params(.image = slot->image,
                                                     .width = width,
                                                     .height = height,
                                                     .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                                                     .usage = k_usage, ));
    if (!slot->tex) {
        destroy_image(gpu, vk, slot);
        return fail(ERR_WRAP_IMAGE);
    }
    /* A wrapped image starts held by the caller. */
    slot->held = 1;
    slot->exported = 0;
    slot->width = width;
    slot->height = height;
    return 0;
}

static int ensure_sem(pl_gpu gpu, struct exp_sem *sem, int export)
{
    return sem->sem || create_sem(gpu, sem, export);
}

static int ensure_sync(pl_gpu gpu, pl_vulkan vk, struct exp_slot *slot)
{
    VkFenceCreateInfo info;
    int ok;
    int i;

    if (slot->fence)
        return 0;
    ok = ensure_sem(gpu, &slot->internal_ready, 0) && ensure_sem(gpu, &slot->internal_done, 0);
    for (i = 0; ok && i < MLT_VULKAN_EXPORT_SIDES; i++)
        ok = ensure_sem(gpu, &slot->side[i].ready, 1) && ensure_sem(gpu, &slot->side[i].done, 1);
    if (!ok)
        return fail(ERR_EXPORT_SEMAPHORES);
    memset(&info, 0, sizeof(info));
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (s_procs.create_fence(vk->device, &info, NULL, &slot->fence) != VK_SUCCESS)
        return fail(ERR_CREATE_FENCE);
    return 0;
}

static int fence_done(const struct exp_slot *slot)
{
    VkResult result;

    if (!slot->fence)
        return 1;
    result = s_procs.fence_status(s_procs.device, slot->fence);
    return result == VK_SUCCESS;
}

/* A side blocks the slot only while it is still attached and has not submitted
   its wait. A side that detached without looking is not blocked; its ready
   semaphore is consumed on the way back in. */
static int side_blocks(int armed, int attached, int seen, int generation)
{
    if (!armed || seen >= generation)
        return 0;
    return attached;
}

/* Wait every semaphore from the previous publish, then give the image back to
   libplacebo. Seen sides contribute their done semaphore. Abandoned sides
   contribute the ready semaphore nobody else will wait on. */
static int release_held(pl_gpu gpu,
                        pl_vulkan vk,
                        struct exp_slot *slot,
                        const struct exp_request *request)
{
    VkSemaphore wait = VK_NULL_HANDLE;
    VkSemaphore prior[MLT_VULKAN_EXPORT_SIDES];
    int nprior = 0;
    int index;
    int i;

    if (!slot->held)
        return 0;
    index = (int) (slot - s_slots);
    for (i = 0; slot->exported && i < MLT_VULKAN_EXPORT_SIDES; i++) {
        struct exp_side *side = &slot->side[i];
        int seen = request && request->side[i].seen[index] >= slot->generation;

        if (side->armed)
            prior[nprior++] = seen ? side->done.sem : side->ready.sem;
    }
    if (nprior > 1) {
        if (!submit(vk, prior, nprior, &slot->internal_done.sem, 1, VK_NULL_HANDLE))
            return fail(ERR_COMBINE_SEMAPHORES);
        wait = slot->internal_done.sem;
    } else if (nprior == 1) {
        wait = prior[0];
    }
    pl_vulkan_release_ex(gpu,
                         pl_vulkan_release_params(
                                 .tex = slot->tex,
                                 .layout = slot->exported ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                          : VK_IMAGE_LAYOUT_UNDEFINED,
                                 .qf = slot->exported ? VK_QUEUE_FAMILY_EXTERNAL
                                                      : VK_QUEUE_FAMILY_IGNORED,
                                 .semaphore = {
                                     .sem = wait,
                                     .value = 0,
                                 }, ));
    slot->held = 0;
    slot->exported = 0;
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++)
        slot->side[i].armed = 0;
    return 0;
}

static int arm_fence(pl_vulkan vk,
                     struct exp_slot *slot,
                     const VkSemaphore *waits,
                     int nwaits,
                     const VkSemaphore *signals,
                     int nsignals)
{
    if (s_procs.reset_fences(vk->device, 1, &slot->fence) != VK_SUCCESS)
        return 0;
    if (submit(vk, waits, nwaits, signals, nsignals, slot->fence))
        return 1;
    /* The fence is unsignaled. A follow-up submit is the only way to free it. */
    submit(vk, NULL, 0, NULL, 0, slot->fence);
    return 0;
}

static int hold_and_signal(pl_gpu gpu,
                           pl_vulkan vk,
                           struct exp_slot *slot,
                           const struct exp_request *request)
{
    VkSemaphore signals[MLT_VULKAN_EXPORT_SIDES];
    int nsignals = 0;
    int i;

    if (!pl_vulkan_hold_ex(gpu,
                           pl_vulkan_hold_params(.tex = slot->tex,
                                                 .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                 .qf = VK_QUEUE_FAMILY_EXTERNAL,
                                                 .semaphore = {
                                                     .sem = slot->internal_ready.sem,
                                                     .value = 0,
                                                 }, )))
        return fail(ERR_HOLD_IMAGE);
    slot->held = 1;
    slot->exported = 1;
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++) {
        if (request->side[i].attached)
            signals[nsignals++] = slot->side[i].ready.sem;
    }
    if (!nsignals || !arm_fence(vk, slot, &slot->internal_ready.sem, 1, signals, nsignals)) {
        submit(vk, &slot->internal_ready.sem, 1, NULL, 0, VK_NULL_HANDLE);
        return fail(ERR_SIGNAL_READY);
    }
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++)
        slot->side[i].armed = request->side[i].attached ? 1 : 0;
    return 0;
}

static int slot_pending(const struct exp_slot *slot, const struct exp_request *request)
{
    int index = (int) (slot - s_slots);
    int i;

    if (!fence_done(slot))
        return 1;
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++) {
        if (side_blocks(slot->side[i].armed,
                        request->side[i].attached,
                        request->side[i].seen[index],
                        slot->generation))
            return 1;
    }
    return 0;
}

static void *handle_pointer(union pl_handle handle)
{
#ifdef _WIN32
    return handle.handle;
#else
    return (void *) (intptr_t) handle.fd;
#endif
}

static void fill_desc(const struct exp_slot *slot, int index, mlt_vulkan_export_desc *desc)
{
    int i;

    memset(desc, 0, sizeof(*desc));
    desc->size = (int) sizeof(*desc);
    desc->slot = index;
    desc->width = slot->width;
    desc->height = slot->height;
    desc->vk_format = (int) VK_FORMAT_R16G16B16A16_SFLOAT;
    desc->usage = (int) k_usage;
    desc->generation = slot->generation;
    desc->memory_size = slot->memory_size;
#ifndef _WIN32
    desc->handle_is_fd = 1;
#endif
    desc->memory = handle_pointer(slot->memory_handle);
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++) {
        desc->side[i].armed = slot->side[i].armed;
        desc->side[i].ready = handle_pointer(slot->side[i].ready.handle);
        desc->side[i].done = handle_pointer(slot->side[i].done.handle);
    }
}

static enum slot_result render_slot(pl_gpu gpu,
                                    pl_vulkan vk,
                                    pl_renderer renderer,
                                    struct exp_slot *slot,
                                    mlt_vulkan_image *image,
                                    const struct exp_request *request,
                                    int width,
                                    int height)
{
    struct pl_frame src;
    struct pl_frame dst;
    struct pl_render_params params;
    struct pl_color_map_params color_map;
    enum slot_result result = SLOT_RENDERED;

    if (slot->tex && (slot->width != width || slot->height != height)) {
        if (release_held(gpu, vk, slot, request))
            return SLOT_FAILED;
        if (!arm_fence(vk, slot, NULL, 0, NULL, 0)) {
            fail(ERR_RESIZE_FENCE);
            return SLOT_FAILED;
        }
        /* The image is still referenced by the queue until the fence signals.
           Leave it in place; the next free pass destroys it. */
        slot->width = 0;
        slot->height = 0;
        return SLOT_BUSY;
    }
    if (slot->tex && slot->width == 0) {
        if (!fence_done(slot))
            return SLOT_BUSY;
        destroy_image(gpu, vk, slot);
    }
    if (!slot->tex && create_image(gpu, vk, slot, width, height))
        return SLOT_FAILED;
    if (release_held(gpu, vk, slot, request))
        return SLOT_FAILED;

    memset(&src, 0, sizeof(src));
    if (!placebo_map_vulkan_image(gpu, (const uint8_t *) image, &src)) {
        fail(ERR_MAP_FRAME);
        return SLOT_FAILED;
    }
    if (request->params.max_content_nits > 0)
        src.color.hdr.max_luma = (float) request->params.max_content_nits;

    memset(&dst, 0, sizeof(dst));
    dst.num_planes = 1;
    dst.planes[0].texture = slot->tex;
    dst.planes[0].components = 4;
    dst.planes[0].component_mapping[0] = 0;
    dst.planes[0].component_mapping[1] = 1;
    dst.planes[0].component_mapping[2] = 2;
    dst.planes[0].component_mapping[3] = 3;
    dst.repr = pl_color_repr_rgb;
    dst.repr.levels = PL_COLOR_LEVELS_FULL;
    dst.color.primaries = PL_COLOR_PRIM_BT_709;
    dst.color.transfer = PL_COLOR_TRC_SCRGB;
    dst.color.hdr.max_luma = request->params.max_display_nits > 0
                                 ? (float) request->params.max_display_nits
                                 : 1000.f;
    dst.color.hdr.min_luma = 0.005f;
    dst.crop.x0 = 0.f;
    dst.crop.y0 = 0.f;
    dst.crop.x1 = (float) width;
    dst.crop.y1 = (float) height;

    color_map = pl_color_map_default_params;
    if (!request->params.tone_mapping)
        color_map.tone_mapping_function = &pl_tone_map_clip;
    params = pl_render_default_params;
    params.color_map_params = &color_map;
    if (!pl_render_image(renderer, &src, &dst, &params)) {
        fail(ERR_RENDER_IMAGE);
        result = SLOT_FAILED;
    } else if (hold_and_signal(gpu, vk, slot, request)) {
        result = SLOT_FAILED;
    } else {
        slot->generation += 1;
    }
    placebo_unmap_vulkan_image(gpu, &src);
    return result;
}

static int any_side_attached(const struct exp_request *request)
{
    int i;

    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++) {
        if (request->side[i].attached)
            return 1;
    }
    return 0;
}

static void snapshot_request(const mlt_vulkan_export_params *params, struct exp_request *request)
{
    int i;

    memset(request, 0, sizeof(*request));
    request->params = *params;
    pthread_mutex_lock(&s_sides_mutex);
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++) {
        request->side[i].attached = s_sides[i].attached;
        memcpy(request->side[i].seen, s_sides[i].seen, sizeof(request->side[i].seen));
    }
    pthread_mutex_unlock(&s_sides_mutex);
}

/* params and desc are this version's whole structs. */
static int render_picture(mlt_vulkan_image *image,
                          const mlt_vulkan_export_params *params,
                          mlt_vulkan_export_desc *desc)
{
    struct exp_request request;
    pl_gpu gpu;
    pl_vulkan vk;
    pl_renderer renderer;
    int width;
    int height;
    int i;
    int pick = -1;

    memset(desc, 0, sizeof(*desc));
    desc->size = (int) sizeof(*desc);
    snapshot_request(params, &request);
    if (!any_side_attached(&request)) {
        if (s_have_desc)
            *desc = s_last;
        return 0;
    }
    width = image->width - image->crop_left - image->crop_right;
    height = image->height - image->crop_top - image->crop_bottom;
    if (width < 1 || height < 1)
        return fail(ERR_NO_VISIBLE_PIXELS);

    /* RENDER then LOCK, the same order as GPU teardown. */
    placebo_render_lock();
    gpu = placebo_gpu_peek();
    vk = gpu ? pl_vulkan_get(gpu) : NULL;
    if (!gpu || !vk) {
        placebo_render_unlock();
        return fail(ERR_DEVICE_NOT_CURRENT);
    }
    s_gpu = gpu;
    if (!load_procs(vk)) {
        placebo_render_unlock();
        return fail(ERR_PROCS_MISSING);
    }
    if (!(gpu->export_caps.sync & k_export)) {
        placebo_render_unlock();
        return fail(ERR_NO_SEMAPHORE_EXPORT);
    }
    renderer = placebo_renderer_get();
    if (!renderer) {
        placebo_render_unlock();
        return fail(ERR_NO_RENDERER);
    }

    for (i = 0; i < MLT_VULKAN_EXPORT_SLOTS; i++) {
        int index = (s_next + i) % MLT_VULKAN_EXPORT_SLOTS;
        struct exp_slot *slot = &s_slots[index];

        if (ensure_sync(gpu, vk, slot)) {
            placebo_render_unlock();
            return 1;
        }
        if (slot->width == 0 && slot->tex && fence_done(slot))
            destroy_image(gpu, vk, slot);
        if (slot_pending(slot, &request))
            continue;
        if (render_slot(gpu, vk, renderer, slot, image, &request, width, height) != SLOT_RENDERED)
            continue;
        fill_desc(slot, index, desc);
        s_last = *desc;
        s_have_desc = 1;
        s_logged_errors = 0;
        s_next = (index + 1) % MLT_VULKAN_EXPORT_SLOTS;
        pick = index;
        break;
    }
    placebo_render_unlock();
    if (pick >= 0)
        return 0;
    if (s_have_desc) {
        *desc = s_last;
        return 0;
    }
    return 1;
}

/* Fields an older caller's structs do not have read as zero, and the ones its
   desc has no room for are not written. */
static int export_render(mlt_vulkan_image *image,
                         const mlt_vulkan_export_params *params,
                         mlt_vulkan_export_desc *desc)
{
    mlt_vulkan_export_params wanted = {0};
    mlt_vulkan_export_desc made;
    int caller_size;
    int room;
    int result;

    if (!image || !params || !desc || image->nb_planes < 1 || params->size < (int) sizeof(int)
        || desc->size < (int) sizeof(int))
        return 1;
    memcpy(&wanted, params, params->size < (int) sizeof(wanted) ? params->size : sizeof(wanted));
    wanted.size = (int) sizeof(wanted);
    caller_size = desc->size;
    result = render_picture(image, &wanted, &made);
    room = caller_size < (int) sizeof(made) ? caller_size : (int) sizeof(made);
    if (!result) {
        memcpy(desc, &made, room);
        desc->size = caller_size;
    }
    return result;
}

static void export_attach(int side)
{
    if (side < 0 || side >= MLT_VULKAN_EXPORT_SIDES)
        return;
    pthread_mutex_lock(&s_sides_mutex);
    s_sides[side].attached = 1;
    pthread_mutex_unlock(&s_sides_mutex);
}

static void export_detach(int side)
{
    if (side < 0 || side >= MLT_VULKAN_EXPORT_SIDES)
        return;
    pthread_mutex_lock(&s_sides_mutex);
    s_sides[side].attached = 0;
    pthread_mutex_unlock(&s_sides_mutex);
}

static void export_submitted(int side, int slot, int generation)
{
    if (side < 0 || side >= MLT_VULKAN_EXPORT_SIDES || slot < 0 || slot >= MLT_VULKAN_EXPORT_SLOTS)
        return;
    pthread_mutex_lock(&s_sides_mutex);
    if (generation > s_sides[side].seen[slot])
        s_sides[side].seen[slot] = generation;
    pthread_mutex_unlock(&s_sides_mutex);
}

static int export_seen(int side, int slot)
{
    int generation = 0;

    if (side < 0 || side >= MLT_VULKAN_EXPORT_SIDES || slot < 0 || slot >= MLT_VULKAN_EXPORT_SLOTS)
        return 0;
    pthread_mutex_lock(&s_sides_mutex);
    generation = s_sides[side].seen[slot];
    pthread_mutex_unlock(&s_sides_mutex);
    return generation;
}

static const mlt_vulkan_export_api s_api = {
    .size = sizeof(mlt_vulkan_export_api),
    .render = export_render,
    .attach = export_attach,
    .detach = export_detach,
    .submitted = export_submitted,
    .seen = export_seen,
};

const mlt_vulkan_export_api *placebo_display_export_api(void)
{
    return &s_api;
}

void placebo_display_export_close(void)
{
    pl_vulkan vk;
    int i;
    int j;

    if (!s_gpu)
        return;
    vk = pl_vulkan_get(s_gpu);
    for (i = 0; i < MLT_VULKAN_EXPORT_SLOTS; i++) {
        struct exp_slot *slot = &s_slots[i];

        destroy_image(s_gpu, vk, slot);
        for (j = 0; j < MLT_VULKAN_EXPORT_SIDES; j++) {
            destroy_sem(s_gpu, &slot->side[j].ready);
            destroy_sem(s_gpu, &slot->side[j].done);
        }
        destroy_sem(s_gpu, &slot->internal_ready);
        destroy_sem(s_gpu, &slot->internal_done);
        if (slot->fence && s_procs.destroy_fence && vk)
            s_procs.destroy_fence(vk->device, slot->fence, NULL);
        memset(slot, 0, sizeof(*slot));
    }
    memset(&s_procs, 0, sizeof(s_procs));
    s_gpu = NULL;
    s_next = 0;
    s_have_desc = 0;
    memset(&s_last, 0, sizeof(s_last));
    s_logged_errors = 0;
    /* Generations start over with the next device, so earlier ones mean nothing. */
    pthread_mutex_lock(&s_sides_mutex);
    for (i = 0; i < MLT_VULKAN_EXPORT_SIDES; i++)
        memset(s_sides[i].seen, 0, sizeof(s_sides[i].seen));
    pthread_mutex_unlock(&s_sides_mutex);
}

#endif /* PL_HAVE_VULKAN && PL_API_VER >= 362 */
