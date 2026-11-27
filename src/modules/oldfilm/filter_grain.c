/*
 * filter_grain.c -- grain filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
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

#include "common.h"
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_slices.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    int noise;
    double contrast;
    double brightness;
    mlt_position pos;
    int min;
    int max_luma;
    uint8_t base_lut[256];
} slice_desc;

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    slice_desc *d = (slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
    uint8_t *p = d->image + (size_t) slice_line_start * d->width * 2;
    const int count = slice_height * d->width;
    const int noise = d->noise;
    const int min_luma = d->min;
    const int max_luma = d->max_luma;
    const uint8_t *lut = d->base_lut;

    if (noise > 0) {
        oldfilm_rand_seed seed;
        oldfilm_init_seed(&seed, d->pos * jobs + index);
        unsigned int sx = seed.x;
        unsigned int sy = seed.y;
#if defined(__SIZEOF_INT128__)
        const uint64_t inv_noise = ((UINT64_C(1) << 63) + (uint64_t) noise - 1) / (uint64_t) noise;
#endif
        for (int n = 0; n < count; n++, p += 2) {
            uint8_t y = p[0];
            if (y > 20) {
                int pix = lut[y];
                sx = 18000u * (sx & 65535u) + (sx >> 16);
                sy = 30903u * (sy & 65535u) + (sy >> 16);
                int r = (int) ((sx << 16) + (sy & 65535u));
                int ar = abs(r);
#if defined(__SIZEOF_INT128__)
                uint32_t q = (uint32_t) (((unsigned __int128) (uint32_t) ar * inv_noise) >> 63);
                int rem = ar - (int) (q * (uint32_t) noise);
#else
                int rem = ar % noise;
#endif
                pix -= rem - noise;
                p[0] = (uint8_t) CLAMP(pix, min_luma, max_luma);
            }
        }
    } else {
        for (int n = 0; n < count; n++, p += 2) {
            uint8_t y = p[0];
            if (y > 20) {
                p[0] = (uint8_t) CLAMP((int) lut[y], min_luma, max_luma);
            }
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
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position pos = mlt_filter_get_position(filter, frame);
    mlt_position len = mlt_filter_get_length2(filter, frame);

    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0 && *image) {
        int noise = mlt_properties_anim_get_int(properties, "noise", pos, len);
        int full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
        slice_desc desc = {
            .image = *image,
            .width = *width,
            .height = *height,
            .noise = noise,
            .contrast = mlt_properties_anim_get_double(properties, "contrast", pos, len) / 100.0,
            .brightness = 127.0
                          * (mlt_properties_anim_get_double(properties, "brightness", pos, len)
                             - 100.0)
                          / 100.0,
            .pos = pos,
            .min = full_range ? 0 : 16,
            .max_luma = full_range ? 255 : 235,
        };
        for (int i = 0; i < 256; i++) {
            desc.base_lut[i] = (uint8_t)
                CLAMP(((double) i - 127.0) * desc.contrast + 127.0 + desc.brightness, 0, 255);
        }
        mlt_slices_run_normal(0, slice_proc, &desc);
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_grain_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "noise", "40");
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "contrast", "160");
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "brightness", "70");
    }
    return filter;
}
