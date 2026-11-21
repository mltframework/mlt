/*
 * gpu_frame.h -- GPU images on an mlt_frame
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

#ifndef GPU_FRAME_H
#define GPU_FRAME_H

#include <libplacebo/gpu.h>

#include <framework/mlt_frame.h>

#include <stdint.h>

/* Request state and result state are kept apart. A request is transient and
   nests while several placebo filters pull upstream on one frame. A result says
   what the frame's current image really is. Mixing them lets one filter clear
   another's request, or lets a non-placebo payload pass for a texture. */

/* Interpret an opaque placebo image payload as a libplacebo texture handle. */
pl_tex placebo_image_get_tex(const uint8_t *image);

/* Mark or clear a request for a placebo private image during an upstream pull.
   A request means "a downstream placebo filter wants private GPU data if a
   converter can provide it"; it does not mean the frame contains such data.
   Nested requests on the same frame are reference-counted, because several
   placebo filters can be stacked during one pull. */
void placebo_frame_set_requested_tex(mlt_frame frame, int requested);

/* Who is asking for a GPU image in a given format. */
enum placebo_request {
    PLACEBO_REQUEST_NONE,     /* a CPU image, or another module's private one */
    PLACEBO_REQUEST_FILTER,   /* a placebo filter pulling its input: wants a pl_tex */
    PLACEBO_REQUEST_EXTERNAL, /* a caller outside placebo that imports the VkImage */
};

/* Classify the current conversion target. A placebo filter marks the frame
   with a request while it pulls, so mlt_image_vulkan without a mark is an
   outside caller, and any GPU format with a mark is a filter. */
enum placebo_request placebo_frame_request(mlt_frame frame, mlt_image_format format);

/* Return non-zero when a frame asked for in this format is downloaded to the
   CPU. mlt_image_none, mlt_image_private and mlt_image_vulkan stay on the GPU. */
int placebo_request_downloads(mlt_image_format format);

/* A texture for a placebo service to render its output into, for a caller that
   asked for requested. It is host readable only when requested is downloaded to
   the CPU, which is the one place that policy is decided. use adds what the
   service needs: PLACEBO_TEX_BLIT_DST for an output it clears, which every
   filter sets so that all filter outputs share one class of idle texture, or
   PLACEBO_TEX_STORABLE for a compute shader. Needs the render lock. */
pl_tex placebo_acquire_output(
    pl_gpu gpu, int w, int h, pl_fmt fmt, mlt_image_format requested, int use);

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

#endif /* GPU_FRAME_H */
