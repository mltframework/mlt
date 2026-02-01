/*
 * gpu_context.h -- shared libplacebo GPU lifecycle
 * Copyright (C) 2025 D-Ogi
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

#ifndef GPU_CONTEXT_H
#define GPU_CONTEXT_H

#include <libplacebo/dispatch.h>
#include <libplacebo/gpu.h>
#include <libplacebo/log.h>
#include <libplacebo/renderer.h>

/* Return the singleton pl_gpu, lazily initialized on first call.
   Returns NULL on failure. Thread-safe. */
pl_gpu placebo_gpu_get(void);

/* Return the singleton pl_dispatch (for shader filter). Thread-safe. */
pl_dispatch placebo_dispatch_get(void);

/* Return the singleton pl_renderer (for render filter). Thread-safe. */
pl_renderer placebo_renderer_get(void);

/* Lock/unlock the render mutex. pl_renderer and pl_dispatch are NOT
   thread-safe, so callers must hold this lock during pl_render_image
   and pl_dispatch calls. */
void placebo_render_lock(void);
void placebo_render_unlock(void);

/* Tear down all GPU resources. Registered via atexit on first init. */
void placebo_gpu_release(void);

#endif /* GPU_CONTEXT_H */
