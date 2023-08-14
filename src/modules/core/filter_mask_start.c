/*
 * filter_mask_start.c -- clone a frame before invoking a filter
 * Copyright (C) 2018-2022 Meltytech, LLC
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
#include <framework/mlt_log.h>

#include <string.h>

static int get_image(mlt_frame frame,
                     uint8_t **image,
                     mlt_image_format *format,
                     int *width,
                     int *height,
                     int writable)
{
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    const char *name = mlt_properties_get(filter_properties, "filter");
    int error = mlt_frame_get_image(frame, image, format, width, height, writable);
    if (!error && name && strcmp("", name)) {
        mlt_frame clone = mlt_frame_clone(frame, 1);
        clone->convert_audio = frame->convert_audio;
        clone->convert_image = frame->convert_image;

        mlt_service_lock(MLT_FILTER_SERVICE(filter));
        mlt_filter instance = mlt_properties_get_data(filter_properties, "instance", NULL);
        // Create the filter if needed.
        if (!instance || !mlt_properties_get(MLT_FILTER_PROPERTIES(instance), "mlt_service")
            || strcmp(name, mlt_properties_get(MLT_FILTER_PROPERTIES(instance), "mlt_service"))) {
            mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
            instance = mlt_factory_filter(profile, name, NULL);
            mlt_properties_set_data(filter_properties,
                                    "instance",
                                    instance,
                                    0,
                                    (mlt_destructor) mlt_filter_close,
                                    NULL);
        }
        if (instance) {
            mlt_properties instance_props = MLT_FILTER_PROPERTIES(instance);
            mlt_properties_pass_list(instance_props, filter_properties, "in out");
            mlt_properties_pass(instance_props, filter_properties, "filter.");
            mlt_properties_clear(filter_properties, "filter.producer.refresh");
            mlt_filter_process(instance, clone);
        } else {
            mlt_properties_debug(filter_properties, "failed to create filter", stderr);
        }
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));

        // Get the image from the clone frame to generate the mask
        uint8_t *cimage = NULL;
        mlt_image_format cformat = mlt_image_rgba;
        int cwidth = *width;
        int cheight = *height;
        error = mlt_frame_get_image(clone, &cimage, &cformat, &cwidth, &cheight, 1);
        if (!error && cimage) {
            // Invert the alpha mask
            for (int i = 0; i < cwidth * cheight * 4; i += 4) {
                cimage[i + 3] = 255 - cimage[i + 3];
            }

            mlt_properties_set_data(frame_properties,
                                    "mask frame",
                                    clone,
                                    0,
                                    (mlt_destructor) mlt_frame_close,
                                    NULL);
        } else {
            mlt_frame_close(clone);
        }
    }
    return error;
}

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, get_image);
    return frame;
}

mlt_filter filter_mask_start_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "filter", arg ? arg : "frei0r.alphaspot");
        filter->process = process;
    }
    return filter;
}
