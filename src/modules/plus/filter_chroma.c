/*
 * filter_chroma.c -- Maps a chroma key to the alpha channel
 * Copyright (C) 2007-2025 Meltytech, LLC
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_slices.h>

#include <stdlib.h>
#include <string.h>

typedef struct
{
    const uint8_t *image;
    uint8_t *alpha;
    int width;
    int height;
    int u_min;
    int u_max;
    int v_min;
    int v_max;
    int fresh_alpha;
} chroma_slice_desc;

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    const chroma_slice_desc *desc = (const chroma_slice_desc *) data;
    int slice_line_start;
    int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    int pairs = (desc->width * slice_height) / 2;
    if (pairs <= 0)
        return 0;

    const uint8_t *restrict p = desc->image + (size_t) slice_line_start * desc->width * 2;
    uint8_t *restrict alpha = desc->alpha + (size_t) slice_line_start * desc->width;
    const int u_min = desc->u_min;
    const int u_max = desc->u_max;
    const int v_min = desc->v_min;
    const int v_max = desc->v_max;

    if (desc->fresh_alpha) {
        for (int i = 0; i < pairs; i++) {
            int u0 = p[1];
            int v0 = p[3];
            int u1 = (u0 + p[5]) >> 1;
            int v1 = (v0 + p[7]) >> 1;
            int k0 = (u0 >= u_min) & (u0 <= u_max) & (v0 >= v_min) & (v0 <= v_max);
            int k1 = (u1 >= u_min) & (u1 <= u_max) & (v1 >= v_min) & (v1 <= v_max);
            alpha[0] = k0 ? 0 : 255;
            alpha[1] = k1 ? 0 : 255;
            alpha += 2;
            p += 4;
        }
    } else {
        for (int i = 0; i < pairs; i++) {
            int u0 = p[1];
            int v0 = p[3];
            int u1 = (u0 + p[5]) >> 1;
            int v1 = (v0 + p[7]) >> 1;
            int k0 = (u0 >= u_min) & (u0 <= u_max) & (v0 >= v_min) & (v0 <= v_max);
            int k1 = (u1 >= u_min) & (u1 <= u_max) & (v1 >= v_min) & (v1 <= v_max);
            if (k0)
                alpha[0] = 0;
            if (k1)
                alpha[1] = 0;
            alpha += 2;
            p += 4;
        }
    }
    return 0;
}

/** Get the images and map the chroma to the alpha of the frame.
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter this = mlt_frame_pop_service(frame);
    mlt_position position = mlt_filter_get_position(this, frame);
    mlt_position length = mlt_filter_get_length2(this, frame);
    int variance = 200
                   * mlt_properties_anim_get_double(MLT_FILTER_PROPERTIES(this),
                                                    "variance",
                                                    position,
                                                    length);
    mlt_color key_val
        = mlt_properties_anim_get_color(MLT_FILTER_PROPERTIES(this), "key", position, length);
    key_val = mlt_color_convert_trc(key_val,
                                    mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc"));
    uint8_t u, v;

    RGB2UV_601_SCALED(key_val.r, key_val.g, key_val.b, u, v);

    *format = mlt_image_yuv422;
    if (mlt_frame_get_image(frame, image, format, width, height, writable) == 0) {
        uint8_t *alpha = mlt_frame_get_alpha(frame);
        int fresh_alpha = 0;
        if (!alpha) {
            int alphasize = *width * *height;
            alpha = mlt_pool_alloc(alphasize);
            mlt_frame_set_alpha(frame, alpha, alphasize, mlt_pool_release);
            fresh_alpha = 1;
            if ((*width * *height) & 1) {
                alpha[alphasize - 1] = 255;
            }
        }
        chroma_slice_desc desc = {
            .image = *image,
            .alpha = alpha,
            .width = *width,
            .height = *height,
            .u_min = (int) u - variance,
            .u_max = (int) u + variance,
            .v_min = (int) v - variance,
            .v_max = (int) v + variance,
            .fresh_alpha = fresh_alpha,
        };
        mlt_slices_run_normal(0, sliced_proc, &desc);
    }

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter this, mlt_frame frame)
{
    mlt_frame_push_service(frame, this);
    mlt_frame_push_service(frame, filter_get_image);
    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_chroma_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter this = mlt_filter_new();
    if (this != NULL) {
        mlt_properties_set(MLT_FILTER_PROPERTIES(this), "key", arg == NULL ? "#0000ff" : arg);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(this), "variance", 0.15);
        this->process = filter_process;
    }
    return this;
}
