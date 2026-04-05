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
    mlt_properties image_effect = mlt_properties_get_properties(properties, "ofx_image_effect");
    mlt_properties params = mlt_properties_get_properties(image_effect, "mltofx_params");
    mlt_properties image_effect_params = mlt_properties_get_properties(image_effect, "params");

    mlt_image_format requested_format = *format;
    mltofx_depths_mask plugin_support_depths = mltofx_plugin_supported_depths(image_effect);
    mltofx_components_mask plugin_support_components = mltofx_plugin_supported_components(
        image_effect);

    int plugin_handles_16bit = plugin_support_depths
                               & (mltofx_depth_short | mltofx_depth_half | mltofx_depth_float);
    int plugin_wants_rgba = plugin_support_components & mltofx_components_rgba;
    int plugin_wants_rgb = plugin_support_components & mltofx_components_rgb;

    if (*format == mlt_image_rgb || *format == mlt_image_rgba || *format == mlt_image_yuv422
        || *format == mlt_image_yuv420p) {
        // Keep 8-bit only when the plugin can handle byte
        if (plugin_support_depths & mltofx_depth_byte) {
            *format = plugin_wants_rgba ? mlt_image_rgba : mlt_image_rgb;
        } else if (plugin_handles_16bit && (plugin_wants_rgba || plugin_wants_rgb)) {
            *format = mlt_image_rgba64;
        } else {
            *format = mlt_image_rgb;
        }
    } else {
        // 10-bit or higher
        if (plugin_handles_16bit && (plugin_wants_rgba || plugin_wants_rgb)) {
            *format = mlt_image_rgba64;
        } else {
            *format = plugin_wants_rgba ? mlt_image_rgba : mlt_image_rgb;
        }
    }

    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0) {
        double pixel_aspect_ratio = mlt_frame_get_aspect_ratio(frame);
        if (pixel_aspect_ratio <= 0.0)
            pixel_aspect_ratio = 1.0;

        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_position length = mlt_filter_get_length2(filter, frame);
        mlt_service_lock(MLT_FILTER_SERVICE(filter));
        int params_count = mlt_properties_count(params);
        for (int i = 0; i < params_count; ++i) {
            char *param_key = mlt_properties_get_name(params, i);
            mlt_properties param = mlt_properties_get_data(params, param_key, NULL);
            const char *param_name = mlt_properties_get(param, "identifier");
            if (param && param_name) {
                char *type = mlt_properties_get(param, "type");
                char *widget = mlt_properties_get(param, "widget");
                if (type != NULL) {
                    if (widget != NULL
                        && (strcmp(widget, "point") == 0 || strcmp(widget, "size") == 0)
                        && strcmp(type, "float") == 0) {
                        mlt_rect value = mlt_properties_anim_get_rect(properties,
                                                                      param_name,
                                                                      position,
                                                                      length);
                        mltofx_param_set_value(image_effect_params,
                                               param_name,
                                               mltofx_prop_double2d,
                                               value);
                    } else if (strcmp(type, "float") == 0) {
                        double value = mlt_properties_anim_get_double(properties,
                                                                      param_name,
                                                                      position,
                                                                      length);
                        mltofx_param_set_value(image_effect_params,
                                               param_name,
                                               mltofx_prop_double,
                                               value);
                    } else if (strcmp(type, "integer") == 0) {
                        int value
                            = mlt_properties_anim_get_int(properties, param_name, position, length);
                        mltofx_param_set_value(image_effect_params,
                                               param_name,
                                               mltofx_prop_int,
                                               value);
                    } else if (strcmp(type, "string") == 0) {
                        int value
                            = mlt_properties_anim_get_int(properties, param_name, position, length);
                        mltofx_param_set_value(image_effect_params,
                                               param_name,
                                               mltofx_prop_int, // for handling option choice
                                               value);
                    } else if (strcmp(type, "boolean") == 0) {
                        int value
                            = mlt_properties_anim_get_int(properties, param_name, position, length);
                        mltofx_param_set_value(image_effect_params,
                                               param_name,
                                               mltofx_prop_int,
                                               value);
                    } else if (strcmp(type, "color") == 0) {
                        mlt_color value = mlt_properties_anim_get_color(properties,
                                                                        param_name,
                                                                        position,
                                                                        length);
                        mltofx_param_set_value(image_effect_params,
                                               param_name,
                                               mltofx_prop_color,
                                               value);
                    }
                }
            }
        }

        struct mlt_image_s src_img;
        mlt_image_set_values(&src_img, *image, *format, *width, *height);

        struct mlt_image_s src_img_copy;
        mlt_image_set_values(&src_img_copy, NULL, *format, *width, *height);

        mlt_image_alloc_data(&src_img_copy);

        uint8_t *src_copy = src_img_copy.data;

        // Determine depth conversion path early so clip metadata can be set before
        // the pre-render OFX actions (GetClipPreferences, GetRegionsOfInterest).
        // Some plugins read clip depth/bounds during those actions
        // to set up their transformation pipeline and fail if the clips have no metadata.
        int use_half = (*format == mlt_image_rgba64)
                       && !(plugin_support_depths & mltofx_depth_short)
                       && (plugin_support_depths & mltofx_depth_half);
        int use_float = (*format == mlt_image_rgba64)
                        && !(plugin_support_depths & mltofx_depth_short)
                        && !(plugin_support_depths & mltofx_depth_half)
                        && (plugin_support_depths & mltofx_depth_float);
        const char *ofx_depth = use_half ? kOfxBitDepthHalf : use_float ? kOfxBitDepthFloat : NULL;

        // Prime both clips with correct metadata (depth, bounds, PAR, rowbytes) before
        // calling any pre-render actions. Use *image as a temporary data pointer — the
        // plugin must not access pixel data outside of Render via clipGetImage.
        mltofx_set_source_clip_data(plugin,
                                    image_effect,
                                    *image,
                                    *width,
                                    *height,
                                    *format,
                                    pixel_aspect_ratio,
                                    ofx_depth);
        mltofx_set_output_clip_data(plugin,
                                    image_effect,
                                    *image,
                                    *width,
                                    *height,
                                    *format,
                                    pixel_aspect_ratio,
                                    ofx_depth);

        // Correct OFX pre-render action order: GetClipPreferences → GetRegionsOfInterest
        // → BeginSequenceRender.
        mltofx_get_clip_preferences(plugin, image_effect);

        // According to OpenFX documentation: Note that hosts that
        // have constant sized imagery need not call this action, only
        // hosts that allow image sizes to vary need call this.
        // mltofx_get_region_of_definition(plugin, image_effect);

        mltofx_get_regions_of_interest(plugin, image_effect, (double) *width, (double) *height);
        mltofx_begin_sequence_render(plugin, image_effect);

        uint16_t *half_src = NULL;
        uint16_t *half_out = NULL;
        float *float_src = NULL;
        float *float_out = NULL;
        if (use_half) {
            int n_pixels = *width * *height;
            half_src = mltofx_rgba64_to_half((const uint16_t *) *image, n_pixels);
            half_out = malloc((size_t) n_pixels * 4 * sizeof(uint16_t));
            if (!half_src || !half_out) {
                free(half_src);
                free(half_out);
                use_half = 0;
            }
        } else if (use_float) {
            int n_pixels = *width * *height;
            float_src = mltofx_rgba64_to_float((const uint16_t *) *image, n_pixels);
            float_out = malloc((size_t) n_pixels * 4 * sizeof(float));
            if (!float_src || !float_out) {
                free(float_src);
                free(float_out);
                use_float = 0;
            }
        } else {
            // Short/byte path: need a separate read-only copy since src and dst are different
            memcpy(src_copy, *image, mlt_image_calculate_size(&src_img));
        }

        // Update clip data pointers to the actual (possibly converted) buffers before render.
        mltofx_set_source_clip_data(plugin,
                                    image_effect,
                                    use_half    ? (uint8_t *) half_src
                                    : use_float ? (uint8_t *) float_src
                                                : src_copy,
                                    *width,
                                    *height,
                                    *format,
                                    pixel_aspect_ratio,
                                    ofx_depth);
        mltofx_set_output_clip_data(plugin,
                                    image_effect,
                                    use_half    ? (uint8_t *) half_out
                                    : use_float ? (uint8_t *) float_out
                                                : *image,
                                    *width,
                                    *height,
                                    *format,
                                    pixel_aspect_ratio,
                                    ofx_depth);

        mltofx_action_render(plugin, image_effect, *width, *height);

        if (use_half) {
            mltofx_half_to_rgba64(half_out, (uint16_t *) *image, *width * *height);
            free(half_src);
            free(half_out);
        } else if (use_float) {
            mltofx_float_to_rgba64(float_out, (uint16_t *) *image, *width * *height);
            free(float_src);
            free(float_out);
        }

        mlt_image_close(&src_img_copy);

        mltofx_end_sequence_render(plugin, image_effect);
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));
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
    mlt_properties image_effect = mlt_properties_get_properties(properties, "ofx_image_effect");
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
    mlt_properties_set_properties(properties, "ofx_image_effect", image_effect);
    mlt_properties_close(image_effect);
    mlt_properties_close(params);
    filter->process = filter_process;
    filter->close = filter_close;
    return filter;
}
