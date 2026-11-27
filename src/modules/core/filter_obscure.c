/*
 * filter_obscure.c -- obscure filter
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
#include <framework/mlt_profile.h>
#include <framework/mlt_slices.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

/** Geometry struct.
*/

struct geometry_s
{
    int nw;
    int nh;
    float x;
    float y;
    float w;
    float h;
    int mask_w;
    int mask_h;
};

/** Parse a value from a geometry string.
*/

static inline float parse_value(char **ptr, int normalization, char delim, float defaults)
{
    float value = defaults;

    if (*ptr != NULL && **ptr != '\0') {
        char *end = NULL;
        value = strtod(*ptr, &end);
        if (end != NULL) {
            if (*end == '%')
                value = (value / 100.0) * normalization;
            while (*end == delim || *end == '%' || (delim == ',' && *end == '/'))
                end++;
        }
        *ptr = end;
    }

    return value;
}

/** Parse a geometry property string.
*/

static void geometry_parse(
    struct geometry_s *geometry, struct geometry_s *defaults, char *property, int nw, int nh)
{
    // Assign normalized width and height
    geometry->nw = nw;
    geometry->nh = nh;

    // Assign from defaults if available
    if (defaults != NULL) {
        geometry->x = defaults->x;
        geometry->y = defaults->y;
        geometry->w = defaults->w;
        geometry->h = defaults->h;
        geometry->mask_w = defaults->mask_w;
        geometry->mask_h = defaults->mask_h;
    } else {
        geometry->x = 0;
        geometry->y = 0;
        geometry->w = nw;
        geometry->h = nh;
        geometry->mask_w = 20;
        geometry->mask_h = 20;
    }

    // Parse the geometry string
    if (property != NULL) {
        char *ptr = property;
        geometry->x = parse_value(&ptr, nw, ',', geometry->x);
        geometry->y = parse_value(&ptr, nh, ':', geometry->y);
        geometry->w = parse_value(&ptr, nw, 'x', geometry->w);
        geometry->h = parse_value(&ptr, nh, ':', geometry->h);
        geometry->mask_w = parse_value(&ptr, nw, 'x', geometry->mask_w);
        geometry->mask_h = parse_value(&ptr, nh, ' ', geometry->mask_h);
    }
}

/** A Timism but not as clean ;-).
*/

static float lerp(float value, float lower, float upper)
{
    if (value < lower)
        return lower;
    else if (upper > lower && value > upper)
        return upper;
    return value;
}

/** Calculate real geometry.
*/

static void geometry_calculate(struct geometry_s *output,
                               struct geometry_s *in,
                               struct geometry_s *out,
                               float position,
                               int ow,
                               int oh)
{
    // Calculate this frames geometry
    output->x = lerp((in->x + (out->x - in->x) * position) / (float) out->nw * ow, 0, ow);
    output->y = lerp((in->y + (out->y - in->y) * position) / (float) out->nh * oh, 0, oh);
    output->w = lerp((in->w + (out->w - in->w) * position) / (float) out->nw * ow,
                     0,
                     ow - output->x);
    output->h = lerp((in->h + (out->h - in->h) * position) / (float) out->nh * oh,
                     0,
                     oh - output->y);
    output->mask_w = lerp(in->mask_w + (out->mask_w - in->mask_w) * position, 1, -1);
    output->mask_h = lerp(in->mask_h + (out->mask_h - in->mask_h) * position, 1, -1);
}

/** The averaging function...
*/

static inline void obscure_average(uint8_t *start, int width, int height, int stride)
{
    int Y = (start[0] + start[2]) / 2;
    int U = start[1];
    int V = start[3];
    int components = width >> 1;
    uint8_t *row = start;

    for (int y = 0; y < height; y++) {
        const uint8_t *p = row;
        for (int x = 0; x < components; x++, p += 4) {
            Y = (Y + p[0]) >> 1;
            U = (U + p[1]) >> 1;
            Y = (Y + p[2]) >> 1;
            V = (V + p[3]) >> 1;
        }
        row += stride;
    }

    uint32_t yuyv = (uint32_t) (uint8_t) Y | ((uint32_t) (uint8_t) U << 8)
                    | ((uint32_t) (uint8_t) Y << 16) | ((uint32_t) (uint8_t) V << 24);
#ifdef __SSE2__
    __m128i vyuyv = _mm_set1_epi32((int) yuyv);
#endif
    row = start;
    for (int y = 0; y < height; y++) {
        uint8_t *p = row;
        int x = 0;
#ifdef __SSE2__
        for (; x <= components - 4; x += 4, p += 16) {
            _mm_storeu_si128((__m128i *) p, vyuyv);
        }
#endif
        for (; x < components; x++, p += 4) {
            memcpy(p, &yuyv, 4);
        }
        row += stride;
    }
}

typedef struct
{
    uint8_t *base;
    int stride;
    int area_w;
    int area_h;
    int mw;
    int mh;
    int tile_rows;
} obscure_slice_desc;

static int obscure_slice_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    const obscure_slice_desc *d = (const obscure_slice_desc *) data;
    int row_start, row_count = mlt_slices_size_slice(jobs, index, d->tile_rows, &row_start);
    const int stride = d->stride;
    const int area_w = d->area_w;
    const int area_h = d->area_h;
    const int mw = d->mw;
    const int mh = d->mh;

    for (int tr = row_start; tr < row_start + row_count; tr++) {
        int h = tr * mh;
        int ah = (h + mh > area_h) ? (area_h - h) : mh;
        if (ah <= 1)
            continue;
        uint8_t *row_base = d->base + h * stride;
        for (int w = 0; w < area_w; w += mw) {
            int aw = (w + mw > area_w) ? (area_w - w) : mw;
            if (aw > 1)
                obscure_average(row_base + (w << 1), aw, ah, stride);
        }
    }
    return 0;
}

/** The obscurer rendering function...
*/

static void obscure_render(uint8_t *image, int width, int height, struct geometry_s result)
{
    (void) height;
    int area_x = result.x;
    int area_y = result.y;
    int area_w = result.w;
    int area_h = result.h;

    int mw = result.mask_w;
    int mh = result.mask_h;

    if (area_w <= 1 || area_h <= 1 || mw <= 0 || mh <= 0)
        return;

    int tile_rows = (area_h + mh - 1) / mh;
    obscure_slice_desc desc = {
        .base = image + area_y * width * 2 + area_x * 2,
        .stride = width << 1,
        .area_w = area_w,
        .area_h = area_h,
        .mw = mw,
        .mh = mh,
        .tile_rows = tile_rows,
    };

    if (tile_rows >= 4 && area_w * area_h >= 65536) {
        mlt_slices_run_normal(0, obscure_slice_proc, &desc);
    } else {
        obscure_slice_proc(0, 0, 1, &desc);
    }
}

/** Do it :-).
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    // Pop the top of stack now
    mlt_filter filter = mlt_frame_pop_service(frame);

    // Get the image from the frame
    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    // Get the image from the frame
    if (error == 0) {
        if (filter != NULL) {
            // Get the filter properties
            mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
            mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));

            // Structures for geometry
            struct geometry_s result;
            struct geometry_s start;
            struct geometry_s end;

            // Retrieve the position
            float position = mlt_filter_get_progress(filter, frame);

            // Now parse the geometries
            geometry_parse(&start,
                           NULL,
                           mlt_properties_get(properties, "start"),
                           profile->width,
                           profile->height);
            geometry_parse(&end,
                           &start,
                           mlt_properties_get(properties, "end"),
                           profile->width,
                           profile->height);

            // Do the calculation
            geometry_calculate(&result, &start, &end, position, *width, *height);

            // Now actually render it
            obscure_render(*image, *width, *height, result);
        }
    }

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Push this on to the service stack
    mlt_frame_push_service(frame, filter);

    // Push the get image call
    mlt_frame_push_get_image(frame, filter_get_image);

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_obscure_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        filter->process = filter_process;
        mlt_properties_set(properties, "start", arg != NULL ? arg : "0%/0%:100%x100%");
        mlt_properties_set(properties, "end", "");
    }
    return filter;
}
