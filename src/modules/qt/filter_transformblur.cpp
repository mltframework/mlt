/*
 * filter_transformblur.cpp -- position/scale/rotation filter with motion blur
 * Copyright (C) 2026 Meltytech, LLC
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
#include <math.h>
#include <string.h>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QTransform>

namespace {

struct TransformState
{
    double x, y, w, h, rotation;
};

TransformState operator-(const TransformState &a, const TransformState &b)
{
    return {a.x - b.x, a.y - b.y, a.w - b.w, a.h - b.h, a.rotation - b.rotation};
}

TransformState operator*(const TransformState &a, double s)
{
    return {a.x * s, a.y * s, a.w * s, a.h * s, a.rotation * s};
}


QTransform build_transform(const TransformState &p,
                           int src_width,
                           int src_height,
                           QPointF outputOffset = QPointF(0, 0))
{
    QTransform t;
    if (!outputOffset.isNull())
        t.translate(-outputOffset.x(), -outputOffset.y());
    t.translate(p.x, p.y);
    if (p.rotation != 0.0) {
        t.translate(p.w / 2.0, p.h / 2.0);
        t.rotate(p.rotation);
        t.translate(-p.w / 2.0, -p.h / 2.0);
    }
    if (src_width > 0 && src_height > 0 && (p.w != src_width || p.h != src_height)) {
        t.scale(p.w / src_width, p.h / src_height);
    }
    return t;
}

// The blur is built from `samples` time-interpolated parameter sets,
// walking from the current frame (sampleIndex 0) back towards the
// neighboring keyframe, covering the fraction of the frame-to-frame delta
// selected by the shutter angle (`frac` = shutter_angle / 360).
TransformState sample_params(
    const TransformState &current, const TransformState &delta, double frac, int samples, int sampleIndex)
{
    double t = (samples > 1) ? frac * sampleIndex / (samples - 1) : 0.0;
    return current - delta * t;
}

// Bounding box (in destination space) swept by the source image across all
// `samples` transforms. Used to restrict rendering to the region that can
// actually end up non-transparent, instead of always processing the full
// profile canvas.
QRectF compute_swept_rect(const TransformState &current,
                          const TransformState &delta,
                          double frac,
                          int samples,
                          int src_width,
                          int src_height)
{
    QRectF sourceRect(0, 0, src_width, src_height);
    QRectF swept;
    for (int s = 0; s < samples; s++) {
        TransformState sampled = sample_params(current, delta, frac, samples, s);
        QRectF mapped = build_transform(sampled, src_width, src_height).mapRect(sourceRect);
        swept = (s == 0) ? mapped : swept.united(mapped);
    }
    // Small margin for the bilinear/antialiasing footprint at the edges.
    swept.adjust(-2, -2, 2, 2);
    return swept;
}

// Read-only inputs shared by every worker slice, plus the region of
// dest_image each one is allowed to write to.
struct BlurSliceContext
{
    const QImage *weightedSource; // premultiplied, channels pre-scaled by 1/samples
    TransformState current;
    TransformState delta;
    double frac; // shutter angle
    int samples;
    int src_width, src_height; // dimensions weightedSource was built from

    // Sub-canvas (the swept bounding box) within the full destination image.
    int subX0, subY0;
    int subWidth, subHeight;

    uint8_t *dest_image;
    int dest_stride; // bytes per row of dest_image (full canvas width * 4)
};

// mlt_slices worker: renders and accumulates every sample, but only for a
// horizontal strip of the sub-canvas. Strips don't overlap, so each worker
// owns an independent QImage and an independent region of dest_image --
// no locking needed.
int sliced_blur_proc(int id, int index, int jobs, void *cookie)
{
    (void) id;
    const BlurSliceContext *ctx = static_cast<const BlurSliceContext *>(cookie);

    int sliceStart = 0;
    int sliceHeight = mlt_slices_size_slice(jobs, index, ctx->subHeight, &sliceStart);
    if (sliceHeight <= 0)
        return 0;

    QImage stripAccum(ctx->subWidth, sliceHeight, QImage::Format_RGBA64_Premultiplied);
    stripAccum.fill(0);

    QPointF stripOffset(ctx->subX0, ctx->subY0 + sliceStart);

    QPainter painter(&stripAccum);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    for (int s = 0; s < ctx->samples; s++) {
        TransformState sampled = sample_params(ctx->current, ctx->delta, ctx->frac, ctx->samples, s);
        painter.setTransform(build_transform(sampled, ctx->src_width, ctx->src_height, stripOffset));
        painter.drawImage(0, 0, *ctx->weightedSource);
    }
    painter.end();

    QImage stripStraight = stripAccum.convertToFormat(QImage::Format_RGBA8888);
    for (int y = 0; y < sliceHeight; y++) {
        uint8_t *dst = ctx->dest_image + (size_t) (ctx->subY0 + sliceStart + y) * ctx->dest_stride
                       + (size_t) ctx->subX0 * 4;
        memcpy(dst, stripStraight.constScanLine(y), (size_t) ctx->subWidth * 4);
    }
    return 0;
}

struct WeightSliceContext
{
    QImage *image; // premultiplied; scaled in place
    double weight;
};

// mlt_slices worker: scales a horizontal strip of rows of ctx->image by
// ctx->weight in place. Strips don't overlap, so no locking needed.
int sliced_weight_proc(int id, int index, int jobs, void *cookie)
{
    (void) id;
    const WeightSliceContext *ctx = static_cast<const WeightSliceContext *>(cookie);

    int sliceStart = 0;
    int sliceHeight = mlt_slices_size_slice(jobs, index, ctx->image->height(), &sliceStart);
    if (sliceHeight <= 0)
        return 0;

    for (int y = sliceStart; y < sliceStart + sliceHeight; y++) {
        quint16 *row = reinterpret_cast<quint16 *>(ctx->image->scanLine(y));
        for (int x = 0; x < ctx->image->width() * 4; x++) {
            row[x] = (quint16) qBound(0.0, row[x] * ctx->weight + 0.5, 65535.0);
        }
    }
    return 0;
}

// Builds the contribution of a single sample to the blurred image.
//
// This uses 16 bits/channel to preserve color accuracy even at higher sample counts
QImage build_weighted_source(const QImage &sourceImage, int samples)
{
    QImage weighted = sourceImage.convertToFormat(QImage::Format_RGBA64_Premultiplied);
    weighted.detach();

    WeightSliceContext ctx;
    ctx.image = &weighted;
    ctx.weight = 1.0 / samples;
    mlt_slices_run_normal(0, sliced_weight_proc, &ctx);

    return weighted;
}

// Renders with motion blur using mlt_slices_run_normal
void render_motion_blur(const QImage &sourceImage,
                        uint8_t *dest_image,
                        int dest_width,
                        int dest_height,
                        const TransformState &current,
                        const TransformState &delta,
                        double frac,
                        int samples,
                        int src_width,
                        int src_height)
{
    QRectF swept = compute_swept_rect(current, delta, frac, samples, src_width, src_height);
    QRect clipped = swept.intersected(QRectF(0, 0, dest_width, dest_height)).toAlignedRect();
    if (clipped.isEmpty())
        return; // Content is entirely outside the frame; nothing to draw.

    QImage weightedSource = build_weighted_source(sourceImage, samples);

    BlurSliceContext ctx;
    ctx.weightedSource = &weightedSource;
    ctx.current = current;
    ctx.delta = delta;
    ctx.frac = frac;
    ctx.samples = samples;
    ctx.src_width = src_width;
    ctx.src_height = src_height;
    ctx.subX0 = clipped.left();
    ctx.subY0 = clipped.top();
    ctx.subWidth = clipped.width();
    ctx.subHeight = clipped.height();
    ctx.dest_image = dest_image;
    ctx.dest_stride = dest_width * 4;

    mlt_slices_run_normal(0, sliced_blur_proc, &ctx);
}

// Render transform only without any blur
void render_transform_only(
    const QImage &sourceImage, QImage &destImage, const TransformState &current, int src_width, int src_height)
{
    QPainter painter(&destImage);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setTransform(build_transform(current, src_width, src_height));
    painter.drawImage(0, 0, sourceImage);
    painter.end();
}

void apply_opacity(uint8_t *dest_image, int width, int height, mlt_image_format format, double opacity)
{
    if (opacity >= 0.999)
        return;
    if (opacity < 0.0)
        opacity = 0.0;

    int pixel_count = width * height;
    if (format == mlt_image_rgba) {
        uint8_t *p = dest_image + 3;
        for (int i = 0; i < pixel_count; i++, p += 4) {
            *p = (uint8_t) qBound(0.0, *p * opacity + 0.5, 255.0);
        }
    } else if (format == mlt_image_rgba64) {
        uint16_t *p = reinterpret_cast<uint16_t *>(dest_image) + 3;
        for (int i = 0; i < pixel_count; i++, p += 4) {
            *p = (uint16_t) qBound(0.0, *p * opacity + 0.5, 65535.0);
        }
    }
}

} // namespace

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
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

    mlt_service_lock(MLT_FILTER_SERVICE(filter));
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    int normalized_width = profile->width;
    int normalized_height = profile->height;

    // On the first frame use delta to the second frame
    bool use_forward_diff = position <= 0;
    mlt_position neighbor_position = use_forward_diff ? position + 1 : position - 1;

    auto fetch_params = [&](mlt_position pos) {
        mlt_rect rect = {0, 0, (double) normalized_width, (double) normalized_height, 1.0};
        double rotation = 0.0;
        if (mlt_properties_get(properties, "rect")) {
            rect = mlt_properties_anim_get_rect(properties, "rect", pos, length);
            if (::strchr(mlt_properties_get(properties, "rect"), '%')) {
                rect.x *= normalized_width;
                rect.y *= normalized_height;
                rect.w *= normalized_width;
                rect.h *= normalized_height;
            }
        }
        if (mlt_properties_get(properties, "rotation")) {
            rotation = mlt_properties_anim_get_double(properties, "rotation", pos, length);
        }
        return TransformState{rect.x, rect.y, rect.w, rect.h, rotation};
    };

    TransformState current = fetch_params(position);
    TransformState neighbor = fetch_params(neighbor_position);
    TransformState delta = use_forward_diff ? (neighbor - current) : (current - neighbor);

    double opacity = 1.0;
    if (mlt_properties_get(properties, "rect")) {
        opacity = mlt_properties_anim_get_rect(properties, "rect", position, length).o;
    }

    double shutter_angle = mlt_properties_exists(properties, "shutter_angle")
                               ? mlt_properties_anim_get_double(properties,
                                                                "shutter_angle",
                                                                position,
                                                                length)
                               : 180.0;
    double frac = shutter_angle / 360.0;
    int samples = mlt_properties_exists(properties, "samples")
                     ? mlt_properties_get_int(properties, "samples")
                     : 16;
    if (samples < 1)
        samples = 1;
    bool has_motion = fabs(delta.x) + fabs(delta.y) + fabs(delta.w) + fabs(delta.h)
                          + fabs(delta.rotation)
                      > 1e-3;
    bool do_blur = frac > 0.0001 && samples > 1 && has_motion;

    *format = choose_image_format(*format);
    uint8_t *src_image = NULL;
    int b_width = 0, b_height = 0;
    error = mlt_frame_get_image(frame, &src_image, format, &b_width, &b_height, 0);

    QImage sourceImage;
    convert_mlt_to_qimage(src_image, &sourceImage, b_width, b_height, *format);

    *width = normalized_width;
    *height = normalized_height;
    struct mlt_image_s dest_image_desc;
    mlt_image_set_values(&dest_image_desc, NULL, *format, *width, *height);
    int image_size = mlt_image_calculate_size(&dest_image_desc);
    uint8_t *dest_image = (uint8_t *) mlt_pool_alloc(image_size);

    QImage destImage;
    convert_mlt_to_qimage(dest_image, &destImage, *width, *height, *format);
    destImage.fill(0);

    if (!do_blur || *format != mlt_image_rgba) {
        // No blur requested or unsupported format
        render_transform_only(sourceImage, destImage, current, b_width, b_height);
    } else {
        render_motion_blur(sourceImage,
                           dest_image,
                           *width,
                           *height,
                           current,
                           delta,
                           frac,
                           samples,
                           b_width,
                           b_height);
    }

    convert_qimage_to_mlt(&destImage, dest_image, *width, *height);
    apply_opacity(dest_image, *width, *height, *format, opacity);

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

mlt_filter filter_transformblur_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter && createQApplicationIfNeeded(MLT_FILTER_SERVICE(filter))) {
        filter->process = filter_process;
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_double(properties, "shutter_angle", 180.0);
        mlt_properties_set_int(properties, "samples", 16);
    } else {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Filter transformblur failed\n");

        if (filter) {
            mlt_filter_close(filter);
        }

        filter = NULL;
    }
    return filter;
}
}
