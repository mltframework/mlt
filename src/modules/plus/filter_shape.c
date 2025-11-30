/*
 * filter_shape.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2008-2025 Meltytech, LLC
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

#include <framework/mlt.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <string.h>

typedef struct
{
    uint8_t *alpha;
    uint8_t *mask;
    int width;
    int height;
    int bpp; // bytes per pixel (1 for separate alpha, 4 for rgba)
    double softness;
    double mix;
    int invert;
    int invert_mask;
    double offset;
    double divisor;
} slice_desc;

typedef struct
{
    uint16_t *alpha;
    uint16_t *mask;
    int width;
    int height;
    double softness;
    double mix;
    int invert;
    int invert_mask;
    double offset;
    double divisor;
} slice_desc16;

static inline double smoothstep(const double e1, const double e2, const double a)
{
    if (a < e1)
        return 0.0;
    if (a >= e2)
        return 1.0;
    double v = (a - e1) / (e2 - e1);
    return (v * v * (3 - 2 * v));
}

static int slice_alpha_add(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * desc->bpp;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * desc->bpp;
    const int channel = desc->bpp == 4 ? 3 : 0;

    for (int i = 0; i < size; ++i) {
        uint32_t a = (uint32_t) (q[i * desc->bpp + channel] ^ desc->invert_mask);
        p[i * desc->bpp + channel] = ((uint8_t) MIN((uint32_t) p[i * desc->bpp + channel] + a, 255))
                                     ^ desc->invert;
    }

    return 0;
}

static int slice_alpha_maximum(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * desc->bpp;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * desc->bpp;
    const int channel = desc->bpp == 4 ? 3 : 0;

    for (int i = 0; i < size; ++i) {
        p[i * desc->bpp + channel] = MAX(p[i * desc->bpp + channel],
                                         (q[i * desc->bpp + channel] ^ desc->invert_mask))
                                     ^ desc->invert;
    }

    return 0;
}

static int slice_alpha_minimum(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * desc->bpp;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * desc->bpp;
    const int channel = desc->bpp == 4 ? 3 : 0;

    for (int i = 0; i < size; ++i) {
        p[i * desc->bpp + channel] = MIN(p[i * desc->bpp + channel],
                                         (q[i * desc->bpp + channel] ^ desc->invert_mask))
                                     ^ desc->invert;
    }

    return 0;
}

static int slice_alpha_overwrite(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * desc->bpp;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * desc->bpp;
    const int channel = desc->bpp == 4 ? 3 : 0;

    for (int i = 0; i < size; ++i) {
        p[i * desc->bpp + channel] = (q[i * desc->bpp + channel] ^ desc->invert_mask)
                                     ^ desc->invert;
    }

    return 0;
}

static int slice_alpha_subtract(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * desc->bpp;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * desc->bpp;

    for (int i = 0; i < size; ++i) {
        uint8_t a = q[i * desc->bpp + (desc->bpp == 4 ? 3 : 0)] ^ desc->invert_mask;
        p[i * desc->bpp] = (p[i * desc->bpp] > a ? p[i * desc->bpp] - a : 0) ^ desc->invert;
    }

    return 0;
}

static int slice_alpha_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((const slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * desc->bpp;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * desc->bpp;
    const int channel = desc->bpp == 4 ? 3 : 0;

    for (int i = 0; i < size; ++i) {
        double a = (double) (q[i * desc->bpp + channel] ^ desc->invert_mask) / desc->divisor;
        a = 1.0 - smoothstep(a, a + desc->softness, desc->mix);
        p[i * desc->bpp + channel] = (uint8_t) (p[i * desc->bpp + channel] * a) ^ desc->invert;
    }

    return 0;
}

static int slice_luma_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((const slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * 2;

    for (int i = 0; i < size; ++i) {
        double a = ((double) (q[i * 2] ^ desc->invert_mask) - desc->offset) / desc->divisor;
        a = smoothstep(a, a + desc->softness, desc->mix);
        p[i] = (uint8_t) (p[i] * a) ^ desc->invert;
    }

    return 0;
}

static int slice_alpha16_add(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = desc->mask + slice_line_start * desc->width * 4;
    const int invert_mask16 = desc->invert_mask ? 65535 : 0;
    const int invert16 = desc->invert ? 65535 : 0;

    for (int i = 0; i < size; ++i) {
        uint32_t a = (uint32_t) (q[i * 4 + 3] ^ invert_mask16);
        p[i * 4 + 3] = ((uint16_t) MIN((uint32_t) p[i * 4 + 3] + a, 65535)) ^ invert16;
    }

    return 0;
}

static int slice_alpha16_maximum(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = desc->mask + slice_line_start * desc->width * 4;
    const int invert_mask16 = desc->invert_mask ? 65535 : 0;
    const int invert16 = desc->invert ? 65535 : 0;

    for (int i = 0; i < size; ++i) {
        p[i * 4 + 3] = MAX(p[i * 4 + 3], (q[i * 4 + 3] ^ invert_mask16)) ^ invert16;
    }

    return 0;
}

static int slice_alpha16_minimum(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = desc->mask + slice_line_start * desc->width * 4;
    const int invert_mask16 = desc->invert_mask ? 65535 : 0;
    const int invert16 = desc->invert ? 65535 : 0;

    for (int i = 0; i < size; ++i) {
        p[i * 4 + 3] = MIN(p[i * 4 + 3], (q[i * 4 + 3] ^ invert_mask16)) ^ invert16;
    }

    return 0;
}

static int slice_alpha16_overwrite(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = desc->mask + slice_line_start * desc->width * 4;
    const int invert_mask16 = desc->invert_mask ? 65535 : 0;
    const int invert16 = desc->invert ? 65535 : 0;

    for (int i = 0; i < size; ++i) {
        p[i * 4 + 3] = (q[i * 4 + 3] ^ invert_mask16) ^ invert16;
    }

    return 0;
}

static int slice_alpha16_subtract(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = desc->mask + slice_line_start * desc->width * 4;
    const int invert_mask16 = desc->invert_mask ? 65535 : 0;
    const int invert16 = desc->invert ? 65535 : 0;

    for (int i = 0; i < size; ++i) {
        uint16_t a = q[i * 4 + 3] ^ invert_mask16;
        p[i * 4 + 3] = (p[i * 4 + 3] > a ? p[i * 4 + 3] - a : 0) ^ invert16;
    }

    return 0;
}

static int slice_alpha16_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = desc->mask + slice_line_start * desc->width * 4;
    const int invert_mask16 = desc->invert_mask ? 65535 : 0;
    const int invert16 = desc->invert ? 65535 : 0;

    for (int i = 0; i < size; ++i) {
        double a = (double) (q[i * 4 + 3] ^ invert_mask16) / desc->divisor;
        a = 1.0 - smoothstep(a, a + desc->softness, desc->mix);
        p[i * 4 + 3] = (uint16_t) (p[i * 4 + 3] * a) ^ invert16;
    }

    return 0;
}

static int slice_luma_rgba_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = ((const slice_desc *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint8_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint8_t *q = desc->mask + slice_line_start * desc->width * 4;

    for (int i = 0; i < size; ++i) {
        // Calculate luma from sRGB
        double luma = 0.2126 * q[i * 4] + 0.7152 * q[i * 4 + 1] + 0.0722 * q[i * 4 + 2];
        double a = (luma - desc->offset) / desc->divisor;
        if (desc->invert_mask)
            a = 1.0 - a;
        a = smoothstep(a, a + desc->softness, desc->mix);
        p[i * 4 + 3] = (uint8_t) (p[i * 4 + 3] * a) ^ desc->invert;
    }

    return 0;
}

static int slice_luma_rgba64_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc16 *desc = ((const slice_desc16 *) data);
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int size = desc->width * slice_height;
    uint16_t *p = desc->alpha + slice_line_start * desc->width * 4;
    const uint16_t *q = (const uint16_t *) desc->mask + slice_line_start * desc->width * 4;

    for (int i = 0; i < size; ++i) {
        // Calculate luma from sRGB using 16-bit values
        double luma = 0.2126 * q[i * 4] + 0.7152 * q[i * 4 + 1] + 0.0722 * q[i * 4 + 2];
        double a = (luma - desc->offset) / desc->divisor;
        if (desc->invert_mask)
            a = 1.0 - a;
        a = smoothstep(a, a + desc->softness, desc->mix);
        p[i * 4 + 3] = (uint16_t) (p[i * 4 + 3] * a) ^ (desc->invert ? 65535 : 0);
    }

    return 0;
}

/** Apply alpha operation based on the operation type */
static void apply_alpha_operation(mlt_filter filter,
                                  slice_desc *desc,
                                  slice_desc16 *desc16,
                                  int is_16bit)
{
    const char *op = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "alpha_operation");

    if (is_16bit) {
        if (op && op[0] != '\0') {
            if (op[0] == 'a')
                mlt_slices_run_normal(0, slice_alpha16_add, desc16);
            else if (op[0] == 's')
                mlt_slices_run_normal(0, slice_alpha16_subtract, desc16);
            else if (!strncmp("ma", op, 2))
                mlt_slices_run_normal(0, slice_alpha16_maximum, desc16);
            else if (!strncmp("mi", op, 2))
                mlt_slices_run_normal(0, slice_alpha16_minimum, desc16);
            else
                mlt_slices_run_normal(0, slice_alpha16_overwrite, desc16);
        } else {
            mlt_slices_run_normal(0, slice_alpha16_overwrite, desc16);
        }
    } else {
        if (op && op[0] != '\0') {
            if (op[0] == 'a')
                mlt_slices_run_normal(0, slice_alpha_add, desc);
            else if (op[0] == 's')
                mlt_slices_run_normal(0, slice_alpha_subtract, desc);
            else if (!strncmp("ma", op, 2))
                mlt_slices_run_normal(0, slice_alpha_maximum, desc);
            else if (!strncmp("mi", op, 2))
                mlt_slices_run_normal(0, slice_alpha_minimum, desc);
            else
                mlt_slices_run_normal(0, slice_alpha_overwrite, desc);
        } else {
            mlt_slices_run_normal(0, slice_alpha_overwrite, desc);
        }
    }
}

/** Handle alpha channel processing for RGBA64 format */
static void process_alpha_rgba64(mlt_filter filter,
                                 uint8_t **image,
                                 uint8_t *mask_img,
                                 int width,
                                 int height,
                                 double softness,
                                 double mix,
                                 int invert,
                                 int invert_mask,
                                 int use_mix)
{
    slice_desc16 desc = {.alpha = (uint16_t *) *image,
                         .mask = (uint16_t *) mask_img,
                         .width = width,
                         .height = height,
                         .softness = softness,
                         .mix = mix,
                         .invert = invert,
                         .invert_mask = invert_mask,
                         .offset = 0.0,
                         .divisor = 65535.0};

    if (use_mix) {
        mlt_slices_run_normal(0, slice_alpha16_proc, &desc);
    } else {
        apply_alpha_operation(filter, NULL, &desc, 1);
    }
}

/** Handle alpha channel processing for RGBA format */
static void process_alpha_rgba(mlt_filter filter,
                               uint8_t **image,
                               uint8_t *mask_img,
                               int width,
                               int height,
                               double softness,
                               double mix,
                               int invert,
                               int invert_mask,
                               int use_mix)
{
    slice_desc desc = {.alpha = *image,
                       .mask = mask_img,
                       .width = width,
                       .height = height,
                       .bpp = 4,
                       .softness = softness,
                       .mix = mix,
                       .invert = invert,
                       .invert_mask = invert_mask,
                       .offset = 0.0,
                       .divisor = 255.0};

    if (use_mix) {
        mlt_slices_run_normal(0, slice_alpha_proc, &desc);
    } else {
        apply_alpha_operation(filter, &desc, NULL, 0);
    }
}

/** Handle YUV422 with separate alpha channel */
static void process_alpha_yuv422(mlt_filter filter,
                                 mlt_frame frame,
                                 mlt_frame mask,
                                 int width,
                                 int height,
                                 double softness,
                                 double mix,
                                 int invert,
                                 int invert_mask,
                                 int use_mix)
{
    uint8_t *p = mlt_frame_get_alpha(frame);
    if (!p) {
        int alphasize = width * height;
        p = mlt_pool_alloc(alphasize);
        memset(p, 255, alphasize);
        mlt_frame_set_alpha(frame, p, alphasize, mlt_pool_release);
    }

    uint8_t *q = mlt_frame_get_alpha(mask);
    if (!q) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter),
                        "failed to get alpha channel from mask: %s\n",
                        mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "resource"));
        int alphasize = width * height;
        q = mlt_pool_alloc(alphasize);
        memset(q, 255, alphasize);
        mlt_frame_set_alpha(mask, q, alphasize, mlt_pool_release);
    }

    slice_desc desc = {.alpha = p,
                       .mask = q,
                       .width = width,
                       .height = height,
                       .bpp = 1,
                       .softness = softness,
                       .mix = mix,
                       .invert = invert,
                       .invert_mask = invert_mask,
                       .offset = 0.0,
                       .divisor = 255.0};

    if (use_mix) {
        mlt_slices_run_normal(0, slice_alpha_proc, &desc);
    } else {
        apply_alpha_operation(filter, &desc, NULL, 0);
    }
}

/** Direct alpha copy without threshold (use_mix = 0, use_luminance = 0) */
static void process_alpha_copy(mlt_frame frame,
                               uint8_t *image,
                               uint8_t *mask_img,
                               mlt_image_format format,
                               int width,
                               int height,
                               int invert_mask)
{
    int size = width * height;

    if (format == mlt_image_rgba64) {
        uint16_t *img16 = (uint16_t *) image;
        uint16_t *q16 = (uint16_t *) mask_img;
        int invert_mask16 = invert_mask ? 65535 : 0;
        for (int i = 0; i < size; i++) {
            double luma = 0.2126 * q16[i * 4] + 0.7152 * q16[i * 4 + 1] + 0.0722 * q16[i * 4 + 2];
            img16[i * 4 + 3] = (uint16_t) luma ^ invert_mask16;
        }
    } else if (format == mlt_image_rgba) {
        uint8_t *q = mask_img;
        for (int i = 0; i < size; i++) {
            double luma = 0.2126 * q[i * 4] + 0.7152 * q[i * 4 + 1] + 0.0722 * q[i * 4 + 2];
            image[i * 4 + 3] = (uint8_t) luma ^ invert_mask;
        }
    } else {
        uint8_t *p = mlt_frame_get_alpha(frame);
        uint8_t *q = mask_img;

        if (!p) {
            int alphasize = width * height;
            p = mlt_pool_alloc(alphasize);
            memset(p, 255, alphasize);
            mlt_frame_set_alpha(frame, p, alphasize, mlt_pool_release);
        }
        for (int i = 0; i < size; i++) {
            p[i] = q[i * 2] ^ invert_mask;
        }
    }
}

/** Derive alpha from luminance of mask image */
static void process_luminance(mlt_frame frame,
                              uint8_t *image,
                              uint8_t *mask_img,
                              mlt_image_format format,
                              int width,
                              int height,
                              double softness,
                              double mix,
                              int invert,
                              int invert_mask)
{
    if (format == mlt_image_rgba) {
        slice_desc desc = {.alpha = image,
                           .mask = mask_img,
                           .width = width,
                           .height = height,
                           .bpp = 4,
                           .softness = softness * (1.0 - mix),
                           .mix = mix,
                           .invert = invert,
                           .invert_mask = invert_mask,
                           .offset = 0.0,
                           .divisor = 255.0};
        mlt_slices_run_normal(0, slice_luma_rgba_proc, &desc);
    } else if (format == mlt_image_rgba64) {
        slice_desc16 desc = {.alpha = (uint16_t *) image,
                             .mask = (uint16_t *) mask_img,
                             .width = width,
                             .height = height,
                             .softness = softness * (1.0 - mix),
                             .mix = mix,
                             .invert = invert,
                             .invert_mask = invert_mask,
                             .offset = 0.0,
                             .divisor = 65535.0};
        mlt_slices_run_normal(0, slice_luma_rgba64_proc, &desc);
    } else {
        int full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
        uint8_t *p = mlt_frame_get_alpha(frame);
        if (!p) {
            int alphasize = width * height;
            p = mlt_pool_alloc(alphasize);
            memset(p, 255, alphasize);
            mlt_frame_set_alpha(frame, p, alphasize, mlt_pool_release);
        }
        slice_desc desc = {.alpha = p,
                           .mask = mask_img,
                           .width = width,
                           .height = height,
                           .bpp = 1,
                           .softness = softness * (1.0 - mix),
                           .mix = mix,
                           .invert = invert,
                           .invert_mask = invert_mask,
                           .offset = full_range ? 0.0 : 16.0,
                           .divisor = full_range ? 255.0 : 235.0};
        mlt_slices_run_normal(0, slice_luma_proc, &desc);
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
    // Fetch the data from the stack (mix, mask, filter)
    double mix = mlt_deque_pop_back_double(MLT_FRAME_IMAGE_STACK(frame));
    mlt_frame mask = mlt_frame_pop_service(frame);
    mlt_filter filter = mlt_frame_pop_service(frame);

    // Obtain the constants
    double softness = mlt_properties_get_double(MLT_FILTER_PROPERTIES(filter), "softness");
    int use_luminance = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "use_luminance");
    int use_mix = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "use_mix");
    int invert = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "invert") * 255;
    int invert_mask = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "invert_mask") * 255;

    if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "reverse")) {
        mix = 1.0 - mix;
        invert = !mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "invert") * 255;
    }

    // Render the frame
    if (*format != mlt_image_rgba && *format != mlt_image_rgba64)
        *format = mlt_image_yuv422;
    if (*format == mlt_image_yuv422)
        *width -= *width % 2;
    if (mlt_frame_get_image(frame, image, format, width, height, 1) == 0
        && (!use_luminance || !use_mix || (int) mix != 1 || invert == 255 || invert_mask == 255)) {
        // Obtain a scaled/distorted mask to match
        uint8_t *mask_img = NULL;
        mlt_image_format mask_fmt = *format;
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(mask), "distort", 1);
        mlt_properties_copy(MLT_FRAME_PROPERTIES(mask), MLT_FRAME_PROPERTIES(frame), "consumer.");

        if (mlt_frame_get_image(mask, &mask_img, &mask_fmt, width, height, 0) == 0) {
            mlt_log_debug(MLT_FILTER_SERVICE(filter),
                          "applying mask: %s ",
                          mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "resource"));

            // Route to appropriate processing function based on format and mode
            if (!use_luminance && mask_fmt == mlt_image_rgba64) {
                process_alpha_rgba64(filter,
                                     image,
                                     mask_img,
                                     *width,
                                     *height,
                                     softness,
                                     mix,
                                     invert,
                                     invert_mask,
                                     use_mix);
            } else if (!use_luminance && mask_fmt == mlt_image_rgba) {
                process_alpha_rgba(filter,
                                   image,
                                   mask_img,
                                   *width,
                                   *height,
                                   softness,
                                   mix,
                                   invert,
                                   invert_mask,
                                   use_mix);
            } else if (!use_luminance) {
                process_alpha_yuv422(filter,
                                     frame,
                                     mask,
                                     *width,
                                     *height,
                                     softness,
                                     mix,
                                     invert,
                                     invert_mask,
                                     use_mix);
            } else if (!use_mix) {
                process_alpha_copy(frame, *image, mask_img, *format, *width, *height, invert_mask);
            } else if ((int) mix != 1 || invert == 255 || invert_mask == 255) {
                mlt_log_debug(MLT_FILTER_SERVICE(filter),
                              "applying luminance to alpha channel format: %s\n",
                              mlt_image_format_name(mask_fmt));
                process_luminance(frame,
                                  *image,
                                  mask_img,
                                  *format,
                                  *width,
                                  *height,
                                  softness,
                                  mix,
                                  invert,
                                  invert_mask);
            }
        } else {
            mlt_log_warning(MLT_FILTER_SERVICE(filter),
                            "failed to get image from mask: %s\n",
                            mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "resource"));
        }
    } else {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "failed to get image from frame\n");
    }

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Obtain the shape instance
    char *resource = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "resource");
    if (!resource)
        return frame;
    char *last_resource = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "_resource");
    mlt_producer producer = mlt_properties_get_data(MLT_FILTER_PROPERTIES(filter), "instance", NULL);

    // Calculate the position and length
    int position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    // If we haven't created the instance or it's changed
    if (producer == NULL || !last_resource || strcmp(resource, last_resource)) {
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        char temp[PATH_MAX];

        // Store the last resource now
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "_resource", resource);

        // This is a hack - the idea is that we can indirectly reference the
        // luma modules pgm or png images by a short cut like %luma01.pgm - we then replace
        // the % with the full path to the image and use it if it exists, if not, check for
        // the file ending in a .png, and failing that, default to a fade in
        if (strchr(resource, '%')) {
            FILE *test;
            snprintf(temp,
                     sizeof(temp),
                     "%s/lumas/%s/%s",
                     mlt_environment("MLT_DATA"),
                     mlt_profile_lumas_dir(profile),
                     strchr(resource, '%') + 1);
            test = mlt_fopen(temp, "r");

            if (test == NULL) {
                strcat(temp, ".png");
                test = mlt_fopen(temp, "r");
            }

            if (test) {
                fclose(test);
                resource = temp;
            }
        }

        producer = mlt_factory_producer(profile, NULL, resource);
        if (producer != NULL)
            mlt_properties_set(MLT_PRODUCER_PROPERTIES(producer), "eof", "loop");
        mlt_properties_set_data(MLT_FILTER_PROPERTIES(filter),
                                "instance",
                                producer,
                                0,
                                (mlt_destructor) mlt_producer_close,
                                NULL);
    }

    // We may still not have a producer in which case, we do nothing
    if (producer != NULL) {
        mlt_frame mask = NULL;
        double alpha_mix = mlt_properties_anim_get_double(MLT_FILTER_PROPERTIES(filter),
                                                          "mix",
                                                          position,
                                                          length);
        mlt_properties_pass(MLT_PRODUCER_PROPERTIES(producer),
                            MLT_FILTER_PROPERTIES(filter),
                            "producer.");
        mlt_properties_clear(MLT_FILTER_PROPERTIES(filter), "producer.refresh");
        mlt_producer_seek(producer, position);
        if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &mask, 0) == 0) {
            char name[64];
            snprintf(name,
                     sizeof(name),
                     "shape %s",
                     mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "_unique_id"));
            mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                                    name,
                                    mask,
                                    0,
                                    (mlt_destructor) mlt_frame_close,
                                    NULL);
            mlt_frame_push_service(frame, filter);
            mlt_frame_push_service(frame, mask);
            mlt_deque_push_back_double(MLT_FRAME_IMAGE_STACK(frame), alpha_mix / 100.0);
            mlt_frame_push_get_image(frame, filter_get_image);
            if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "audio_match")) {
                mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "meta.mixdown", 1);
                mlt_properties_set_double(MLT_FRAME_PROPERTIES(frame),
                                          "meta.volume",
                                          alpha_mix / 100.0);
            }
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "always_scale", 1);
        }
    }

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_shape_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "resource", arg);
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "mix", "100");
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "use_mix", 1);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "audio_match", 1);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "invert", 0);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "softness", 0.1);
        filter->process = filter_process;
    }
    return filter;
}
