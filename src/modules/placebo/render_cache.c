/*
 * render_cache.c -- keep one libplacebo renderer per kind of render call
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

#include "render_cache.h"

#include "gpu_context.h"
#include "idle_trim.h"

/* A pipeline uses a renderer per site and geometry, and a transition or
   deinterlace adds several. When the cache is too small, every call evicts the
   renderer the next call needs. */
#define CACHE_SIZE 16

typedef struct
{
    int site;
    int src_w, src_h, dst_w, dst_h;
    pl_fmt src_fmt, dst_fmt;
} renderer_key;

/* Guarded by the render lock. A slot is free when its renderer is NULL. used is
   when the renderer last served a call: the oldest is evicted first, and ones
   idle for too long are released by the idle thread. */
static struct
{
    renderer_key key;
    int64_t used;
    pl_renderer renderer;
} s_cache[CACHE_SIZE];

static renderer_key make_key(enum placebo_renderer_site site,
                             const struct pl_frame *src,
                             const struct pl_frame *dst)
{
    pl_tex src_tex = src->num_planes ? src->planes[0].texture : NULL;
    pl_tex dst_tex = dst->num_planes ? dst->planes[0].texture : NULL;
    renderer_key key = {.site = site};

    if (src_tex) {
        key.src_w = src_tex->params.w;
        key.src_h = src_tex->params.h;
        key.src_fmt = src_tex->params.format;
    }
    if (dst_tex) {
        key.dst_w = dst_tex->params.w;
        key.dst_h = dst_tex->params.h;
        key.dst_fmt = dst_tex->params.format;
    }
    return key;
}

static int same_key(const renderer_key *a, const renderer_key *b)
{
    return a->site == b->site && a->src_w == b->src_w && a->src_h == b->src_h
           && a->dst_w == b->dst_w && a->dst_h == b->dst_h && a->src_fmt == b->src_fmt
           && a->dst_fmt == b->dst_fmt;
}

/* The slot holding a renderer for the key, or -1. */
static int find_slot(const renderer_key *key)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (s_cache[i].renderer && same_key(&s_cache[i].key, key))
            return i;
    }
    return -1;
}

/* A free slot, otherwise the one used longest ago. */
static int victim_slot(void)
{
    int victim = 0;

    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!s_cache[i].renderer)
            return i;
        if (s_cache[i].used < s_cache[victim].used)
            victim = i;
    }
    return victim;
}

/* Run by the idle thread: destroy the renderers that have been idle too long. */
static int trim(pl_gpu gpu)
{
    int64_t now = placebo_now_ms();
    int released = 0;

    (void) gpu;
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (s_cache[i].renderer && now - s_cache[i].used > placebo_idle_ms()) {
            pl_renderer_destroy(&s_cache[i].renderer);
            released++;
        }
    }
    return released;
}

static pl_renderer renderer_for(const renderer_key *key)
{
    int slot = find_slot(key);

    if (slot < 0) {
        pl_gpu gpu = placebo_gpu_get();
        if (!gpu)
            return NULL;
        slot = victim_slot();
        if (s_cache[slot].renderer)
            pl_renderer_destroy(&s_cache[slot].renderer);
        s_cache[slot].renderer = pl_renderer_create(gpu->log, gpu);
        s_cache[slot].key = *key;
        placebo_idle_trim_start(trim);
    }
    s_cache[slot].used = placebo_now_ms();
    return s_cache[slot].renderer;
}

int placebo_render(enum placebo_renderer_site site,
                   const struct pl_frame *src,
                   const struct pl_frame *dst,
                   const struct pl_render_params *params)
{
    renderer_key key = make_key(site, src, dst);
    pl_renderer renderer = renderer_for(&key);

    return renderer && pl_render_image(renderer, src, dst, params);
}

void placebo_render_cache_drain(void)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (s_cache[i].renderer)
            pl_renderer_destroy(&s_cache[i].renderer);
    }
}
