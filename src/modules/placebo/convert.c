/*
 * convert.c -- libplacebo image format converter
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

#include "color_space.h"
#include "gpu_context.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_pool.h>

#include <libplacebo/gpu.h>
#include <libplacebo/renderer.h>

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

static int tex_is_rgba8(pl_tex tex)
{
    pl_fmt fmt = tex ? tex->params.format : NULL;
    return fmt && fmt->type == PL_FMT_UNORM && fmt->num_components >= 4
           && fmt->component_depth[0] == 8 && fmt->texel_size == 4;
}

static int output_is_rgb(mlt_image_format format)
{
    return format == mlt_image_rgb || format == mlt_image_rgba || format == mlt_image_rgba64;
}

/* Quantize to rgba8 without changing the transfer. A CPU filter that writes
   the image (box_blur) can then run, and color_transform encodes the TRC. */
static int quantize_to_rgba8(
    mlt_frame frame, pl_gpu gpu, pl_tex src, int width, int height, pl_tex *out)
{
    pl_fmt rgba8 = rgba_texture_format(gpu);
    pl_renderer renderer = placebo_renderer_get();
    const char *trc_name = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc");
    mlt_color_trc trc = trc_name ? mlt_image_color_trc_id(trc_name) : mlt_color_trc_none;
    enum pl_color_transfer transfer = placebo_transfer_from_mlt(trc);
    if (!rgba8 || !renderer || !src || transfer == PL_COLOR_TRC_UNKNOWN)
        return 1;

    struct pl_color_space color = placebo_color_space_for_trc(frame, transfer);
    placebo_render_lock();
    pl_tex dst = pl_tex_create(gpu,
                               pl_tex_params(.w = width,
                                             .h = height,
                                             .format = rgba8,
                                             .sampleable = true,
                                             .renderable = true,
                                             .host_readable = true, ));
    if (!dst) {
        placebo_render_unlock();
        return 1;
    }
    struct pl_frame pl_src = {
        .num_planes = 1,
        .planes = {{
            .texture = src,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = color,
    };
    struct pl_frame pl_dst = pl_src;
    pl_dst.planes[0].texture = dst;
    struct pl_render_params params = pl_render_default_params;
    int ok = pl_render_image(renderer, &pl_src, &pl_dst, &params);
    if (!ok)
        pl_tex_destroy(gpu, &dst);
    placebo_render_unlock();
    if (!ok)
        return 1;
    *out = dst;
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
    pl_tex quantized = NULL;
    if (!tex)
        return 1;
    /* Half-float is linear light. Bit-casting it to rgba8 scrambles the
       picture. An RGB request is a CPU filter, so quantize and leave
       color_trc alone for color_transform. A YUV request stays on the GPU
       until the consumer has encoded the transfer. */
    if (!tex_is_rgba8(tex)) {
        if (!output_is_rgb(output_format)
            || quantize_to_rgba8(frame, gpu, tex, width, height, &quantized)) {
            pl_fmt fmt = tex->params.format;
            mlt_log_error(NULL,
                          "[placebo] Refusing to download %s as rgba8 for %s\n",
                          fmt && fmt->name ? fmt->name : "unknown",
                          mlt_image_format_name(output_format));
            return 1;
        }
        tex = quantized;
    }

    int size = mlt_image_format_size(mlt_image_rgba, width, height, NULL);
    uint8_t *output = mlt_pool_alloc(size);
    if (!output) {
        if (quantized) {
            placebo_render_lock();
            pl_tex_destroy(gpu, &quantized);
            placebo_render_unlock();
        }
        return 1;
    }

    placebo_render_lock();
    int ok = pl_tex_download(gpu,
                             pl_tex_transfer_params(.tex = tex,
                                                    .row_pitch = width * 4,
                                                    .ptr = output, ));
    if (quantized)
        pl_tex_destroy(gpu, &quantized);
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
    /* RGBA is only an intermediate. Returning 0 here tells mlt_frame_convert_image
       the request is satisfied, so a yuv422 consumer (sdl2) is handed RGBA.
       Return an error and let the later converters finish the request. */
    if (output_format != mlt_image_rgba)
        return 1;
    return 0;
}

int placebo_convert_image(mlt_frame frame,
                          uint8_t **image,
                          mlt_image_format *format,
                          mlt_image_format output_format)
{
    // Succeed quickly if there is nothing for any converter to do
    if (*format == output_format && *format != mlt_image_private)
        return 0;

    // Fail fast if not converting to or from mlt_image_private
    if (*format != mlt_image_private && output_format != mlt_image_private)
        return 1;

    /* A Vulkan AVFrame is not RGBA and not a pl_tex. Pass it through when the
       caller still wants a private image (placebo.render maps it). Refuse a
       CPU format so it cannot be read as pixels; the render filter downloads
       after it has replaced the image with a pl_tex. */
    if (placebo_frame_is_vulkan(frame, *format)) {
        if (output_format == mlt_image_private)
            return 0;
        return 1;
    }

    /* Keep an existing texture when the caller asked for a private image.
       mlt_frame_get_image always converts to the requested format, and that
       request stays mlt_image_private after a placebo filter has cleared
       _placebo_requested. Downloading here would read a linear half-float
       texture as rgba8 before the consumer has encoded it. Upload still
       requires _placebo_requested, so a CPU image is not pulled onto the GPU
       just because some other module asked for private. */
    if (output_format == mlt_image_private && placebo_frame_is_tex(frame, *format))
        return 0;

    pl_gpu gpu = placebo_gpu_get();
    pl_fmt rgba_fmt = rgba_texture_format(gpu);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int width = mlt_properties_get_int(props, "width");
    int height = mlt_properties_get_int(props, "height");

    if (!gpu || !rgba_fmt || !image || width < 1 || height < 1)
        return 1;

    mlt_log_debug(NULL,
                  "[placebo] Converting from format %s to %s\n",
                  mlt_image_format_name(*format),
                  mlt_image_format_name(output_format));
    if (placebo_frame_wants_tex(frame, output_format))
        return upload_rgba(frame, image, format, width, height, gpu, rgba_fmt);

    if (placebo_frame_is_tex(frame, *format))
        return download_rgba(frame, image, format, output_format, width, height, gpu);

    return 1;
}
