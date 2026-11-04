/*
 * vulkan_frame.c -- map an FFmpeg Vulkan frame onto a libplacebo frame
 * Copyright (C) 2026 D-Ogi
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

#include "vulkan_frame.h"

#ifdef PLACEBO_HAVE_LAVU
#include <vulkan/vulkan.h>
/* libplacebo's libav helpers are static inline in this translation unit.
   vulkan.h must be included first so AVVkFrame mapping is compiled in. */
#include <libplacebo/utils/libav.h>

#include <string.h>
#endif

int placebo_vulkan_frame_supported(void)
{
#ifdef PL_HAVE_LAV_VULKAN
    return 1;
#else
    return 0;
#endif
}

int placebo_map_vulkan_image(pl_gpu gpu, const uint8_t *image, struct pl_frame *out)
{
#ifdef PL_HAVE_LAV_VULKAN
    const AVFrame *frame = (const AVFrame *) image;
    if (!gpu || !frame || !out || frame->format != AV_PIX_FMT_VULKAN)
        return 0;
    memset(out, 0, sizeof(*out));
    /* NULL textures: pl_map_avframe wraps the VkImages and installs the
       timeline-semaphore acquire/release callbacks. A bare wrap would hang. */
    return pl_map_avframe(gpu, out, NULL, frame) ? 1 : 0;
#else
    (void) gpu;
    (void) image;
    (void) out;
    return 0;
#endif
}

void placebo_unmap_vulkan_image(pl_gpu gpu, struct pl_frame *frame)
{
#ifdef PL_HAVE_LAV_VULKAN
    if (gpu && frame)
        pl_unmap_avframe(gpu, frame);
#else
    (void) gpu;
    (void) frame;
#endif
}
