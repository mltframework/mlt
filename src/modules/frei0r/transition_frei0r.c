/*
 * transition_frei0r.c -- frei0r transition
 * Copyright (c) 2008 Marco Gittler <g.marco@freenet.de>
 * Copyright (C) 2009-2020 Meltytech, LLC
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

#include "frei0r_helper.h"
#include <framework/mlt.h>
#include <string.h>

static int transition_get_image(mlt_frame a_frame,
                                uint8_t **image,
                                mlt_image_format *format,
                                int *width,
                                int *height,
                                int writable)
{
    mlt_frame b_frame = mlt_frame_pop_frame(a_frame);
    mlt_transition transition = mlt_frame_pop_service(a_frame);
    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);
    mlt_properties a_props = MLT_FRAME_PROPERTIES(a_frame);
    mlt_properties b_props = MLT_FRAME_PROPERTIES(b_frame);
    int invert = mlt_properties_get_int(properties, "invert");
    uint8_t *images[] = {NULL, NULL, NULL};
    int request_width = *width;
    int request_height = *height;
    int error = 0;

    // Get the B-frame.
    *format = mlt_image_rgba;
    error = mlt_frame_get_image(b_frame, &images[1], format, width, height, 0);
    if (error)
        return error;

    if (b_frame->convert_image && (*width != request_width || *height != request_height)) {
        mlt_properties_set_int(b_props, "convert_image_width", request_width);
        mlt_properties_set_int(b_props, "convert_image_height", request_height);
        b_frame->convert_image(b_frame, &images[1], format, *format);
        *width = request_width;
        *height = request_height;
    }

    const char *service_name = mlt_properties_get(properties, "mlt_service");
    int is_cairoblend = service_name && !strcmp("frei0r.cairoblend", service_name);
    const char *blend_mode = mlt_properties_get(b_props, CAIROBLEND_MODE_PROPERTY);

    // An optimization for cairoblend in normal (over) mode and opaque B frame.
    if (is_cairoblend
        && (!mlt_properties_get(properties, "0")
            || mlt_properties_get_double(properties, "0") == 1.0)
        && (!mlt_properties_get(properties, "1")
            || !strcmp("normal", mlt_properties_get(properties, "1")))
        && (!blend_mode || !strcmp("normal", blend_mode))
        // Check if the alpha channel is entirely opaque.
        && mlt_image_rgba_opaque(images[1], *width, *height)) {
        if (invert) {
            error = mlt_frame_get_image(a_frame, image, format, width, height, 0);
        } else {
            // Pass all required frame properties
            mlt_properties_pass_list(a_props,
                                     b_props,
                                     "progressive,distort,colorspace,full_range,force_full_luma,"
                                     "top_field_first,color_trc");
            *image = images[1];
        }
    } else {
        error = mlt_frame_get_image(a_frame, &images[0], format, width, height, 0);
        if (error)
            return error;

        if (a_frame->convert_image && (*width != request_width || *height != request_height)) {
            mlt_properties_set_int(a_props, "convert_image_width", request_width);
            mlt_properties_set_int(a_props, "convert_image_height", request_height);
            a_frame->convert_image(a_frame, &images[0], format, *format);
            *width = request_width;
            *height = request_height;
        }

        mlt_position position = mlt_transition_get_position(transition, a_frame);
        mlt_profile profile = mlt_service_profile(MLT_TRANSITION_SERVICE(transition));
        double time = (double) position / mlt_profile_fps(profile);
        int length = mlt_transition_get_length(transition);

        // Special cairoblend handling for an override from the cairoblend_mode filter.
        if (is_cairoblend) {
            mlt_properties_set(a_props, CAIROBLEND_MODE_PROPERTY, blend_mode);
        }

        process_frei0r_item(MLT_TRANSITION_SERVICE(transition),
                            position,
                            time,
                            length,
                            !invert ? a_frame : b_frame,
                            images,
                            width,
                            height);

        *width = mlt_properties_get_int(!invert ? a_props : b_props, "width");
        *height = mlt_properties_get_int(!invert ? a_props : b_props, "height");
        *image = mlt_properties_get_data(!invert ? a_props : b_props, "image", NULL);
    }
    return error;
}

mlt_frame transition_process(mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame)
{
    mlt_frame_push_service(a_frame, transition);
    mlt_frame_push_frame(a_frame, b_frame);
    mlt_frame_push_get_image(a_frame, transition_get_image);
    return a_frame;
}

void transition_close(mlt_transition transition)
{
    destruct(MLT_TRANSITION_PROPERTIES(transition));
    transition->close = NULL;
    mlt_transition_close(transition);
}
