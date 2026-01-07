/*
 * filter_brightness.c -- brightness, fade, and opacity filter
 * Copyright (C) 2003-2026 Meltytech, LLC
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
#include <framework/mlt_image.h>
#include <framework/mlt_slices.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct sliced_desc
{
    mlt_image image;
    double level;
    double alpha_level;
    int full_range;
};

static int sliced_proc(int id, int index, int jobs, void *cookie)
{
    (void) id; // unused
    struct sliced_desc *ctx = ((struct sliced_desc *) cookie);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, ctx->image->height, &slice_line_start);

    // Only process if level is something other than 1
    if (ctx->level != 1.0) {
        if (ctx->image->format == mlt_image_yuv422) {
            int32_t m = ctx->level * (1 << 16);
            int32_t n = 128 * ((1 << 16) - m);
            int min = ctx->full_range ? 0 : 16;
            int max_luma = ctx->full_range ? 255 : 235;
            int max_chroma = ctx->full_range ? 255 : 240;
            for (int line = 0; line < slice_height; line++) {
                uint8_t *p = ctx->image->planes[0]
                             + ((slice_line_start + line) * ctx->image->strides[0]);
                for (int pixel = 0; pixel < ctx->image->width; pixel++) {
                    p[0] = CLAMP((p[0] * m) >> 16, min, max_luma);
                    p[1] = CLAMP((p[1] * m + n) >> 16, min, max_chroma);
                    p += 2;
                }
            }
        } else if (ctx->image->format == mlt_image_rgba) {
            for (int line = 0; line < slice_height; line++) {
                uint8_t *p = ctx->image->planes[0]
                             + ((slice_line_start + line) * ctx->image->strides[0]);
                for (int pixel = 0; pixel < ctx->image->width; pixel++) {
                    p[0] = CLAMP(round((double) p[0] * ctx->level), 0, 255);
                    p[1] = CLAMP(round((double) p[1] * ctx->level), 0, 255);
                    p[2] = CLAMP(round((double) p[2] * ctx->level), 0, 255);
                    p += 4;
                }
            }
        } else if (ctx->image->format == mlt_image_rgb) {
            for (int line = 0; line < slice_height; line++) {
                uint8_t *p = ctx->image->planes[0]
                             + ((slice_line_start + line) * ctx->image->strides[0]);
                for (int pixel = 0; pixel < ctx->image->width; pixel++) {
                    p[0] = CLAMP(round((double) p[0] * ctx->level), 0, 255);
                    p[1] = CLAMP(round((double) p[1] * ctx->level), 0, 255);
                    p[2] = CLAMP(round((double) p[2] * ctx->level), 0, 255);
                    p += 3;
                }
            }
        } else if (ctx->image->format == mlt_image_rgba64) {
            for (int row = 0; row < slice_height; row++) {
                uint16_t *p = (uint16_t *) ctx->image->planes[0]
                              + ((slice_line_start + row) * ctx->image->strides[0] / 2);
                for (int pixel = 0; pixel < ctx->image->width; pixel++) {
                    p[0] = CLAMP(round((double) p[0] * ctx->level), 0, 65535);
                    p[1] = CLAMP(round((double) p[1] * ctx->level), 0, 65535);
                    p[2] = CLAMP(round((double) p[2] * ctx->level), 0, 65535);
                    p += 4;
                }
            }
        }
    }

    // Process the alpha channel if requested.
    if (ctx->alpha_level != 1.0) {
        if (ctx->image->format == mlt_image_rgba) {
            for (int line = 0; line < slice_height; line++) {
                uint8_t *p = ctx->image->planes[0]
                             + ((slice_line_start + line) * ctx->image->strides[0]) + 3;
                int components_in_row = ctx->image->width * 4;
                for (int col = 0; col < components_in_row; col += 4) {
                    p[col] = CLAMP(round((double) p[col] * ctx->alpha_level), 0, 255);
                }
            }
        } else if (ctx->image->format == mlt_image_rgba64) {
            for (int row = 0; row < slice_height; row++) {
                uint16_t *p = (uint16_t *) ctx->image->planes[0]
                              + ((slice_line_start + row) * ctx->image->strides[0] / 2) + 3;
                int components_in_row = ctx->image->width * 4;
                for (int col = 0; col < components_in_row; col += 4) {
                    p[col] = CLAMP(round((double) p[col] * ctx->alpha_level), 0, 65535);
                }
            }
        } else if (ctx->image->planes[3]) {
            for (int line = 0; line < slice_height; line++) {
                uint8_t *p = ctx->image->planes[3]
                             + ((slice_line_start + line) * ctx->image->strides[3]);
                for (int pixel = 0; pixel < ctx->image->width; pixel++) {
                    p[pixel] = CLAMP(round((double) p[pixel] * ctx->alpha_level), 0, 255);
                }
            }
        }
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
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    double level = 1.0;
    double alpha_level = 1.0;

    // Use animated "level" property only if it has been set since init
    char *level_property = mlt_properties_get(properties, "level");
    if (level_property != NULL) {
        level = mlt_properties_anim_get_double(properties, "level", position, length);
    } else {
        // Get level using old "start,"end" mechanics
        // Get the starting brightness level
        level = fabs(mlt_properties_get_double(properties, "start"));

        // If there is an end adjust gain to the range
        if (mlt_properties_get(properties, "end") != NULL) {
            // Determine the time position of this frame in the transition duration
            double end = fabs(mlt_properties_get_double(properties, "end"));
            level += (end - level) * mlt_filter_get_progress(filter, frame);
        }
    }

    // Do not cause an image conversion unless there is real work to do.
    if (level != 1.0) {
        switch (*format) {
        case mlt_image_rgb:
        case mlt_image_rgba:
        case mlt_image_rgba64:
            break;
        case mlt_image_yuv422:
            if (mlt_properties_get_int(properties, "rgb_only")) {
                *format = mlt_image_rgba;
            }
            break;
        case mlt_image_movit:
        case mlt_image_opengl_texture:
            *format = mlt_image_rgba;
            break;
        case mlt_image_none:
        case mlt_image_invalid:
            *format = mlt_image_rgba;
            break;
        case mlt_image_yuv420p:
            if (mlt_properties_get_int(properties, "rgb_only")) {
                *format = mlt_image_rgba;
            } else {
                *format = mlt_image_yuv422;
            }
            break;
        case mlt_image_yuv422p16:
        case mlt_image_yuv420p10:
        case mlt_image_yuv444p10:
            *format = mlt_image_rgba64;
            break;
        }
    }

    // Get the image
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (*format != mlt_image_rgb && *format != mlt_image_rgba && *format != mlt_image_rgba64
        && *format != mlt_image_yuv422)
        level = 1.0;

    alpha_level = mlt_properties_exists(properties, "alpha")
                      ? mlt_properties_anim_get_double(properties, "alpha", position, length)
                      : 1.0;
    if (alpha_level < 0.0) {
        alpha_level = level;
    }

    // Only process if we have no error.
    if (!error && (level != 1.0 || alpha_level != 1.0)) {
        int threads = mlt_properties_get_int(properties, "threads");
        struct sliced_desc desc;
        struct mlt_image_s proc_image;
        mlt_image_set_values(&proc_image, *image, *format, *width, *height);
        if (alpha_level != 1.0 && proc_image.format != mlt_image_rgba
            && proc_image.format != mlt_image_rgba64) {
            proc_image.planes[3] = mlt_frame_get_alpha(frame);
            proc_image.strides[3] = proc_image.width;
            if (!proc_image.planes[3]) {
                // Alpha will be needed but it does not exist yet. Create opaque alpha.
                mlt_image_alloc_alpha(&proc_image);
                mlt_image_fill_opaque(&proc_image);
                mlt_frame_set_alpha(frame,
                                    proc_image.planes[3],
                                    proc_image.width * proc_image.height,
                                    proc_image.release_alpha);
            }
        }
        desc.level = level;
        desc.alpha_level = alpha_level;
        desc.image = &proc_image;
        desc.full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");

        threads = CLAMP(threads, 0, mlt_slices_count_normal());
        if (threads == 1) {
            sliced_proc(0, 0, 1, &desc);
        } else {
            mlt_slices_run_normal(threads, sliced_proc, &desc);
        }
    }

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_brightness_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "start", arg == NULL ? "1" : arg);
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "level", NULL);
    }
    return filter;
}
