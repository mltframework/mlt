/*
 * transition_qtblend.cpp -- Qt composite transition
 * Copyright (c) 2016-2025 Jean-Baptiste Mardelle <jb@kdenlive.org>
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
#include <stdio.h>
#include <string.h>
#include <QImage>
#include <QPainter>
#include <QTransform>

static int get_image(mlt_frame a_frame,
                     uint8_t **image,
                     mlt_image_format *format,
                     int *width,
                     int *height,
                     int writable)
{
    int error = 0;
    mlt_frame b_frame = mlt_frame_pop_frame(a_frame);
    mlt_properties b_properties = MLT_FRAME_PROPERTIES(b_frame);
    mlt_properties properties = MLT_FRAME_PROPERTIES(a_frame);
    mlt_transition transition = MLT_TRANSITION(mlt_frame_pop_service(a_frame));
    mlt_properties transition_properties = MLT_TRANSITION_PROPERTIES(transition);

    uint8_t *b_image = NULL;
    // hasAlpha indicates whether the source material has an alpha channel
    bool hasAlpha = *format == mlt_image_rgba;
    double opacity = 1.0;
    QTransform transform;

    // Determine length
    mlt_position length = mlt_transition_get_length(transition);
    // Get current position
    mlt_position position = mlt_transition_get_position(transition, a_frame);

    // Obtain the normalized width and height from the a_frame
    mlt_profile profile = mlt_service_profile(MLT_TRANSITION_SERVICE(transition));
    int b_width = mlt_properties_get_int(b_properties, "meta.media.width");
    int b_height = mlt_properties_get_int(b_properties, "meta.media.height");
    int normalized_width = profile->width;
    int normalized_height = profile->height;

    // reference rect
    mlt_rect rect = {0, 0, (double) normalized_width, (double) normalized_height, 1.0};

    bool distort = mlt_properties_get_int(transition_properties, "distort");
    double consumer_ar = mlt_profile_sar(profile);

    // Check the producer's native format before fetching image
    int sourceFormat = mlt_properties_get_int(b_properties, "format");
    if (sourceFormat == mlt_image_rgba || sourceFormat == mlt_image_rgb) {
        hasAlpha = sourceFormat == mlt_image_rgba;
        *format = mlt_image_rgba;
    }

    if (b_height == 0) {
        b_width = *width;
        b_height = *height;
    }
    double b_ar = mlt_frame_get_aspect_ratio(b_frame);
    double b_dar = b_ar * b_width / b_height;
    double geometry_dar = consumer_ar * normalized_width / normalized_height;
    double transformScale = 1.;
    // forceAlpha is true if some operation makes it mandatory to perform the alpha compositing, like padding or scaling
    bool forceAlpha = b_dar != geometry_dar;
    if (forceAlpha) {
        // Ensure we ask for an image with same dar as consumer
        b_width = geometry_dar * b_height;
    }

    if (!distort && (b_height < *height || b_width < *width)) {
        // Source image is smaller than profile, request full frame
        if (b_dar > geometry_dar) {
            transformScale = b_dar / geometry_dar;
        } else {
            transformScale = geometry_dar / b_dar;
        }
        b_width = *width;
        b_height = *height;
    }

    if (mlt_properties_get(transition_properties, "rect")) {
        rect = mlt_properties_anim_get_rect(transition_properties, "rect", position, length);
        if (::strchr(mlt_properties_get(transition_properties, "rect"), '%')) {
            // We have percentage values, scale to frame size
            rect.x *= normalized_width;
            rect.y *= normalized_height;
            rect.w *= normalized_width;
            rect.h *= normalized_height;
        }
    } else {
        // Optimization, request profile sized image
        b_width = normalized_width;
        b_height = normalized_height;
    }
    int request_width = *width;
    int request_height = *height;

    double scalex = mlt_profile_scale_width(profile, *width);
    double scaley = mlt_profile_scale_height(profile, *height);

    // Adjust to preview scaling
    if (scalex != 1.0) {
        // We are using consumer scaling, fetch a lower resolution image too
        rect.x *= scalex;
        rect.w *= scalex;
        if (scalex < 1.) {
            b_width *= scalex;
        }
    }
    if (scaley != 1.0) {
        rect.y *= scaley;
        rect.h *= scaley;
        if (scaley < 1.) {
            b_height *= scaley;
        }
    }

    transform.translate(rect.x, rect.y);
    opacity = rect.o;
    if (!forceAlpha
        && (opacity < 1 || rect.x != 0 || rect.y != 0 || (rect.x + rect.w != *width)
            || (rect.y + rect.h != *height))) {
        // we will process operations on top frame, so also process b_frame
        forceAlpha = true;
    }
    // Ensure we don't request an image with a 0 width or height
    b_width = qMax(1, b_width);
    b_height = qMax(1, b_height);
    /*} else {
        b_height = *height;
        b_width = *width;
    }*/

    if (mlt_frame_get_aspect_ratio(b_frame) == 0) {
        mlt_frame_set_aspect_ratio(b_frame, consumer_ar);
    }

    if (mlt_properties_get(transition_properties, "rotation")) {
        double angle
            = mlt_properties_anim_get_double(transition_properties, "rotation", position, length);
        if (angle != 0.0) {
            if (mlt_properties_get_int(transition_properties, "rotate_center")) {
                transform.translate(rect.w / 2.0, rect.h / 2.0);
                transform.rotate(angle);
                transform.translate(-rect.w / 2.0, -rect.h / 2.0);
            } else {
                transform.rotate(angle);
            }
            forceAlpha = true;
        }
    }

    // This is not a field-aware transform.
    mlt_properties_set_int(b_properties, "consumer.progressive", 1);

    // Suppress padding and aspect normalization.
    char *interps = mlt_properties_get(properties, "consumer.rescale");
    if (interps)
        interps = strdup(interps);

    // Check profile dar vs image dar image
    if (!forceAlpha && rect.w == -1 && b_dar != mlt_profile_dar(profile)) {
        // Activate transparency if the clips don't have the same aspect ratio
        forceAlpha = true;
    }
    if (!forceAlpha
        && (mlt_properties_get_int(transition_properties, "compositing") != 0 || b_width < *width
            || b_height < *height)) {
        forceAlpha = true;
    }

    // Check if we have transparency
    bool imageFetched = false;
    if (!forceAlpha) {
        if (!hasAlpha || *format == mlt_image_rgba) {
            // fetch image in native format
            error = mlt_frame_get_image(b_frame, &b_image, format, &b_width, &b_height, 0);
            imageFetched = true;
            if (!hasAlpha && (*format == mlt_image_rgba || mlt_frame_get_alpha(b_frame))) {
                hasAlpha = true;
            }
            if (hasAlpha && *format == mlt_image_rgba) {
                hasAlpha = !mlt_image_rgba_opaque(b_image, b_width, b_height);
            }
        }
        if (!hasAlpha) {
            // Pass all required frame properties
            mlt_properties_pass_list(properties,
                                     b_properties,
                                     "progressive,distort,colorspace,full_range,force_full_luma,"
                                     "top_field_first,color_trc");
            // Prepare output image
            if (b_frame->convert_image && (b_width != request_width || b_height != request_height)) {
                mlt_properties_set_int(b_properties, "convert_image_width", request_width);
                mlt_properties_set_int(b_properties, "convert_image_height", request_height);
                b_frame->convert_image(b_frame, &b_image, format, *format);
                *width = request_width;
                *height = request_height;
            } else {
                *width = b_width;
                *height = b_height;
            }
            *image = b_image;
            free(interps);
            return 0;
        }
    }
    if (!imageFetched) {
        *format = mlt_image_rgba;
        error = mlt_frame_get_image(b_frame, &b_image, format, &b_width, &b_height, 0);
    }

    if (b_frame->convert_image && (*format != mlt_image_rgba)) {
        b_frame->convert_image(b_frame, &b_image, format, mlt_image_rgba);
    }
    *format = mlt_image_rgba;
    if (distort) {
        if (b_width != 0 && b_height != 0) {
            transform.scale(rect.w / b_width, rect.h / b_height);
        }
    } else if (rect.w > 0 && rect.h > 0) {
        double resize_dar = rect.w * consumer_ar / rect.h;
        double scale;
        if (b_dar > resize_dar) {
            scale = rect.w / b_width;
            if (b_dar < geometry_dar) {
                scale *= transformScale;
            }
        } else {
            scale = rect.h / b_height;
            if (b_dar > geometry_dar) {
                scale *= transformScale;
            }
        }
        transform.translate((rect.w - (b_width * scale)) / 2.0, (rect.h - (b_height * scale)) / 2.0);
        transform.scale(scale, scale);
    }

    // Get bottom frame
    uint8_t *a_image = NULL;
    error = mlt_frame_get_image(a_frame, &a_image, format, width, height, 0);
    if (error) {
        free(interps);
        return error;
    }
    // Prepare output image
    int image_size = mlt_image_format_size(*format, *width, *height, NULL);
    *image = (uint8_t *) mlt_pool_alloc(image_size);

    // Copy bottom frame in output
    memcpy(*image, a_image, image_size);

    // We don't do subpixel smoothing for nearest neighbour interpolation
    // so people can use that to upscale pixel art and keep the hard edges.
    bool hqPainting = interps && strcmp(interps, "nearest") != 0;

    // convert bottom mlt image to qimage
    QImage bottomImg;
    convert_mlt_to_qimage_rgba(*image, &bottomImg, *width, *height);

    // convert top mlt image to qimage
    QImage topImg;
    convert_mlt_to_qimage_rgba(b_image, &topImg, b_width, b_height);

    // setup Qt drawing
    QPainter painter(&bottomImg);
    painter.setCompositionMode(
        (QPainter::CompositionMode) mlt_properties_get_int(transition_properties, "compositing"));
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, hqPainting);
    painter.setTransform(transform);
    painter.setOpacity(opacity);

    // Composite top frame
    painter.drawImage(0, 0, topImg);

    // finish Qt drawing
    painter.end();
    convert_qimage_to_mlt_rgba(&bottomImg, *image, *width, *height);
    mlt_frame_set_image(a_frame, *image, image_size, mlt_pool_release);
    // Remove potentially large image on the B frame.
    mlt_frame_set_image(b_frame, NULL, 0, NULL);
    free(interps);
    return error;
}

static mlt_frame process(mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame)
{
    mlt_frame_push_service(a_frame, transition);
    mlt_frame_push_frame(a_frame, b_frame);
    mlt_frame_push_get_image(a_frame, get_image);
    return a_frame;
}

extern "C" {

mlt_transition transition_qtblend_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       void *arg)
{
    mlt_transition transition = mlt_transition_new();

    if (transition) {
        mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);

        if (!createQApplicationIfNeeded(MLT_TRANSITION_SERVICE(transition))) {
            mlt_transition_close(transition);
            return NULL;
        }
        transition->process = process;
        mlt_properties_set_int(properties, "_transition_type", 1); // video only
        mlt_properties_set(properties, "rect", (char *) arg);
        mlt_properties_set_int(properties, "compositing", 0);
        mlt_properties_set_int(properties, "distort", 0);
        mlt_properties_set_int(properties, "rotate_center", 0);
    }

    return transition;
}

} // extern "C"
