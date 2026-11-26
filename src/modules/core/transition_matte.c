/*
 * transition_matte.c -- replace alpha channel of track
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
 * Author: Maksym Veremeyenko <verem@m1stereo.tv>
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

#include <framework/mlt.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

static inline void copy_Y_to_A_scaled_luma_span(uint8_t *alpha_a, const uint8_t *image_b, int count)
{
    int i = 0;
#ifdef __SSE2__
    const __m128i v_mask = _mm_set1_epi16(0x00FF);
    const __m128i v_16 = _mm_set1_epi16(16);
    const __m128i v_235 = _mm_set1_epi16(235);
    const __m128i v_299 = _mm_set1_epi16(299);

    for (; i <= count - 16; i += 16) {
        __m128i y0 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (image_b + i * 2)), v_mask);
        __m128i y1 = _mm_and_si128(_mm_loadu_si128((const __m128i *) (image_b + i * 2 + 16)),
                                   v_mask);

        y0 = _mm_sub_epi16(_mm_max_epi16(_mm_min_epi16(y0, v_235), v_16), v_16);
        y1 = _mm_sub_epi16(_mm_max_epi16(_mm_min_epi16(y1, v_235), v_16), v_16);

        y0 = _mm_srli_epi16(_mm_mullo_epi16(y0, v_299), 8);
        y1 = _mm_srli_epi16(_mm_mullo_epi16(y1, v_299), 8);

        _mm_storeu_si128((__m128i *) (alpha_a + i), _mm_packus_epi16(y0, y1));
    }
#endif
    for (; i < count; i++) {
        unsigned int p = image_b[2 * i];

        if (p < 16)
            p = 16;
        if (p > 235)
            p = 235;
        /* p = (p - 16) * 255 / 219; */
        p -= 16;
        p = ((p << 8) + (p * 43)) >> 8;

        alpha_a[i] = (uint8_t) p;
    }
}

static void copy_Y_to_A_scaled_luma(
    uint8_t *alpha_a, int stride_a, uint8_t *image_b, int stride_b, int width, int height)
{
    if (stride_a == width && stride_b == width * 2) {
        copy_Y_to_A_scaled_luma_span(alpha_a, image_b, width * height);
        return;
    }
    for (int j = 0; j < height; j++) {
        copy_Y_to_A_scaled_luma_span(alpha_a, image_b, width);
        alpha_a += stride_a;
        image_b += stride_b;
    }
}

/** Get the image.
*/

static int transition_get_image(mlt_frame a_frame,
                                uint8_t **image,
                                mlt_image_format *format,
                                int *width,
                                int *height,
                                int writable)
{
    // Get the b frame from the stack
    mlt_frame b_frame = mlt_frame_pop_frame(a_frame);

    mlt_frame_get_image(a_frame, image, format, width, height, 1);

    // Get the properties of the a frame
    mlt_properties a_props = MLT_FRAME_PROPERTIES(a_frame);

    int width_a = mlt_properties_get_int(a_props, "width"), width_b = width_a,
        height_a = mlt_properties_get_int(a_props, "height"), height_b = height_a;

    uint8_t *alpha_a, *image_b;

    // This transition is yuv422 only
    *format = mlt_image_yuv422;

    // Get the image from the a frame
    mlt_frame_get_image(b_frame, &image_b, format, &width_b, &height_b, 1);

    // Create the alpha channel based on the b_image dimensions
    int alpha_width = MIN(width_a, width_b);
    int alpha_height = MIN(height_a, height_b);
    int size = alpha_width * alpha_height;
    alpha_a = mlt_pool_alloc(size);
    if (alpha_width < width_a)
        memset(alpha_a, 255, size);
    mlt_frame_set_alpha(a_frame, alpha_a, size, mlt_pool_release);

    // Copy luma from image_b to a_frame's new alpha channel
    copy_Y_to_A_scaled_luma(alpha_a, width_a, image_b, width_b * 2, alpha_width, alpha_height);

    // Extract the a_frame image info
    *width = mlt_properties_get_int(a_props, "width");
    *height = mlt_properties_get_int(a_props, "height");
    *image = mlt_properties_get_data(a_props, "image", NULL);

    return 0;
}

/** Matte transition processing.
*/

static mlt_frame transition_process(mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame)
{
    // Push the b_frame on to the stack
    mlt_frame_push_frame(a_frame, b_frame);

    // Push the transition method
    mlt_frame_push_get_image(a_frame, transition_get_image);

    return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_matte_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *lumafile)
{
    mlt_transition transition = mlt_transition_new();
    if (transition != NULL) {
        // Set the methods
        transition->process = transition_process;

        // Inform apps and framework that this is a video only transition
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "_transition_type", 1);

        return transition;
    }
    return NULL;
}
