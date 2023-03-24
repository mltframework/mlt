/*
 * filter_charcoal.c -- charcoal filter
 * Copyright (C) 2003-2022 Meltytech, LLC
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
#include <framework/mlt_profile.h>
#include <framework/mlt_slices.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static inline int get_Y(uint8_t *pixels, int width, int height, int x, int y, int max_luma)
{
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return max_luma;
    } else {
        uint8_t *pixel = pixels + y * (width << 1) + (x << 1);
        return *pixel;
    }
}

static inline int sqrti(int n)
{
    int p = 0;
    int q = 1;
    int r = n;
    int h = 0;

    while (q <= n)
        q = q << 2;

    while (q != 1) {
        q = q >> 2;
        h = p + q;
        p = p >> 1;
        if (r >= h) {
            p = p + q;
            r = r - h;
        }
    }

    return p;
}

typedef struct
{
    uint8_t *image;
    uint8_t *dest;
    int width;
    int height;
    int x_scatter;
    int y_scatter;
    int min;
    int max_luma;
    int max_chroma;
    int invert;
    int invert_luma;
    float scale;
    float mix;
} slice_desc;

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    slice_desc *d = (slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
    uint8_t *p = d->dest + slice_line_start * d->width * 2;
    uint8_t *q = d->image + slice_line_start * d->width * 2;
    int matrix[3][3];
    int sum1;
    int sum2;
    float sum;
    int val;

    for (int y = slice_line_start; y < slice_line_start + slice_height; y++) {
        for (int x = 0; x < d->width; x++) {
            // Populate the matrix
            matrix[0][0] = get_Y(d->image,
                                 d->width,
                                 d->height,
                                 x - d->x_scatter,
                                 y - d->y_scatter,
                                 d->max_luma);
            matrix[0][1] = get_Y(d->image, d->width, d->height, x, y - d->y_scatter, d->max_luma);
            matrix[0][2] = get_Y(d->image,
                                 d->width,
                                 d->height,
                                 x + d->x_scatter,
                                 y - d->y_scatter,
                                 d->max_luma);
            matrix[1][0] = get_Y(d->image, d->width, d->height, x - d->x_scatter, y, d->max_luma);
            matrix[1][2] = get_Y(d->image, d->width, d->height, x + d->x_scatter, y, d->max_luma);
            matrix[2][0] = get_Y(d->image,
                                 d->width,
                                 d->height,
                                 x - d->x_scatter,
                                 y + d->y_scatter,
                                 d->max_luma);
            matrix[2][1] = get_Y(d->image, d->width, d->height, x, y + d->y_scatter, d->max_luma);
            matrix[2][2] = get_Y(d->image,
                                 d->width,
                                 d->height,
                                 x + d->x_scatter,
                                 y + d->y_scatter,
                                 d->max_luma);

            // Do calculations
            sum1 = (matrix[2][0] - matrix[0][0]) + ((matrix[2][1] - matrix[0][1]) << 1)
                   + (matrix[2][2] - matrix[2][0]);
            sum2 = (matrix[0][2] - matrix[0][0]) + ((matrix[1][2] - matrix[1][0]) << 1)
                   + (matrix[2][2] - matrix[2][0]);
            sum = d->scale * sqrti(sum1 * sum1 + sum2 * sum2);

            // Assign value
            *p++ = !d->invert ? (sum >= d->min && sum <= d->max_luma ? d->invert_luma - sum
                                 : sum < d->min                      ? d->max_luma
                                                                     : d->min)
                              : (sum >= d->min && sum <= d->max_luma ? sum
                                 : sum < d->min                      ? d->min
                                                                     : d->max_luma);
            q++;
            val = 128 + d->mix * (*q++ - 128);
            val = CLAMP(val, d->min, d->max_chroma);
            *p++ = val;
        }
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
    // Get the filter
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    // Get the image
    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 0);

    // Only process if we have no error and a valid colour space
    if (error == 0) {
        int size = *width * *height * 2;
        int full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
        // Get the charcoal scatter value
        int x_scatter = mlt_properties_anim_get_double(properties, "x_scatter", position, length);
        int y_scatter = mlt_properties_anim_get_double(properties, "y_scatter", position, length);
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        double scale_x = mlt_profile_scale_width(profile, *width);
        double scale_y = mlt_profile_scale_height(profile, *height);
        if (scale_x > 0.0 || scale_y > 0.0) {
            x_scatter = MAX(1, lrint(x_scatter * scale_x));
            y_scatter = MAX(1, lrint(y_scatter * scale_y));
        }

        slice_desc desc
            = {// We need to create a new frame as this effect modifies the input
               .image = *image,
               .dest = mlt_pool_alloc(size),
               .width = *width,
               .height = *height,
               .x_scatter = x_scatter,
               .y_scatter = y_scatter,
               .min = full_range ? 0 : 16,
               .max_luma = full_range ? 255 : 235,
               .max_chroma = full_range ? 255 : 240,
               .invert = mlt_properties_anim_get_int(properties, "invert", position, length),
               .invert_luma = full_range ? 255 : 251,
               .scale = mlt_properties_anim_get_double(properties, "scale", position, length),
               .mix = mlt_properties_anim_get_double(properties, "mix", position, length)};
        mlt_slices_run_normal(0, slice_proc, &desc);

        // Return the created image
        *image = desc.dest;

        // Store new and destroy old
        mlt_frame_set_image(frame, *image, size, mlt_pool_release);
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

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_charcoal_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "x_scatter", 1);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "y_scatter", 1);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "scale", 1.5);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "mix", 0.0);
    }
    return filter;
}
