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
    uint8_t *rlut;
    uint8_t *glut;
    uint8_t *blut;
    uint16_t *rlut16;
    uint16_t *glut16;
    uint16_t *blut16;
} sliced_desc;

// Helper function to calculate lift-gamma-gain to a normalized [0.0, 1.0] value
static inline void calc_lift_gamma_gain(double normalized,
                                        double *r,
                                        double *g,
                                        double *b,
                                        double rlift,
                                        double glift,
                                        double blift,
                                        double rgamma,
                                        double ggamma,
                                        double bgamma,
                                        double rgain,
                                        double ggain,
                                        double bgain)
{
    // Convert to gamma 2.2
    double gamma22 = pow(normalized, 1.0 / 2.2);
    *r = gamma22;
    *g = gamma22;
    *b = gamma22;

    // Apply lift
    *r += rlift * (1.0 - *r);
    *g += glift * (1.0 - *g);
    *b += blift * (1.0 - *b);

    // Clamp negative values
    *r = MAX(*r, 0.0);
    *g = MAX(*g, 0.0);
    *b = MAX(*b, 0.0);

    // Apply gamma
    *r = pow(*r, 2.2 / rgamma);
    *g = pow(*g, 2.2 / ggamma);
    *b = pow(*b, 2.2 / bgamma);

    // Apply gain
    *r *= pow(rgain, 1.0 / rgamma);
    *g *= pow(ggain, 1.0 / ggamma);
    *b *= pow(bgain, 1.0 / bgamma);

    // Clamp values
    *r = CLAMP(*r, 0.0, 1.0);
    *g = CLAMP(*g, 0.0, 1.0);
    *b = CLAMP(*b, 0.0, 1.0);
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
        // Free existing LUTs to force regeneration
        free(self->rlut);
        free(self->glut);
        free(self->blut);
        free(self->rlut16);
        free(self->glut16);
        free(self->blut16);
        self->rlut = NULL;
        self->glut = NULL;
        self->blut = NULL;
        self->rlut16 = NULL;
        self->glut16 = NULL;
        self->blut16 = NULL;
    }

    // Determine which LUT is needed for this format
    int need_lut8 = (format == mlt_image_rgb || format == mlt_image_rgba);
    int need_lut16 = (format == mlt_image_rgba64);

    // Allocate and generate 8-bit LUT if needed
    if (need_lut8 && !self->rlut) {
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

        for (int i = 0; i < 256; i++) {
            double r, g, b;
            calc_lift_gamma_gain((double) i / 255.0,
                                 &r,
                                 &g,
                                 &b,
                                 rlift,
                                 glift,
                                 blift,
                                 rgamma,
                                 ggamma,
                                 bgamma,
                                 rgain,
                                 ggain,
                                 bgain);

            // Update 8-bit LUT
            self->rlut[i] = lrint(r * 255.0);
            self->glut[i] = lrint(g * 255.0);
            self->blut[i] = lrint(b * 255.0);
        }
    }

    // Allocate and generate 16-bit LUT if needed
    if (need_lut16 && !self->rlut16) {
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

        for (int i = 0; i < 65536; i++) {
            double r, g, b;
            calc_lift_gamma_gain((double) i / 65535.0,
                                 &r,
                                 &g,
                                 &b,
                                 rlift,
                                 glift,
                                 blift,
                                 rgamma,
                                 ggamma,
                                 bgamma,
                                 rgain,
                                 ggain,
                                 bgain);

            // Update 16-bit LUT
            self->rlut16[i] = lrint(r * 65535.0);
            self->glut16[i] = lrint(g * 65535.0);
            self->blut16[i] = lrint(b * 65535.0);
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
            *sample = desc->rlut[*sample];
            sample++;
            *sample = desc->glut[*sample];
            sample++;
            *sample = desc->blut[*sample];
            sample++;
        }
        break;
    case mlt_image_rgba:
        while (--total) {
            *sample = desc->rlut[*sample];
            sample++;
            *sample = desc->glut[*sample];
            sample++;
            *sample = desc->blut[*sample];
            sample++;
            sample++; // Skip alpha
        }
        break;
    case mlt_image_rgba64: {
        int pixels = desc->width * slice_height;
        uint16_t *s = (uint16_t *) sample;
        for (int p = 0; p < pixels; p++) {
            *s = desc->rlut16[*s];
            s++;
            *s = desc->glut16[*s];
            s++;
            *s = desc->blut16[*s];
            s++;
            s++; // Skip alpha
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
