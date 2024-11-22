/*
 * filter_freeze.c -- simple frame freezing filter
 * Copyright (C) 2007 Jean-Baptiste Mardelle <jb@kdenlive.org>
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

#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_property.h>
#include <framework/mlt_service.h>
#include <framework/mlt_tokeniser.h>

#include <stdio.h>
#include <string.h>

static mlt_properties normalizers = NULL;

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    // Get the image
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);

    mlt_frame freeze_frame = NULL;
    ;
    int freeze_before = mlt_properties_get_int(properties, "freeze_before");
    int freeze_after = mlt_properties_get_int(properties, "freeze_after");
    mlt_position pos = mlt_properties_get_position(properties, "frame")
                       + mlt_producer_get_in(mlt_frame_get_original_producer(frame));
    mlt_position currentpos = mlt_filter_get_position(filter, frame);

    int do_freeze = 0;
    if (freeze_before == 0 && freeze_after == 0) {
        do_freeze = 1;
    } else if (freeze_before != 0 && pos > currentpos) {
        do_freeze = 1;
    } else if (freeze_after != 0 && pos < currentpos) {
        do_freeze = 1;
    }

    if (do_freeze == 1) {
        mlt_service_lock(MLT_FILTER_SERVICE(filter));
        freeze_frame = mlt_properties_get_data(properties, "freeze_frame", NULL);
        if (!freeze_frame || mlt_properties_get_position(properties, "_frame") != pos) {
            // freeze_frame has not been fetched yet or is not useful, so fetch it and cache it.
            // get parent producer
            mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
            mlt_producer_seek(producer, pos);

            // Get the frame
            mlt_service service = MLT_PRODUCER_SERVICE(producer);
            mlt_service_get_frame(service, &freeze_frame, 0);

            mlt_properties freeze_properties = MLT_FRAME_PROPERTIES(freeze_frame);
            mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);

            mlt_properties_set(freeze_properties,
                               "consumer.rescale",
                               mlt_properties_get(frame_properties, "consumer.rescale"));
            mlt_properties_set_double(freeze_properties,
                                      "aspect_ratio",
                                      mlt_frame_get_aspect_ratio(frame));
            mlt_properties_set_int(freeze_properties,
                                   "progressive",
                                   mlt_properties_get_int(props, "progressive"));
            mlt_properties_set_int(freeze_properties,
                                   "consumer.progressive",
                                   mlt_properties_get_int(frame_properties, "consumer.progressive"));
            mlt_properties_set_data(properties,
                                    "freeze_frame",
                                    freeze_frame,
                                    0,
                                    (mlt_destructor) mlt_frame_close,
                                    NULL);
            mlt_properties_set_position(properties, "_frame", pos);
            // Check if we have normalizers
            int hasNormalizers = 0;
            for (int i = 0; i < mlt_service_filter_count(service); i++) {
                mlt_filter filter = mlt_service_filter(service, i);
                if (filter && mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_loader") == 1) {
                    hasNormalizers = 1;
                    break;
                }
            }
            if (hasNormalizers == 0) {
                // Tokeniser
                mlt_tokeniser tokeniser = mlt_tokeniser_init();

                // We only need to load the normalizing properties once
                if (normalizers == NULL) {
                    char temp[PATH_MAX];
                    snprintf(temp, sizeof(temp), "%s/core/loader.ini", mlt_environment("MLT_DATA"));
                    normalizers = mlt_properties_load(temp);
                    mlt_factory_register_for_clean_up(normalizers, (mlt_destructor) mlt_properties_close);
                }

                // Apply normalizers
                for (int i = 0; i < mlt_properties_count(normalizers); i++) {
                    int j = 0;
                    int created = 0;
                    char *value = mlt_properties_get_value(normalizers, i);
                    mlt_tokeniser_parse_new(tokeniser, value, ",");
                    for (j = 0; !created && j < mlt_tokeniser_count(tokeniser); j++) {
                        const char *filter_name = mlt_tokeniser_get_string(tokeniser, j);
                        mlt_filter norm_filter = mlt_factory_filter(mlt_service_profile(MLT_FILTER_SERVICE(filter)), filter_name, NULL);
                        mlt_filter_process(norm_filter, freeze_frame);
                    }
                }

                // Close the tokeniser
                mlt_tokeniser_close(tokeniser);
            }
        }
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));

        // Get frozen image
        uint8_t *buffer = NULL;
        int error = mlt_frame_get_image(freeze_frame, &buffer, format, width, height, 1);

        // Copy it to current frame
        int size = mlt_image_format_size(*format, *width, *height, NULL);
        uint8_t *image_copy = mlt_pool_alloc(size);
        memcpy(image_copy, buffer, size);
        *image = image_copy;
        mlt_frame_set_image(frame, *image, size, mlt_pool_release);

        uint8_t *alpha_buffer = mlt_frame_get_alpha(freeze_frame);

        if (alpha_buffer) {
            int alphasize = *width * *height;
            uint8_t *alpha_copy = mlt_pool_alloc(alphasize);
            memcpy(alpha_copy, alpha_buffer, alphasize);
            mlt_frame_set_alpha(frame, alpha_copy, alphasize, mlt_pool_release);
        }
        return error;
    }

    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Push the filter on to the stack
    mlt_frame_push_service(frame, filter);

    // Push the frame filter
    mlt_frame_push_get_image(frame, filter_get_image);

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_freeze_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        // Set the frame which will be chosen for freeze
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "frame", "0");

        // If freeze_after = 1, only frames after the "frame" value will be frozen
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "freeze_after", "0");

        // If freeze_before = 1, only frames before the "frame" value will be frozen
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "freeze_before", "0");
    }
    return filter;
}
