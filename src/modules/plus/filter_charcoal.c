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

#ifdef __SSE2__
#include <emmintrin.h>
#endif

static inline int get_Y(const uint8_t *pixels, int width, int height, int x, int y, int max_luma)
{
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return max_luma;
    } else {
        const uint8_t *pixel = pixels + y * (width << 1) + (x << 1);
        return *pixel;
    }
}

typedef struct
{
    uint8_t *image;
    uint8_t *dest;
    int width;
    int height;
    int x_scatter;
    int y_scatter;
    int max_luma;
    int zero_mix;
    uint8_t luma_lut[1276];
    uint8_t chroma_lut[256];
} slice_desc;

static inline void charcoal_pixel_border(
    const slice_desc *d, int x, int y, uint8_t *p, const uint8_t *q)
{
    int m00 = get_Y(d->image, d->width, d->height, x - d->x_scatter, y - d->y_scatter, d->max_luma);
    int m01 = get_Y(d->image, d->width, d->height, x, y - d->y_scatter, d->max_luma);
    int m02 = get_Y(d->image, d->width, d->height, x + d->x_scatter, y - d->y_scatter, d->max_luma);
    int m10 = get_Y(d->image, d->width, d->height, x - d->x_scatter, y, d->max_luma);
    int m12 = get_Y(d->image, d->width, d->height, x + d->x_scatter, y, d->max_luma);
    int m20 = get_Y(d->image, d->width, d->height, x - d->x_scatter, y + d->y_scatter, d->max_luma);
    int m21 = get_Y(d->image, d->width, d->height, x, y + d->y_scatter, d->max_luma);
    int m22 = get_Y(d->image, d->width, d->height, x + d->x_scatter, y + d->y_scatter, d->max_luma);

    int sum1 = (m22 - m00) + ((m21 - m01) << 1);
    int sum2 = (m02 - m00) + ((m12 - m10) << 1) + (m22 - m20);
    int idx = (int) sqrtf((float) (sum1 * sum1 + sum2 * sum2));

    p[0] = d->luma_lut[idx];
    p[1] = d->chroma_lut[q[1]];
}

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *d = (const slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
    const int width = d->width;
    const int height = d->height;
    const int xs = d->x_scatter;
    const int ys = d->y_scatter;
    const int stride = width << 1;
    const int xs2 = xs << 1;
    const uint8_t *luma_lut = d->luma_lut;
    const uint8_t *chroma_lut = d->chroma_lut;
    const int has_interior_x = (xs >= 0 && xs * 2 < width);
    const int x_end = width - xs;
    const uint8_t zero_chroma = chroma_lut[128];

    uint8_t *p = d->dest + slice_line_start * stride;
    const uint8_t *q = d->image + slice_line_start * stride;

    for (int y = slice_line_start; y < slice_line_start + slice_height; y++) {
        int y_top = y - ys;
        int y_bot = y + ys;
        if (!has_interior_x || y_top < 0 || y_top >= height || y_bot < 0 || y_bot >= height) {
            for (int x = 0; x < width; x++) {
                charcoal_pixel_border(d, x, y, p, q);
                p += 2;
                q += 2;
            }
            continue;
        }

        // Left border
        for (int x = 0; x < xs; x++) {
            charcoal_pixel_border(d, x, y, p, q);
            p += 2;
            q += 2;
        }

        // Interior: zero bounds checks
        const uint8_t *row_top = d->image + y_top * stride;
        const uint8_t *row_mid = q - xs2; // d->image + y * stride
        const uint8_t *row_bot = d->image + y_bot * stride;
        int x = xs;

#ifdef __SSE2__
        const __m128i mask_y = _mm_set1_epi16(0x00FF);
        const int x_vec_end = x_end - 7;
        for (; x < x_vec_end; x += 8) {
            const int off = x << 1;
            __m128i m00 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_top + off - xs2)),
                                        mask_y);
            __m128i m01 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_top + off)), mask_y);
            __m128i m02 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_top + off + xs2)),
                                        mask_y);
            __m128i m10 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_mid + off - xs2)),
                                        mask_y);
            __m128i m12 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_mid + off + xs2)),
                                        mask_y);
            __m128i m20 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_bot + off - xs2)),
                                        mask_y);
            __m128i m21 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_bot + off)), mask_y);
            __m128i m22 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (row_bot + off + xs2)),
                                        mask_y);

            __m128i sum1 = _mm_add_epi16(_mm_sub_epi16(m22, m00),
                                         _mm_slli_epi16(_mm_sub_epi16(m21, m01), 1));
            __m128i sum2 = _mm_add_epi16(_mm_add_epi16(_mm_sub_epi16(m02, m00),
                                                       _mm_slli_epi16(_mm_sub_epi16(m12, m10), 1)),
                                         _mm_sub_epi16(m22, m20));

            __m128i lo = _mm_unpacklo_epi16(sum1, sum2);
            __m128i hi = _mm_unpackhi_epi16(sum1, sum2);
            __m128i sq_lo = _mm_madd_epi16(lo, lo);
            __m128i sq_hi = _mm_madd_epi16(hi, hi);

            __m128i idx_lo = _mm_cvttps_epi32(_mm_sqrt_ps(_mm_cvtepi32_ps(sq_lo)));
            __m128i idx_hi = _mm_cvttps_epi32(_mm_sqrt_ps(_mm_cvtepi32_ps(sq_hi)));

            int32_t idx[8];
            _mm_storeu_si128((__m128i *) idx, idx_lo);
            _mm_storeu_si128((__m128i *) (idx + 4), idx_hi);

            if (d->zero_mix) {
                for (int k = 0; k < 8; k++) {
                    p[k * 2] = luma_lut[idx[k]];
                    p[k * 2 + 1] = zero_chroma;
                }
            } else {
                for (int k = 0; k < 8; k++) {
                    p[k * 2] = luma_lut[idx[k]];
                    p[k * 2 + 1] = chroma_lut[q[k * 2 + 1]];
                }
            }
            p += 16;
            q += 16;
        }
#endif

        for (; x < x_end; x++) {
            const int off = x << 1;
            int m00 = row_top[off - xs2];
            int m01 = row_top[off];
            int m02 = row_top[off + xs2];
            int m10 = row_mid[off - xs2];
            int m12 = row_mid[off + xs2];
            int m20 = row_bot[off - xs2];
            int m21 = row_bot[off];
            int m22 = row_bot[off + xs2];

            int sum1 = (m22 - m00) + ((m21 - m01) << 1);
            int sum2 = (m02 - m00) + ((m12 - m10) << 1) + (m22 - m20);
            int idx = (int) sqrtf((float) (sum1 * sum1 + sum2 * sum2));

            p[0] = luma_lut[idx];
            p[1] = chroma_lut[q[1]];
            p += 2;
            q += 2;
        }

        // Right border
        for (int x_r = x_end; x_r < width; x_r++) {
            charcoal_pixel_border(d, x_r, y, p, q);
            p += 2;
            q += 2;
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

        int min = full_range ? 0 : 16;
        int max_luma = full_range ? 255 : 235;
        int max_chroma = full_range ? 255 : 240;
        int invert = mlt_properties_anim_get_int(properties, "invert", position, length);
        int invert_luma = full_range ? 255 : 251;
        float scale = mlt_properties_anim_get_double(properties, "scale", position, length);
        float mix = mlt_properties_anim_get_double(properties, "mix", position, length);

        slice_desc desc;
        // We need to create a new frame as this effect modifies the input
        desc.image = *image;
        desc.dest = mlt_pool_alloc(size);
        desc.width = *width;
        desc.height = *height;
        desc.x_scatter = x_scatter;
        desc.y_scatter = y_scatter;
        desc.max_luma = max_luma;
        desc.zero_mix = (mix == 0.0f);

        for (int i = 0; i < 1276; i++) {
            float sum = scale * i;
            desc.luma_lut[i] = !invert ? (sum >= min && sum <= max_luma ? invert_luma - sum
                                          : sum < min                   ? max_luma
                                                                        : min)
                                       : (sum >= min && sum <= max_luma ? sum
                                          : sum < min                   ? min
                                                                        : max_luma);
        }
        for (int i = 0; i < 256; i++) {
            int val = 128 + mix * (i - 128);
            desc.chroma_lut[i] = CLAMP(val, min, max_chroma);
        }

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
