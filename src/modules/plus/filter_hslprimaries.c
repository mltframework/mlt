/*
 * filter_hslprimaries.c
 * Copyright (C) 2024-2025 Meltytech, LLC
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

#include "hsl.h"

#include <framework/mlt.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif

static const float MAX_OVERLAP_RANGE = 29.9;

enum {
    HSL_REGION_RED,
    HSL_REGION_YELLOW,
    HSL_REGION_GREEN,
    HSL_REGION_CYAN,
    HSL_REGION_BLUE,
    HSL_REGION_MAGENTA,
    HSL_REGION_COUNT,
};

struct
{
    float beg;
    float mid;
    float end;
} hueRegions[HSL_REGION_COUNT] = {
    [HSL_REGION_RED] = {330.0 / 360.0, 0.0 / 360.0, 30.0 / 360.0},
    [HSL_REGION_YELLOW] = {30.0 / 360.0, 60.0 / 360.0, 90.0 / 360.0},
    [HSL_REGION_GREEN] = {90.0 / 360.0, 120.0 / 360.0, 150.0 / 360.0},
    [HSL_REGION_CYAN] = {150.0 / 360.0, 180.0 / 360.0, 210.0 / 360.0},
    [HSL_REGION_BLUE] = {210.0 / 360.0, 240.0 / 360.0, 270.0 / 360.0},
    [HSL_REGION_MAGENTA] = {270.0 / 360.0, 300.0 / 360.0, 330.0 / 360.0},
};

typedef struct
{
    mlt_filter filter;
    uint8_t *image;
    mlt_image_format format;
    int width;
    int height;
    float h_shift[HSL_REGION_COUNT];
    float s_scale[HSL_REGION_COUNT];
    float l_scale[HSL_REGION_COUNT];
    float overlap;
    float overlap_range;
} sliced_desc;

static inline long fast_lrint_double(double x)
{
#ifdef __SSE2__
    return _mm_cvtsd_si32(_mm_set_sd(x));
#else
    return lrint(x);
#endif
}

static float u8_to_float_lut[256];
static int u8_lut_initialized = 0;

static void init_u8_lut(void)
{
    if (!u8_lut_initialized) {
        for (int i = 0; i < 256; i++) {
            u8_to_float_lut[i] = (float) ((double) i / 255.0);
        }
        u8_lut_initialized = 1;
    }
}

static inline int hueToRegion(float h)
{
    for (int i = 0; i < HSL_REGION_COUNT; i++) {
        if (h < hueRegions[i].end) {
            return i;
        }
    }
    return HSL_REGION_RED;
}

static inline int adjust_pixel(float *r, float *g, float *b, const sliced_desc *desc)
{
    float h, s, l;
    rgbToHsl(*r, *g, *b, &h, &s, &l);
    if (s == 0.0f) {
        // No color. Do not adjust.
        return 0;
    }

    int region = hueToRegion(h);
    float h_shift = desc->h_shift[region];
    float s_scale = desc->s_scale[region];
    float l_scale = desc->l_scale[region];

    if (desc->overlap != 0.0f) {
        int o_region = region;
        float o_strength = 0.0f;
        float overlap_range = desc->overlap_range;
        if (region == HSL_REGION_RED) {
            // Special case to handle red wrap around
            if (h > hueRegions[HSL_REGION_RED].beg
                && h < (hueRegions[HSL_REGION_RED].beg + overlap_range)) {
                o_region = HSL_REGION_MAGENTA;
                o_strength = (h - hueRegions[region].beg) / desc->overlap_range;
            } else if (h < hueRegions[HSL_REGION_RED].end
                       && h > (hueRegions[HSL_REGION_RED].end - overlap_range)) {
                o_region = HSL_REGION_YELLOW;
                o_strength = (hueRegions[region].end - h) / desc->overlap_range;
            }
        } else if (h < (hueRegions[region].beg + overlap_range)) {
            o_region = (region - 1 + HSL_REGION_COUNT) % HSL_REGION_COUNT;
            o_strength = (h - hueRegions[region].beg) / desc->overlap_range;
        } else if (h > (hueRegions[region].end - overlap_range)) {
            o_region = (region + 1) % HSL_REGION_COUNT;
            o_strength = (hueRegions[region].end - h) / desc->overlap_range;
        }
        if (o_region != region) {
            double inv_strength = 1.0 - o_strength;
            float edge_hshift = (desc->h_shift[region] + desc->h_shift[o_region]) / 2.0;
            h_shift = (desc->h_shift[region] * o_strength) + (edge_hshift * inv_strength);
            float edge_sscale = (desc->s_scale[region] + desc->s_scale[o_region]) / 2.0;
            s_scale = (desc->s_scale[region] * o_strength) + (edge_sscale * inv_strength);
            float edge_lscale = (desc->l_scale[region] + desc->l_scale[o_region]) / 2.0;
            l_scale = (desc->l_scale[region] * o_strength) + (edge_lscale * inv_strength);
        }
    }

    if (h_shift == 0.0f && s_scale == 1.0f && l_scale == 1.0f) {
        // No adjustment for this pixel
        return 0;
    }

    // Apply the adjustment
    h = h + h_shift;
    if (h >= 1.0f || h <= -1.0f) {
        h -= (float) (int) h;
    }
    s = s * s_scale;
    s = s < 0.0f ? 0.0f : s > 1.0f ? 1.0f : s;
    l = l * l_scale;
    l = l < 0.0f ? 0.0f : l > 1.0f ? 1.0f : l;
    hslToRgb(h, s, l, r, g, b);
    return 1;
}

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    sliced_desc *desc = ((sliced_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    int total = desc->width * slice_height;

    switch (desc->format) {
    case mlt_image_rgb: {
        uint8_t *restrict sample = desc->image + slice_line_start * desc->width * 3;
        const float *restrict lut = u8_to_float_lut;
        for (int i = 0; i < total; i++) {
            float r = lut[sample[0]];
            float g = lut[sample[1]];
            float b = lut[sample[2]];
            if (adjust_pixel(&r, &g, &b, desc)) {
                sample[0] = (uint8_t) fast_lrint_double((double) r * 255.0);
                sample[1] = (uint8_t) fast_lrint_double((double) g * 255.0);
                sample[2] = (uint8_t) fast_lrint_double((double) b * 255.0);
            }
            sample += 3;
        }
        break;
    }
    case mlt_image_rgba: {
        uint8_t *restrict sample = desc->image + slice_line_start * desc->width * 4;
        const float *restrict lut = u8_to_float_lut;
        for (int i = 0; i < total; i++) {
            float r = lut[sample[0]];
            float g = lut[sample[1]];
            float b = lut[sample[2]];
            if (adjust_pixel(&r, &g, &b, desc)) {
                sample[0] = (uint8_t) fast_lrint_double((double) r * 255.0);
                sample[1] = (uint8_t) fast_lrint_double((double) g * 255.0);
                sample[2] = (uint8_t) fast_lrint_double((double) b * 255.0);
            }
            sample += 4;
        }
        break;
    }
    case mlt_image_rgba64: {
        uint16_t *restrict sample = (uint16_t *) desc->image + slice_line_start * desc->width * 4;
        const float inv65535 = 1.0f / 65535.0f;
        for (int i = 0; i < total; i++) {
            float r = (float) sample[0] * inv65535;
            float g = (float) sample[1] * inv65535;
            float b = (float) sample[2] * inv65535;
            if (adjust_pixel(&r, &g, &b, desc)) {
                sample[0] = (uint16_t) fast_lrint_double((double) r * 65535.0);
                sample[1] = (uint16_t) fast_lrint_double((double) g * 65535.0);
                sample[2] = (uint16_t) fast_lrint_double((double) b * 65535.0);
            }
            sample += 4;
        }
        break;
    }
    default:
        mlt_log_error(MLT_FILTER_SERVICE(desc->filter),
                      "Invalid image format: %s\n",
                      mlt_image_format_name(desc->format));
        break;
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
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    sliced_desc desc;
    desc.h_shift[HSL_REGION_RED]
        = mlt_properties_anim_get_double(properties, "h_shift_red", position, length);
    desc.s_scale[HSL_REGION_RED]
        = mlt_properties_anim_get_double(properties, "s_scale_red", position, length);
    desc.l_scale[HSL_REGION_RED]
        = mlt_properties_anim_get_double(properties, "l_scale_red", position, length);
    desc.h_shift[HSL_REGION_YELLOW]
        = mlt_properties_anim_get_double(properties, "h_shift_yellow", position, length);
    desc.s_scale[HSL_REGION_YELLOW]
        = mlt_properties_anim_get_double(properties, "s_scale_yellow", position, length);
    desc.l_scale[HSL_REGION_YELLOW]
        = mlt_properties_anim_get_double(properties, "l_scale_yellow", position, length);
    desc.h_shift[HSL_REGION_GREEN]
        = mlt_properties_anim_get_double(properties, "h_shift_green", position, length);
    desc.s_scale[HSL_REGION_GREEN]
        = mlt_properties_anim_get_double(properties, "s_scale_green", position, length);
    desc.l_scale[HSL_REGION_GREEN]
        = mlt_properties_anim_get_double(properties, "l_scale_green", position, length);
    desc.h_shift[HSL_REGION_CYAN]
        = mlt_properties_anim_get_double(properties, "h_shift_cyan", position, length);
    desc.s_scale[HSL_REGION_CYAN]
        = mlt_properties_anim_get_double(properties, "s_scale_cyan", position, length);
    desc.l_scale[HSL_REGION_CYAN]
        = mlt_properties_anim_get_double(properties, "l_scale_cyan", position, length);
    desc.h_shift[HSL_REGION_BLUE]
        = mlt_properties_anim_get_double(properties, "h_shift_blue", position, length);
    desc.s_scale[HSL_REGION_BLUE]
        = mlt_properties_anim_get_double(properties, "s_scale_blue", position, length);
    desc.l_scale[HSL_REGION_BLUE]
        = mlt_properties_anim_get_double(properties, "l_scale_blue", position, length);
    desc.h_shift[HSL_REGION_MAGENTA]
        = mlt_properties_anim_get_double(properties, "h_shift_magenta", position, length);
    desc.s_scale[HSL_REGION_MAGENTA]
        = mlt_properties_anim_get_double(properties, "s_scale_magenta", position, length);
    desc.l_scale[HSL_REGION_MAGENTA]
        = mlt_properties_anim_get_double(properties, "l_scale_magenta", position, length);
    desc.overlap = mlt_properties_anim_get_double(properties, "overlap", position, length);

    // Check if there is any processing to do
    int no_change = 1;
    for (int i = 0; i < HSL_REGION_COUNT; i++) {
        if (desc.h_shift[i] != 0.0 || desc.s_scale[i] != 100.0 || desc.l_scale[i] != 100.0) {
            no_change = 0;
            break;
        }
    }
    if (no_change) {
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }
    int error = 0;

    // Make sure the format is acceptable
    if (*format != mlt_image_rgb && *format != mlt_image_rgba && *format != mlt_image_rgba64) {
        *format = mlt_image_rgb;
    }

    // Get the image
    writable = 1;
    error = mlt_frame_get_image(frame, image, format, width, height, writable);
    if (!error) {
        // Scale the parameters down to [0-1]
        for (int i = 0; i < HSL_REGION_COUNT; i++) {
            desc.h_shift[i] /= 360.0;
            desc.s_scale[i] /= 100.0;
            desc.l_scale[i] /= 100.0;
        }
        desc.overlap /= 100.0;
        init_u8_lut();
        desc.overlap_range = desc.overlap * MAX_OVERLAP_RANGE / 360.0;
        // Perform the processing
        desc.format = *format;
        desc.height = *height;
        desc.width = *width;
        desc.image = *image;
        mlt_slices_run_normal(0, sliced_proc, &desc);
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_hslprimaries_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_double(properties, "h_shift_red", 0);
        mlt_properties_set_double(properties, "s_scale_red", 100);
        mlt_properties_set_double(properties, "l_scale_red", 100);
        mlt_properties_set_double(properties, "h_shift_yellow", 0);
        mlt_properties_set_double(properties, "s_scale_yellow", 100);
        mlt_properties_set_double(properties, "l_scale_yellow", 100);
        mlt_properties_set_double(properties, "h_shift_green", 0);
        mlt_properties_set_double(properties, "s_scale_green", 100);
        mlt_properties_set_double(properties, "l_scale_green", 100);
        mlt_properties_set_double(properties, "h_shift_cyan", 0);
        mlt_properties_set_double(properties, "s_scale_cyan", 100);
        mlt_properties_set_double(properties, "l_scale_cyan", 100);
        mlt_properties_set_double(properties, "h_shift_blue", 0);
        mlt_properties_set_double(properties, "s_scale_blue", 100);
        mlt_properties_set_double(properties, "l_scale_blue", 100);
        mlt_properties_set_double(properties, "h_shift_magenta", 0);
        mlt_properties_set_double(properties, "s_scale_magenta", 100);
        mlt_properties_set_double(properties, "l_scale_magenta", 100);
        mlt_properties_set_double(properties, "overlap", 0);
        filter->process = filter_process;
    }
    return filter;
}
