/*
 * filter_qtblend.cpp -- Qt composite filter
 * Copyright (C) 2015-2025 Meltytech, LLC
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

#include "common.h"
#include <framework/mlt.h>
#include <math.h>   // sin()
#include <stdlib.h> // calloc(), free()
#include <string.h> // strchr()
#include <QImage>
#include <QPainter>
#include <QTransform>

/** Get the image.
*/
static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    int error = 0;
    // Get the filter
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);

    // Get the properties
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    bool hasAlpha = false;

    // Only process if we have no error and a valid colour space
    mlt_service_lock(MLT_FILTER_SERVICE(filter));
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));

    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    // Check transform
    QTransform transform;
    int normalized_width = profile->width;
    int normalized_height = profile->height;
    double consumer_ar = mlt_profile_sar(profile);

    // Destination rect
    mlt_rect rect = {0,
                     0,
                     normalized_width * mlt_profile_scale_width(profile, *width),
                     normalized_height * mlt_profile_scale_height(profile, *height),
                     1.0};
    int b_width = mlt_properties_get_int(frame_properties, "meta.media.width");
    int b_height = mlt_properties_get_int(frame_properties, "meta.media.height");
    bool distort = mlt_properties_get_int(properties, "distort");

    if (b_height == 0) {
        b_width = normalized_width;
        b_height = normalized_height;
    }
    // Special case - aspect_ratio = 0
    if (mlt_frame_get_aspect_ratio(frame) == 0) {
        mlt_frame_set_aspect_ratio(frame, consumer_ar);
    }
    double b_ar = mlt_frame_get_aspect_ratio(frame);
    double b_dar = b_ar * b_width / b_height;
    double opacity = 1.0;

    // If the _qtblend_scaled property is defined, a qtblend filter was already applied
    int qtblendRescaled = mlt_properties_get_int(frame_properties, "qtblend_scaled");
    if (mlt_properties_get(properties, "rect")) {
        rect = mlt_properties_anim_get_rect(properties, "rect", position, length);
        if (::strchr(mlt_properties_get(properties, "rect"), '%')) {
            rect.x *= normalized_width;
            rect.y *= normalized_height;
            rect.w *= normalized_width;
            rect.h *= normalized_height;
        }
        if (qtblendRescaled) {
            // Another qtblend filter was already applied
            // In this case, the *width and *height are set to the source resolution to ensure we don't lose too much details on multiple scaling operations
            // We requested a image with full media resolution, adjust rect to profile
            // Check if we have consumer scaling enabled since we cannot use *width and *height
            double consumerScale = mlt_properties_get_double(frame_properties,
                                                             "qtblend_preview_scaling");
            if (consumerScale > 0.) {
                b_width *= consumerScale;
                b_height *= consumerScale;
            }

            // Always request an image that follows the consumer aspect ratio
            double consumer_dar = normalized_width * consumer_ar / normalized_height;
            int tmpWidth = b_width;
            int tmpHeight = b_height;
            double scaleFactor = qMax(*width / rect.w, *height / rect.h);
            if (scaleFactor > 1.) {
                // Use the highest necessary resolution image
                tmpWidth *= scaleFactor;
                tmpHeight *= scaleFactor;
            }
            if (consumer_dar > b_dar) {
                *width = qBound(qRound(normalized_width * consumerScale),
                                tmpWidth,
                                MLT_QTBLEND_MAX_DIMENSION);
                *height = qRound(*width * consumer_ar * normalized_height / normalized_width);
            } else {
                *height = qBound(qRound(normalized_height * consumerScale),
                                 tmpHeight,
                                 MLT_QTBLEND_MAX_DIMENSION);
                *width = qRound(*height * normalized_width / normalized_height / consumer_ar);
            }
            // Adjust rect to new scaling
            double scale = (double) *width / normalized_width;
            if (scale != 1.0) {
                rect.x *= scale;
                rect.w *= scale;
            }
            scale = (double) *height / normalized_height;
            if (scale != 1.0) {
                rect.y *= scale;
                rect.h *= scale;
            }
        } else {
            // First instance of a qtblend filter
            double scale = mlt_profile_scale_width(profile, *width);
            // Store consumer scaling for further uses
            mlt_properties_set_int(frame_properties, "qtblend_scaled", 1);
            mlt_properties_set_double(frame_properties, "qtblend_preview_scaling", scale);
            // Apply scaling
            if (scale != 1.0) {
                rect.x *= scale;
                rect.w *= scale;
                if (distort) {
                    b_width *= scale;
                } else {
                    // Apply consumer scaling to the source image request
                    b_width *= scale;
                    b_height *= scale;
                }
            }
            scale = mlt_profile_scale_height(profile, *height);
            if (scale != 1.0) {
                rect.y *= scale;
                rect.h *= scale;
                if (distort) {
                    b_height *= scale;
                }
            }
        }
        transform.translate(rect.x, rect.y);
        opacity = rect.o;
        hasAlpha = rect.o < 1 || rect.x != 0 || rect.y != 0 || rect.w != *width || rect.h != *height
                   || rect.w / b_dar < *height || rect.h * b_dar < *width || b_width < *width
                   || b_height < *height;
    } else {
        b_width = *width;
        b_height = *height;
        if (b_width < normalized_width || b_height < normalized_height) {
            hasAlpha = true;
        }
    }

    if (mlt_properties_get(properties, "rotation")) {
        double angle = mlt_properties_anim_get_double(properties, "rotation", position, length);
        if (angle != 0.0) {
            if (mlt_properties_get_int(properties, "rotate_center")) {
                transform.translate(rect.w / 2.0, rect.h / 2.0);
                transform.rotate(angle);
                transform.translate(-rect.w / 2.0, -rect.h / 2.0);
            } else {
                // old style rotation (from top left corner) to keep compatibility
                transform.rotate(angle);
            }
            hasAlpha = true;
        }
    }
    if (!hasAlpha && mlt_properties_get_int(properties, "compositing") != 0) {
        hasAlpha = true;
    }

    if (!hasAlpha) {
        uint8_t *src_image = NULL;
        error = mlt_frame_get_image(frame, &src_image, format, &b_width, &b_height, 0);
        if (*format == mlt_image_rgba || mlt_frame_get_alpha(frame)) {
            hasAlpha = true;
        } else {
            // Prepare output image
            *image = src_image;
            *width = b_width;
            *height = b_height;
            return 0;
        }
    }

    // fetch image
    *format = mlt_image_rgba;
    uint8_t *src_image = NULL;
    error = mlt_frame_get_image(frame, &src_image, format, &b_width, &b_height, 0);

    // Put source buffer into QImage
    QImage sourceImage;
    convert_mlt_to_qimage_rgba(src_image, &sourceImage, b_width, b_height);

    int image_size = mlt_image_format_size(*format, *width, *height, NULL);

    // resize to rect
    if (distort) {
        transform.scale(rect.w / b_width, rect.h / b_height);
    } else {
        double scale;
        double resize_dar = rect.w * consumer_ar / rect.h;
        if (b_dar >= resize_dar) {
            scale = rect.w / b_width;
        } else {
            scale = rect.h / b_height * b_ar;
        }
        // Center image in rect
        transform.translate((rect.w - (b_width * scale)) / 2.0, (rect.h - (b_height * scale)) / 2.0);
        transform.scale(scale, scale);
    }

    uint8_t *dest_image = NULL;
    dest_image = (uint8_t *) mlt_pool_alloc(image_size);

    QImage destImage;
    convert_mlt_to_qimage_rgba(dest_image, &destImage, *width, *height);
    destImage.fill(mlt_properties_get_int(properties, "background_color"));

    QPainter painter(&destImage);
    painter.setCompositionMode(
        (QPainter::CompositionMode) mlt_properties_get_int(properties, "compositing"));
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setTransform(transform);
    painter.setOpacity(opacity);
    // Composite top frame
    painter.drawImage(0, 0, sourceImage);
    // finish Qt drawing
    painter.end();
    convert_qimage_to_mlt_rgba(&destImage, dest_image, *width, *height);
    *image = dest_image;
    mlt_frame_set_image(frame, *image, *width * *height * 4, mlt_pool_release);
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

extern "C" {

mlt_filter filter_qtblend_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter && createQApplicationIfNeeded(MLT_FILTER_SERVICE(filter))) {
        filter->process = filter_process;
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_int(properties, "rotate_center", 0);
    } else {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Filter qtblend failed\n");

        if (filter) {
            mlt_filter_close(filter);
        }

        filter = NULL;
    }
    return filter;
}
}
