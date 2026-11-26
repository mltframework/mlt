/*
 * filter_tcolor.c -- tcolor filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
 * Copyright (c) 2022 Meltytech, LLC
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

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    uint8_t lut_cb[256];
    uint8_t lut_cr[256];
} slice_desc;

static int do_slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    slice_desc *desc = (slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int line_size = desc->width * 2;
    const uint8_t *lut_cb = desc->lut_cb;
    const uint8_t *lut_cr = desc->lut_cr;
    uint8_t *p = desc->image + (size_t) slice_line_start * line_size;
    const size_t total_bytes = (size_t) slice_height * line_size;
    for (size_t x = 0; x < total_bytes; x += 4) {
        p[x + 1] = lut_cb[p[x + 1]];
        p[x + 3] = lut_cr[p[x + 3]];
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
        slice_desc desc;
        double over_cr = mlt_properties_anim_get_double(properties, "oversaturate_cr", pos, len)
                         / 100.0;
        double over_cb = mlt_properties_anim_get_double(properties, "oversaturate_cb", pos, len)
                         / 100.0;
        for (int i = 0; i < 256; i++) {
            desc.lut_cb[i] = (uint8_t) CLAMP(((double) i - 127.0) * over_cb + 127.0, 0, 255);
            desc.lut_cr[i] = (uint8_t) CLAMP(((double) i - 127.0) * over_cr + 127.0, 0, 255);
        }
        desc.image = *image;
        desc.width = *width;
        desc.height = *height;
        mlt_slices_run_normal(0, do_slice_proc, &desc);
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_tcolor_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "oversaturate_cr", "190");
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "oversaturate_cb", "190");
    }
    return filter;
}
