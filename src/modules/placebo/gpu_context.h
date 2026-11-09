/*
 * gpu_context.h -- shared libplacebo GPU lifecycle
 * Copyright (C) 2025-2026 D-Ogi
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

#ifndef GPU_CONTEXT_H
#define GPU_CONTEXT_H

#include <libplacebo/dispatch.h>
#include <libplacebo/gpu.h>
#include <libplacebo/log.h>
#include <libplacebo/renderer.h>

#include <framework/mlt_frame.h>

#include <stdint.h>

#define MLT_PLACEBO_IMAGE_PRIVATE "mlt_image_private"

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

/* Create the singleton as a presentable Vulkan device and publish it.
   extensions are Vulkan instance extensions (the SDL surface extensions).
   make_surface is called with the new VkInstance while the GPU lock is held
   and returns a VkSurfaceKHR as uint64_t. The surface is destroyed by this
   function if device creation fails, and the caller must drop its copy.
   Returns 0 when the singleton is a Vulkan device, including when one was
   already published. Returns non-zero when the singleton is another backend
   or Vulkan setup fails. Does not set the permanent failure flag, so a later
   headless init can still run. The device survives consumer close. */
int placebo_vulkan_init_display(const char *const *extensions,
                                int count,
                                uint64_t (*make_surface)(void *instance, void *opaque),
                                void *opaque);

/* Publish the singleton Vulkan device on mlt_global_properties() as
   "placebo.vulkan" so avformat can decode with the same VkDevice.
   No-op for the properties object when the backend is not Vulkan or the
   property is already set. When a GPU is already up, also marks the placebo
   context published so placebo.scale and placebo.color_transform can init.
   Those two filters must not call this. Handles inside the properties are
   not owned (destructor NULL).
   Keys: instance, phys_dev, device, get_proc_addr, features,
   inst_extensions, nb_inst_extensions, dev_extensions, nb_dev_extensions,
   queue_graphics_index/count, queue_compute_index/count,
   queue_transfer_index/count, queue_decode_index/count (decode index is -1
   when the device has no video decode queue), queue_encode_index/count
   (encode index is -1 when the device has no video encode queue),
   lock_queue, unlock_queue, lock_ctx. */
void placebo_vulkan_publish(void);

/* Non-zero after placebo_vulkan_publish() or a Vulkan display init has
   published a live GPU. Does not create a device. */
int placebo_context_is_published(void);

/* Non-zero when a published Vulkan device has a video decode queue. */
int placebo_vulkan_has_decode_queue(void);

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

/* Return non-zero when the frame currently stores an mlt_image_vulkan texture. */
int placebo_frame_is_vulkan(mlt_frame frame, mlt_image_format format);

/* Ask upstream for a GPU image. An mlt_image_vulkan request is kept. Otherwise
   ask for one when this device can decode, else a placebo private texture. */
void placebo_request_gpu_image(mlt_frame frame, mlt_image_format *format);

/* Pull a GPU image with a balanced, nestable placebo request marker.
   The caller retains its original requested format separately for output. */
int placebo_frame_get_gpu_image(mlt_frame frame,
                                uint8_t **image,
                                mlt_image_format *format,
                                int *width,
                                int *height,
                                int writable);

/* Store a libplacebo texture on the frame as mlt_image_private.
   The texture lifetime is owned by the frame image property. */
int placebo_frame_set_tex(mlt_frame frame, pl_tex tex);

#endif /* GPU_CONTEXT_H */
