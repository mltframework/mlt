/*
 * filter_lift_gamma_gain.c
 * Copyright (C) 2014-2025 Meltytech, LLC
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
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uint8_t *rlut;
    uint8_t *glut;
    uint8_t *blut;
    uint16_t *rlut16;
    uint16_t *glut16;
    uint16_t *blut16;
    int lut8_valid;
    int lut16_valid;
    double rlift, glift, blift;
    double rgamma, ggamma, bgamma;
    double rgain, ggain, bgain;
} private_data;

typedef struct
{
    mlt_filter filter;
    uint8_t *image;
    mlt_image_format format;
    int width;
    int height;
    const uint8_t *rlut;
    const uint8_t *glut;
    const uint8_t *blut;
    const uint16_t *rlut16;
    const uint16_t *glut16;
    const uint16_t *blut16;
} sliced_desc;

typedef struct
{
    uint16_t *rlut16;
    uint16_t *glut16;
    uint16_t *blut16;
    double rlift, glift, blift;
    double rgamma_exp, ggamma_exp, bgamma_exp;
    double rgain_scale, ggain_scale, bgain_scale;
    int g_same_as_r;
    int b_same_as_r;
    int b_same_as_g;
} lut16_desc;

// Helper function to calculate lift-gamma-gain using precomputed loop invariants
static inline void calc_lift_gamma_gain_fast(double normalized,
                                             double *r,
                                             double *g,
                                             double *b,
                                             double rlift,
                                             double glift,
                                             double blift,
                                             double rgamma_exp,
                                             double ggamma_exp,
                                             double bgamma_exp,
                                             double rgain_scale,
                                             double ggain_scale,
                                             double bgain_scale,
                                             int g_same_as_r,
                                             int b_same_as_r,
                                             int b_same_as_g)
{
    // Convert to gamma 2.2
    const double gamma22 = pow(normalized, 1.0 / 2.2);
    const double one_minus_gamma22 = 1.0 - gamma22;

    double rv = gamma22 + rlift * one_minus_gamma22;
    rv = MAX(rv, 0.0);
    rv = pow(rv, rgamma_exp) * rgain_scale;
    rv = CLAMP(rv, 0.0, 1.0);
    *r = rv;

    double gv;
    if (g_same_as_r) {
        gv = rv;
    } else {
        gv = gamma22 + glift * one_minus_gamma22;
        gv = MAX(gv, 0.0);
        gv = pow(gv, ggamma_exp) * ggain_scale;
        gv = CLAMP(gv, 0.0, 1.0);
    }
    *g = gv;

    double bv;
    if (b_same_as_r) {
        bv = rv;
    } else if (b_same_as_g) {
        bv = gv;
    } else {
        bv = gamma22 + blift * one_minus_gamma22;
        bv = MAX(bv, 0.0);
        bv = pow(bv, bgamma_exp) * bgain_scale;
        bv = CLAMP(bv, 0.0, 1.0);
    }
    *b = bv;
}

static int lut16_slice_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    const lut16_desc *d = (const lut16_desc *) data;
    int start;
    const int count = mlt_slices_size_slice(jobs, index, 65536, &start);
    const int end = start + count;
    uint16_t *restrict rlut16 = d->rlut16;
    uint16_t *restrict glut16 = d->glut16;
    uint16_t *restrict blut16 = d->blut16;
    const double rlift = d->rlift, glift = d->glift, blift = d->blift;
    const double rgamma_exp = d->rgamma_exp, ggamma_exp = d->ggamma_exp, bgamma_exp = d->bgamma_exp;
    const double rgain_scale = d->rgain_scale, ggain_scale = d->ggain_scale,
                 bgain_scale = d->bgain_scale;
    const int g_same_as_r = d->g_same_as_r;
    const int b_same_as_r = d->b_same_as_r;
    const int b_same_as_g = d->b_same_as_g;

    for (int i = start; i < end; i++) {
        double r, g, b;
        calc_lift_gamma_gain_fast((double) i / 65535.0,
                                  &r,
                                  &g,
                                  &b,
                                  rlift,
                                  glift,
                                  blift,
                                  rgamma_exp,
                                  ggamma_exp,
                                  bgamma_exp,
                                  rgain_scale,
                                  ggain_scale,
                                  bgain_scale,
                                  g_same_as_r,
                                  b_same_as_r,
                                  b_same_as_g);
        rlut16[i] = lrint(r * 65535.0);
        glut16[i] = lrint(g * 65535.0);
        blut16[i] = lrint(b * 65535.0);
    }
    return 0;
}

static int refresh_lut(mlt_filter filter, mlt_frame frame, mlt_image_format format)
{
    private_data *self = (private_data *) filter->child;
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    double rlift = mlt_properties_anim_get_double(properties, "lift_r", position, length);
    double glift = mlt_properties_anim_get_double(properties, "lift_g", position, length);
    double blift = mlt_properties_anim_get_double(properties, "lift_b", position, length);
    double rgamma = mlt_properties_anim_get_double(properties, "gamma_r", position, length);
    double ggamma = mlt_properties_anim_get_double(properties, "gamma_g", position, length);
    double bgamma = mlt_properties_anim_get_double(properties, "gamma_b", position, length);
    double rgain = mlt_properties_anim_get_double(properties, "gain_r", position, length);
    double ggain = mlt_properties_anim_get_double(properties, "gain_g", position, length);
    double bgain = mlt_properties_anim_get_double(properties, "gain_b", position, length);

    int params_changed = (self->rlift != rlift || self->glift != glift || self->blift != blift
                          || self->rgamma != rgamma || self->ggamma != ggamma
                          || self->bgamma != bgamma || self->rgain != rgain || self->ggain != ggain
                          || self->bgain != bgain);

    if (params_changed) {
        self->lut8_valid = 0;
        self->lut16_valid = 0;
    }

    // Determine which LUT is needed for this format
    int need_lut8 = (format == mlt_image_rgb || format == mlt_image_rgba);
    int need_lut16 = (format == mlt_image_rgba64);

    if ((need_lut8 && !self->lut8_valid) || (need_lut16 && !self->lut16_valid)) {
        const double rgamma_exp = 2.2 / rgamma;
        const double ggamma_exp = 2.2 / ggamma;
        const double bgamma_exp = 2.2 / bgamma;
        const double rgain_scale = pow(rgain, 1.0 / rgamma);
        const double ggain_scale = pow(ggain, 1.0 / ggamma);
        const double bgain_scale = pow(bgain, 1.0 / bgamma);
        const int g_same_as_r = (glift == rlift && ggamma == rgamma && ggain == rgain);
        const int b_same_as_r = (blift == rlift && bgamma == rgamma && bgain == rgain);
        const int b_same_as_g = (blift == glift && bgamma == ggamma && bgain == ggain);

        // Allocate and generate 8-bit LUT if needed
        if (need_lut8 && !self->lut8_valid) {
            if (!self->rlut) {
                self->rlut = malloc(256 * sizeof(uint8_t));
                self->glut = malloc(256 * sizeof(uint8_t));
                self->blut = malloc(256 * sizeof(uint8_t));

                if (!self->rlut || !self->glut || !self->blut) {
                    mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate 8-bit LUTs\n");
                    free(self->rlut);
                    free(self->glut);
                    free(self->blut);
                    self->rlut = NULL;
                    self->glut = NULL;
                    self->blut = NULL;
                    return 1;
                }
            }

            uint8_t *restrict rlut = self->rlut;
            uint8_t *restrict glut = self->glut;
            uint8_t *restrict blut = self->blut;
            for (int i = 0; i < 256; i++) {
                double r, g, b;
                calc_lift_gamma_gain_fast((double) i / 255.0,
                                          &r,
                                          &g,
                                          &b,
                                          rlift,
                                          glift,
                                          blift,
                                          rgamma_exp,
                                          ggamma_exp,
                                          bgamma_exp,
                                          rgain_scale,
                                          ggain_scale,
                                          bgain_scale,
                                          g_same_as_r,
                                          b_same_as_r,
                                          b_same_as_g);

                // Update 8-bit LUT
                rlut[i] = lrint(r * 255.0);
                glut[i] = lrint(g * 255.0);
                blut[i] = lrint(b * 255.0);
            }
            self->lut8_valid = 1;
        }

        // Allocate and generate 16-bit LUT if needed
        if (need_lut16 && !self->lut16_valid) {
            if (!self->rlut16) {
                self->rlut16 = malloc(65536 * sizeof(uint16_t));
                self->glut16 = malloc(65536 * sizeof(uint16_t));
                self->blut16 = malloc(65536 * sizeof(uint16_t));

                if (!self->rlut16 || !self->glut16 || !self->blut16) {
                    mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate 16-bit LUTs\n");
                    free(self->rlut16);
                    free(self->glut16);
                    free(self->blut16);
                    self->rlut16 = NULL;
                    self->glut16 = NULL;
                    self->blut16 = NULL;
                    return 1;
                }
            }

            lut16_desc ldesc = {.rlut16 = self->rlut16,
                                .glut16 = self->glut16,
                                .blut16 = self->blut16,
                                .rlift = rlift,
                                .glift = glift,
                                .blift = blift,
                                .rgamma_exp = rgamma_exp,
                                .ggamma_exp = ggamma_exp,
                                .bgamma_exp = bgamma_exp,
                                .rgain_scale = rgain_scale,
                                .ggain_scale = ggain_scale,
                                .bgain_scale = bgain_scale,
                                .g_same_as_r = g_same_as_r,
                                .b_same_as_r = b_same_as_r,
                                .b_same_as_g = b_same_as_g};
            mlt_slices_run_normal(0, lut16_slice_proc, &ldesc);
            self->lut16_valid = 1;
        }
    }

    // Store the values that created the LUT so that
    // changes can be detected.
    if (params_changed) {
        self->rlift = rlift;
        self->glift = glift;
        self->blift = blift;
        self->rgamma = rgamma;
        self->ggamma = ggamma;
        self->bgamma = bgamma;
        self->rgain = rgain;
        self->ggain = ggain;
        self->bgain = bgain;
    }
    return 0;
}

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const sliced_desc *desc = (const sliced_desc *) data;
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int pixels = desc->width * slice_height;

    switch (desc->format) {
    case mlt_image_rgb: {
        uint8_t *restrict sample = desc->image + (size_t) slice_line_start * desc->width * 3;
        const uint8_t *restrict rlut = desc->rlut;
        const uint8_t *restrict glut = desc->glut;
        const uint8_t *restrict blut = desc->blut;
        int p = 0;
        for (; p + 4 <= pixels; p += 4, sample += 12) {
            sample[0] = rlut[sample[0]];
            sample[1] = glut[sample[1]];
            sample[2] = blut[sample[2]];
            sample[3] = rlut[sample[3]];
            sample[4] = glut[sample[4]];
            sample[5] = blut[sample[5]];
            sample[6] = rlut[sample[6]];
            sample[7] = glut[sample[7]];
            sample[8] = blut[sample[8]];
            sample[9] = rlut[sample[9]];
            sample[10] = glut[sample[10]];
            sample[11] = blut[sample[11]];
        }
        for (; p < pixels; p++, sample += 3) {
            sample[0] = rlut[sample[0]];
            sample[1] = glut[sample[1]];
            sample[2] = blut[sample[2]];
        }
        break;
    }
    case mlt_image_rgba: {
        uint8_t *restrict sample = desc->image + (size_t) slice_line_start * desc->width * 4;
        const uint8_t *restrict rlut = desc->rlut;
        const uint8_t *restrict glut = desc->glut;
        const uint8_t *restrict blut = desc->blut;
        int p = 0;
        for (; p + 4 <= pixels; p += 4, sample += 16) {
            sample[0] = rlut[sample[0]];
            sample[1] = glut[sample[1]];
            sample[2] = blut[sample[2]];
            sample[4] = rlut[sample[4]];
            sample[5] = glut[sample[5]];
            sample[6] = blut[sample[6]];
            sample[8] = rlut[sample[8]];
            sample[9] = glut[sample[9]];
            sample[10] = blut[sample[10]];
            sample[12] = rlut[sample[12]];
            sample[13] = glut[sample[13]];
            sample[14] = blut[sample[14]];
        }
        for (; p < pixels; p++, sample += 4) {
            sample[0] = rlut[sample[0]];
            sample[1] = glut[sample[1]];
            sample[2] = blut[sample[2]];
        }
        break;
    }
    case mlt_image_rgba64: {
        uint16_t *restrict s = (uint16_t *) (desc->image
                                             + (size_t) slice_line_start * desc->width * 8);
        const uint16_t *restrict rlut16 = desc->rlut16;
        const uint16_t *restrict glut16 = desc->glut16;
        const uint16_t *restrict blut16 = desc->blut16;
        int p = 0;
        for (; p + 4 <= pixels; p += 4, s += 16) {
            s[0] = rlut16[s[0]];
            s[1] = glut16[s[1]];
            s[2] = blut16[s[2]];
            s[4] = rlut16[s[4]];
            s[5] = glut16[s[5]];
            s[6] = blut16[s[6]];
            s[8] = rlut16[s[8]];
            s[9] = glut16[s[9]];
            s[10] = blut16[s[10]];
            s[12] = rlut16[s[12]];
            s[13] = glut16[s[13]];
            s[14] = blut16[s[14]];
        }
        for (; p < pixels; p++, s += 4) {
            s[0] = rlut16[s[0]];
            s[1] = glut16[s[1]];
            s[2] = blut16[s[2]];
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
    int error = 0;

    // Change the format if necessary
    switch (*format) {
    case mlt_image_yuv420p:
    case mlt_image_yuv422:
        *format = mlt_image_rgb;
        break;
    case mlt_image_yuv422p16:
    case mlt_image_yuv420p10:
    case mlt_image_yuv444p10:
        *format = mlt_image_rgba64;
        break;
    default:
        break;
    }

    // Get the image
    writable = 1;
    error = mlt_frame_get_image(frame, image, format, width, height, writable);

    if (!error && *image) {
        // Regenerate the LUT if necessary and copy pointers atomically
        mlt_service_lock(MLT_FILTER_SERVICE(filter));
        error = refresh_lut(filter, frame, *format);
        if (!error) {
            private_data *self = (private_data *) filter->child;
            sliced_desc desc;
            desc.filter = filter;
            desc.image = *image;
            desc.format = *format;
            desc.width = *width;
            desc.height = *height;
            desc.rlut = self->rlut;
            desc.glut = self->glut;
            desc.blut = self->blut;
            desc.rlut16 = self->rlut16;
            desc.glut16 = self->glut16;
            desc.blut16 = self->blut16;

            // Apply the LUT with copied pointers
            mlt_slices_run_normal(0, sliced_proc, &desc);
        }
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    private_data *self = (private_data *) filter->child;

    if (self) {
        free(self->rlut);
        free(self->glut);
        free(self->blut);
        free(self->rlut16);
        free(self->glut16);
        free(self->blut16);
        free(self);
    }
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

mlt_filter filter_lift_gamma_gain_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *self = (private_data *) calloc(1, sizeof(private_data));

    if (filter && self) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

        self->rlift = self->glift = self->blift = 0.0;
        self->rgamma = self->ggamma = self->bgamma = 1.0;
        self->rgain = self->ggain = self->bgain = 1.0;

        // Initialize filter properties
        mlt_properties_set_double(properties, "lift_r", self->rlift);
        mlt_properties_set_double(properties, "lift_g", self->glift);
        mlt_properties_set_double(properties, "lift_b", self->blift);
        mlt_properties_set_double(properties, "gamma_r", self->rgamma);
        mlt_properties_set_double(properties, "gamma_g", self->ggamma);
        mlt_properties_set_double(properties, "gamma_b", self->bgamma);
        mlt_properties_set_double(properties, "gain_r", self->rgain);
        mlt_properties_set_double(properties, "gain_g", self->ggain);
        mlt_properties_set_double(properties, "gain_b", self->bgain);

        filter->close = filter_close;
        filter->process = filter_process;
        filter->child = self;
    } else {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Filter lift_gamma_gain init failed\n");
        mlt_filter_close(filter);
        filter = NULL;
        free(self);
    }

    return filter;
}
