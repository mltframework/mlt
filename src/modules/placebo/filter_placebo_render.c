/*
 * filter_placebo_render.c -- GPU-accelerated renderer via libplacebo
 * Copyright (C) 2025 D-Ogi
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

#include <libplacebo/gpu.h>
#include <libplacebo/renderer.h>
/* pl_deband_params, pl_dither_params, pl_color_map_params all come from renderer.h */

#include <stdlib.h>
#include <string.h>

/* Map upscaler/downscaler name to pl_filter_config */
static const struct pl_filter_config *lookup_scaler(const char *name)
{
    if (!name || !*name)
        return &pl_filter_ewa_lanczos;
    if (!strcmp(name, "bilinear"))
        return &pl_filter_bilinear;
    if (!strcmp(name, "catmull_rom"))
        return &pl_filter_catmull_rom;
    if (!strcmp(name, "mitchell"))
        return &pl_filter_mitchell;
    if (!strcmp(name, "lanczos"))
        return &pl_filter_lanczos;
    if (!strcmp(name, "ewa_lanczos"))
        return &pl_filter_ewa_lanczos;
    if (!strcmp(name, "spline36"))
        return &pl_filter_spline36;
    return &pl_filter_ewa_lanczos;
}

/* Map dithering name to pl_dither_method */
static enum pl_dither_method lookup_dither(const char *name)
{
    if (!name || !*name || !strcmp(name, "blue"))
        return PL_DITHER_BLUE_NOISE;
    if (!strcmp(name, "ordered_lut"))
        return PL_DITHER_ORDERED_LUT;
    if (!strcmp(name, "white"))
        return PL_DITHER_WHITE_NOISE;
    return PL_DITHER_BLUE_NOISE;
}

/* Map tonemapping name to pl_tone_map_function */
static const struct pl_tone_map_function *lookup_tonemap(const char *name)
{
    if (!name || !*name || !strcmp(name, "auto"))
        return &pl_tone_map_auto;
    if (!strcmp(name, "clip"))
        return &pl_tone_map_clip;
    if (!strcmp(name, "mobius"))
        return &pl_tone_map_mobius;
    if (!strcmp(name, "reinhard"))
        return &pl_tone_map_reinhard;
    if (!strcmp(name, "hable"))
        return &pl_tone_map_hable;
    if (!strcmp(name, "bt.2390") || !strcmp(name, "bt2390"))
        return &pl_tone_map_bt2390;
    if (!strcmp(name, "spline"))
        return &pl_tone_map_spline;
    return &pl_tone_map_auto;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_properties filter_props = MLT_FILTER_PROPERTIES(filter);

    pl_gpu gpu = placebo_gpu_get();
    pl_renderer renderer = placebo_renderer_get();
    if (!gpu || !renderer) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "GPU not available, passing frame through\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    /* W2 fix: verify RGBA8 format is supported */
    pl_fmt rgba_fmt = pl_find_named_fmt(gpu, "rgba8");
    if (!rgba_fmt) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "GPU does not support rgba8 format\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    /* Request RGBA from upstream */
    *format = mlt_image_rgba;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    if (error || !*image)
        return error;

    int w = *width;
    int h = *height;
    size_t stride = w * 4;

    /* W4 fix: hold render lock for all GPU operations */
    placebo_render_lock();

    /* Create source texture and upload */
    pl_tex src_tex = pl_tex_create(gpu,
                                   pl_tex_params(.w = w,
                                                 .h = h,
                                                 .format = rgba_fmt,
                                                 .sampleable = true,
                                                 .host_writable = true, ));
    if (!src_tex) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to create source texture\n");
        placebo_render_unlock();
        return 0; /* pass through original frame data */
    }

    /* C4 fix: check upload return */
    if (!pl_tex_upload(gpu,
                       pl_tex_transfer_params(.tex = src_tex,
                                              .row_pitch = stride,
                                              .ptr = *image, ))) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "GPU texture upload failed\n");
        pl_tex_destroy(gpu, &src_tex);
        placebo_render_unlock();
        return 0;
    }

    /* Create destination texture */
    pl_tex dst_tex = pl_tex_create(gpu,
                                   pl_tex_params(.w = w,
                                                 .h = h,
                                                 .format = rgba_fmt,
                                                 .renderable = true,
                                                 .host_readable = true, ));
    if (!dst_tex) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to create dest texture\n");
        pl_tex_destroy(gpu, &src_tex);
        placebo_render_unlock();
        return 0;
    }

    /* Build source and target pl_frames */
    struct pl_frame pl_src = {
        .num_planes = 1,
        .planes = {{
            .texture = src_tex,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = pl_color_space_srgb,
    };

    struct pl_frame pl_dst = {
        .num_planes = 1,
        .planes = {{
            .texture = dst_tex,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = pl_color_space_srgb,
    };

    /* Configure render params from filter properties */
    const char *preset = mlt_properties_get(filter_props, "preset");
    struct pl_render_params params;
    if (preset && !strcmp(preset, "high_quality"))
        params = pl_render_high_quality_params;
    else if (preset && !strcmp(preset, "fast"))
        params = pl_render_fast_params;
    else
        params = pl_render_default_params;

    /* Scalers */
    const char *upscaler = mlt_properties_get(filter_props, "upscaler");
    const char *downscaler = mlt_properties_get(filter_props, "downscaler");
    params.upscaler = lookup_scaler(upscaler);
    params.downscaler = lookup_scaler(downscaler);

    /* Debanding */
    struct pl_deband_params deband_params = pl_deband_default_params;
    if (mlt_properties_get_int(filter_props, "deband")) {
        int iterations = mlt_properties_get_int(filter_props, "deband_iterations");
        if (iterations > 0)
            deband_params.iterations = iterations;
        params.deband_params = &deband_params;
    } else {
        params.deband_params = NULL;
    }

    /* Dithering */
    const char *dither_name = mlt_properties_get(filter_props, "dithering");
    struct pl_dither_params dither_params = pl_dither_default_params;
    if (dither_name && !strcmp(dither_name, "none")) {
        params.dither_params = NULL;
    } else {
        dither_params.method = lookup_dither(dither_name);
        params.dither_params = &dither_params;
    }

    /* Tone mapping */
    const char *tonemap_name = mlt_properties_get(filter_props, "tonemapping");
    struct pl_color_map_params color_map_params = pl_color_map_default_params;
    color_map_params.tone_mapping_function = lookup_tonemap(tonemap_name);
    params.color_map_params = &color_map_params;

    /* Render */
    if (!pl_render_image(renderer, &pl_src, &pl_dst, &params)) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "pl_render_image failed\n");
    }

    /* C4 fix: check download return */
    if (!pl_tex_download(gpu,
                         pl_tex_transfer_params(.tex = dst_tex,
                                                .row_pitch = stride,
                                                .ptr = *image, ))) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "GPU texture download failed\n");
    }

    /* Cleanup textures */
    pl_tex_destroy(gpu, &src_tex);
    pl_tex_destroy(gpu, &dst_tex);

    placebo_render_unlock();

    return 0;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_placebo_render_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        mlt_properties props = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set(props, "preset", "default");
        mlt_properties_set(props, "upscaler", "ewa_lanczos");
        mlt_properties_set(props, "downscaler", "mitchell");
        mlt_properties_set_int(props, "deband", 0);
        mlt_properties_set_int(props, "deband_iterations", 1);
        mlt_properties_set(props, "dithering", "blue");
        mlt_properties_set(props, "tonemapping", "auto");
    }
    return filter;
}
