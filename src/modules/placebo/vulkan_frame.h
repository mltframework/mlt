/*
 * vulkan_frame.h -- map an mlt_image_vulkan texture onto a libplacebo frame
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

#ifndef VULKAN_FRAME_H
#define VULKAN_FRAME_H

#include <framework/mlt_frame.h>

#include <libplacebo/renderer.h>

/* Map an mlt_image_vulkan payload into out. Returns non-zero on success.
   The VkImages stay owned by the payload. Call unmap before replacing it. */
int placebo_map_vulkan_image(pl_gpu gpu, const uint8_t *image, struct pl_frame *out);

void placebo_unmap_vulkan_image(pl_gpu gpu, struct pl_frame *frame);

/* Transfer already stored on this image. A Vulkan payload has its own curve.
   A libplacebo texture does not, so the frame property is that curve. */
static inline mlt_color_trc placebo_frame_color_trc(mlt_frame frame,
                                                    const uint8_t *image,
                                                    mlt_image_format format)
{
    if (format == mlt_image_vulkan && image)
        return (mlt_color_trc) ((const mlt_vulkan_image *) image)->color_trc;
    return mlt_image_color_trc_id(mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc"));
}

/* Store tex on the frame. color_trc is the transfer just rendered. It is written
   to a Vulkan image and to the frame property only after the image is installed.
   A caller that asked for mlt_image_vulkan receives a Vulkan texture (VkImage
   plus a timeline semaphore). Any other request keeps the libplacebo texture as
   mlt_image_private. On failure tex is destroyed and the property is unchanged. */
int placebo_frame_publish_tex(mlt_frame frame,
                              pl_tex tex,
                              mlt_image_format requested,
                              uint8_t **image,
                              mlt_image_format *format,
                              mlt_color_trc color_trc);

#endif /* VULKAN_FRAME_H */
