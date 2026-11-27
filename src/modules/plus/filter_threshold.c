/*
 * filter_threshold.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2008-2022 Meltytech, LLC
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <string.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

static inline void fill_yuv422_const(uint8_t *p, int size, uint8_t y_val)
{
    int i = 0;
#ifdef __SSE2__
    __m128i fill = _mm_set1_epi16((short) ((uint16_t) y_val | (128u << 8)));
    for (; i <= size - 16; i += 16) {
        _mm_storeu_si128((__m128i *) (p + i), fill);
    }
#endif
    for (; i < size; i += 2) {
        p[i] = y_val;
        p[i + 1] = 128;
    }
}

static void threshold_yuv422(uint8_t *p,
                             const uint8_t *alpha,
                             int pixels,
                             int midpoint,
                             int use_alpha,
                             int invert,
                             int full_luma)
{
    int size = pixels * 2;
    uint8_t white = full_luma ? 255 : 235;
    uint8_t black = full_luma ? 0 : 16;
    uint8_t A = invert ? white : black;
    uint8_t B = invert ? black : white;

    if (!use_alpha) {
        if (midpoint <= 0) {
            fill_yuv422_const(p, size, B);
        } else if (midpoint > 255) {
            fill_yuv422_const(p, size, A);
        } else {
            int i = 0;
#ifdef __SSE2__
            __m128i mask_y = _mm_set1_epi16(0x00FF);
            __m128i vmid = _mm_set1_epi16((short) midpoint);
            __m128i pixA = _mm_set1_epi16((short) ((uint16_t) A | (128u << 8)));
            __m128i pixB = _mm_set1_epi16((short) ((uint16_t) B | (128u << 8)));
            for (; i <= size - 16; i += 16) {
                __m128i v = _mm_loadu_si128((const __m128i *) (p + i));
                __m128i y16 = _mm_and_si128(v, mask_y);
                __m128i cmp = _mm_cmplt_epi16(y16, vmid);
                __m128i out = _mm_or_si128(_mm_and_si128(cmp, pixA), _mm_andnot_si128(cmp, pixB));
                _mm_storeu_si128((__m128i *) (p + i), out);
            }
#endif
            for (; i < size; i += 2) {
                p[i] = (p[i] < midpoint) ? A : B;
                p[i + 1] = 128;
            }
        }
    } else {
        if (alpha) {
            if (midpoint <= 0) {
                fill_yuv422_const(p, size, B);
            } else if (midpoint > 255) {
                fill_yuv422_const(p, size, A);
            } else {
                int j = 0;
#ifdef __SSE2__
                __m128i sign_flip = _mm_set1_epi8((char) 0x80);
                __m128i vmid = _mm_set1_epi8((char) (midpoint ^ 0x80));
                __m128i vecA = _mm_set1_epi8((char) A);
                __m128i vecB = _mm_set1_epi8((char) B);
                __m128i vec128 = _mm_set1_epi8((char) 128);
                for (; j <= pixels - 16; j += 16) {
                    __m128i a = _mm_loadu_si128((const __m128i *) (alpha + j));
                    __m128i cmp = _mm_cmplt_epi8(_mm_xor_si128(a, sign_flip), vmid);
                    __m128i y = _mm_or_si128(_mm_and_si128(cmp, vecA), _mm_andnot_si128(cmp, vecB));
                    _mm_storeu_si128((__m128i *) (p + j * 2), _mm_unpacklo_epi8(y, vec128));
                    _mm_storeu_si128((__m128i *) (p + j * 2 + 16), _mm_unpackhi_epi8(y, vec128));
                }
#endif
                for (; j < pixels; j++) {
                    p[j * 2] = (alpha[j] < midpoint) ? A : B;
                    p[j * 2 + 1] = 128;
                }
            }
        } else {
            fill_yuv422_const(p, size, B);
        }
    }
}

/** Get the images and apply the luminance of the mask to the alpha of the frame.
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);

    // Render the frame
    *format = mlt_image_yuv422;
    if (mlt_frame_get_image(frame, image, format, width, height, writable) == 0) {
        mlt_properties properties = mlt_filter_properties(filter);
        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_position length = mlt_filter_get_length2(filter, frame);
        int midpoint = mlt_properties_anim_get_int(properties, "midpoint", position, length);
        int use_alpha = mlt_properties_get_int(properties, "use_alpha");
        int invert = mlt_properties_get_int(properties, "invert");
        int full_luma = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_luma");
        const uint8_t *alpha = use_alpha ? mlt_frame_get_alpha(frame) : NULL;
        threshold_yuv422(*image, alpha, *width * *height, midpoint, use_alpha, invert, full_luma);
    }

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_threshold_init(mlt_profile profile,
                                 mlt_service_type type,
                                 const char *id,
                                 char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "midpoint", 128);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "use_alpha", 0);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "invert", 0);
        filter->process = filter_process;
    }
    return filter;
}
