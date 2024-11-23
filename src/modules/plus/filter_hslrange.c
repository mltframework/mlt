/*
 * filter_hslrange.cpp
 * Copyright (C) 2024 Meltytech, LLC
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

typedef struct
{
    mlt_filter filter;
    uint8_t *image;
    mlt_image_format format;
    int width;
    int height;
    float hue_center;
    float hue_range;
    float hue_range_max;
    float hue_range_min;
    float blend;
    float blend_range;
    float blend_threshold;
    float h_shift;
    float s_scale;
    float l_scale;
} sliced_desc;

static void adjust_pixel(uint8_t *sample, sliced_desc *desc)
{
    float h, s, l;
    rgbToHsl(sample[0] / 255.0, sample[1] / 255.0, sample[2] / 255.0, &h, &s, &l);
    if (s == 0) {
        // No color. Do not adjust.
        return;
    }

    float hue_distance = 1.0;
    if (desc->hue_range_max > desc->hue_range_min) {
        if (h < desc->hue_range_max && h > desc->hue_range_min) {
            hue_distance = fabs(desc->hue_center - h);
        }
    } else {
        // Handle the case were the range wraps around 0
        if (h < desc->hue_range_max) {
            hue_distance = fabs(desc->hue_range - (desc->hue_range_max - h));
        } else if (h > desc->hue_range_min) {
            hue_distance = fabs(desc->hue_range - (h - desc->hue_range_min));
        }
    }
    if (hue_distance >= 1.0) {
        // This sample is outside the range. Do not adjust
        return;
    }

    float h_shift = desc->h_shift;
    float s_scale = desc->s_scale;
    float l_scale = desc->l_scale;

    if (hue_distance > desc->blend_threshold) {
        float blend_strength = 1.0 - (hue_distance - desc->blend_threshold) / desc->blend_range;
        h_shift = h_shift * blend_strength;
        s_scale = (desc->s_scale * blend_strength) + (1.0 - blend_strength);
        l_scale = (desc->l_scale * blend_strength) + (1.0 - blend_strength);
    }

    if (h_shift == 0.0 && s_scale == 1.0 && l_scale == 1.0) {
        // No adjustment for this pixel
        return;
    }

    // Apply the adjustment
    h = h + h_shift;
    h = fmod(h, 1.0);
    s = s * s_scale;
    s = s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s;
    l = l * l_scale;
    l = l < 0.0 ? 0.0 : l > 1.0 ? 1.0 : l;
    float r, g, b;
    hslToRgb(h, s, l, &r, &g, &b);
    sample[0] = lrint(r * 255.0);
    sample[1] = lrint(g * 255.0);
    sample[2] = lrint(b * 255.0);
}

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    sliced_desc *desc = ((sliced_desc *) data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    int total = desc->width * slice_height + 1;
    uint8_t *sample = desc->image
                      + slice_line_start
                            * mlt_image_format_size(desc->format, desc->width, 1, NULL);

    switch (desc->format) {
    case mlt_image_rgb:
        while (--total) {
            adjust_pixel(sample, desc);
            sample += 3;
        }
        break;
    case mlt_image_rgba:
        while (--total) {
            adjust_pixel(sample, desc);
            sample += 4;
        }
        break;
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
    desc.hue_center = mlt_properties_anim_get_double(properties, "hue_center", position, length);
    desc.hue_range = mlt_properties_anim_get_double(properties, "hue_range", position, length);
    desc.blend = mlt_properties_anim_get_double(properties, "blend", position, length);
    desc.h_shift = mlt_properties_anim_get_double(properties, "h_shift", position, length);
    desc.s_scale = mlt_properties_anim_get_double(properties, "s_scale", position, length);
    desc.l_scale = mlt_properties_anim_get_double(properties, "l_scale", position, length);

    // Check if there is any processing to do
    if (desc.hue_range == 0.0
        || (desc.h_shift == 0.0 && desc.s_scale == 100.0 && desc.l_scale == 100.0)) {
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }
    int error = 0;

    // Make sure the format is acceptable
    if (*format != mlt_image_rgb && *format != mlt_image_rgba) {
        *format = mlt_image_rgb;
    }

    // Get the image
    writable = 1;
    error = mlt_frame_get_image(frame, image, format, width, height, writable);
    if (!error) {
        // Scale the parameters down to [0-1]
        desc.hue_center /= 360.0;
        desc.hue_range /= 360.0;
        desc.blend /= 100.0;
        desc.h_shift /= 360.0;
        desc.s_scale /= 100.0;
        desc.l_scale /= 100.0;
        // Precompute some variables
        desc.hue_range /= 2; // Range from center, not whole range
        desc.hue_range_min = desc.hue_center - desc.hue_range;
        if (desc.hue_range_min < 0.0)
            desc.hue_range_min += 1.0;
        desc.hue_range_max = fmod(desc.hue_center + desc.hue_range, 1.0);
        desc.blend_range = desc.hue_range * desc.blend;
        desc.blend_threshold = desc.hue_range - desc.blend_range;
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

mlt_filter filter_hslrange_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_double(properties, "hue_center", 180);
        mlt_properties_set_double(properties, "hue_range", 0);
        mlt_properties_set_double(properties, "blend", 0);
        mlt_properties_set_double(properties, "h_shift", 0);
        mlt_properties_set_double(properties, "s_scale", 100);
        mlt_properties_set_double(properties, "l_scale", 100);
        filter->process = filter_process;
    }
    return filter;
}
