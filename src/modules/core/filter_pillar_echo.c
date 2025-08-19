/*
 * filter_pillar_echo.c -- filter to interpolate pixels outside an area of interest
 * Copyright (c) 2020-2025 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "image_proc.h"

#include <framework/mlt.h>
#include <framework/mlt_log.h>

#include <math.h>
#include <string.h>

/** Constrain a rect to be within the max dimensions
*/
static mlt_rect constrain_rect(mlt_rect rect, int max_x, int max_y)
{
    if (rect.x < 0) {
        rect.w = rect.w + rect.x;
        rect.x = 0;
    }
    if (rect.y < 0) {
        rect.h = rect.h + rect.y;
        rect.y = 0;
    }
    if (rect.x + rect.w < 0) {
        rect.w = 0;
    }
    if (rect.y + rect.h < 0) {
        rect.h = 0;
    }
    if (rect.x + rect.w > max_x) {
        rect.w = max_x - rect.x;
    }
    if (rect.y + rect.h > max_y) {
        rect.h = max_y - rect.y;
    }
    return rect;
}

typedef struct
{
    mlt_image src;
    mlt_image dst;
    mlt_rect rect;
} scale_sliced_desc;

static int scale_sliced_proc_rgba32(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    scale_sliced_desc *desc = ((scale_sliced_desc *) data);
    mlt_image src = desc->src;
    mlt_image dst = desc->dst;
    mlt_rect rect = desc->rect;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    double srcScale = rect.h / (double) src->height;
    int linesize = src->width * 4;
    uint8_t *d = (uint8_t *) ((char *) dst->data + (slice_line_start * linesize));
    for (int y = slice_line_start; y < slice_line_end; y++) {
        double srcY = rect.y + (double) y * srcScale;
        int srcYindex = floor(srcY);
        double fbottom = srcY - srcYindex;
        double ftop = 1.0 - fbottom;

        int x = 0;
        for (x = 0; x < src->width; x++) {
            double srcX = rect.x + (double) x * srcScale;
            int srcXindex = floor(srcX);
            double fright = srcX - srcXindex;
            double fleft = 1.0 - fright;

            double valueSum[] = {0.0, 0.0, 0.0, 0.0};
            double factorSum[] = {0.0, 0.0, 0.0, 0.0};

            uint8_t *s = (uint8_t *) ((char *) src->data + (srcYindex * linesize) + (srcXindex * 4));

            // Top Left
            double ftl = ftop * fleft;
            valueSum[0] += s[0] * ftl;
            factorSum[0] += ftl;
            valueSum[1] += s[1] * ftl;
            factorSum[1] += ftl;
            valueSum[2] += s[2] * ftl;
            factorSum[2] += ftl;
            valueSum[3] += s[3] * ftl;
            factorSum[3] += ftl;

            // Top Right
            if (x < src->width - 1) {
                double ftr = ftop * fright;
                valueSum[0] += s[4] * ftr;
                factorSum[0] += ftr;
                valueSum[1] += s[5] * ftr;
                factorSum[1] += ftr;
                valueSum[2] += s[6] * ftr;
                factorSum[2] += ftr;
                valueSum[3] += s[7] * ftr;
                factorSum[3] += ftr;
            }

            if (y < src->height - 1) {
                uint8_t *sb = s + linesize;

                // Bottom Left
                double fbl = fbottom * fleft;
                valueSum[0] += sb[0] * fbl;
                factorSum[0] += fbl;
                valueSum[1] += sb[1] * fbl;
                factorSum[1] += fbl;
                valueSum[2] += sb[2] * fbl;
                factorSum[2] += fbl;
                valueSum[3] += sb[3] * fbl;
                factorSum[3] += fbl;

                // Bottom Right
                if (x < src->width - 1) {
                    double fbr = fbottom * fright;
                    valueSum[0] += sb[4] * fbr;
                    factorSum[0] += fbr;
                    valueSum[1] += sb[5] * fbr;
                    factorSum[1] += fbr;
                    valueSum[2] += sb[6] * fbr;
                    factorSum[2] += fbr;
                    valueSum[3] += sb[7] * fbr;
                    factorSum[3] += fbr;
                }
            }

            d[0] = (uint8_t) round(valueSum[0] / factorSum[0]);
            d[1] = (uint8_t) round(valueSum[1] / factorSum[1]);
            d[2] = (uint8_t) round(valueSum[2] / factorSum[2]);
            d[3] = (uint8_t) round(valueSum[3] / factorSum[3]);
            d += 4;
        }
    }
    return 0;
}

static int scale_sliced_proc_rgba64(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    scale_sliced_desc *desc = ((scale_sliced_desc *) data);
    mlt_image src = desc->src;
    mlt_image dst = desc->dst;
    mlt_rect rect = desc->rect;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, src->height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    double srcScale = rect.h / (double) src->height;
    int linesize = src->width * 4 * 2;
    uint16_t *d = (uint16_t *)((uint8_t *)dst->data + (slice_line_start * linesize));
    for (int y = slice_line_start; y < slice_line_end; y++) {
        double srcY = rect.y + (double) y * srcScale;
        int srcYindex = floor(srcY);
        double fbottom = srcY - srcYindex;
        double ftop = 1.0 - fbottom;

        int x = 0;
        for (x = 0; x < src->width; x++) {
            double srcX = rect.x + (double) x * srcScale;
            int srcXindex = floor(srcX);
            double fright = srcX - srcXindex;
            double fleft = 1.0 - fright;

            double valueSum[] = {0.0, 0.0, 0.0, 0.0};
            double factorSum[] = {0.0, 0.0, 0.0, 0.0};

            uint16_t *s = (uint16_t *)((uint8_t *)src->data + (srcYindex * linesize) + (srcXindex * 4));

            // Top Left
            double ftl = ftop * fleft;
            valueSum[0] += s[0] * ftl;
            factorSum[0] += ftl;
            valueSum[1] += s[1] * ftl;
            factorSum[1] += ftl;
            valueSum[2] += s[2] * ftl;
            factorSum[2] += ftl;
            valueSum[3] += s[3] * ftl;
            factorSum[3] += ftl;

            // Top Right
            if (x < src->width - 1) {
                double ftr = ftop * fright;
                valueSum[0] += s[4] * ftr;
                factorSum[0] += ftr;
                valueSum[1] += s[5] * ftr;
                factorSum[1] += ftr;
                valueSum[2] += s[6] * ftr;
                factorSum[2] += ftr;
                valueSum[3] += s[7] * ftr;
                factorSum[3] += ftr;
            }

            if (y < src->height - 1) {
                uint16_t *sb = s + linesize;

                // Bottom Left
                double fbl = fbottom * fleft;
                valueSum[0] += sb[0] * fbl;
                factorSum[0] += fbl;
                valueSum[1] += sb[1] * fbl;
                factorSum[1] += fbl;
                valueSum[2] += sb[2] * fbl;
                factorSum[2] += fbl;
                valueSum[3] += sb[3] * fbl;
                factorSum[3] += fbl;

                // Bottom Right
                if (x < src->width - 1) {
                    double fbr = fbottom * fright;
                    valueSum[0] += sb[4] * fbr;
                    factorSum[0] += fbr;
                    valueSum[1] += sb[5] * fbr;
                    factorSum[1] += fbr;
                    valueSum[2] += sb[6] * fbr;
                    factorSum[2] += fbr;
                    valueSum[3] += sb[7] * fbr;
                    factorSum[3] += fbr;
                }
            }

            d[0] = (uint16_t) round(valueSum[0] / factorSum[0]);
            d[1] = (uint16_t) round(valueSum[1] / factorSum[1]);
            d[2] = (uint16_t) round(valueSum[2] / factorSum[2]);
            d[3] = (uint16_t) round(valueSum[3] / factorSum[3]);
            d += 4;
        }
    }
    return 0;
}

/** Perform a bilinear scale from the rect inside the source to fill the destination
  *
  * \param src a pointer to the source image
  * \param dst a pointer to the destination image
  * \param rect the area of interest in the src to be scaled to fit the dst
  */

static void bilinear_scale_rgba(mlt_image src, mlt_image dst, mlt_rect rect)
{
    scale_sliced_desc desc;
    desc.src = src;
    desc.dst = dst;
    desc.rect = rect;

    // Crop out a section of the rect that has the same aspect ratio as the image
    double destAr = (double) src->width / (double) src->height;
    double sourceAr = rect.w / rect.h;
    if (sourceAr > destAr) {
        // Crop sides to fit height
        desc.rect.w = rect.w * destAr / sourceAr;
        desc.rect.x = rect.x + (rect.w - desc.rect.w) / 2.0;
    } else if (destAr > sourceAr) {
        // Crop top and bottom to fit width.
        desc.rect.h = rect.h * sourceAr / destAr;
        desc.rect.y = rect.y + (rect.h - desc.rect.h) / 2.0;
    }
    if (src->format == mlt_image_rgb) {
        mlt_slices_run_normal(0, scale_sliced_proc_rgba32, &desc);
    } else {
        mlt_slices_run_normal(0, scale_sliced_proc_rgba64, &desc);
    }
}

/** Copy pixels from source to destination
  *
  * \param src a pointer to the source image
  * \param dst a pointer to the destination image
  * \param width the number of values in each row
  * \param rect the area of interest in the src to be copied to the dst
  */

static void blit_rect(mlt_image src, mlt_image dst, mlt_rect rect)
{
    int blitHeight = rect.h;
    int blitWidth = rect.w * 4;
    int linesize = src->width * 4;
    if (src->format == mlt_image_rgba) {
        uint8_t *s = (uint8_t *)((uint8_t *)src->data + (int) rect.y * linesize + (int) rect.x * 4);
        uint8_t *d = (uint8_t *)((uint8_t *)dst->data + (int) rect.y * linesize + (int) rect.x * 4);
        while (blitHeight--) {
            memcpy(d, s, blitWidth);
            s += linesize;
            d += linesize;
        }
    } else {
        uint16_t *s = (uint16_t *)((uint8_t *)src->data + (int) rect.y * linesize + (int) rect.x * 4);
        uint16_t *d = (uint16_t *)((uint8_t *)dst->data + (int) rect.y * linesize + (int) rect.x * 4);
        while (blitHeight--) {
            memcpy(d, s, blitWidth * 2);
            s += linesize;
            d += linesize;
        }
    }
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    char *rect_str = mlt_properties_get(filter_properties, "rect");
    if (!rect_str) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "rect property not set\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    mlt_rect rect = mlt_properties_anim_get_rect(filter_properties, "rect", position, length);
    if (strchr(rect_str, '%')) {
        rect.x *= profile->width;
        rect.w *= profile->width;
        rect.y *= profile->height;
        rect.h *= profile->height;
    }
    double scale = mlt_profile_scale_width(profile, *width);
    rect.x *= scale;
    rect.w *= scale;
    scale = mlt_profile_scale_height(profile, *height);
    rect.y *= scale;
    rect.h *= scale;
    rect = constrain_rect(rect, profile->width * scale, profile->height * scale);

    if (rect.w < 1 || rect.h < 1) {
        mlt_log_info(MLT_FILTER_SERVICE(filter), "rect invalid\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    if (*format != mlt_image_rgba64) {
        *format = mlt_image_rgba;
    }
    error = mlt_frame_get_image(frame, image, format, width, height, 0);

    if (error)
        return error;
    if (rect.x <= 0 && rect.y <= 0 && rect.w >= *width && rect.h >= *height) {
        // Rect fills the image. Nothing to blur
        return error;
    }

    double blur = mlt_properties_anim_get_double(filter_properties, "blur", position, length);
    // Convert from percent to pixels.
    blur = blur * (double) profile->width * mlt_profile_scale_width(profile, *width) / 100.0;
    blur = MAX(round(blur), 0);

    struct mlt_image_s src;
    mlt_image_set_values(&src, *image, *format, *width, *height);

    struct mlt_image_s dst;
    mlt_image_set_values(&dst, NULL, *format, *width, *height);
    mlt_image_alloc_data(&dst);

    bilinear_scale_rgba(&src, &dst, rect);
    if (blur != 0) {
        mlt_image_box_blur(&dst, blur, blur, 0);
    }
    blit_rect(&src, &dst, rect);

    *image = dst.data;
    mlt_frame_set_image(frame, dst.data, 0, dst.release_data);

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_pillar_echo_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set(properties, "rect", "0% 0% 10% 10%");
        mlt_properties_set_double(properties, "blur", 4.0);
        filter->process = filter_process;
    } else {
        mlt_log_error(NULL, "Filter pillar_echo initialization failed\n");
    }
    return filter;
}
