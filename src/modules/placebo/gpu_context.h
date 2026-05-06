/*
 * gpu_context.h -- shared libplacebo GPU lifecycle
 * Copyright (C) 2025-2026 D-Ogi
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

#include <framework/mlt_frame.h>

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

/* Tear down all GPU resources. Registered for cleanup on first init. */
void placebo_gpu_release(void);

/* Interpret an opaque placebo image payload as a libplacebo texture handle. */
pl_tex placebo_image_get_tex(const uint8_t *image);

/* Mark or clear a request for a placebo private image during an upstream pull.
   This is intentionally separate from the actual output marker used by
   placebo_frame_is_tex()/placebo_frame_set_tex(). A request means "a downstream
   placebo filter wants private GPU data if a converter can provide it"; it does
   not mean the frame currently contains such data. Nested requests on the same
   frame are reference-counted because multiple placebo filters can be stacked
   during one pull. */
void placebo_frame_set_requested_tex(mlt_frame frame, int requested);

/* Return non-zero when the current conversion target is a placebo private image
   request, not merely any mlt_image_private use by some other module. */
int placebo_frame_wants_tex(mlt_frame frame, mlt_image_format format);

/* Return non-zero when the frame currently stores a placebo private image.
   This checks the actual image payload marker, not the transient request state. */
int placebo_frame_is_tex(mlt_frame frame, mlt_image_format format);

/* Store a libplacebo texture on the frame as mlt_image_private.
   The texture lifetime is owned by the frame image property. */
int placebo_frame_set_tex(mlt_frame frame, pl_tex tex);

#endif /* GPU_CONTEXT_H */
