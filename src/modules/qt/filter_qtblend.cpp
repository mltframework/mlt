/*
 * filter_qtblend.cpp -- Qt composite filter
 * Copyright (C) 2015-2026 Meltytech, LLC
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

#define MLT_QTBLEND_MAX_DIMENSION (16000)

// Motion blur sums samples with this format. 16 bits per channel leaves enough headroom for
// color accuracy.
static const QImage::Format MLT_QTBLEND_ACCUMULATION_FORMAT = QImage::Format_RGBA64_Premultiplied;

// Animated destination geometry at one position, scaled to the output size.
struct Geometry
{
    mlt_rect rect;
    double rotation;
};

struct TransformContext
{
    mlt_properties properties;
    mlt_position length;
    int b_width;
    int b_height;
    double b_dar;
    double consumer_ar;
    bool distort;
};

/** Read the animated geometry for one position, in output coordinates.
*/
static Geometry get_geometry(mlt_properties properties,
                             mlt_position position,
                             mlt_position length,
                             int normalized_width,
                             int normalized_height,
                             double scale_x,
                             double scale_y)
{
    Geometry geometry;
    geometry.rect = {0, 0, (double) normalized_width, (double) normalized_height, 1.0};
    geometry.rotation = 0.0;

    if (mlt_properties_get(properties, "rect")) {
        geometry.rect = mlt_properties_anim_get_rect(properties, "rect", position, length);
        if (::strchr(mlt_properties_get(properties, "rect"), '%')) {
            geometry.rect.x *= normalized_width;
            geometry.rect.y *= normalized_height;
            geometry.rect.w *= normalized_width;
            geometry.rect.h *= normalized_height;
        }
    }
    geometry.rect.x *= scale_x;
    geometry.rect.w *= scale_x;
    geometry.rect.y *= scale_y;
    geometry.rect.h *= scale_y;

    if (mlt_properties_get(properties, "rotation")) {
        geometry.rotation = mlt_properties_anim_get_double(properties, "rotation", position, length);
    }
    return geometry;
}

/** Build the source to destination transform for one geometry.
*/
static QTransform build_transform(const TransformContext &ctx,
                                  mlt_position position,
                                  const Geometry &geometry)
{
    const mlt_rect &rect = geometry.rect;
    QTransform transform;
    transform.translate(rect.x, rect.y);

    if (geometry.rotation != 0.0) {
        if (mlt_properties_get(ctx.properties, "rotate_anchor")) {
            mlt_rect anchor = mlt_properties_anim_get_rect(ctx.properties,
                                                           "rotate_anchor",
                                                           position,
                                                           ctx.length);
            // Use custom anchor point (x,y are normalized 0-1 coordinates)
            double anchor_x = anchor.x * rect.w;
            double anchor_y = anchor.y * rect.h;
            transform.translate(anchor_x, anchor_y);
            transform.rotate(geometry.rotation);
            transform.translate(-anchor_x, -anchor_y);
        } else if (mlt_properties_get_int(ctx.properties, "rotate_center")) {
            // old style rotation (from center) to keep compatibility, equivalent to rotate_anchor = 0.5, 0.5
            transform.translate(rect.w / 2.0, rect.h / 2.0);
            transform.rotate(geometry.rotation);
            transform.translate(-rect.w / 2.0, -rect.h / 2.0);
        } else {
            // old style rotation (from top left corner) to keep compatibility, equivalent to rotate_anchor = 0, 0
            transform.rotate(geometry.rotation);
        }
    }

    // resize to rect
    if (ctx.distort) {
        if (rect.w != ctx.b_width || rect.h != ctx.b_height) {
            transform.scale(rect.w / ctx.b_width, rect.h / ctx.b_height);
        }
    } else {
        double scale;
        double resize_dar = rect.w * ctx.consumer_ar / rect.h;
        if (ctx.b_dar >= resize_dar) {
            scale = rect.w / ctx.b_width;
        } else {
            scale = rect.h / ctx.b_height;
        }
        // Center image in rect
        transform.translate((rect.w - (ctx.b_width * scale)) / 2.0,
                            (rect.h - (ctx.b_height * scale)) / 2.0);
        transform.scale(scale, scale);
    }
    return transform;
}

/** Geometry of blur sample \p index, walking back from the current frame
 * towards the neighboring one over the fraction of the motion the shutter is
 * open for.
*/
static Geometry sample_geometry(
    const Geometry &current, const Geometry &delta, double shutter, int samples, int index)
{
    double t = samples > 1 ? shutter * index / (samples - 1) : 0.0;
    Geometry geometry = current;
    geometry.rect.x -= delta.rect.x * t;
    geometry.rect.y -= delta.rect.y * t;
    geometry.rect.w -= delta.rect.w * t;
    geometry.rect.h -= delta.rect.h * t;
    geometry.rotation -= delta.rotation * t;
    return geometry;
}

struct WeightSlice
{
    uint8_t *bits;
    qsizetype stride;
    int components; // per row
    int rows;
    double weight;
};

static int weight_slice_proc(int id, int index, int jobs, void *cookie)
{
    (void) id;
    WeightSlice *slice = (WeightSlice *) cookie;
    int start = 0;
    int height = mlt_slices_size_slice(jobs, index, slice->rows, &start);

    for (int y = start; y < start + height; y++) {
        quint16 *row = reinterpret_cast<quint16 *>(slice->bits + (size_t) y * slice->stride);
        for (int i = 0; i < slice->components; i++) {
            row[i] = (quint16) qBound(0.0, row[i] * slice->weight + 0.5, 65535.0);
        }
    }
    return 0;
}

/** Premultiply the source into the accumulation format and scale it by
 * \p weight, so that summing every sample with Plus cannot saturate.
*/
static QImage weight_source(const QImage &sourceImage, double weight)
{
    // The source is straight alpha and the accumulation format is premultiplied
    QImage weighted = sourceImage.convertToFormat(MLT_QTBLEND_ACCUMULATION_FORMAT);
    WeightSlice slice = {weighted.bits(),
                         weighted.bytesPerLine(),
                         weighted.width() * 4,
                         weighted.height(),
                         weight};
    mlt_slices_run_normal(0, weight_slice_proc, &slice);
    return weighted;
}

struct BlurSlice
{
    const TransformContext *ctx;
    mlt_position position;
    const QImage *source; // premultiplied and pre-scaled
    const Geometry *current;
    const Geometry *delta;
    double shutter;
    int samples;
    bool hq;
    // Accumulation buffer, covering origin..origin+size of the canvas.
    uint8_t *bits;
    qsizetype stride;
    int width;
    int rows;
    QPoint origin;
};

static int blur_slice_proc(int id, int index, int jobs, void *cookie)
{
    (void) id;
    BlurSlice *slice = (BlurSlice *) cookie;
    int start = 0;
    int height = mlt_slices_size_slice(jobs, index, slice->rows, &start);
    if (height <= 0) {
        return 0;
    }

    QImage strip(slice->bits + (size_t) start * slice->stride,
                 slice->width,
                 height,
                 slice->stride,
                 MLT_QTBLEND_ACCUMULATION_FORMAT);
    QTransform toBox = QTransform::fromTranslate(-slice->origin.x(), -slice->origin.y() - start);

    QPainter painter(&strip);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, slice->hq);
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    for (int i = 0; i < slice->samples; i++) {
        Geometry geometry
            = sample_geometry(*slice->current, *slice->delta, slice->shutter, slice->samples, i);
        painter.setTransform(build_transform(*slice->ctx, slice->position, geometry) * toBox);
        painter.drawImage(0, 0, *slice->source);
    }
    painter.end();
    return 0;
}

/** Sum every motion blur sample into one premultiplied image. Returns a null
 * image, and leaves \p box empty, when the blur falls outside the canvas.
*/
static QImage accumulate_blur(const TransformContext &ctx,
                              mlt_position position,
                              const QImage &sourceImage,
                              const Geometry &current,
                              const Geometry &delta,
                              double shutter,
                              int samples,
                              int canvas_width,
                              int canvas_height,
                              bool hq,
                              QRect *box)
{
    // Region the source sweeps across all samples, with a small margin for the
    // interpolation footprint at the edges.
    QRectF sourceRect(0, 0, ctx.b_width, ctx.b_height);
    QRectF swept;
    for (int i = 0; i < samples; i++) {
        Geometry geometry = sample_geometry(current, delta, shutter, samples, i);
        QRectF mapped = build_transform(ctx, position, geometry).mapRect(sourceRect);
        swept = i == 0 ? mapped : swept.united(mapped);
    }
    swept.adjust(-2, -2, 2, 2);
    *box = swept.intersected(QRectF(0, 0, canvas_width, canvas_height)).toAlignedRect();
    if (box->isEmpty()) {
        return QImage();
    }

    QImage accumulation(box->size(), MLT_QTBLEND_ACCUMULATION_FORMAT);
    accumulation.fill(Qt::transparent);
    QImage weighted = weight_source(sourceImage, 1.0 / samples);

    BlurSlice slice = {&ctx,
                       position,
                       &weighted,
                       &current,
                       &delta,
                       shutter,
                       samples,
                       hq,
                       accumulation.bits(),
                       accumulation.bytesPerLine(),
                       accumulation.width(),
                       accumulation.height(),
                       box->topLeft()};
    mlt_slices_run_normal(0, blur_slice_proc, &slice);
    return accumulation;
}

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
    int normalized_width = profile->width;
    int normalized_height = profile->height;
    double consumer_ar = mlt_profile_sar(profile);

    // Destination rect
    mlt_rect rect = {0, 0, (double) normalized_width, (double) normalized_height, 1.0};
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

    // Scaling applied to the rect below, so that motion blur can read the
    // neighboring frame's geometry in the same coordinates.
    double rect_scale_x = 1.0;
    double rect_scale_y = 1.0;

    // If the _qtblend_scaled property is defined, a qtblend filter was already applied
    double qtblendScaleX = qMin(1., mlt_properties_get_double(frame_properties, "qtblend_scalingx"));
    if (mlt_properties_get(properties, "rect")) {
        rect = mlt_properties_anim_get_rect(properties, "rect", position, length);
        if (::strchr(mlt_properties_get(properties, "rect"), '%')) {
            rect.x *= normalized_width;
            rect.y *= normalized_height;
            rect.w *= normalized_width;
            rect.h *= normalized_height;
        }
    }
    if (qtblendScaleX > 0.) {
        // Another qtblend filter was already applied
        // In this case, the *width and *height are set to the source resolution to ensure we don't lose too much details on multiple scaling operations
        // We requested a image with full media resolution, adjust rect to profile
        // Check if we have consumer scaling enabled since we cannot use *width and *height
        double qtblendScaleY = qMin(1.,
                                    mlt_properties_get_double(frame_properties, "qtblend_scalingy"));
        // Consumer scaling was already applied to b_width/b_height
        // Always request an image that follows the consumer aspect ratio
        int tmpWidth = b_width;
        int tmpHeight = b_height;
        double scaleFactor = qMax(*width / rect.w, *height / rect.h);
        if (scaleFactor > 1.) {
            // Use the highest necessary resolution image
            tmpWidth *= scaleFactor;
            tmpHeight *= scaleFactor;
        }
        if (consumer_ar * normalized_height / normalized_width < 1.) {
            *width = qBound(qRound(normalized_width * qtblendScaleX),
                            tmpWidth,
                            MLT_QTBLEND_MAX_DIMENSION);
            *height = qRound(*width * consumer_ar * normalized_height / normalized_width);
        } else {
            *height = qBound(qRound(normalized_height * qtblendScaleY),
                             tmpHeight,
                             MLT_QTBLEND_MAX_DIMENSION);
            *width = qRound(*height * normalized_width / normalized_height / consumer_ar);
        }
        // Adjust rect to new scaling
        rect_scale_x = (double) *width / normalized_width;
        rect_scale_y = (double) *height / normalized_height;
        double scale = rect_scale_x;
        if (scale != 1.0) {
            rect.x *= scale;
            rect.w *= scale;
        }
        scale = rect_scale_y;
        if (scale != 1.0) {
            rect.y *= scale;
            rect.h *= scale;
        }
    } else {
        // First instance of a qtblend filter
        // Check if requested frame size is scaled
        double scalex = mlt_profile_scale_width(profile, *width);
        double scaley = mlt_profile_scale_height(profile, *height);
        rect_scale_x = scalex;
        rect_scale_y = scaley;

        // Store consumer scaling for further uses
        mlt_properties_set_double(frame_properties, "qtblend_scalingx", scalex);
        mlt_properties_set_double(frame_properties, "qtblend_scalingy", scaley);
        // Apply scaling
        if (scalex != 1.0) {
            rect.x *= scalex;
            rect.w *= scalex;
            if (b_width < normalized_width) {
                // Adjust scale so that we don't request too small images
                scalex = qBound(scalex, normalized_width * scalex / b_width, 1.);
            }
            // Apply consumer scaling to the source image
            if (scalex < 1.) {
                b_width *= scalex;
                b_height *= scalex;
            }
        }
        if (scaley != 1.0) {
            rect.y *= scaley;
            rect.h *= scaley;
        }
    }

    // Normalize source dimensions to consumer PAR to handle anamorphic sources
    normalize_mlt_source_size(b_ar, consumer_ar, &b_width, b_height);

    // Fix for bug #1228 and optimization.
    // Adjust requested dimension so MLT (libswscale) does the preliminary downscaling.
    // Using a step defined by MLT_QT_MIPMAP_STEP provides a tight bound to target resolution
    // (maximizing quality and preventing QPainter aliasing) while keeping the requested
    // dimensions stable across small animation increments.
    if (rect.w > 0 && rect.h > 0 && b_width > 0 && b_height > 0) {
        double scaleTarget;
        if (distort) {
            scaleTarget = qMax(rect.w / b_width, rect.h / b_height);
        } else {
            double resize_dar = rect.w * consumer_ar / rect.h;
            if (b_dar >= resize_dar) {
                scaleTarget = rect.w / b_width;
            } else {
                scaleTarget = rect.h / b_height;
            }
        }
        adjust_mlt_mipmap_size(scaleTarget, &b_width, &b_height);
    }

    opacity = rect.o;
    hasAlpha = rect.o < 1 || rect.x != 0 || rect.y != 0 || rect.w != *width || rect.h != *height
               || rect.w / b_dar < *height || rect.h * b_dar < *width || b_width != *width
               || b_height != *height;

    double rotation = 0.0;
    if (mlt_properties_get(properties, "rotation")) {
        rotation = mlt_properties_anim_get_double(properties, "rotation", position, length);
        if (rotation != 0.0) {
            hasAlpha = true;
        }
    }
    if (!hasAlpha && mlt_properties_get_int(properties, "compositing") != 0) {
        hasAlpha = true;
    }

    // Motion blur, measured from the geometry change between this frame and its
    // neighbor. Both samples and shutter_angle default to 0, so it is disabled
    // unless a host asks for it.
    Geometry current = {rect, rotation};
    Geometry delta = {{0, 0, 0, 0, 0}, 0};
    bool blur = false;
    double shutter = 0.0;
    int samples = 0;
    if (mlt_properties_get(properties, "shutter_angle")) {
        shutter = mlt_properties_anim_get_double(properties, "shutter_angle", position, length)
                  / 360.0;
    }
    if (mlt_properties_exists(properties, "samples")) {
        samples = mlt_properties_get_int(properties, "samples");
    }
    if (shutter > 0.0 && samples > 1) {
        // The first frame has no predecessor, so measure forward instead.
        bool forward = position <= 0;
        Geometry neighbor = get_geometry(properties,
                                         forward ? position + 1 : position - 1,
                                         length,
                                         normalized_width,
                                         normalized_height,
                                         rect_scale_x,
                                         rect_scale_y);
        const Geometry &to = forward ? neighbor : current;
        const Geometry &from = forward ? current : neighbor;
        delta.rect.x = to.rect.x - from.rect.x;
        delta.rect.y = to.rect.y - from.rect.y;
        delta.rect.w = to.rect.w - from.rect.w;
        delta.rect.h = to.rect.h - from.rect.h;
        delta.rotation = to.rotation - from.rotation;
        blur = fabs(delta.rect.x) + fabs(delta.rect.y) + fabs(delta.rect.w) + fabs(delta.rect.h)
                   + fabs(delta.rotation)
               > 1e-3;
        if (blur) {
            // The blur spreads the source beyond the rect, so it always needs
            // to be composited rather than passed through.
            hasAlpha = true;
        }
    }

    if (!hasAlpha) {
        uint8_t *src_image = NULL;
        error = mlt_frame_get_image(frame, &src_image, format, &b_width, &b_height, 0);
        if (*format == mlt_image_rgba || *format == mlt_image_rgba64 || mlt_frame_get_alpha(frame)) {
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
    *format = choose_image_format(*format);
    uint8_t *src_image = NULL;
    error = mlt_frame_get_image(frame, &src_image, format, &b_width, &b_height, 0);

    // Put source buffer into QImage
    QImage sourceImage;
    convert_mlt_to_qimage(src_image, &sourceImage, b_width, b_height, *format);

    struct mlt_image_s image_desc;
    mlt_image_set_values(&image_desc, NULL, *format, *width, *height);
    int image_size = mlt_image_calculate_size(&image_desc);

    char *interps = mlt_properties_get(frame_properties, "consumer.rescale");
    bool hqPainting = interps && strcmp(interps, "nearest") && strcmp(interps, "neighbor");

    TransformContext ctx = {properties, length, b_width, b_height, b_dar, consumer_ar, distort};

    uint8_t *dest_image = NULL;
    dest_image = (uint8_t *) mlt_pool_alloc(image_size);

    QImage destImage;
    convert_mlt_to_qimage(dest_image, &destImage, *width, *height, *format);
    destImage.fill(mlt_properties_get_int(properties, "background_color"));

    QPainter painter(&destImage);
    painter.setCompositionMode(
        (QPainter::CompositionMode) mlt_properties_get_int(properties, "compositing"));
    painter.setOpacity(opacity);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, hqPainting);
    if (blur) {
        // The samples are summed separately, then composited in one pass so
        // that compositing mode, opacity and background behave as usual.
        QRect box;
        QImage blurred = accumulate_blur(ctx,
                                         position,
                                         sourceImage,
                                         current,
                                         delta,
                                         shutter,
                                         samples,
                                         *width,
                                         *height,
                                         hqPainting,
                                         &box);
        if (!blurred.isNull()) {
            painter.drawImage(box.topLeft(), blurred);
        }
    } else {
        painter.setTransform(build_transform(ctx, position, current));
        // Composite top frame
        painter.drawImage(0, 0, sourceImage);
    }
    // finish Qt drawing
    painter.end();

    convert_qimage_to_mlt(&destImage, dest_image, *width, *height);
    *image = dest_image;
    mlt_frame_set_image(frame, *image, image_size, mlt_pool_release);
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
