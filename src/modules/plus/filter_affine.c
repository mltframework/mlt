/*
 * filter_affine.c -- affine filter
 * Copyright (C) 2003-2021 Meltytech, LLC
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

#include <framework/mlt.h>
#include <framework/mlt_filter.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MLT_AFFINE_COUNT_PROPERTY "filter_affine.count"

/** Do it :-).
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    // Get the filter
    mlt_filter filter = mlt_frame_pop_service(frame);

    // Get the properties
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

    // Get the image
    int error = 0;
    *format = mlt_image_rgba;

    mlt_service_lock(MLT_FILTER_SERVICE(filter));
    mlt_producer producer = mlt_properties_get_data(properties, "producer", NULL);
    mlt_transition transition = mlt_properties_get_data(properties, "transition", NULL);
    mlt_frame a_frame = NULL;
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    char *background = mlt_properties_get(properties, "background");
    char *previous = mlt_properties_get(properties, "_background");

    if (producer == NULL || (background && previous && strcmp(background, previous))) {
        producer = mlt_factory_producer(profile, NULL, background);
        mlt_properties_set_data(properties,
                                "producer",
                                producer,
                                0,
                                (mlt_destructor) mlt_producer_close,
                                NULL);
        mlt_properties_set(properties, "_background", background);
    }

    if (transition == NULL) {
        transition = mlt_factory_transition(profile, "affine", NULL);
        mlt_properties_set_data(properties,
                                "transition",
                                transition,
                                0,
                                (mlt_destructor) mlt_transition_close,
                                NULL);
        if (transition)
            mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "b_alpha", 1);
    }

    if (producer != NULL && transition != NULL) {
        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
        mlt_position in = mlt_filter_get_in(filter);
        mlt_position out = mlt_filter_get_out(filter);
        double consumer_ar = mlt_profile_sar(profile);
        mlt_transition_set_in_and_out(transition, in, out);
        if (out > 0) {
            mlt_properties_set_position(MLT_PRODUCER_PROPERTIES(producer), "length", out - in + 1);
            mlt_producer_set_in_and_out(producer, in, out);
        }
        mlt_producer_seek(producer, in + position);
        mlt_properties_pass(MLT_PRODUCER_PROPERTIES(producer), properties, "producer.");
        mlt_properties_pass(MLT_TRANSITION_PROPERTIES(transition), properties, "transition.");
        mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &a_frame, 0);
        mlt_frame_set_position(a_frame, in + position);

        // Set the rescale interpolation to match the frame
        mlt_properties_set(MLT_FRAME_PROPERTIES(a_frame),
                           "consumer.rescale",
                           mlt_properties_get(frame_properties, "consumer.rescale"));

        // Special case - aspect_ratio = 0
        if (mlt_frame_get_aspect_ratio(frame) == 0)
            mlt_frame_set_aspect_ratio(frame, consumer_ar);
        if (mlt_frame_get_aspect_ratio(a_frame) == 0)
            mlt_frame_set_aspect_ratio(a_frame, consumer_ar);

        // Add the affine transition onto the frame stack
        mlt_transition_process(transition, a_frame, frame);

        if (mlt_properties_get_int(properties, "use_normalized")
            || mlt_properties_get_int(properties, "use_normalized")) {
            // Use the normalized width & height
            mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
            *width = profile->width;
            *height = profile->height;
        }

        // Prescale the image before applying affine filters if more than 1
        if (mlt_properties_get_int(frame_properties, MLT_AFFINE_COUNT_PROPERTY) > 1) {
            mlt_properties_set_int(frame_properties, "always_scale", 1);
        }

        mlt_frame_get_image(a_frame, image, format, width, height, writable);
        mlt_properties_set_data(frame_properties,
                                "affine_frame",
                                a_frame,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);
        mlt_frame_set_image(frame, *image, *width * *height * 4, NULL);
        uint8_t *alpha = mlt_frame_get_alpha(a_frame);
        if (alpha) {
            mlt_frame_set_alpha(frame, alpha, *width * *height, NULL);
        }
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));
    } else {
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));
    }

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Push the frame filter
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);

    // Count the number of affine filters on this frame
    if (mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), MLT_AFFINE_COUNT_PROPERTY)) {
        int count = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), MLT_AFFINE_COUNT_PROPERTY);
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), MLT_AFFINE_COUNT_PROPERTY, count + 1);
    } else {
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), MLT_AFFINE_COUNT_PROPERTY, 1);
    }

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_affine_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "background", arg ? arg : "colour:0");
    }
    return filter;
}
