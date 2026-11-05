/*
 * vulkan_frame.h -- map an FFmpeg Vulkan frame onto a libplacebo frame
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

#include <libplacebo/renderer.h>

/* Non-zero when this build can map an AV_PIX_FMT_VULKAN AVFrame. */
int placebo_vulkan_frame_supported(void);

/* Map a vulkan mlt_image_private payload (an AVFrame *) into out.
   Returns non-zero on success. tex may be NULL; hw frames do not need one. */
int placebo_map_vulkan_image(pl_gpu gpu, const uint8_t *image, struct pl_frame *out);

void placebo_unmap_vulkan_image(pl_gpu gpu, struct pl_frame *frame);

#endif /* VULKAN_FRAME_H */
