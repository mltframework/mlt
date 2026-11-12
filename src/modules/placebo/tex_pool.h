/*
 * tex_pool.h -- reuse libplacebo textures from frame to frame
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

#ifndef TEX_POOL_H
#define TEX_POOL_H

#include <libplacebo/gpu.h>
#include <libplacebo/vulkan.h>

#include <stdint.h>

/* Some drivers give every large image its own dedicated allocation. Windows
   then clears each new allocation on the copy engine, so a full-size texture
   created and destroyed on every frame costs more than the rendering does.
   These calls keep finished textures and hand them out again.

   The contents of an acquired texture are undefined. The pool is guarded by
   the render lock. Idle textures are released by the idle thread (idle_trim.h),
   so a stopped player does not keep video memory. */

/* Same as pl_tex_create, but returns an idle texture of identical size,
   format, and usage when there is one. Release it with placebo_tex_recycle.
   A request with initial data or a memory handle gets a texture of its own,
   which recycling destroys. */
pl_tex placebo_tex_acquire(pl_gpu gpu, const struct pl_tex_params *params);

/* What a texture from placebo_tex_acquire_work is also used for. */
enum placebo_tex_use {
    PLACEBO_TEX_STORABLE = 1 << 0,      /* written by compute shaders */
    PLACEBO_TEX_HOST_READABLE = 1 << 1, /* downloaded to the CPU */
    PLACEBO_TEX_BLIT_DST = 1 << 2,      /* cleared with pl_frame_clear */
};

/* A sampleable, renderable texture, with the extra uses asked for. */
pl_tex placebo_tex_acquire_work(pl_gpu gpu, int w, int h, pl_fmt fmt, int use);

/* An RGBA texture to render a filter's output into. It is host readable so a
   consumer that wants pixels can download it without another copy, and it can be
   cleared, which a filter needs for letterbox padding. Every such output shares
   one class of idle texture. */
pl_tex placebo_tex_acquire_target(pl_gpu gpu, int w, int h, pl_fmt fmt);

/* Return a texture to the pool and clear *tex. A texture that did not come
   from the pool is destroyed instead. The pool marks its textures through
   their user data, so nothing else may set it. gpu may be NULL once the GPU is
   gone: the texture itself is then lost, but the pool's record of it is freed. */
void placebo_tex_recycle(pl_gpu gpu, pl_tex *tex);

/* The timeline semaphore that goes with this texture while it is shared with
   another Vulkan user. It is created on first use and then kept with the
   texture, because libplacebo may still wait on it when the texture is reused.
   *value is the last value signaled, or the value the other user has completed
   if that is higher; the next hold signals value + 1. A signal the other user
   has queued but not completed is unknown to the pool, so that user must
   report its final value to placebo_tex_set_semaphore_value. Only a texture
   from the pool has a semaphore. Any other texture gets VK_NULL_HANDLE. */
VkSemaphore placebo_tex_semaphore(pl_gpu gpu, pl_tex tex, uint64_t *value);

/* Record the last value signaled on the texture's semaphore. */
void placebo_tex_set_semaphore_value(pl_tex tex, uint64_t value);

/* Destroy every idle texture and forget the rest. Textures still in use are
   left to their owners, as they were before the pool existed. */
void placebo_tex_pool_drain(pl_gpu gpu);

#endif /* TEX_POOL_H */
