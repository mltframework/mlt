/*
 * Copyright (c) 2022-2023 Meltytech, LLC
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

#include "image_proc.h"

#include <framework/mlt_log.h>
#include <framework/mlt_slices.h>

#include <math.h>

typedef struct
{
    mlt_image src;
    mlt_image dst;
    int radius;
} blur_slice_desc;

static int blur_h_proc_rgba(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    int accumulator[] = {0, 0, 0, 0};
    int x = 0;
    int y = 0;
    int step = 4;
    int linesize = step * desc->src->width;
    int radius = desc->radius;

    if (desc->radius > (desc->src->width / 2)) {
        radius = desc->src->width / 2;
    }
    double diameter = (radius * 2) + 1;

    for (y = slice_line_start; y < slice_line_end; y++) {
        uint8_t *first = (uint8_t *) ((char *) desc->src->data + (y * linesize));
        uint8_t *last = first + linesize - step;
        uint8_t *s1 = first;
        uint8_t *s2 = first;
        uint8_t *d = (uint8_t *) ((char *) desc->dst->data + (y * linesize));
        accumulator[0] = first[0] * (radius + 1);
        accumulator[1] = first[1] * (radius + 1);
        accumulator[2] = first[2] * (radius + 1);
        accumulator[3] = first[3] * (radius + 1);

        for (x = 0; x < radius; x++) {
            accumulator[0] += s1[0];
            accumulator[1] += s1[1];
            accumulator[2] += s1[2];
            accumulator[3] += s1[3];
            s1 += step;
        }
        for (x = 0; x <= radius; x++) {
            accumulator[0] += s1[0] - first[0];
            accumulator[1] += s1[1] - first[1];
            accumulator[2] += s1[2] - first[2];
            accumulator[3] += s1[3] - first[3];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            d[3] = lrint((double) accumulator[3] / diameter);
            s1 += step;
            d += step;
        }
        for (x = radius + 1; x < desc->src->width - radius; x++) {
            accumulator[0] += s1[0] - s2[0];
            accumulator[1] += s1[1] - s2[1];
            accumulator[2] += s1[2] - s2[2];
            accumulator[3] += s1[3] - s2[3];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            d[3] = lrint((double) accumulator[3] / diameter);
            s1 += step;
            s2 += step;
            d += step;
        }
        for (x = desc->src->width - radius; x < desc->src->width; x++) {
            accumulator[0] += last[0] - s2[0];
            accumulator[1] += last[1] - s2[1];
            accumulator[2] += last[2] - s2[2];
            accumulator[3] += last[3] - s2[3];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            d[3] = lrint((double) accumulator[3] / diameter);
            s2 += step;
            d += step;
        }
    }
    return 0;
}

static int blur_v_proc_rgba(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_row_start,
        slice_width = mlt_slices_size_slice(jobs, index, desc->src->width, &slice_row_start);
    int slice_row_end = slice_row_start + slice_width;
    int accumulator[] = {0, 0, 0, 0};
    int x = 0;
    int y = 0;
    int step = 4;
    int linesize = step * desc->src->width;
    int radius = desc->radius;

    if (desc->radius > (desc->src->height / 2)) {
        radius = desc->src->height / 2;
    }
    double diameter = (radius * 2) + 1;

    for (x = slice_row_start; x < slice_row_end; x++) {
        uint8_t *first = (uint8_t *) ((char *) desc->src->data + (x * step));
        uint8_t *last = first + (linesize * (desc->src->height - 1));
        uint8_t *s1 = first;
        uint8_t *s2 = first;
        uint8_t *d = (uint8_t *) ((char *) desc->dst->data + (x * step));
        accumulator[0] = first[0] * (radius + 1);
        accumulator[1] = first[1] * (radius + 1);
        accumulator[2] = first[2] * (radius + 1);
        accumulator[3] = first[3] * (radius + 1);

        for (y = 0; y < radius; y++) {
            accumulator[0] += s1[0];
            accumulator[1] += s1[1];
            accumulator[2] += s1[2];
            accumulator[3] += s1[3];
            s1 += linesize;
        }
        for (y = 0; y <= radius; y++) {
            accumulator[0] += s1[0] - first[0];
            accumulator[1] += s1[1] - first[1];
            accumulator[2] += s1[2] - first[2];
            accumulator[3] += s1[3] - first[3];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            d[3] = lrint((double) accumulator[3] / diameter);
            s1 += linesize;
            d += linesize;
        }
        for (y = radius + 1; y < desc->src->height - radius; y++) {
            accumulator[0] += s1[0] - s2[0];
            accumulator[1] += s1[1] - s2[1];
            accumulator[2] += s1[2] - s2[2];
            accumulator[3] += s1[3] - s2[3];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            d[3] = lrint((double) accumulator[3] / diameter);
            s1 += linesize;
            s2 += linesize;
            d += linesize;
        }
        for (y = desc->src->height - radius; y < desc->src->height; y++) {
            accumulator[0] += last[0] - s2[0];
            accumulator[1] += last[1] - s2[1];
            accumulator[2] += last[2] - s2[2];
            accumulator[3] += last[3] - s2[3];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            d[3] = lrint((double) accumulator[3] / diameter);
            s2 += linesize;
            d += linesize;
        }
    }
    return 0;
}

static int blur_h_proc_rgbx(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    int accumulator[] = {0, 0, 0};
    int x = 0;
    int y = 0;
    int step = 4;
    int linesize = step * desc->src->width;
    int radius = desc->radius;

    if (desc->radius > (desc->src->width / 2)) {
        radius = desc->src->width / 2;
    }
    double diameter = (radius * 2) + 1;

    for (y = slice_line_start; y < slice_line_end; y++) {
        uint8_t *first = (uint8_t *) ((char *) desc->src->data + (y * linesize));
        uint8_t *last = first + linesize - step;
        uint8_t *s1 = first;
        uint8_t *s2 = first;
        uint8_t *d = (uint8_t *) ((char *) desc->dst->data + (y * linesize));
        accumulator[0] = first[0] * (radius + 1);
        accumulator[1] = first[1] * (radius + 1);
        accumulator[2] = first[2] * (radius + 1);

        for (x = 0; x < radius; x++) {
            accumulator[0] += s1[0];
            accumulator[1] += s1[1];
            accumulator[2] += s1[2];
            s1 += step;
        }
        for (x = 0; x <= radius; x++) {
            accumulator[0] += s1[0] - first[0];
            accumulator[1] += s1[1] - first[1];
            accumulator[2] += s1[2] - first[2];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            s1 += step;
            d += step;
        }
        for (x = radius + 1; x < desc->src->width - radius; x++) {
            accumulator[0] += s1[0] - s2[0];
            accumulator[1] += s1[1] - s2[1];
            accumulator[2] += s1[2] - s2[2];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            s1 += step;
            s2 += step;
            d += step;
        }
        for (x = desc->src->width - radius; x < desc->src->width; x++) {
            accumulator[0] += last[0] - s2[0];
            accumulator[1] += last[1] - s2[1];
            accumulator[2] += last[2] - s2[2];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            s2 += step;
            d += step;
        }
    }
    return 0;
}

static int blur_v_proc_rgbx(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_row_start,
        slice_width = mlt_slices_size_slice(jobs, index, desc->src->width, &slice_row_start);
    int slice_row_end = slice_row_start + slice_width;
    int accumulator[] = {0, 0, 0};
    int x = 0;
    int y = 0;
    int step = 4;
    int linesize = step * desc->src->width;
    int radius = desc->radius;

    if (desc->radius > (desc->src->height / 2)) {
        radius = desc->src->height / 2;
    }
    double diameter = (radius * 2) + 1;

    for (x = slice_row_start; x < slice_row_end; x++) {
        uint8_t *first = (uint8_t *) ((char *) desc->src->data + (x * step));
        uint8_t *last = first + (linesize * (desc->src->height - 1));
        uint8_t *s1 = first;
        uint8_t *s2 = first;
        uint8_t *d = (uint8_t *) ((char *) desc->dst->data + (x * step));
        accumulator[0] = first[0] * (radius + 1);
        accumulator[1] = first[1] * (radius + 1);
        accumulator[2] = first[2] * (radius + 1);

        for (y = 0; y < radius; y++) {
            accumulator[0] += s1[0];
            accumulator[1] += s1[1];
            accumulator[2] += s1[2];
            s1 += linesize;
        }
        for (y = 0; y <= radius; y++) {
            accumulator[0] += s1[0] - first[0];
            accumulator[1] += s1[1] - first[1];
            accumulator[2] += s1[2] - first[2];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            s1 += linesize;
            d += linesize;
        }
        for (y = radius + 1; y < desc->src->height - radius; y++) {
            accumulator[0] += s1[0] - s2[0];
            accumulator[1] += s1[1] - s2[1];
            accumulator[2] += s1[2] - s2[2];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            s1 += linesize;
            s2 += linesize;
            d += linesize;
        }
        for (y = desc->src->height - radius; y < desc->src->height; y++) {
            accumulator[0] += last[0] - s2[0];
            accumulator[1] += last[1] - s2[1];
            accumulator[2] += last[2] - s2[2];
            d[0] = lrint((double) accumulator[0] / diameter);
            d[1] = lrint((double) accumulator[1] / diameter);
            d[2] = lrint((double) accumulator[2] / diameter);
            s2 += linesize;
            d += linesize;
        }
    }
    return 0;
}

/** Perform a box blur
 *
 * This function uses a sliding window accumulator method - applied
 * horizontally first and then vertically.
 *
 * \param self the Image object
 * \param hradius the radius of the horizontal blur in pixels
 * \param vradius radius of the vertical blur in pixels
 * \param preserve_alpha exclude the alpha channel from the blur operation
 */

void mlt_image_box_blur(mlt_image self, int hradius, int vradius, int preserve_alpha)
{
    if (self->format != mlt_image_rgba) {
        mlt_log(NULL,
                MLT_LOG_ERROR,
                "Image type %s not supported by box blur\n",
                mlt_image_format_name(self->format));
        return;
    }
    // The horizontal blur is performed into a temporary image.
    // The vertical blur is performed back into the original image.
    struct mlt_image_s tmpimage;
    mlt_image_set_values(&tmpimage, NULL, self->format, self->width, self->height);
    mlt_image_alloc_data(&tmpimage);
    if (self->alpha) {
        mlt_image_alloc_alpha(&tmpimage);
    }

    blur_slice_desc desc;
    if (preserve_alpha) {
        desc.src = self, desc.dst = &tmpimage, desc.radius = hradius,
        mlt_slices_run_normal(0, blur_h_proc_rgbx, &desc);
        desc.src = &tmpimage, desc.dst = self, desc.radius = vradius,
        mlt_slices_run_normal(0, blur_v_proc_rgbx, &desc);
    } else {
        desc.src = self, desc.dst = &tmpimage, desc.radius = hradius,
        mlt_slices_run_normal(0, blur_h_proc_rgba, &desc);
        desc.src = &tmpimage, desc.dst = self, desc.radius = vradius,
        mlt_slices_run_normal(0, blur_v_proc_rgba, &desc);
    }

    mlt_image_close(&tmpimage);
}
