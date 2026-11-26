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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    int w2;
    int h2;
    int inner_dx;
    int outer_dx;
    const double *delta_lut;
    uint8_t outer_y_lut[256];
    uint8_t outer_c_lut[256];
} slice_desc;

static inline void apply_outer_span(uint8_t *restrict p,
                                    int count,
                                    const uint8_t *restrict y_lut,
                                    const uint8_t *restrict c_lut)
{
    int i = 0;
    for (; i + 4 <= count; i += 4, p += 8) {
        p[0] = y_lut[p[0]];
        p[1] = c_lut[p[1]];
        p[2] = y_lut[p[2]];
        p[3] = c_lut[p[3]];
        p[4] = y_lut[p[4]];
        p[5] = c_lut[p[5]];
        p[6] = y_lut[p[6]];
        p[7] = c_lut[p[7]];
    }
    for (; i < count; i++, p += 2) {
        p[0] = y_lut[p[0]];
        p[1] = c_lut[p[1]];
    }
}

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *d = (const slice_desc *) data;
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
    const int width = d->width;
    const int w2 = d->w2;
    const int h2 = d->h2;
    const int inner_dx = d->inner_dx;
    const int outer_dx = d->outer_dx;
    const int64_t inner_sq = (int64_t) inner_dx * inner_dx;
    const int64_t outer_sq = (int64_t) outer_dx * outer_dx;
    const double *restrict delta_lut = d->delta_lut;
    const uint8_t *restrict outer_y_lut = d->outer_y_lut;
    const uint8_t *restrict outer_c_lut = d->outer_c_lut;

    for (int y = slice_line_start; y < slice_line_start + slice_height; y++) {
        uint8_t *restrict row = d->image + (size_t) y * width * 2;
        const int64_t dy = (int64_t) y - h2;
        const int64_t dy2 = dy * dy;

        if (dy2 >= outer_sq) {
            /* Entire row is outside outer_dx: constant outer opacity LUT */
            apply_outer_span(row, width, outer_y_lut, outer_c_lut);
            continue;
        }

        /* Compute horizontal half-width of outer boundary: dy2 + rx^2 < outer_sq */
        int outer_rx = (int) sqrt((double) (outer_sq - 1 - dy2));
        while ((int64_t) (outer_rx + 1) * (outer_rx + 1) + dy2 < outer_sq)
            outer_rx++;
        while (outer_rx >= 0 && (int64_t) outer_rx * outer_rx + dy2 >= outer_sq)
            outer_rx--;

        int x_ring_l = w2 - outer_rx;
        int x_ring_r = w2 + outer_rx + 1;
        if (x_ring_l < 0)
            x_ring_l = 0;
        if (x_ring_l > width)
            x_ring_l = width;
        if (x_ring_r < 0)
            x_ring_r = 0;
        if (x_ring_r > width)
            x_ring_r = width;

        /* Left outer span [0, x_ring_l) */
        if (x_ring_l > 0) {
            apply_outer_span(row, x_ring_l, outer_y_lut, outer_c_lut);
        }

        /* Compute inner untouched span [x_in_l, x_in_r) where dy2 + rx^2 < inner_sq */
        int x_in_l = x_ring_r;
        int x_in_r = x_ring_r;
        if (dy2 < inner_sq) {
            int inner_rx = (int) sqrt((double) (inner_sq - 1 - dy2));
            while ((int64_t) (inner_rx + 1) * (inner_rx + 1) + dy2 < inner_sq)
                inner_rx++;
            while (inner_rx >= 0 && (int64_t) inner_rx * inner_rx + dy2 >= inner_sq)
                inner_rx--;
            x_in_l = w2 - inner_rx;
            x_in_r = w2 + inner_rx + 1;
            if (x_in_l < x_ring_l)
                x_in_l = x_ring_l;
            if (x_in_l > x_ring_r)
                x_in_l = x_ring_r;
            if (x_in_r < x_in_l)
                x_in_r = x_in_l;
            if (x_in_r > x_ring_r)
                x_in_r = x_ring_r;
        }

        /* Left ring span [x_ring_l, x_in_l): monotonically decreasing dx up to w2 */
        int x_left_end = (x_in_l < w2) ? x_in_l : w2;
        if (x_ring_l < x_left_end) {
            int64_t rx = (int64_t) x_ring_l - w2;
            int64_t dist2 = dy2 + rx * rx;
            int dx = (int) sqrt((double) dist2);
            while ((int64_t) (dx + 1) * (dx + 1) <= dist2)
                dx++;
            while (dx > 0 && (int64_t) dx * dx > dist2)
                dx--;
            uint8_t *restrict p = row + (size_t) x_ring_l * 2;
            for (int x = x_ring_l; x < x_left_end; x++, p += 2) {
                const double delta = delta_lut[dx - inner_dx];
                p[0] = (uint8_t) (p[0] * delta);
                p[1] = (uint8_t) ((p[1] - 127.0) * delta + 127.0);
                /* Advance dist2 for rx -> rx + 1 */
                dist2 += 2 * rx + 1;
                rx++;
                while (dx > 0 && (int64_t) dx * dx > dist2)
                    dx--;
            }
        }

        /* If no inner hole (dy2 >= inner_sq), continue monotonic increasing from w2 to x_in_l */
        int x_mid_start = (x_ring_l > w2) ? x_ring_l : w2;
        if (x_mid_start < x_in_l) {
            int64_t rx = (int64_t) x_mid_start - w2;
            int64_t dist2 = dy2 + rx * rx;
            int dx = (int) sqrt((double) dist2);
            while ((int64_t) (dx + 1) * (dx + 1) <= dist2)
                dx++;
            while (dx > 0 && (int64_t) dx * dx > dist2)
                dx--;
            uint8_t *restrict p = row + (size_t) x_mid_start * 2;
            for (int x = x_mid_start; x < x_in_l; x++, p += 2) {
                const double delta = delta_lut[dx - inner_dx];
                p[0] = (uint8_t) (p[0] * delta);
                p[1] = (uint8_t) ((p[1] - 127.0) * delta + 127.0);
                dist2 += 2 * rx + 1;
                rx++;
                while ((int64_t) (dx + 1) * (dx + 1) <= dist2)
                    dx++;
            }
        }

        /* Inner span [x_in_l, x_in_r): untouched (skipped completely!) */

        /* Right ring span [x_in_r, x_ring_r): monotonically increasing dx */
        if (x_in_r < x_ring_r) {
            int64_t rx = (int64_t) x_in_r - w2;
            int64_t dist2 = dy2 + rx * rx;
            int dx = (int) sqrt((double) dist2);
            while ((int64_t) (dx + 1) * (dx + 1) <= dist2)
                dx++;
            while (dx > 0 && (int64_t) dx * dx > dist2)
                dx--;
            uint8_t *restrict p = row + (size_t) x_in_r * 2;
            for (int x = x_in_r; x < x_ring_r; x++, p += 2) {
                const double delta = delta_lut[dx - inner_dx];
                p[0] = (uint8_t) (p[0] * delta);
                p[1] = (uint8_t) ((p[1] - 127.0) * delta + 127.0);
                dist2 += 2 * rx + 1;
                rx++;
                while ((int64_t) (dx + 1) * (dx + 1) <= dist2)
                    dx++;
            }
        }

        /* Right outer span [x_ring_r, width) */
        if (x_ring_r < width) {
            apply_outer_span(row + (size_t) x_ring_r * 2, width - x_ring_r, outer_y_lut, outer_c_lut);
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
        double smooth = mlt_properties_anim_get_double(filter_props, "smooth", pos, len) * 100.0
                        * scale;
        double radius = mlt_properties_anim_get_double(filter_props, "radius", pos, len) * *width;
        double cx = mlt_properties_anim_get_double(filter_props, "x", pos, len) * *width;
        double cy = mlt_properties_anim_get_double(filter_props, "y", pos, len) * *height;
        double opacity = mlt_properties_anim_get_double(filter_props, "opacity", pos, len);
        int mode = mlt_properties_get_int(filter_props, "mode");

        double r_in = radius - smooth;
        double r_out = radius + smooth;
        int inner_dx = (r_in > 0.0) ? (int) ceil(r_in) : 0;
        int outer_dx = (r_out > 0.0) ? (int) ceil(r_out) : 0;
        if (outer_dx < inner_dx)
            outer_dx = inner_dx;

        /* Precompute 1D delta LUT for integer dx in [inner_dx, outer_dx) */
        int lut_len = outer_dx - inner_dx;
        double stack_lut[1024];
        double *delta_lut = (lut_len <= 1024) ? stack_lut
                                              : (double *) malloc((size_t) lut_len * sizeof(double));
        if (lut_len > 0 && !delta_lut)
            return 1;

        volatile double denom = 2.0 * smooth;
        for (int i = 0; i < lut_len; i++) {
            int dx = inner_dx + i;
            double delta = (r_out - dx) / denom;
            if (mode == 1) {
                delta = pow(cos(((1.0 - delta) * M_PI / 2.0)), 3.0);
            }
            delta_lut[i] = MAX(opacity, delta);
        }

        slice_desc desc = {.image = *image,
                           .width = *width,
                           .height = *height,
                           .w2 = (int) cx,
                           .h2 = (int) cy,
                           .inner_dx = inner_dx,
                           .outer_dx = outer_dx,
                           .delta_lut = delta_lut};

        double outer_delta = MAX(opacity, 0.0);
        for (int v = 0; v < 256; v++) {
            desc.outer_y_lut[v] = (uint8_t) (v * outer_delta);
            desc.outer_c_lut[v] = (uint8_t) ((v - 127.0) * outer_delta + 127.0);
        }

        mlt_slices_run_normal(0, slice_proc, &desc);

        if (delta_lut != stack_lut)
            free(delta_lut);
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
