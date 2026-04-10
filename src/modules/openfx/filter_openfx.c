/*
 * filter_openfx.c -- filter Video through OpenFX plugins
 * Copyright (C) 2025-2026 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mlt_openfx.h"

#include <framework/mlt.h>
#include <ofxImageEffect.h>
#include <string.h>

extern OfxHost MltOfxHost;
extern mlt_properties mltofx_context;
extern mlt_properties mltofx_dl;

static const char OFX_IMAGE_EFFECT[] = "_ofx_image_effect";

static mlt_image_format select_image_format(mlt_image_format incoming,
                                            mltofx_depths_mask plugin_support_depths,
                                            int plugin_handles_16bit,
                                            int plugin_wants_rgba,
                                            int plugin_wants_rgb)
{
    // Honour the incoming bit-depth: stay at 8-bit when the plugin handles byte,
    // only escalate to 16-bit when it cannot.
    if (incoming == mlt_image_rgb || incoming == mlt_image_rgba || incoming == mlt_image_yuv422
        || incoming == mlt_image_yuv420p) {
        if (plugin_support_depths & mltofx_depth_byte)
            return plugin_wants_rgba ? mlt_image_rgba : mlt_image_rgb;
        else if (plugin_handles_16bit && (plugin_wants_rgba || plugin_wants_rgb))
            return mlt_image_rgba64;
        else
            return mlt_image_rgb;
    } else {
        // 10-bit or higher — prefer 16-bit OFX path when available
        if (plugin_handles_16bit && (plugin_wants_rgba || plugin_wants_rgb))
            return mlt_image_rgba64;
        else
            return plugin_wants_rgba ? mlt_image_rgba : mlt_image_rgb;
    }
}

static void update_plugin_params(mlt_properties properties,
                                 mlt_properties image_effect_params,
                                 mlt_properties params,
                                 mlt_position position,
                                 mlt_position length)
{
    int params_count = mlt_properties_count(params);
    for (int i = 0; i < params_count; ++i) {
        char *param_key = mlt_properties_get_name(params, i);
        mlt_properties param = mlt_properties_get_data(params, param_key, NULL);
        if (!param)
            continue;
        char *param_name = mlt_properties_get(param, "identifier");
        if (!param_name)
            continue;
        char *type = mlt_properties_get(param, "type");
        char *widget = mlt_properties_get(param, "widget");
        if (!type)
            continue;
        if (widget && (strcmp(widget, "point") == 0 || strcmp(widget, "size") == 0)
            && strcmp(type, "float") == 0) {
            mlt_rect value = mlt_properties_anim_get_rect(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_double2d, value);
        } else if (strcmp(type, "float") == 0) {
            double value = mlt_properties_anim_get_double(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_double, value);
        } else if (strcmp(type, "integer") == 0 || strcmp(type, "boolean") == 0) {
            int value = mlt_properties_anim_get_int(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_int, value);
        } else if (strcmp(type, "string") == 0) {
            char *value = mlt_properties_anim_get(properties, param_name, position, length);
            if (value)
                mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_string, value);
        } else if (strcmp(type, "color") == 0) {
            mlt_color value
                = mlt_properties_anim_get_color(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_color, value);
        }
    }
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    OfxPlugin *plugin = mlt_properties_get_data(properties, "ofx_plugin", NULL);
    mlt_properties image_effect = mlt_properties_get_properties(properties, OFX_IMAGE_EFFECT);
    mlt_properties params = mlt_properties_get_properties(image_effect, "mltofx_params");
    mlt_properties image_effect_params = mlt_properties_get_properties(image_effect, "params");

    mlt_image_format requested_format = *format;
    mltofx_depths_mask plugin_support_depths = mltofx_plugin_supported_depths(image_effect);
    mltofx_components_mask plugin_support_components = mltofx_plugin_supported_components(
        image_effect);

    // Format negotiation
    const int plugin_handles_16bit = plugin_support_depths
                                     & (mltofx_depth_short | mltofx_depth_half | mltofx_depth_float);
    const int plugin_wants_rgba = plugin_support_components & mltofx_components_rgba;
    const int plugin_wants_rgb = plugin_support_components & mltofx_components_rgb;
    *format = select_image_format(*format,
                                  plugin_support_depths,
                                  plugin_handles_16bit,
                                  plugin_wants_rgba,
                                  plugin_wants_rgb);

    const int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    if (error)
        return error;

    double pixel_aspect_ratio = mlt_frame_get_aspect_ratio(frame);
    if (pixel_aspect_ratio <= 0.0)
        pixel_aspect_ratio = 1.0;

    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    // Determine depth conversion needed for the 16-bit path (short > half > float).
    int use_half = (*format == mlt_image_rgba64) && !(plugin_support_depths & mltofx_depth_short)
                   && (plugin_support_depths & mltofx_depth_half);
    int use_float = (*format == mlt_image_rgba64) && !(plugin_support_depths & mltofx_depth_short)
                    && !(plugin_support_depths & mltofx_depth_half)
                    && (plugin_support_depths & mltofx_depth_float);

    // Allocate and convert buffers outside the lock — these are per-frame operations
    // and do not touch shared plugin instance state.
    uint16_t *half_src = NULL, *half_out = NULL;
    float *float_src = NULL, *float_out = NULL;
    uint8_t *src_copy = NULL;
    struct mlt_image_s src_img_copy;

    if (use_half) {
        int n = *width * *height;
        half_src = mltofx_rgba64_to_half((const uint16_t *) *image, n);
        half_out = malloc((size_t) n * 4 * sizeof(uint16_t));
        if (!half_src || !half_out) {
            mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate half-float buffers\n");
            free(half_src);
            free(half_out);
            return 1;
        }
    } else if (use_float) {
        // float_out is also used as prime_buf so it must be allocated before the lock.
        int n = *width * *height;
        float_out = malloc((size_t) n * 4 * sizeof(float));
        float_src = mltofx_rgba64_to_float((const uint16_t *) *image, n);
        if (!float_out || !float_src) {
            mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate float buffers\n");
            free(float_out);
            free(float_src);
            return 1;
        }
    } else {
        // Short/byte path: keep a read-only source copy separate from the output buffer.
        struct mlt_image_s src_img;
        mlt_image_set_values(&src_img, *image, *format, *width, *height);
        mlt_image_set_values(&src_img_copy, NULL, *format, *width, *height);
        mlt_image_alloc_data(&src_img_copy);
        src_copy = src_img_copy.data;
        memcpy(src_copy, *image, mlt_image_calculate_size(&src_img));
    }

    const char *ofx_depth = use_half ? kOfxBitDepthHalf : use_float ? kOfxBitDepthFloat : NULL;

    // For the float path, prime_buf must be the output-sized float buffer so that
    // clip metadata advertises the correct row bytes during GetClipPreferences / GetROI.
    uint8_t *prime_buf = use_float ? (uint8_t *) float_out : *image;

    uint8_t *render_src = use_half    ? (uint8_t *) half_src
                          : use_float ? (uint8_t *) float_src
                                      : src_copy;
    uint8_t *render_dst = use_half    ? (uint8_t *) half_out
                          : use_float ? (uint8_t *) float_out
                                      : *image;

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    update_plugin_params(properties, image_effect_params, params, position, length);

    // Prime both clips with correct metadata before pre-render actions.
    // Some plugins (e.g. spatial transforms) read clip depth/bounds during
    // GetClipPreferences / GetRegionsOfInterest to set up their pipeline.
    mltofx_set_source_clip_data(plugin,
                                image_effect,
                                prime_buf,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth);
    mltofx_set_output_clip_data(plugin,
                                image_effect,
                                prime_buf,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth);

    // OFX pre-render action order: GetClipPreferences → GetRegionsOfInterest → BeginSequenceRender
    mltofx_get_clip_preferences(plugin, image_effect);
    mltofx_get_regions_of_interest(plugin, image_effect, (double) *width, (double) *height);
    mltofx_begin_sequence_render(plugin, image_effect);

    // Point clips at the actual render buffers before calling Render.
    mltofx_set_source_clip_data(plugin,
                                image_effect,
                                render_src,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth);
    mltofx_set_output_clip_data(plugin,
                                image_effect,
                                render_dst,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth);

    mltofx_action_render(plugin, image_effect, *width, *height);
    mltofx_end_sequence_render(plugin, image_effect);

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    // Convert output back to rgba64 in-place — outside the lock, per-frame only.
    if (use_half) {
        mltofx_half_to_rgba64(half_out, (uint16_t *) *image, *width * *height);
        free(half_src);
        free(half_out);
    } else if (use_float) {
        mltofx_float_to_rgba64(float_out, (uint16_t *) *image, *width * *height);
        free(float_src);
        free(float_out);
    } else {
        mlt_image_close(&src_img_copy);
    }

    if (*format != requested_format) {
        frame->convert_image(frame, image, format, mlt_image_rgba);
        *format = mlt_image_rgba;
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    OfxPlugin *plugin = mlt_properties_get_data(properties, "ofx_plugin", NULL);
    mlt_properties image_effect = mlt_properties_get_properties(properties, OFX_IMAGE_EFFECT);
    mltofx_destroy_instance(plugin, image_effect);
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

mlt_filter filter_openfx_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (!filter) {
        mlt_log_error(filter, "Unable to create filter: %s\n", id);
        return NULL;
    }
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_properties_set(properties, "resource", arg);
    if (strncmp(id, "openfx.", 7)) {
        mlt_log_error(filter, "Invalid ID: %s\n", id);
        mlt_filter_close(filter);
        return NULL;
    }
    mlt_properties_set(properties, "_pluginid", id + 7);

    mlt_properties pb = mlt_properties_get_properties(mltofx_context, id);
    char *dli = mlt_properties_get(pb, "dli");
    int index = mlt_properties_get_int(pb, "index");
    void *dlhandle = mlt_properties_get_data(mltofx_dl, dli, NULL);
    OfxGetPluginFn GetPluginFn = dlsym(dlhandle, "OfxGetPlugin");
    if (!GetPluginFn) {
        mlt_log_error(filter, "Failed to get OfxGetPlugin function from plugin: %s\n", id);
        mlt_filter_close(filter);
        return NULL;
    }
    OfxPlugin *pt = GetPluginFn(index);
    if (!pt) {
        mlt_log_error(filter, "Failed to get plugin from OfxGetPlugin: %s\n", id);
        mlt_filter_close(filter);
        return NULL;
    }
    mlt_properties params = mlt_properties_new();
    mlt_properties image_effect = mltofx_fetch_params(pt, params, NULL);
    mltofx_create_instance(pt, image_effect);

    mlt_properties_set_properties(image_effect, "begin_sequence_props", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_rod_in_args"));
    mlt_properties_set_properties(image_effect, "end_sequence_props", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_rod_out_args"));
    mlt_properties_set_properties(image_effect, "get_rod_in_args", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_roi_in_args"));
    mlt_properties_set_properties(image_effect, "get_rod_out_args", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_roi_out_args"));
    mlt_properties_set_properties(image_effect, "get_roi_in_args", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_roi_in_args"));
    mlt_properties_set_properties(image_effect, "get_roi_out_args", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_roi_out_args"));
    mlt_properties_set_properties(image_effect, "get_clippref_args", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "get_clippref_args"));
    mlt_properties_set_properties(image_effect, "render_in_args", mlt_properties_new());
    mlt_properties_close(mlt_properties_get_properties(image_effect, "render_in_args"));
    mlt_properties_set_data(properties, "ofx_plugin", pt, 0, NULL, NULL);
    mlt_properties_set_properties(properties, OFX_IMAGE_EFFECT, image_effect);
    mlt_properties_close(image_effect);
    mlt_properties_close(params);
    filter->process = filter_process;
    filter->close = filter_close;
    return filter;
}
