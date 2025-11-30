/*
 * filter_frei0r.c -- frei0r filter
 * Copyright (c) 2008 Marco Gittler <g.marco@freenet.de>
 * Copyright (C) 2009-2025 Meltytech, LLC
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
#include <framework/mlt_pool.h>
#include <framework/mlt_slices.h>

#include "frei0r_helper.h"
#include <string.h>

typedef struct
{
    uint8_t *rgba_image;
    uint16_t *rgba64_image;
    int width;
    int height;
} alpha_copy_desc;

static int copy_alpha_rgba_to_rgba64_slice(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const alpha_copy_desc *desc = (const alpha_copy_desc *) data;
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int slice_pixels = desc->width * slice_height;
    const uint8_t *src = desc->rgba_image + slice_line_start * desc->width * 4;
    uint16_t *dst = desc->rgba64_image + slice_line_start * desc->width * 4;

    for (int i = 0; i < slice_pixels; i++) {
        // Convert 8-bit to 16-bit
        dst[i * 4 + 3] = ((uint16_t) src[i * 4 + 3]) * 257; // 257 = 65535 / 255
    }

    return 0;
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
    int alpha_only = mlt_properties_get_int(filter_props, "_alpha_only");

    if (alpha_only) {
        // Except when using alpha0ps to view the alpha channel
        const char *service_name = mlt_properties_get(filter_props, "mlt_service");
        if (service_name && !strcmp(service_name, "frei0r.alpha0ps"))
            alpha_only = mlt_properties_get_double(filter_props, "0") < 0.2;
    }
    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "frei0r filter_get_image called for format %d (%s), alpha_only=%d\n",
                  *format,
                  mlt_image_format_name(*format),
                  alpha_only);

    // Check if we need special rgba64 handling for alpha-only filters
    if (alpha_only && *format == mlt_image_rgba64) {
        // First get the rgba64 image
        int error = mlt_frame_get_image(frame, image, format, width, height, 0);

        // Now request the same image as rgba using convert_image
        if (!error && *image && *format == mlt_image_rgba64 && frame->convert_image) {
            // Convert to rgba
            mlt_frame temp_frame = mlt_frame_clone(frame, 0);
            uint8_t *rgba64_image = *image;
            error = frame->convert_image(temp_frame, image, format, mlt_image_rgba);
            if (!error && *image && *format == mlt_image_rgba) {
                // Process with frei0r using rgba image
                mlt_position position = mlt_filter_get_position(filter, frame);
                mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
                double time = (double) position / mlt_profile_fps(profile);
                int length = mlt_filter_get_length2(filter, frame);

                error = process_frei0r_item(MLT_FILTER_SERVICE(filter),
                                            position,
                                            time,
                                            length,
                                            temp_frame,
                                            image,
                                            width,
                                            height);

                if (!error) {
                    // Copy alpha channel from rgba to rgba64
                    alpha_copy_desc desc = {.rgba_image = *image,
                                            .rgba64_image = (uint16_t *) rgba64_image,
                                            .width = *width,
                                            .height = *height};
                    mlt_slices_run_normal(0, copy_alpha_rgba_to_rgba64_slice, &desc);
                }
            } else {
                mlt_log_warning(MLT_FILTER_SERVICE(filter),
                                "alpha-only frei0r filter failed to convert frame to rgba\n");
            }
            mlt_frame_close(temp_frame);
            *image = rgba64_image;
            *format = mlt_image_rgba64;
        } else {
            mlt_log_warning(MLT_FILTER_SERVICE(filter),
                            "alpha-only frei0r filter failed to get rgba64 image from frame\n");
            error = 1;
        }

        return error;
    }

    // Normal processing for non-rgba64 or non-alpha-only filters
    *format = mlt_image_rgba;
    mlt_log_debug(MLT_FILTER_SERVICE(filter), "frei0r %dx%d\n", *width, *height);
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0 && *image) {
        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        double time = (double) position / mlt_profile_fps(profile);
        int length = mlt_filter_get_length2(filter, frame);
        process_frei0r_item(MLT_FILTER_SERVICE(filter),
                            position,
                            time,
                            length,
                            frame,
                            image,
                            width,
                            height);
    }

    return error;
}

mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

void filter_close(mlt_filter filter)
{
    destruct(MLT_FILTER_PROPERTIES(filter));
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}
