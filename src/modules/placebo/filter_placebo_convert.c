/*
 * filter_placebo_convert.c -- libplacebo image format converter
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

#include "gpu_context.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_pool.h>

#include <libplacebo/gpu.h>

static pl_fmt rgba_texture_format(pl_gpu gpu)
{
    return gpu ? pl_find_named_fmt(gpu, "rgba8") : NULL;
}

static int upload_rgba(mlt_frame frame,
                       uint8_t **image,
                       mlt_image_format *format,
                       int width,
                       int height,
                       pl_gpu gpu,
                       pl_fmt rgba_fmt)
{
    if (*format != mlt_image_rgba) {
        if (mlt_frame_next_convert_image(frame, image, format, mlt_image_rgba))
            return 1;
    }
    if (!*image)
        return 1;

    size_t stride = width * 4;
    pl_tex tex = NULL;

    placebo_render_lock();
    tex = pl_tex_create(gpu,
                        pl_tex_params(.w = width,
                                      .h = height,
                                      .format = rgba_fmt,
                                      .sampleable = true,
                                      .renderable = true,
                                      .host_readable = true,
                                      .host_writable = true, ));
    if (!tex) {
        placebo_render_unlock();
        return 1;
    }
    if (!pl_tex_upload(gpu,
                       pl_tex_transfer_params(.tex = tex, .row_pitch = stride, .ptr = *image, ))) {
        pl_tex_destroy(gpu, &tex);
        placebo_render_unlock();
        return 1;
    }
    placebo_render_unlock();

    *format = mlt_image_private;
    *image = (uint8_t *) tex;
    if (placebo_frame_set_tex(frame, tex)) {
        placebo_render_lock();
        pl_tex_destroy(gpu, &tex);
        placebo_render_unlock();
        *image = NULL;
        return 1;
    }
    return 0;
}

static int download_rgba(mlt_frame frame,
                         uint8_t **image,
                         mlt_image_format *format,
                         mlt_image_format output_format,
                         int width,
                         int height,
                         pl_gpu gpu)
{
    pl_tex tex = placebo_image_get_tex(*image);
    if (!tex)
        return 1;

    int size = mlt_image_format_size(mlt_image_rgba, width, height, NULL);
    uint8_t *output = mlt_pool_alloc(size);
    if (!output)
        return 1;

    placebo_render_lock();
    int ok = pl_tex_download(gpu,
                             pl_tex_transfer_params(.tex = tex,
                                                    .row_pitch = width * 4,
                                                    .ptr = output, ));
    placebo_render_unlock();
    if (!ok) {
        mlt_pool_release(output);
        return 1;
    }

    *image = output;
    *format = mlt_image_rgba;
    mlt_properties_set(MLT_FRAME_PROPERTIES(frame), "mlt_image_format", NULL);
    if (mlt_frame_set_image(frame, output, size, mlt_pool_release)) {
        mlt_pool_release(output);
        *image = NULL;
        return 1;
    }
    if (output_format != mlt_image_rgba)
        return mlt_frame_next_convert_image(frame, image, format, output_format);
    return 0;
}

static int convert_image(mlt_frame frame,
                         uint8_t **image,
                         mlt_image_format *format,
                         mlt_image_format output_format)
{
    if (*format == output_format && *format != mlt_image_private)
        return 0;

    if (placebo_frame_is_tex(frame, *format) && placebo_frame_wants_tex(frame, output_format))
        return 0;

    pl_gpu gpu = placebo_gpu_get();
    pl_fmt rgba_fmt = rgba_texture_format(gpu);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int width = mlt_properties_get_int(props, "width");
    int height = mlt_properties_get_int(props, "height");

    if (!gpu || !rgba_fmt || !image || width < 1 || height < 1)
        return 1;

    mlt_log_verbose(NULL,
                    "[placebo.convert] Converting from format %d to %d\n",
                    *format,
                    output_format);
    if (placebo_frame_wants_tex(frame, output_format))
        return upload_rgba(frame, image, format, width, height, gpu, rgba_fmt);

    if (placebo_frame_is_tex(frame, *format))
        return download_rgba(frame, image, format, output_format, width, height, gpu);

    return 1;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_convert_image(frame, convert_image);
    return frame;
}

mlt_filter filter_placebo_convert_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter)
        filter->process = filter_process;
    return filter;
}