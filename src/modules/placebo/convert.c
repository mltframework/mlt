/*
 * convert.c -- libplacebo image format converter
 * Copyright (C) 2026 D-Ogi
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

#include "color_space.h"
#include "gpu_context.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_pool.h>

#include <libplacebo/gpu.h>
#include <libplacebo/renderer.h>

static const char *tex_name(pl_tex tex)
{
    pl_fmt fmt = tex ? tex->params.format : NULL;
    return fmt && fmt->name ? fmt->name : "unknown";
}

/* Debug on every frame. The first rgba64 conversion in each direction is
   info, so -verbose shows it once. */
static void log_conversion(mlt_image_format from,
                           const char *from_layout,
                           mlt_image_format to,
                           const char *to_layout)
{
    int level = MLT_LOG_DEBUG;
    if (from == mlt_image_rgba64 || to == mlt_image_rgba64) {
        static int logged_to_rgba64;
        static int logged_from_rgba64;
        int *slot = to == mlt_image_rgba64 ? &logged_to_rgba64 : &logged_from_rgba64;
        if (!*slot) {
            *slot = 1;
            level = MLT_LOG_INFO;
        }
    }
    mlt_log(NULL,
            level,
            "[placebo] Converting from format %s (%s) to %s (%s)\n",
            mlt_image_format_name(from),
            from_layout ? from_layout : "cpu",
            mlt_image_format_name(to),
            to_layout ? to_layout : "cpu");
}

static int output_is_rgb8(mlt_image_format format)
{
    return format == mlt_image_rgb || format == mlt_image_rgba;
}

static int upload_tex(mlt_frame frame,
                      uint8_t **image,
                      mlt_image_format *format,
                      int width,
                      int height,
                      pl_gpu gpu,
                      pl_fmt rgba_fmt)
{
    if (!*image || !rgba_fmt)
        return 1;

    size_t stride = (size_t) width * rgba_fmt->texel_size;
    pl_tex tex = NULL;
    mlt_image_format src_format = *format;

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

    log_conversion(src_format, "cpu", mlt_image_private, rgba_fmt->name);
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

/* Copy into dst_fmt without changing the transfer. An rgba64 download is
   rgba16 unorm. An 8-bit RGB download is rgba8. */
static int render_to_format(
    mlt_frame frame, pl_gpu gpu, pl_tex src, pl_fmt dst_fmt, int width, int height, pl_tex *out)
{
    pl_renderer renderer = placebo_renderer_get();
    const char *trc_name = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc");
    mlt_color_trc trc = trc_name ? mlt_image_color_trc_id(trc_name) : mlt_color_trc_none;
    enum pl_color_transfer transfer = placebo_transfer_from_mlt(trc);
    pl_fmt src_fmt = src ? src->params.format : NULL;
    if (!dst_fmt || !renderer || !src)
        return 1;
    /* Unorm bit-depth changes have no curve to apply. A float texture with
       no transfer is linear light, and guessing sRGB would encode it. */
    if (transfer == PL_COLOR_TRC_UNKNOWN) {
        if (!src_fmt || src_fmt->type != PL_FMT_UNORM)
            return 1;
        transfer = PL_COLOR_TRC_SRGB;
    }

    struct pl_color_space color = placebo_color_space_for_trc(frame, transfer);
    placebo_render_lock();
    pl_tex dst = pl_tex_create(gpu,
                               pl_tex_params(.w = width,
                                             .h = height,
                                             .format = dst_fmt,
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
    pl_tex converted = NULL;
    mlt_image_format produced = mlt_image_rgba;
    const char *src_layout;
    if (!tex)
        return 1;
    src_layout = tex_name(tex);

    /* Half-float is linear light. Bit-casting it to rgba8 or rgba64 scrambles
       the picture. rgba64 is 16-bit unorm, so render into rgba16 and leave
       color_trc alone. An 8-bit RGB request quantizes to rgba8 for the CPU
       filter. A YUV request stays on the GPU until the consumer has encoded
       the transfer. */
    if (output_format == mlt_image_rgba64) {
        pl_fmt rgba16 = placebo_rgba16_format(gpu);
        produced = mlt_image_rgba64;
        if (!rgba16
            || (!placebo_tex_is(tex, rgba16)
                && render_to_format(frame, gpu, tex, rgba16, width, height, &converted))) {
            mlt_log_error(NULL,
                          "[placebo] Refusing to download %s as rgba16 for %s\n",
                          src_layout,
                          mlt_image_format_name(output_format));
            return 1;
        }
        if (converted)
            tex = converted;
    } else if (!placebo_tex_is(tex, placebo_rgba8_format(gpu))) {
        pl_fmt rgba8 = placebo_rgba8_format(gpu);
        if (!output_is_rgb8(output_format) || !rgba8
            || render_to_format(frame, gpu, tex, rgba8, width, height, &converted)) {
            mlt_log_error(NULL,
                          "[placebo] Refusing to download %s as rgba8 for %s\n",
                          src_layout,
                          mlt_image_format_name(output_format));
            return 1;
        }
        tex = converted;
    }

    log_conversion(*format, src_layout, output_format, tex_name(tex));

    int size = mlt_image_format_size(produced, width, height, NULL);
    uint8_t *output = mlt_pool_alloc(size);
    if (!output) {
        if (converted) {
            placebo_render_lock();
            pl_tex_destroy(gpu, &converted);
            placebo_render_unlock();
        }
        return 1;
    }

    placebo_render_lock();
    int ok = pl_tex_download(gpu,
                             pl_tex_transfer_params(.tex = tex,
                                                    .row_pitch = (size_t) width
                                                                 * tex->params.format->texel_size,
                                                    .ptr = output, ));
    if (converted)
        pl_tex_destroy(gpu, &converted);
    placebo_render_unlock();
    if (!ok) {
        mlt_pool_release(output);
        return 1;
    }

    *image = output;
    *format = produced;
    mlt_properties_set(MLT_FRAME_PROPERTIES(frame), "mlt_image_format", NULL);
    if (mlt_frame_set_image(frame, output, size, mlt_pool_release)) {
        mlt_pool_release(output);
        *image = NULL;
        return 1;
    }
    /* Returning 0 tells mlt_frame_convert_image the request is satisfied.
       rgba is only an intermediate for a yuv422 consumer (sdl2), which would
       upload it as YUY2. rgba64 is the requested format, so it is finished. */
    if (output_format != produced)
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

    // Fail fast if not converting to or from a GPU image.
    if (*format != mlt_image_private && *format != mlt_image_vulkan
        && output_format != mlt_image_private && output_format != mlt_image_vulkan)
        return 1;

    /* A Vulkan texture is not a pixel buffer and not a pl_tex. Pass it
       through when the caller still wants a private image (the next placebo
       filter maps it). Refuse a CPU format so it cannot be read as pixels. */
    if (placebo_frame_is_vulkan(frame, *format)) {
        if (output_format == mlt_image_private || output_format == mlt_image_vulkan)
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
    if ((output_format == mlt_image_private || output_format == mlt_image_vulkan)
        && placebo_frame_is_tex(frame, *format))
        return 0;

    pl_gpu gpu = placebo_gpu_get();
    pl_fmt rgba8 = placebo_rgba8_format(gpu);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int width = mlt_properties_get_int(props, "width");
    int height = mlt_properties_get_int(props, "height");

    if (!gpu || !rgba8 || !image || width < 1 || height < 1)
        return 1;

    if (output_format == mlt_image_vulkan || placebo_frame_wants_tex(frame, output_format)) {
        pl_fmt upload_fmt = rgba8;
        if (*format == mlt_image_rgba64) {
            pl_fmt rgba16 = placebo_rgba16_format(gpu);
            if (rgba16)
                upload_fmt = rgba16;
            else if (mlt_frame_next_convert_image(frame, image, format, mlt_image_rgba))
                return 1;
        } else if (*format != mlt_image_rgba) {
            if (mlt_frame_next_convert_image(frame, image, format, mlt_image_rgba))
                return 1;
        }
        return upload_tex(frame, image, format, width, height, gpu, upload_fmt);
    }

    if (placebo_frame_is_tex(frame, *format))
        return download_rgba(frame, image, format, output_format, width, height, gpu);

    return 1;
}
