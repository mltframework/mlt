/*
 * filter_gamma.c -- gamma filter
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_slices.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    double gamma;
    int valid;
    uint8_t lookup[256];
} gamma_lut_cache;

typedef struct
{
    uint8_t *image;
    const uint8_t *lookup;
    int width;
    int height;
} gamma_slice_desc;

static inline void gamma_apply_span(uint8_t *p, int total_bytes, const uint8_t *lookup)
{
    int x = 0;
    for (; x <= total_bytes - 16; x += 16) {
        p[x + 0] = lookup[p[x + 0]];
        p[x + 2] = lookup[p[x + 2]];
        p[x + 4] = lookup[p[x + 4]];
        p[x + 6] = lookup[p[x + 6]];
        p[x + 8] = lookup[p[x + 8]];
        p[x + 10] = lookup[p[x + 10]];
        p[x + 12] = lookup[p[x + 12]];
        p[x + 14] = lookup[p[x + 14]];
    }
    for (; x < total_bytes; x += 2) {
        p[x] = lookup[p[x]];
    }
}

static int gamma_slice_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    const gamma_slice_desc *desc = (const gamma_slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    int line_size = desc->width * 2;
    gamma_apply_span(desc->image + slice_line_start * line_size,
                     slice_height * line_size,
                     desc->lookup);
    return 0;
}

/** Do it :-).
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0) {
        // Get the gamma value
        double gamma = mlt_properties_anim_get_double(properties, "gamma", position, length);

        if (gamma != 1.0) {
            uint8_t lookup[256];
            mlt_service_lock(MLT_FILTER_SERVICE(filter));
            gamma_lut_cache *cache = mlt_properties_get_data(properties, "_lut_cache", NULL);
            if (!cache) {
                cache = calloc(1, sizeof(*cache));
                if (cache)
                    mlt_properties_set_data(properties, "_lut_cache", cache, 0, free, NULL);
            }
            if (cache && cache->valid && cache->gamma == gamma) {
                memcpy(lookup, cache->lookup, 256);
            } else {
                double exp = 1 / gamma;
                for (int i = 0; i < 256; i++)
                    lookup[i] = (uint8_t) (pow((double) i / 255.0, exp) * 255);
                if (cache) {
                    cache->gamma = gamma;
                    cache->valid = 1;
                    memcpy(cache->lookup, lookup, 256);
                }
            }
            mlt_service_unlock(MLT_FILTER_SERVICE(filter));

            int total_bytes = *width * *height * 2;
            if (total_bytes >= 8 * 1024 * 1024) {
                gamma_slice_desc desc = {
                    .image = *image,
                    .lookup = lookup,
                    .width = *width,
                    .height = *height,
                };
                mlt_slices_run_normal(0, gamma_slice_proc, &desc);
            } else {
                gamma_apply_span(*image, total_bytes, lookup);
            }
        }
    }

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_gamma_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "gamma", arg == NULL ? "1" : arg);
    }
    return filter;
}
