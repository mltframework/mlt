/*
 * filter_openfx.c -- filter Video through OpenFX plugins
 * Copyright (C) 2025 Meltytech, LLC
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
#include "ofxImageEffect.h"
#include <framework/mlt.h>
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
    mlt_properties image_effect = mlt_properties_get_data(properties, "ofx_image_effect", NULL);
    mlt_properties params = mlt_properties_get_data(image_effect, "mltofx_params", NULL);
    mlt_properties image_effect_params = mlt_properties_get_data(image_effect, "params", NULL);

    mlt_image_format requested_format = *format;
    mltofx_depths_mask plugin_support_depths = mltofx_plugin_supported_depths(image_effect);
    mltofx_components_mask plugin_support_components = mltofx_plugin_supported_components(
        image_effect);

    if ((plugin_support_depths & mltofx_depth_short)
        && (plugin_support_components & (mltofx_components_rgba | mltofx_components_rgb))
        && !(*format == mlt_image_rgba || *format == mlt_image_rgb)) {
        *format = mlt_image_rgba64;
    } else if ((plugin_support_components & mltofx_components_rgba) && !(*format == mlt_image_rgb)) {
        *format = mlt_image_rgba;
    } else {
        *format = mlt_image_rgb;
    }

    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0) {
        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_position length = mlt_filter_get_length2(filter, frame);
        int params_count = mlt_properties_count(params);

        for (int i = 0; i < params_count; ++i) {
            char *iprop_name = mlt_properties_get_name(params, i);

            char *mlt_value = mlt_properties_get(properties, iprop_name);

            if (mlt_value != NULL) {
                mlt_properties param = mlt_properties_get_data_at(params, i, NULL);

                char *type = mlt_properties_get(param, "type");
                char *widget = mlt_properties_get(param, "widget");

                if (type != NULL) {
                    if (widget != NULL
                        && (strcmp(widget, "2dpoint") == 0 || strcmp(widget, "2dsize") == 0)
                        && strcmp(type, "double") == 0) {
                        mlt_rect value = mlt_properties_anim_get_rect(properties,
                                                                      iprop_name,
                                                                      position,
                                                                      length);
                        mltofx_param_set_value(image_effect_params,
                                               iprop_name,
                                               mltofx_prop_double2d,
                                               value);
                    } else if (strcmp(type, "double") == 0) {
                        double value = mlt_properties_anim_get_double(properties,
                                                                      iprop_name,
                                                                      position,
                                                                      length);
                        mltofx_param_set_value(image_effect_params,
                                               iprop_name,
                                               mltofx_prop_double,
                                               value);
                    } else if (strcmp(type, "integer") == 0) {
                        int value
                            = mlt_properties_anim_get_int(properties, iprop_name, position, length);
                        mltofx_param_set_value(image_effect_params,
                                               iprop_name,
                                               mltofx_prop_int,
                                               value);
                    } else if (strcmp(type, "string") == 0) {
                        int value
                            = mlt_properties_anim_get_int(properties, iprop_name, position, length);
                        mltofx_param_set_value(
                            image_effect_params,
                            iprop_name,
                            mltofx_prop_int, /* for handling option choice TODO: do something better */
                            value);
                    } else if (strcmp(type, "boolean") == 0) {
                        int value
                            = mlt_properties_anim_get_int(properties, iprop_name, position, length);
                        mltofx_param_set_value(image_effect_params,
                                               iprop_name,
                                               mltofx_prop_int,
                                               value);
                    } else if (strcmp(type, "color") == 0) {
                        mlt_color value = mlt_properties_anim_get_color(properties,
                                                                        iprop_name,
                                                                        position,
                                                                        length);
                        mltofx_param_set_value(image_effect_params,
                                               iprop_name,
                                               mltofx_prop_color,
                                               value);
                    }
                }
            }
        }

        mltofx_begin_sequence_render(plugin, image_effect);

        /* According to OpenFX documentation: Note that hosts that
	   have constant sized imagery need not call this action, only
	   hosts that allow image sizes to vary need call this. */
        /* mltofx_get_region_of_definition(plugin, image_effect); */

        mltofx_get_regions_of_interest(plugin, image_effect, (double) *width, (double) *height);
        mltofx_get_clip_preferences(plugin, image_effect);

        struct mlt_image_s src_img;
        mlt_image_set_values(&src_img, *image, *format, *width, *height);

        struct mlt_image_s src_img_copy;
        mlt_image_set_values(&src_img_copy, NULL, *format, *width, *height);

        mlt_image_alloc_data(&src_img_copy);

        uint8_t *src_copy = src_img_copy.data;

        memcpy(src_copy, *image, mlt_image_calculate_size(&src_img));
        mltofx_set_source_clip_data(plugin, image_effect, src_copy, *width, *height, *format);
        mltofx_set_output_clip_data(plugin, image_effect, *image, *width, *height, *format);

        mlt_service_lock(MLT_FILTER_SERVICE(filter));
        mltofx_action_render(plugin, image_effect, *width, *height);
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));

        mlt_image_close(&src_img_copy);

        mltofx_end_sequence_render(plugin, image_effect);
    }

    if (*format != requested_format) {
        int convert_error = frame->convert_image(frame, image, format, requested_format);
        *format = requested_format;

        /* If conversion fail fallback to rgba which is widely supported */
        if (convert_error) {
            frame->convert_image(frame, image, format, mlt_image_rgba);
            *format = mlt_image_rgba;
        }
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
    mlt_properties image_effect = mlt_properties_get_data(properties, "ofx_image_effect", NULL);

    mltofx_destroy_instance(plugin, image_effect);
}

mlt_filter filter_openfx_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter this = mlt_filter_new();
    if (this != NULL) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(this);
        this->process = filter_process;
        this->close = filter_close;

        /*
	  WIP
	  resource, _pluginid are properties set to the filter properties
	  also the parameters are set in the filter properties so I need
	  to make sure that no plugin will create property with the same
	  name as 'resource', '_pluginid' and other default filter properties
	  thats why modules like jackrack use to set parameters properties based
	  on numbered labels such as 0,1,2,3,4,...etc because there is no default
	  property like that.
	*/

        mlt_properties_set(properties, "resource", arg);
        if (!strncmp(id, "openfx.", 7)) {
            mlt_properties_set(properties, "_pluginid", id + 7);

            mlt_properties pb = (mlt_properties) mlt_properties_get_data(mltofx_context, id, NULL);

            char *dli = mlt_properties_get(pb, "dli");
            int index = mlt_properties_get_int(pb, "index");

            dli[0] = 'g';

            OfxPlugin *(*GetPluginFn)(int nth) = mlt_properties_get_data(mltofx_dl, dli, NULL);

            if (GetPluginFn != NULL) {
                OfxPlugin *pt = GetPluginFn(index);
                if (pt == NULL)
                    return NULL;

                mlt_properties params = mlt_properties_new();
                mlt_properties image_effect = mltofx_fetch_params(pt, params, NULL);

                mltofx_create_instance(pt, image_effect);

                mlt_properties begin_sequence_props = mlt_properties_new();
                mlt_properties end_sequence_props = mlt_properties_new();
                mlt_properties_set_data(image_effect,
                                        "begin_sequence_props",
                                        begin_sequence_props,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                mlt_properties_set_data(image_effect,
                                        "end_sequence_props",
                                        end_sequence_props,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);

                mlt_properties get_rod_in_args = mlt_properties_new();
                mlt_properties get_rod_out_args = mlt_properties_new();
                mlt_properties_set_data(image_effect,
                                        "get_rod_in_args",
                                        get_rod_in_args,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                mlt_properties_set_data(image_effect,
                                        "get_rod_out_args",
                                        get_rod_out_args,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);

                mlt_properties get_roi_in_args = mlt_properties_new();
                mlt_properties get_roi_out_args = mlt_properties_new();
                mlt_properties_set_data(image_effect,
                                        "get_roi_in_args",
                                        get_roi_in_args,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                mlt_properties_set_data(image_effect,
                                        "get_roi_out_args",
                                        get_roi_out_args,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);

                mlt_properties get_clippref_args = mlt_properties_new();
                mlt_properties_set_data(image_effect,
                                        "get_clippref_args",
                                        get_clippref_args,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);

                mlt_properties render_in_args = mlt_properties_new();
                mlt_properties_set_data(image_effect,
                                        "render_in_args",
                                        render_in_args,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);

                mlt_properties_set_data(properties, "ofx_plugin", pt, 0, NULL, NULL);
                mlt_properties_set_data(properties,
                                        "ofx_image_effect",
                                        image_effect,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
            }
        }
    }
    return this;
}
