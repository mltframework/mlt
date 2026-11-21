/*
 * gpu_frame.c -- GPU images on an mlt_frame
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

#include "gpu_frame.h"

#include "gpu_context.h"
#include "tex_pool.h"

#include <framework/mlt_log.h>
#include <framework/mlt_properties.h>

#include <string.h>

static void placebo_tex_destroy(void *ptr)
{
    pl_tex tex = (pl_tex) ptr;
    if (!tex)
        return;
    pl_gpu gpu = placebo_gpu_peek();
    placebo_render_lock();
    placebo_tex_recycle(gpu, &tex);
    placebo_render_unlock();
}

pl_tex placebo_image_get_tex(const uint8_t *image)
{
    return (pl_tex) image;
}

void placebo_frame_set_requested_tex(mlt_frame frame, int requested)
{
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int count = mlt_properties_get_int(props, "_placebo_requested");

    if (requested) {
        count++;
    } else if (count > 0) {
        count--;
    }

    mlt_properties_set_int(props, "_placebo_requested", count);
}

enum placebo_request placebo_frame_request(mlt_frame frame, mlt_image_format format)
{
    if (format != mlt_image_private && format != mlt_image_vulkan)
        return PLACEBO_REQUEST_NONE;
    if (mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "_placebo_requested") > 0)
        return PLACEBO_REQUEST_FILTER;
    return format == mlt_image_vulkan ? PLACEBO_REQUEST_EXTERNAL : PLACEBO_REQUEST_NONE;
}

int placebo_request_downloads(mlt_image_format format)
{
    return format != mlt_image_none && !mlt_image_format_is_gpu(format);
}

pl_tex placebo_acquire_output(
    pl_gpu gpu, int w, int h, pl_fmt fmt, mlt_image_format requested, int use)
{
    if (placebo_request_downloads(requested))
        use |= PLACEBO_TEX_HOST_READABLE;
    return placebo_tex_acquire_work(gpu, w, h, fmt, use);
}

int placebo_frame_is_tex(mlt_frame frame, mlt_image_format format)
{
    const char *image_format = mlt_properties_get(MLT_FRAME_PROPERTIES(frame),
                                                  MLT_PLACEBO_IMAGE_PRIVATE);
    return format == mlt_image_private && image_format && !strcmp(image_format, "placebo");
}

int placebo_frame_is_vulkan(mlt_frame frame, mlt_image_format format)
{
    (void) frame;
    return format == mlt_image_vulkan;
}

void placebo_request_gpu_image(mlt_frame frame, mlt_image_format *format)
{
    (void) frame;
    if (!format || *format == mlt_image_vulkan)
        return;
    if (placebo_vulkan_has_decode_queue())
        *format = mlt_image_vulkan;
    else
        *format = mlt_image_private;
}

int placebo_frame_get_gpu_image(mlt_frame frame,
                                uint8_t **image,
                                mlt_image_format *format,
                                int *width,
                                int *height,
                                int writable)
{
    placebo_frame_set_requested_tex(frame, 1);
    placebo_request_gpu_image(frame, format);
    int error = mlt_frame_get_image(frame, image, format, width, height, writable);
    placebo_frame_set_requested_tex(frame, 0);
    if (!error && !*image) {
        mlt_log_error(NULL, "[placebo] GPU image request returned no image\n");
        return 1;
    }
    return error;
}

int placebo_frame_set_tex(mlt_frame frame, pl_tex tex)
{
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    mlt_properties_set_int(props, "format", mlt_image_private);
    mlt_properties_set(props, MLT_PLACEBO_IMAGE_PRIVATE, "placebo");
    mlt_properties_clear(props, "alpha");
    return mlt_frame_set_image(frame, (uint8_t *) tex, 0, placebo_tex_destroy);
}
