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

#include <ctype.h>
#include <framework/mlt.h>
#include <ofxImageEffect.h>
#include <string.h>

extern OfxHost MltOfxHost;
extern mlt_properties mltofx_context;
extern mlt_properties mltofx_dl;

static const char OFX_IMAGE_EFFECT[] = "_ofx_image_effect";
static const char NATRON_FORMAT_CHOICE_PARAM[] = "NatronParamFormatChoice";
static const char NATRON_FORMAT_SIZE_PARAM[] = "NatronParamFormatSize";

typedef struct
{
    int saw_choice;
    int width;
    int height;
} natron_format_compat_state;

static int parse_choice_dimensions(const char *choice, int *out_w, int *out_h)
{
    if (!choice || !out_w || !out_h)
        return 0;

    for (const char *p = choice; *p; ++p) {
        if (!isdigit((unsigned char) *p))
            continue;

        // Only start at the beginning of a numeric token, not the middle of one.
        if (p > choice && isdigit((unsigned char) p[-1]))
            continue;

        const char *q = p;
        int w = 0;
        while (isdigit((unsigned char) *q)) {
            w = w * 10 + (*q - '0');
            ++q;
        }

        if (*q != 'x' && *q != 'X')
            continue;
        ++q;
        if (!isdigit((unsigned char) *q))
            continue;

        int h = 0;
        while (isdigit((unsigned char) *q)) {
            h = h * 10 + (*q - '0');
            ++q;
        }

        // Height token must also end cleanly (space/end/punctuation), not in the middle of digits.
        if (isdigit((unsigned char) *q))
            continue;

        if (w > 0 && h > 0) {
            *out_w = w;
            *out_h = h;
            return 1;
        }
    }

    return 0;
}

static void natron_compat_track_choice(natron_format_compat_state *state,
                                       const char *param_name,
                                       const char *value)
{
    if (!state || !param_name || !value)
        return;
    if (strcmp(param_name, NATRON_FORMAT_CHOICE_PARAM) != 0)
        return;

    state->saw_choice = 1;
    parse_choice_dimensions(value, &state->width, &state->height);
}

static void natron_compat_apply_size(mlt_properties image_effect_params,
                                     const natron_format_compat_state *state)
{
    if (!image_effect_params || !state)
        return;
    if (!state->saw_choice || state->width <= 0 || state->height <= 0)
        return;

    mltofx_param_set_value(image_effect_params,
                           (char *) NATRON_FORMAT_SIZE_PARAM,
                           mltofx_prop_int2d,
                           state->width,
                           state->height);
}

static void reset_image_effect_action_props(mlt_properties image_effect, const char *name)
{
    mlt_properties old_props = mlt_properties_get_properties(image_effect, name);
    mlt_properties_set_properties(image_effect, name, mlt_properties_new());
    if (old_props)
        mlt_properties_close(old_props);
}

static void init_image_effect_action_props(mlt_properties image_effect)
{
    reset_image_effect_action_props(image_effect, "begin_sequence_props");
    reset_image_effect_action_props(image_effect, "end_sequence_props");
    reset_image_effect_action_props(image_effect, "get_rod_in_args");
    reset_image_effect_action_props(image_effect, "get_rod_out_args");
    reset_image_effect_action_props(image_effect, "get_roi_in_args");
    reset_image_effect_action_props(image_effect, "get_roi_out_args");
    reset_image_effect_action_props(image_effect, "get_clippref_args");
    reset_image_effect_action_props(image_effect, "render_in_args");
}

static mlt_properties create_image_effect_instance(OfxPlugin *plugin,
                                                   mlt_properties filter_properties,
                                                   int sync_param_backlink)
{
    mlt_properties params = mlt_properties_new();
    if (!params)
        return NULL;

    mlt_properties image_effect = mltofx_fetch_params(plugin, params, NULL);
    if (!image_effect) {
        mlt_properties_close(params);
        return NULL;
    }

    if (sync_param_backlink) {
        mlt_properties image_effect_params = mlt_properties_get_properties(image_effect, "params");
        int param_count = mlt_properties_count(image_effect_params);
        for (int i = 0; i < param_count; ++i) {
            char *param_name = mlt_properties_get_name(image_effect_params, i);
            mlt_properties param = mlt_properties_get_properties(image_effect_params, param_name);
            if (!param)
                continue;
            mlt_properties param_props = mlt_properties_get_properties(param, "p");
            if (!param_props)
                continue;
            mlt_properties_set_data(param_props,
                                    "_filter_properties",
                                    filter_properties,
                                    0,
                                    NULL,
                                    NULL);
        }
    }

    mltofx_create_instance(plugin, image_effect);
    init_image_effect_action_props(image_effect);
    mlt_properties_set_data(image_effect,
                            "_runtime_params",
                            params,
                            0,
                            (mlt_destructor) mlt_properties_close,
                            NULL);
    return image_effect;
}

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
    natron_format_compat_state natron_compat = {0};

    int params_count = mlt_properties_count(params);
    for (int i = 0; i < params_count; ++i) {
        char *param_key = mlt_properties_get_name(params, i);
        mlt_properties param = mlt_properties_get_data(params, param_key, NULL);
        if (!param)
            continue;
        char *param_name = mlt_properties_get(param, "identifier");
        if (!param_name)
            continue;
        if (!mlt_properties_exists(properties, param_name))
            continue;
        char *type = mlt_properties_get(param, "type");
        char *widget = mlt_properties_get(param, "widget");
        if (!type)
            continue;
        if (widget && (strcmp(widget, "point") == 0 || strcmp(widget, "size") == 0)
            && strcmp(type, "float") == 0) {
            mlt_rect value = mlt_properties_anim_get_rect(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_double2d, value);
        } else if (widget && (strcmp(widget, "point") == 0 || strcmp(widget, "size") == 0)
                   && strcmp(type, "integer") == 0) {
            mlt_rect value = mlt_properties_anim_get_rect(properties, param_name, position, length);
            int x = (int) value.x;
            int y = (int) value.y;
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_int2d, x, y);
        } else if (strcmp(type, "float") == 0) {
            double value = mlt_properties_anim_get_double(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_double, value);
        } else if (strcmp(type, "integer") == 0 || strcmp(type, "boolean") == 0) {
            int value = mlt_properties_anim_get_int(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_int, value);
        } else if (strcmp(type, "string") == 0) {
            char *value = mlt_properties_anim_get(properties, param_name, position, length);
            if (value) {
                natron_compat_track_choice(&natron_compat, param_name, value);
                mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_string, value);
            }
        } else if (strcmp(type, "color") == 0) {
            mlt_color value
                = mlt_properties_anim_get_color(properties, param_name, position, length);
            mltofx_param_set_value(image_effect_params, param_name, mltofx_prop_color, value);
        }
    }

    natron_compat_apply_size(image_effect_params, &natron_compat);
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
    mlt_properties base_image_effect = mlt_properties_get_properties(properties, OFX_IMAGE_EFFECT);
    int allow_frame_threading = mlt_properties_get_int(properties, "_allow_frame_threading");
    int top_left_origin = mlt_properties_get_int(properties, "mlt_origin");
    mlt_properties image_effect = base_image_effect;
    if (allow_frame_threading) {
        image_effect = create_image_effect_instance(plugin, properties, 0);
        if (!image_effect)
            image_effect = base_image_effect;
    }

    mlt_properties params = mlt_properties_get_properties(image_effect, "mltofx_params");
    mlt_properties image_effect_params = mlt_properties_get_properties(image_effect, "params");
    int lock_service = (image_effect == base_image_effect);

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

    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    double render_scale_x = mlt_profile_scale_width(profile, *width);
    double render_scale_y = mlt_profile_scale_height(profile, *height);

    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    double ofx_time = (double) position;

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

    if (lock_service)
        mlt_service_lock(MLT_FILTER_SERVICE(filter));

    // In serialized mode, image_effect is shared across frames; update scale under lock.
    mltofx_set_render_scale(image_effect, render_scale_x, render_scale_y);
    mltofx_set_project_properties(image_effect,
                                  *width,
                                  *height,
                                  pixel_aspect_ratio,
                                  mlt_profile_fps(profile),
                                  (double) length);

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
                                ofx_depth,
                                top_left_origin);
    mltofx_set_output_clip_data(plugin,
                                image_effect,
                                prime_buf,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth,
                                top_left_origin);

    // OFX pre-render action order:
    // GetClipPreferences -> GetRegionOfDefinition -> GetRegionsOfInterest -> BeginSequenceRender
    mltofx_get_clip_preferences(plugin, image_effect);
    mltofx_get_region_of_definition(plugin, image_effect, ofx_time);
    mltofx_get_regions_of_interest(plugin, image_effect, ofx_time, (double) *width, (double) *height);
    mltofx_begin_sequence_render(plugin, image_effect, ofx_time);

    // Point clips at the actual render buffers before calling Render.
    mltofx_set_source_clip_data(plugin,
                                image_effect,
                                render_src,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth,
                                top_left_origin);
    mltofx_set_output_clip_data(plugin,
                                image_effect,
                                render_dst,
                                *width,
                                *height,
                                *format,
                                pixel_aspect_ratio,
                                ofx_depth,
                                top_left_origin);

    mltofx_action_render(plugin, image_effect, ofx_time, *width, *height);
    mltofx_end_sequence_render(plugin, image_effect, ofx_time);

    if (lock_service)
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    if (image_effect != base_image_effect) {
        mltofx_destroy_instance(plugin, image_effect);
        mlt_properties_close(image_effect);
    }

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
        mlt_frame_convert_image(frame, image, format, mlt_image_rgba);
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
    mlt_properties image_effect = create_image_effect_instance(pt, properties, 1);
    if (!image_effect) {
        mlt_log_error(filter, "Failed to create OpenFX image effect instance: %s\n", id);
        mlt_filter_close(filter);
        return NULL;
    }

    mlt_properties_set_int(properties,
                           "_allow_frame_threading",
                           mltofx_allows_frame_threading(image_effect));
    mlt_log_info(filter,
                 "OpenFX threading mode for %s: %s\n",
                 id,
                 mlt_properties_get_int(properties, "_allow_frame_threading") ? "frame-threaded"
                                                                              : "serialized");
    mlt_properties_set_data(properties, "ofx_plugin", pt, 0, NULL, NULL);
    mlt_properties_set_properties(properties, OFX_IMAGE_EFFECT, image_effect);
    mlt_properties_close(image_effect);
    filter->process = filter_process;
    filter->close = filter_close;
    return filter;
}
