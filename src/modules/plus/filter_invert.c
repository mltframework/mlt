/*
 * filter_invert.c -- invert filter
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

static void invert_yuv422(uint8_t *p, int total_bytes, int full_range)
{
    int min = full_range ? 0 : 16;
    int max_luma = full_range ? 255 : 235;
    int max_chroma = full_range ? 255 : 240;
    int invert_luma = full_range ? 255 : 251;
    int x = 0;

#ifdef __SSE2__
    if (full_range) {
        __m128i v255 = _mm_set1_epi8((char) 255);
        __m128i v_add1 = _mm_set1_epi16((short) 0x0100);
        for (; x <= total_bytes - 16; x += 16) {
            __m128i v = _mm_loadu_si128((const __m128i *) (p + x));
            v = _mm_subs_epu8(v255, v);
            v = _mm_adds_epu8(v, v_add1);
            _mm_storeu_si128((__m128i *) (p + x), v);
        }
    } else {
        __m128i v_base = _mm_set1_epi16((short) ((255u << 8) | 251u));
        __m128i v_add1 = _mm_set1_epi16((short) 0x0100);
        __m128i v_min = _mm_set1_epi8((char) 16);
        __m128i v_max = _mm_set1_epi16((short) ((240u << 8) | 235u));
        for (; x <= total_bytes - 16; x += 16) {
            __m128i v = _mm_loadu_si128((const __m128i *) (p + x));
            v = _mm_subs_epu8(v_base, v);
            v = _mm_adds_epu8(v, v_add1);
            v = _mm_max_epu8(v, v_min);
            v = _mm_min_epu8(v, v_max);
            _mm_storeu_si128((__m128i *) (p + x), v);
        }
    }
#endif

    for (; x < total_bytes; x += 2) {
        p[x] = CLAMP(invert_luma - p[x], min, max_luma);
        p[x + 1] = CLAMP(256 - p[x + 1], min, max_chroma);
    }
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    // Get the image
    mlt_filter filter = mlt_frame_pop_service(frame);

    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    // Only process if we have no error and a valid color space
    if (error == 0 && *format == mlt_image_yuv422) {
        int full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
        invert_yuv422(*image, *width * *height * 2, full_range);

        int mask = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "alpha");
        if (mask) {
            int size = *width * *height;
            uint8_t *alpha = mlt_pool_alloc(size);
            memset(alpha, mask, size);
            mlt_frame_set_alpha(frame, alpha, size, mlt_pool_release);
        }
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

mlt_filter filter_invert_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
    }
    return filter;
}
