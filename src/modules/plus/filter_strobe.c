/*
 * filter_strobe.c -- simple strobing filter
 * Copyright (C) 2020 Martin Sandsmark <martin.sandsmark@kde.org>
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

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    if (error) {
        return error;
    }

    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    int invert = mlt_properties_anim_get_int(properties, "strobe_invert", position, length);
    int interval = mlt_properties_anim_get_int(properties, "interval", position, length);

    int do_strobe = (position % (interval + 1)) > interval / 2;

    if (invert) {
        do_strobe = !do_strobe;
    }

    if (do_strobe != 1) {
        return 0;
    }

    assert(*width >= 0);
    assert(*height >= 0);
    size_t pixelCount = *width * *height;

    if (*format == mlt_image_rgba) {
        uint8_t *bytes = *image;
        for (size_t i = 3; i < pixelCount * 4; i += 4) {
            bytes[i] = 0;
        }
        // Clear any alpha buffer that may be attached to the frame
        mlt_frame_set_alpha(frame, NULL, 0, NULL);
    } else {
        uint8_t *alpha = mlt_pool_alloc(pixelCount);
        memset(alpha, 0, pixelCount);
        mlt_frame_set_alpha(frame, alpha, pixelCount, mlt_pool_release);
    }

    return 0;
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

mlt_filter filter_strobe_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        // If strobe_invert == 1, the odd number of frames will be filtered out
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "strobe_invert", "0");
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "interval", "1");
    }
    return filter;
}
