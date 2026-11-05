/*
 * filter_placebo_scale.c -- GPU fit, scale, and pad via libplacebo
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
#include "convert.h"
#include "gpu_context.h"
#include "vulkan_frame.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_profile.h>

#include <libplacebo/filters.h>
#include <libplacebo/gpu.h>
#include <libplacebo/renderer.h>

#include <math.h>
#include <string.h>

/* Same fit as filter_resize.c: scale to maximise the picture inside the
   output frame, then center it. distort stretches to the full frame. */
static void letterbox_rect(int out_w,
                           int out_h,
                           int src_w,
                           int src_h,
                           double src_sar,
                           double dst_sar,
                           int norm_w,
                           int norm_h,
                           int distort,
                           int *content_w,
                           int *content_h,
                           int *off_x,
                           int *off_y)
{
    int cw = out_w;
    int ch = out_h;

    if (!distort && src_w > 0 && src_h > 0 && out_w > 0 && out_h > 0 && norm_w > 0 && norm_h > 0
        && src_sar > 0.0 && dst_sar > 0.0) {
        double input_ar = src_sar * src_w / (double) src_h;
        double output_ar = dst_sar * out_w / (double) out_h;
        if (input_ar > 0.0 && output_ar > 0.0) {
            int scaled_width = (int) rint((input_ar * norm_w) / output_ar);
            int scaled_height = norm_h;
            if (scaled_width > norm_w) {
                scaled_width = norm_w;
                scaled_height = (int) rint((output_ar * norm_h) / input_ar);
            }
            cw = (int) rint(scaled_width * (double) out_w / norm_w);
            ch = (int) rint(scaled_height * (double) out_h / norm_h);
        }
    }

    if (cw < 1)
        cw = 1;
    if (ch < 1)
        ch = 1;
    if (cw > out_w)
        cw = out_w;
    if (ch > out_h)
        ch = out_h;

    *content_w = cw;
    *content_h = ch;
    *off_x = (out_w - cw) / 2;
    *off_y = (out_h - ch) / 2;
}

static void apply_consumer_scaler(struct pl_render_params *params, const char *rescale)
{
    if (!rescale || !strcmp(rescale, "bilinear") || !strcmp(rescale, "fast_bilinear")) {
        params->upscaler = &pl_filter_bilinear;
        params->downscaler = &pl_filter_bilinear;
    } else if (!strcmp(rescale, "nearest") || !strcmp(rescale, "neighbor")) {
        params->upscaler = &pl_filter_nearest;
        params->downscaler = &pl_filter_nearest;
    }
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));

    double aspect_ratio = mlt_deque_pop_back_double(MLT_FRAME_IMAGE_STACK(frame));
    double consumer_aspect = profile ? mlt_profile_sar(profile) : 0.0;
    if (*width <= 0 || *height <= 0) {
        if (profile) {
            if (*width <= 0)
                *width = profile->width;
            if (*height <= 0)
                *height = profile->height;
        }
    }

    int out_w = *width;
    int out_h = *height;
    if (aspect_ratio == 0.0)
        aspect_ratio = consumer_aspect > 0.0 ? consumer_aspect : 1.0;
    if (consumer_aspect <= 0.0)
        consumer_aspect = aspect_ratio;
    mlt_properties_set_double(properties, "aspect_ratio", aspect_ratio);

    const char *rescale = mlt_properties_get(properties, "consumer.rescale");
    pl_gpu gpu = placebo_gpu_get();
    pl_renderer renderer = placebo_renderer_get();
    pl_fmt rgba_fmt = gpu ? pl_find_named_fmt(gpu, "rgba8") : NULL;
    if ((rescale && !strcmp(rescale, "none")) || !gpu || !renderer || !rgba_fmt || out_w < 1
        || out_h < 1) {
        if (!gpu || !renderer || !rgba_fmt)
            mlt_log_warning(MLT_FILTER_SERVICE(filter),
                            "GPU not available, passing frame through\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    int distort = mlt_properties_get_int(properties, "distort");
    int norm_w = profile ? profile->width : out_w;
    int norm_h = profile ? profile->height : out_h;
    int aspect_w = mlt_properties_get_int(properties, "meta.media.width");
    int aspect_h = mlt_properties_get_int(properties, "meta.media.height");
    int have_aspect = aspect_w > 0 && aspect_h > 0;
    int content_w = out_w;
    int content_h = out_h;
    int off_x = 0;
    int off_y = 0;
    if (have_aspect) {
        letterbox_rect(out_w,
                       out_h,
                       aspect_w,
                       aspect_h,
                       aspect_ratio,
                       consumer_aspect,
                       norm_w,
                       norm_h,
                       distort,
                       &content_w,
                       &content_h,
                       &off_x,
                       &off_y);
        /* Resampling a different height mixes fields. Match filter_rescale.c. */
        if (content_h != aspect_h)
            mlt_properties_set_int(properties, "consumer.progressive", 1);
    }

    mlt_properties_set_int(properties, "resize_width", out_w);
    mlt_properties_set_int(properties, "resize_height", out_h);
    mlt_properties_set_int(properties, "distort", 0);

    /* Ask avformat for a Vulkan texture. A software frame is uploaded by
       placebo_convert_image because _placebo_requested is set. The producer
       overwrites the requested size with the decoded size; the fit above
       uses the profile size, not that request. requested is the caller's
       format so an rgba64 consumer is fitted into rgba16. */
    mlt_image_format requested = *format;
    placebo_frame_set_requested_tex(frame, 1);
    placebo_request_gpu_image(frame, format);
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    placebo_frame_set_requested_tex(frame, 0);
    if (error || !*image)
        return error;

    if (!have_aspect) {
        aspect_w = *width;
        aspect_h = *height;
        letterbox_rect(out_w,
                       out_h,
                       aspect_w,
                       aspect_h,
                       aspect_ratio,
                       consumer_aspect,
                       norm_w,
                       norm_h,
                       distort,
                       &content_w,
                       &content_h,
                       &off_x,
                       &off_y);
    }

    if (!mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_logged_geometry")) {
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_logged_geometry", 1);
        mlt_log_info(MLT_FILTER_SERVICE(filter),
                     "fit %dx%d (sar %.4f) into %dx%d (sar %.4f) content %dx%d at %d,%d\n",
                     aspect_w,
                     aspect_h,
                     aspect_ratio,
                     out_w,
                     out_h,
                     consumer_aspect,
                     content_w,
                     content_h,
                     off_x,
                     off_y);
    }

    int is_tex = placebo_frame_is_tex(frame, *format);
    int is_vulkan = placebo_frame_is_vulkan(frame, *format);
    if (!is_tex && !is_vulkan) {
        mlt_log_error(MLT_FILTER_SERVICE(filter),
                      "Expected a placebo texture or Vulkan texture, got %s (%s)\n",
                      mlt_image_format_name(*format),
                      mlt_properties_get(properties, MLT_PLACEBO_IMAGE_PRIVATE));
        return 1;
    }
    if (is_tex && !placebo_image_get_tex(*image))
        return 1;

    /* A linear texture stays linear. A Vulkan frame still in its source
       transfer is encoded to sRGB. rgba64 is rgba16; otherwise rgba8, or
       half-float while the frame stays linear and private. */
    struct pl_color_space dst_color = pl_color_space_srgb;
    pl_fmt dst_fmt = rgba_fmt;
    pl_fmt encoded = NULL;
    placebo_output_format(gpu, frame, requested, &dst_color, &encoded);
    if (encoded)
        dst_fmt = encoded;

    placebo_render_lock();
    pl_tex dst_tex = pl_tex_create(gpu,
                                   pl_tex_params(.w = out_w,
                                                 .h = out_h,
                                                 .format = dst_fmt,
                                                 .sampleable = true,
                                                 .renderable = true,
                                                 .host_readable = true, ));
    if (!dst_tex) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to create dest texture\n");
        placebo_render_unlock();
        return 1;
    }

    struct pl_frame mapped = {0};
    struct pl_frame pl_src = {0};
    const struct pl_frame *src = &pl_src;
    int mapped_vulkan = 0;
    if (is_vulkan) {
        if (!placebo_map_vulkan_image(gpu, *image, &mapped)) {
            mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to map Vulkan frame\n");
            pl_tex_destroy(gpu, &dst_tex);
            placebo_render_unlock();
            return 1;
        }
        src = &mapped;
        mapped_vulkan = 1;
    } else {
        pl_src = (struct pl_frame){
            .num_planes = 1,
            .planes = {{
                .texture = placebo_image_get_tex(*image),
                .components = 4,
                .component_mapping = {0, 1, 2, 3},
            }},
            .repr = pl_color_repr_rgb,
            .color = dst_color,
        };
    }

    struct pl_frame pl_dst = {
        .num_planes = 1,
        .planes = {{
            .texture = dst_tex,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = dst_color,
    };
    const float black[3] = {0.f, 0.f, 0.f};
    pl_frame_clear(gpu, &pl_dst, black);
    pl_dst.crop = (pl_rect2df){
        .x0 = off_x,
        .y0 = off_y,
        .x1 = off_x + content_w,
        .y1 = off_y + content_h,
    };

    struct pl_render_params params = pl_render_default_params;
    apply_consumer_scaler(&params, rescale);
    if (!pl_render_image(renderer, src, &pl_dst, &params)) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "pl_render_image failed\n");
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        pl_tex_destroy(gpu, &dst_tex);
        placebo_render_unlock();
        return 1;
    }
    if (mapped_vulkan)
        placebo_unmap_vulkan_image(gpu, &mapped);
    placebo_render_unlock();

    *width = out_w;
    *height = out_h;
    if (!distort)
        mlt_frame_set_aspect_ratio(frame, consumer_aspect);
    mlt_properties_set_int(properties, "width", out_w);
    mlt_properties_set_int(properties, "height", out_h);
    if (placebo_frame_publish_tex(frame,
                                  dst_tex,
                                  requested,
                                  image,
                                  format,
                                  placebo_frame_color_trc(frame, *image, *format)))
        return 1;
    return 0;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_deque_push_back_double(MLT_FRAME_IMAGE_STACK(frame), mlt_frame_get_aspect_ratio(frame));
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    mlt_frame_prepend_convert_image(frame, placebo_convert_image);
    return frame;
}

mlt_filter filter_placebo_scale_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (!filter)
        return NULL;
    /* Returning NULL lets the loader fall through to swscale. */
    if (!placebo_gpu_get()) {
        mlt_filter_close(filter);
        return NULL;
    }
    placebo_vulkan_publish();
    filter->process = filter_process;
    /* producer_loader skips the resizer line when this is set. */
    mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_loader_no_resize", 1);
    return filter;
}
