/*
 * Copyright (c) 2022-2025 Meltytech, LLC
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
#include <stdint.h>

typedef struct
{
    mlt_image src;
    mlt_image dst;
    int radius;
} blur_slice_desc;

/*
 * For odd diameter = 2 * radius + 1 and non-negative integer acc,
 * acc / (double) diameter can never be a half-integer, so
 * lrint((double) acc / diameter) == (acc + radius) / diameter.
 * Using a 40-bit fixed-point reciprocal is exact for all 8-bit sums up to
 * diameter <= 16385 (max sum < 2^22, sum * diam < 2^36 << 2^40).
 */
#define BLUR_INV_SHIFT 40
#define BLUR_V_TILE_W 64

static inline uint8_t blur_div_u8(int acc, int radius, uint64_t inv_diam)
{
    return (uint8_t) (((uint64_t) (acc + radius) * inv_diam) >> BLUR_INV_SHIFT);
}

static int blur_h_proc_rgba(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    const int width = desc->src->width;
    const int step = 4;
    const int linesize = step * width;
    int radius = desc->radius;

    if (radius > (width / 2)) {
        radius = width / 2;
    }
    const int diameter = (radius * 2) + 1;
    const uint64_t inv_diam = ((1ULL << BLUR_INV_SHIFT) + (uint64_t) diameter - 1)
                              / (uint64_t) diameter;
    const int mid_end = width - radius;

    for (int y = slice_line_start; y < slice_line_end; y++) {
        const uint8_t *first = desc->src->planes[0] + (y * linesize);
        const uint8_t *last = first + linesize - step;
        const uint8_t *s1 = first;
        const uint8_t *s2 = first;
        uint8_t *d = desc->dst->planes[0] + (y * linesize);
        const int f0 = first[0], f1 = first[1], f2 = first[2], f3 = first[3];
        const int l0 = last[0], l1 = last[1], l2 = last[2], l3 = last[3];
        int a0 = f0 * (radius + 1);
        int a1 = f1 * (radius + 1);
        int a2 = f2 * (radius + 1);
        int a3 = f3 * (radius + 1);

        for (int x = 0; x < radius; x++) {
            a0 += s1[0];
            a1 += s1[1];
            a2 += s1[2];
            a3 += s1[3];
            s1 += step;
        }
        for (int x = 0; x <= radius; x++) {
            a0 += s1[0] - f0;
            a1 += s1[1] - f1;
            a2 += s1[2] - f2;
            a3 += s1[3] - f3;
            d[0] = blur_div_u8(a0, radius, inv_diam);
            d[1] = blur_div_u8(a1, radius, inv_diam);
            d[2] = blur_div_u8(a2, radius, inv_diam);
            d[3] = blur_div_u8(a3, radius, inv_diam);
            s1 += step;
            d += step;
        }
        for (int x = radius + 1; x < mid_end; x++) {
            a0 += s1[0] - s2[0];
            a1 += s1[1] - s2[1];
            a2 += s1[2] - s2[2];
            a3 += s1[3] - s2[3];
            d[0] = blur_div_u8(a0, radius, inv_diam);
            d[1] = blur_div_u8(a1, radius, inv_diam);
            d[2] = blur_div_u8(a2, radius, inv_diam);
            d[3] = blur_div_u8(a3, radius, inv_diam);
            s1 += step;
            s2 += step;
            d += step;
        }
        for (int x = mid_end; x < width; x++) {
            a0 += l0 - s2[0];
            a1 += l1 - s2[1];
            a2 += l2 - s2[2];
            a3 += l3 - s2[3];
            d[0] = blur_div_u8(a0, radius, inv_diam);
            d[1] = blur_div_u8(a1, radius, inv_diam);
            d[2] = blur_div_u8(a2, radius, inv_diam);
            d[3] = blur_div_u8(a3, radius, inv_diam);
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
    int slice_col_start,
        slice_width = mlt_slices_size_slice(jobs, index, desc->src->width, &slice_col_start);
    const int slice_col_end = slice_col_start + slice_width;
    const int height = desc->src->height;
    const int linesize = 4 * desc->src->width;
    int radius = desc->radius;

    if (radius > (height / 2)) {
        radius = height / 2;
    }
    const int diameter = (radius * 2) + 1;
    const uint64_t inv_diam = ((1ULL << BLUR_INV_SHIFT) + (uint64_t) diameter - 1)
                              / (uint64_t) diameter;
    const int mid_end = height - radius;
    const uint8_t *src_base = desc->src->planes[0];
    uint8_t *dst_base = desc->dst->planes[0];
    int32_t acc[BLUR_V_TILE_W * 4];

    for (int tx = slice_col_start; tx < slice_col_end; tx += BLUR_V_TILE_W) {
        const int tw = (tx + BLUR_V_TILE_W <= slice_col_end) ? BLUR_V_TILE_W : (slice_col_end - tx);
        const int n = tw * 4;
        const int byte_off = tx * 4;
        const uint8_t *first_row = src_base + byte_off;
        const uint8_t *last_row = first_row + (size_t) linesize * (height - 1);
        const uint8_t *s1 = first_row;
        const uint8_t *s2 = first_row;
        uint8_t *d = dst_base + byte_off;

        for (int k = 0; k < n; k++) {
            acc[k] = first_row[k] * (radius + 1);
        }
        for (int y = 0; y < radius; y++) {
            for (int k = 0; k < n; k++) {
                acc[k] += s1[k];
            }
            s1 += linesize;
        }
        for (int y = 0; y <= radius; y++) {
            for (int k = 0; k < n; k++) {
                int v = acc[k] + s1[k] - first_row[k];
                acc[k] = v;
                d[k] = blur_div_u8(v, radius, inv_diam);
            }
            s1 += linesize;
            d += linesize;
        }
        for (int y = radius + 1; y < mid_end; y++) {
            for (int k = 0; k < n; k++) {
                int v = acc[k] + s1[k] - s2[k];
                acc[k] = v;
                d[k] = blur_div_u8(v, radius, inv_diam);
            }
            s1 += linesize;
            s2 += linesize;
            d += linesize;
        }
        for (int y = mid_end; y < height; y++) {
            for (int k = 0; k < n; k++) {
                int v = acc[k] + last_row[k] - s2[k];
                acc[k] = v;
                d[k] = blur_div_u8(v, radius, inv_diam);
            }
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
    const int width = desc->src->width;
    const int step = 4;
    const int linesize = step * width;
    int radius = desc->radius;

    if (radius > (width / 2)) {
        radius = width / 2;
    }
    const int diameter = (radius * 2) + 1;
    const uint64_t inv_diam = ((1ULL << BLUR_INV_SHIFT) + (uint64_t) diameter - 1)
                              / (uint64_t) diameter;
    const int mid_end = width - radius;

    for (int y = slice_line_start; y < slice_line_end; y++) {
        const uint8_t *first = desc->src->planes[0] + (y * linesize);
        const uint8_t *last = first + linesize - step;
        const uint8_t *s1 = first;
        const uint8_t *s2 = first;
        uint8_t *d = desc->dst->planes[0] + (y * linesize);
        const int f0 = first[0], f1 = first[1], f2 = first[2];
        const int l0 = last[0], l1 = last[1], l2 = last[2];
        int a0 = f0 * (radius + 1);
        int a1 = f1 * (radius + 1);
        int a2 = f2 * (radius + 1);

        for (int x = 0; x < radius; x++) {
            a0 += s1[0];
            a1 += s1[1];
            a2 += s1[2];
            s1 += step;
        }
        for (int x = 0; x <= radius; x++) {
            a0 += s1[0] - f0;
            a1 += s1[1] - f1;
            a2 += s1[2] - f2;
            d[0] = blur_div_u8(a0, radius, inv_diam);
            d[1] = blur_div_u8(a1, radius, inv_diam);
            d[2] = blur_div_u8(a2, radius, inv_diam);
            s1 += step;
            d += step;
        }
        for (int x = radius + 1; x < mid_end; x++) {
            a0 += s1[0] - s2[0];
            a1 += s1[1] - s2[1];
            a2 += s1[2] - s2[2];
            d[0] = blur_div_u8(a0, radius, inv_diam);
            d[1] = blur_div_u8(a1, radius, inv_diam);
            d[2] = blur_div_u8(a2, radius, inv_diam);
            s1 += step;
            s2 += step;
            d += step;
        }
        for (int x = mid_end; x < width; x++) {
            a0 += l0 - s2[0];
            a1 += l1 - s2[1];
            a2 += l2 - s2[2];
            d[0] = blur_div_u8(a0, radius, inv_diam);
            d[1] = blur_div_u8(a1, radius, inv_diam);
            d[2] = blur_div_u8(a2, radius, inv_diam);
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
    int slice_col_start,
        slice_width = mlt_slices_size_slice(jobs, index, desc->src->width, &slice_col_start);
    const int slice_col_end = slice_col_start + slice_width;
    const int height = desc->src->height;
    const int linesize = 4 * desc->src->width;
    int radius = desc->radius;

    if (radius > (height / 2)) {
        radius = height / 2;
    }
    const int diameter = (radius * 2) + 1;
    const uint64_t inv_diam = ((1ULL << BLUR_INV_SHIFT) + (uint64_t) diameter - 1)
                              / (uint64_t) diameter;
    const int mid_end = height - radius;
    const uint8_t *src_base = desc->src->planes[0];
    uint8_t *dst_base = desc->dst->planes[0];
    int32_t acc[BLUR_V_TILE_W * 4];

    for (int tx = slice_col_start; tx < slice_col_end; tx += BLUR_V_TILE_W) {
        const int tw = (tx + BLUR_V_TILE_W <= slice_col_end) ? BLUR_V_TILE_W : (slice_col_end - tx);
        const int n = tw * 4;
        const int byte_off = tx * 4;
        const uint8_t *first_row = src_base + byte_off;
        const uint8_t *last_row = first_row + (size_t) linesize * (height - 1);
        const uint8_t *s1 = first_row;
        const uint8_t *s2 = first_row;
        uint8_t *d = dst_base + byte_off;

        for (int k = 0; k < n; k += 4) {
            acc[k] = first_row[k] * (radius + 1);
            acc[k + 1] = first_row[k + 1] * (radius + 1);
            acc[k + 2] = first_row[k + 2] * (radius + 1);
        }
        for (int y = 0; y < radius; y++) {
            for (int k = 0; k < n; k += 4) {
                acc[k] += s1[k];
                acc[k + 1] += s1[k + 1];
                acc[k + 2] += s1[k + 2];
            }
            s1 += linesize;
        }
        for (int y = 0; y <= radius; y++) {
            for (int k = 0; k < n; k += 4) {
                int v0 = acc[k] + s1[k] - first_row[k];
                int v1 = acc[k + 1] + s1[k + 1] - first_row[k + 1];
                int v2 = acc[k + 2] + s1[k + 2] - first_row[k + 2];
                acc[k] = v0;
                acc[k + 1] = v1;
                acc[k + 2] = v2;
                d[k] = blur_div_u8(v0, radius, inv_diam);
                d[k + 1] = blur_div_u8(v1, radius, inv_diam);
                d[k + 2] = blur_div_u8(v2, radius, inv_diam);
            }
            s1 += linesize;
            d += linesize;
        }
        for (int y = radius + 1; y < mid_end; y++) {
            for (int k = 0; k < n; k += 4) {
                int v0 = acc[k] + s1[k] - s2[k];
                int v1 = acc[k + 1] + s1[k + 1] - s2[k + 1];
                int v2 = acc[k + 2] + s1[k + 2] - s2[k + 2];
                acc[k] = v0;
                acc[k + 1] = v1;
                acc[k + 2] = v2;
                d[k] = blur_div_u8(v0, radius, inv_diam);
                d[k + 1] = blur_div_u8(v1, radius, inv_diam);
                d[k + 2] = blur_div_u8(v2, radius, inv_diam);
            }
            s1 += linesize;
            s2 += linesize;
            d += linesize;
        }
        for (int y = mid_end; y < height; y++) {
            for (int k = 0; k < n; k += 4) {
                int v0 = acc[k] + last_row[k] - s2[k];
                int v1 = acc[k + 1] + last_row[k + 1] - s2[k + 1];
                int v2 = acc[k + 2] + last_row[k + 2] - s2[k + 2];
                acc[k] = v0;
                acc[k + 1] = v1;
                acc[k + 2] = v2;
                d[k] = blur_div_u8(v0, radius, inv_diam);
                d[k + 1] = blur_div_u8(v1, radius, inv_diam);
                d[k + 2] = blur_div_u8(v2, radius, inv_diam);
            }
            s2 += linesize;
            d += linesize;
        }
    }
    return 0;
}

static int blur_h_proc_rgba64(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    const int width = desc->src->width;
    const int step = 4;
    const int linesize = step * width;
    int radius = desc->radius;

    if (radius > (width / 2)) {
        radius = width / 2;
    }
    const int diameter = (radius * 2) + 1;
    const int mid_end = width - radius;

    for (int y = slice_line_start; y < slice_line_end; y++) {
        const uint16_t *first = (const uint16_t *) desc->src->planes[0] + (y * linesize);
        const uint16_t *last = first + linesize - step;
        const uint16_t *s1 = first;
        const uint16_t *s2 = first;
        uint16_t *d = (uint16_t *) desc->dst->planes[0] + (y * linesize);
        int a0 = first[0] * (radius + 1);
        int a1 = first[1] * (radius + 1);
        int a2 = first[2] * (radius + 1);
        int a3 = first[3] * (radius + 1);

        for (int x = 0; x < radius; x++) {
            a0 += s1[0];
            a1 += s1[1];
            a2 += s1[2];
            a3 += s1[3];
            s1 += step;
        }
        for (int x = 0; x <= radius; x++) {
            a0 += s1[0] - first[0];
            a1 += s1[1] - first[1];
            a2 += s1[2] - first[2];
            a3 += s1[3] - first[3];
            d[0] = (uint16_t) ((a0 + radius) / diameter);
            d[1] = (uint16_t) ((a1 + radius) / diameter);
            d[2] = (uint16_t) ((a2 + radius) / diameter);
            d[3] = (uint16_t) ((a3 + radius) / diameter);
            s1 += step;
            d += step;
        }
        for (int x = radius + 1; x < mid_end; x++) {
            a0 += s1[0] - s2[0];
            a1 += s1[1] - s2[1];
            a2 += s1[2] - s2[2];
            a3 += s1[3] - s2[3];
            d[0] = (uint16_t) ((a0 + radius) / diameter);
            d[1] = (uint16_t) ((a1 + radius) / diameter);
            d[2] = (uint16_t) ((a2 + radius) / diameter);
            d[3] = (uint16_t) ((a3 + radius) / diameter);
            s1 += step;
            s2 += step;
            d += step;
        }
        for (int x = mid_end; x < width; x++) {
            a0 += last[0] - s2[0];
            a1 += last[1] - s2[1];
            a2 += last[2] - s2[2];
            a3 += last[3] - s2[3];
            d[0] = (uint16_t) ((a0 + radius) / diameter);
            d[1] = (uint16_t) ((a1 + radius) / diameter);
            d[2] = (uint16_t) ((a2 + radius) / diameter);
            d[3] = (uint16_t) ((a3 + radius) / diameter);
            s2 += step;
            d += step;
        }
    }
    return 0;
}

static int blur_v_proc_rgba64(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_col_start,
        slice_width = mlt_slices_size_slice(jobs, index, desc->src->width, &slice_col_start);
    const int slice_col_end = slice_col_start + slice_width;
    const int height = desc->src->height;
    const int linesize = 4 * desc->src->width;
    int radius = desc->radius;

    if (radius > (height / 2)) {
        radius = height / 2;
    }
    const int diameter = (radius * 2) + 1;
    const int mid_end = height - radius;
    const uint16_t *src_base = (const uint16_t *) desc->src->planes[0];
    uint16_t *dst_base = (uint16_t *) desc->dst->planes[0];
    int32_t acc[BLUR_V_TILE_W * 4];

    for (int tx = slice_col_start; tx < slice_col_end; tx += BLUR_V_TILE_W) {
        const int tw = (tx + BLUR_V_TILE_W <= slice_col_end) ? BLUR_V_TILE_W : (slice_col_end - tx);
        const int n = tw * 4;
        const int off = tx * 4;
        const uint16_t *first_row = src_base + off;
        const uint16_t *last_row = first_row + (size_t) linesize * (height - 1);
        const uint16_t *s1 = first_row;
        const uint16_t *s2 = first_row;
        uint16_t *d = dst_base + off;

        for (int k = 0; k < n; k++) {
            acc[k] = first_row[k] * (radius + 1);
        }
        for (int y = 0; y < radius; y++) {
            for (int k = 0; k < n; k++) {
                acc[k] += s1[k];
            }
            s1 += linesize;
        }
        for (int y = 0; y <= radius; y++) {
            for (int k = 0; k < n; k++) {
                int v = acc[k] + s1[k] - first_row[k];
                acc[k] = v;
                d[k] = (uint16_t) ((v + radius) / diameter);
            }
            s1 += linesize;
            d += linesize;
        }
        for (int y = radius + 1; y < mid_end; y++) {
            for (int k = 0; k < n; k++) {
                int v = acc[k] + s1[k] - s2[k];
                acc[k] = v;
                d[k] = (uint16_t) ((v + radius) / diameter);
            }
            s1 += linesize;
            s2 += linesize;
            d += linesize;
        }
        for (int y = mid_end; y < height; y++) {
            for (int k = 0; k < n; k++) {
                int v = acc[k] + last_row[k] - s2[k];
                acc[k] = v;
                d[k] = (uint16_t) ((v + radius) / diameter);
            }
            s2 += linesize;
            d += linesize;
        }
    }
    return 0;
}

static int blur_h_proc_rgbx64(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    blur_slice_desc *desc = ((blur_slice_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    const int width = desc->src->width;
    const int step = 4;
    const int linesize = step * width;
    int radius = desc->radius;

    if (radius > (width / 2)) {
        radius = width / 2;
    }
    const int diameter = (radius * 2) + 1;
    const int mid_end = width - radius;

    for (int y = slice_line_start; y < slice_line_end; y++) {
        const uint16_t *first = (const uint16_t *) desc->src->planes[0] + (y * linesize);
        const uint16_t *last = first + linesize - step;
        const uint16_t *s1 = first;
        const uint16_t *s2 = first;
        uint16_t *d = (uint16_t *) desc->dst->planes[0] + (y * linesize);
        int a0 = first[0] * (radius + 1);
        int a1 = first[1] * (radius + 1);
        int a2 = first[2] * (radius + 1);

        for (int x = 0; x < radius; x++) {
            a0 += s1[0];
            a1 += s1[1];
            a2 += s1[2];
            s1 += step;
        }
        for (int x = 0; x <= radius; x++) {
            a0 += s1[0] - first[0];
            a1 += s1[1] - first[1];
            a2 += s1[2] - first[2];
            d[0] = (uint16_t) ((a0 + radius) / diameter);
            d[1] = (uint16_t) ((a1 + radius) / diameter);
            d[2] = (uint16_t) ((a2 + radius) / diameter);
            s1 += step;
            d += step;
        }
        for (int x = radius + 1; x < mid_end; x++) {
            a0 += s1[0] - s2[0];
            a1 += s1[1] - s2[1];
            a2 += s1[2] - s2[2];
            d[0] = (uint16_t) ((a0 + radius) / diameter);
            d[1] = (uint16_t) ((a1 + radius) / diameter);
            d[2] = (uint16_t) ((a2 + radius) / diameter);
            s1 += step;
            s2 += step;
            d += step;
        }
        for (int x = mid_end; x < width; x++) {
            a0 += last[0] - s2[0];
            a1 += last[1] - s2[1];
            a2 += last[2] - s2[2];
            d[0] = (uint16_t) ((a0 + radius) / diameter);
            d[1] = (uint16_t) ((a1 + radius) / diameter);
            d[2] = (uint16_t) ((a2 + radius) / diameter);
            s2 += step;
            d += step;
        }
    }
    return 0;
}

static int blur_v_proc_rgbx64(int id, int index, int jobs, void *data)
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
    int linesize = step * desc->src->width * 2;
    int radius = desc->radius;

    if (desc->radius > (desc->src->height / 2)) {
        radius = desc->src->height / 2;
    }
    int diameter = (radius * 2) + 1;

    for (x = slice_row_start; x < slice_row_end; x++) {
        uint16_t *first = (uint16_t *) desc->src->planes[0] + (x * step);
        uint16_t *last = first + (linesize * (desc->src->height - 1));
        uint16_t *s1 = first;
        uint16_t *s2 = first;
        uint16_t *d = (uint16_t *) desc->dst->planes[0] + (x * step);
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
            d[0] = (uint16_t) ((accumulator[0] + radius) / diameter);
            d[1] = (uint16_t) ((accumulator[1] + radius) / diameter);
            d[2] = (uint16_t) ((accumulator[2] + radius) / diameter);
            s1 += linesize;
            d += linesize;
        }
        for (y = radius + 1; y < desc->src->height - radius; y++) {
            accumulator[0] += s1[0] - s2[0];
            accumulator[1] += s1[1] - s2[1];
            accumulator[2] += s1[2] - s2[2];
            d[0] = (uint16_t) ((accumulator[0] + radius) / diameter);
            d[1] = (uint16_t) ((accumulator[1] + radius) / diameter);
            d[2] = (uint16_t) ((accumulator[2] + radius) / diameter);
            s1 += linesize;
            s2 += linesize;
            d += linesize;
        }
        for (y = desc->src->height - radius; y < desc->src->height; y++) {
            accumulator[0] += last[0] - s2[0];
            accumulator[1] += last[1] - s2[1];
            accumulator[2] += last[2] - s2[2];
            d[0] = (uint16_t) ((accumulator[0] + radius) / diameter);
            d[1] = (uint16_t) ((accumulator[1] + radius) / diameter);
            d[2] = (uint16_t) ((accumulator[2] + radius) / diameter);
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
    if (self->format != mlt_image_rgba && self->format != mlt_image_rgba64) {
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

    mlt_slices_proc h_proc;
    mlt_slices_proc v_proc;
    if (preserve_alpha && self->format == mlt_image_rgba) {
        h_proc = blur_h_proc_rgbx;
        v_proc = blur_v_proc_rgbx;
    } else if (self->format == mlt_image_rgba) {
        h_proc = blur_h_proc_rgba;
        v_proc = blur_v_proc_rgba;
    } else if (preserve_alpha && self->format == mlt_image_rgba64) {
        h_proc = blur_h_proc_rgbx64;
        v_proc = blur_v_proc_rgbx64;
    } else { // self->format == mlt_image_rgba64
        h_proc = blur_h_proc_rgba64;
        v_proc = blur_v_proc_rgba64;
    }

    blur_slice_desc desc;
    desc.src = self, desc.dst = &tmpimage, desc.radius = hradius,
    mlt_slices_run_normal(0, h_proc, &desc);
    desc.src = &tmpimage, desc.dst = self, desc.radius = vradius,
    mlt_slices_run_normal(0, v_proc, &desc);

    mlt_image_close(&tmpimage);
}
