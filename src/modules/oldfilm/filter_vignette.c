/*
 * filter_vignette.c -- vignette filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
 * Copyright (c) 2009-2022 Meltytech, LLC
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

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    double smooth;
    double radius;
    double cx, cy;
    double opacity;
    int mode;
} slice_desc;

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    slice_desc *d = (slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
    uint8_t *p = d->image + slice_line_start * d->width * 2;
    int w2 = d->cx, h2 = d->cy;
    double delta = 1.0;

    for (int y = slice_line_start; y < slice_line_start + slice_height; y++) {
        int h2_pow2 = pow(y - h2, 2.0);
        for (int x = 0; x < d->width; x++, p += 2) {
            int dx = sqrt(h2_pow2 + pow(x - w2, 2.0));
            if (d->radius - d->smooth > dx) { // center, make not darker
                continue;
            } else if (d->radius + d->smooth <= dx) { // max dark after smooth area
                delta = 0.0;
            } else {
                // linear pos from of opacity (from 0 to 1)
                delta = (d->radius + d->smooth - dx) / (2.0 * d->smooth);
                if (d->mode == 1) {
                    // make cos for smoother non linear fade
                    delta = pow(cos(((1.0 - delta) * M_PI / 2.0)), 3.0);
                }
            }
            delta = MAX(d->opacity, delta);
            p[0] = p[0] * delta;
            p[1] = (p[1] - 127.0) * delta + 127.0;
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
    mlt_filter filter = mlt_frame_pop_service(frame);
    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0 && *image) {
        mlt_properties filter_props = MLT_FILTER_PROPERTIES(filter);
        mlt_position pos = mlt_filter_get_position(filter, frame);
        mlt_position len = mlt_filter_get_length2(filter, frame);
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        double scale = mlt_profile_scale_width(profile, *width);
        slice_desc desc
            = {.image = *image,
               .width = *width,
               .height = *height,
               .smooth = mlt_properties_anim_get_double(filter_props, "smooth", pos, len) * 100.0
                         * scale,
               .radius = mlt_properties_anim_get_double(filter_props, "radius", pos, len) * *width,
               .cx = mlt_properties_anim_get_double(filter_props, "x", pos, len) * *width,
               .cy = mlt_properties_anim_get_double(filter_props, "y", pos, len) * *height,
               .opacity = mlt_properties_anim_get_double(filter_props, "opacity", pos, len),
               .mode = mlt_properties_get_int(filter_props, "mode")};
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

mlt_filter filter_vignette_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "smooth", 0.8);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "radius", 0.5);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "x", 0.5);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "y", 0.5);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "opacity", 0.0);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "mode", 0);
    }
    return filter;
}
