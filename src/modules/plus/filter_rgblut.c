/*
 * filter_rgblut.c -- generic RGB look-up table filter with string interface
 * Copyright (C) 2014 Janne Liljeblad
 * Author: Janne Liljeblad
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
#include <framework/mlt_slices.h>
#include <framework/mlt_tokeniser.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uint8_t r_lut[256];
    uint8_t g_lut[256];
    uint8_t b_lut[256];
    char *r_str;
    char *g_str;
    char *b_str;
    int initialized;
} private_data;

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    const uint8_t *r_lut;
    const uint8_t *g_lut;
    const uint8_t *b_lut;
} slice_desc;

/** Fill channel lut with integers parsed from property string.
*/
static void fill_channel_lut(uint8_t lut[256], const char *channel_table_str)
{
    if (channel_table_str) {
        mlt_tokeniser tokeniser = mlt_tokeniser_init();
        mlt_tokeniser_parse_new(tokeniser, (char *) channel_table_str, ";");

        // Only create lut from string if tokens count exactly right
        if (tokeniser->count == 256) {
            for (int i = 0; i < 256; i++) {
                lut[i] = (uint8_t) atoi(tokeniser->tokens[i]);
            }
            mlt_tokeniser_close(tokeniser);
            return;
        }
        mlt_tokeniser_close(tokeniser);
    }

    // Fill lut with linear no-op table
    for (int i = 0; i < 256; i++) {
        lut[i] = (uint8_t) i;
    }
}

static void refresh_channel_lut(uint8_t lut[256], char **cached_str, const char *new_str, int force)
{
    int same = (!force
                && ((*cached_str == NULL && new_str == NULL)
                    || (*cached_str != NULL && new_str != NULL
                        && strcmp(*cached_str, new_str) == 0)));
    if (!same) {
        fill_channel_lut(lut, new_str);
        free(*cached_str);
        *cached_str = new_str ? strdup(new_str) : NULL;
    }
}

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    const slice_desc *desc = (const slice_desc *) data;
    int slice_line_start;
    const int slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
    const int pixels = desc->width * slice_height;
    uint8_t *restrict p = desc->image + (size_t) slice_line_start * desc->width * 3;
    const uint8_t *restrict r_lut = desc->r_lut;
    const uint8_t *restrict g_lut = desc->g_lut;
    const uint8_t *restrict b_lut = desc->b_lut;

    int i = 0;
    for (; i + 4 <= pixels; i += 4, p += 12) {
        p[0] = r_lut[p[0]];
        p[1] = g_lut[p[1]];
        p[2] = b_lut[p[2]];
        p[3] = r_lut[p[3]];
        p[4] = g_lut[p[4]];
        p[5] = b_lut[p[5]];
        p[6] = r_lut[p[6]];
        p[7] = g_lut[p[7]];
        p[8] = b_lut[p[8]];
        p[9] = r_lut[p[9]];
        p[10] = g_lut[p[10]];
        p[11] = b_lut[p[11]];
    }
    for (; i < pixels; i++, p += 3) {
        p[0] = r_lut[p[0]];
        p[1] = g_lut[p[1]];
        p[2] = b_lut[p[2]];
    }

    return 0;
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
    // Get the image
    mlt_filter filter = mlt_frame_pop_service(frame);

    *format = mlt_image_rgb;
    int error = mlt_frame_get_image(frame, image, format, width, height, 0);

    // Only process if we have no error and a valid colour space
    if (error == 0) {
        private_data *self = (private_data *) filter->child;
        mlt_properties props = MLT_FILTER_PROPERTIES(filter);
        uint8_t r_lut[256], g_lut[256], b_lut[256];

        mlt_service_lock(MLT_FILTER_SERVICE(filter));
        const char *r_str = mlt_properties_get(props, "R_table");
        const char *g_str = mlt_properties_get(props, "G_table");
        const char *b_str = mlt_properties_get(props, "B_table");
        refresh_channel_lut(self->r_lut, &self->r_str, r_str, !self->initialized);
        refresh_channel_lut(self->g_lut, &self->g_str, g_str, !self->initialized);
        refresh_channel_lut(self->b_lut, &self->b_str, b_str, !self->initialized);
        self->initialized = 1;
        memcpy(r_lut, self->r_lut, sizeof(r_lut));
        memcpy(g_lut, self->g_lut, sizeof(g_lut));
        memcpy(b_lut, self->b_lut, sizeof(b_lut));
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));

        slice_desc desc = {.image = *image,
                           .width = *width,
                           .height = *height,
                           .r_lut = r_lut,
                           .g_lut = g_lut,
                           .b_lut = b_lut};
        mlt_slices_run_normal(0, slice_proc, &desc);
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

static void filter_close(mlt_filter filter)
{
    private_data *self = (private_data *) filter->child;
    if (self) {
        free(self->r_str);
        free(self->g_str);
        free(self->b_str);
        free(self);
    }
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

/** Constructor for the filter.
*/

mlt_filter filter_rgblut_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *self = (private_data *) calloc(1, sizeof(private_data));
    if (filter != NULL && self != NULL) {
        filter->child = self;
        filter->close = filter_close;
        filter->process = filter_process;
    } else {
        mlt_filter_close(filter);
        free(self);
        filter = NULL;
    }
    return filter;
}
