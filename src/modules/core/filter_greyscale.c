/*
 * filter_greyscale.c -- greyscale filter
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <stdio.h>
#include <stdlib.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

/** Do it :-).
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    if (error == 0) {
        uint8_t *p = *image;
        int total_bytes = *width * *height * 2;
        int x = 0;
#ifdef __SSE2__
        __m128i mask_y = _mm_set1_epi16(0x00FF);
        __m128i chroma128 = _mm_set1_epi16((short) 0x8000);
        for (; x <= total_bytes - 16; x += 16) {
            __m128i v = _mm_loadu_si128((const __m128i *) (p + x));
            v = _mm_or_si128(_mm_and_si128(v, mask_y), chroma128);
            _mm_storeu_si128((__m128i *) (p + x), v);
        }
#endif
        for (; x < total_bytes; x += 2) {
            p[x + 1] = 128;
        }
    }
    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_greyscale_init(mlt_profile profile,
                                 mlt_service_type type,
                                 const char *id,
                                 char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL)
        filter->process = filter_process;
    return filter;
}
