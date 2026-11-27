/*
 * filter_sepia.c -- sepia filter
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

#ifdef __SSE2__
#include <emmintrin.h>
#endif

static inline void sepia_span(uint8_t *p, int len, uint8_t u, uint8_t v)
{
    int x = 0;
#ifdef __SSE2__
    __m128i mask_y = _mm_set1_epi16(0x00FF);
    __m128i v_uv = _mm_set1_epi32((int) (((uint32_t) u << 8) | ((uint32_t) v << 24)));
    for (; x <= len - 16; x += 16) {
        __m128i vec = _mm_loadu_si128((const __m128i *) (p + x));
        vec = _mm_or_si128(_mm_and_si128(vec, mask_y), v_uv);
        _mm_storeu_si128((__m128i *) (p + x), vec);
    }
#endif
    for (; x <= len - 4; x += 4) {
        p[x + 1] = u;
        p[x + 3] = v;
    }
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

    // Get the image
    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    // Only process if we have no error and a valid colour space
    if (error == 0 && *image) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_position length = mlt_filter_get_length2(filter, frame);
        uint8_t u = (uint8_t) mlt_properties_anim_get_int(properties, "u", position, length);
        uint8_t v = (uint8_t) mlt_properties_anim_get_int(properties, "v", position, length);
        int w = *width;
        int h = *height;
        int line_size = w * 2;

        if ((w & 1) == 0) {
            sepia_span(*image, line_size * h, u, v);
        } else {
            int even_len = (w - 1) * 2;
            for (int y = 0; y < h; y++) {
                uint8_t *p = *image + y * line_size;
                sepia_span(p, even_len, u, v);
                p[line_size - 1] = u;
            }
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

mlt_filter filter_sepia_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "u", "75");
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "v", "150");
    }
    return filter;
}
