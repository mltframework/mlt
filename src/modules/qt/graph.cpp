/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

#include "graph.h"
#include <math.h>
#include <QPainterPath>
#include <QVector>

QVector<QColor> get_graph_colors(mlt_properties filter_properties, int position, int length)
{
    QVector<QColor> colors;

    // Find user specified colors for the gradient
    while (true) {
        QString prop_name = QString("color.") + QString::number(colors.size() + 1);
        if (mlt_properties_exists(filter_properties, prop_name.toUtf8().constData())) {
            mlt_color mcolor = mlt_properties_anim_get_color(filter_properties,
                                                             prop_name.toUtf8().constData(),
                                                             position,
                                                             length);
            colors.append(QColor(mcolor.r, mcolor.g, mcolor.b, mcolor.a));
        } else {
            break;
        }
    }

    if (colors.size() == 0) {
        // No colors found - use a default.
        colors.append(QColor(Qt::white));
    }

    return colors;
}

/*
 * Apply the "bgcolor" and "angle" parameters to the QPainter
 */
void setup_graph_painter(
    QPainter &p, QRectF &r, mlt_properties filter_properties, int position, int length)
{
    mlt_color bg_color
        = mlt_properties_anim_get_color(filter_properties, "bgcolor", position, length);
    double angle = mlt_properties_anim_get_double(filter_properties, "angle", position, length);

    p.setRenderHint(QPainter::Antialiasing);

    // Fill background
    if (bg_color.r || bg_color.g || bg_color.g || bg_color.a) {
        QColor qbgcolor(bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        p.fillRect(0, 0, p.device()->width(), p.device()->height(), qbgcolor);
    }

    // Apply rotation
    if (angle) {
        p.translate(r.x() + r.width() / 2, r.y() + r.height() / 2);
        p.rotate(angle);
        p.translate(-(r.x() + r.width() / 2), -(r.y() + r.height() / 2));
    }
}

/*
 * Apply the "thickness", "gorient" and "color.x" parameters to the QPainter
 */
void setup_graph_pen(
    QPainter &p, QRectF &r, mlt_properties filter_properties, double scale, int position, int length)
{
    int thickness = mlt_properties_anim_get_int(filter_properties, "thickness", position, length)
                    * scale;
    QString gorient = mlt_properties_get(filter_properties, "gorient");
    QVector<QColor> colors = get_graph_colors(filter_properties, position, length);
    QPen pen;
    pen.setWidth(qAbs(thickness));

    if (colors.size() == 1) {
        // Only use one color
        pen.setBrush(colors[0]);
    } else {
        QLinearGradient gradient;
        if (gorient.startsWith("h", Qt::CaseInsensitive)) {
            gradient.setStart(r.x(), r.y());
            gradient.setFinalStop(r.x() + r.width(), r.y());
        } else { // Vertical
            gradient.setStart(r.x(), r.y());
            gradient.setFinalStop(r.x(), r.y() + r.height());
        }

        qreal step = 1.0 / (colors.size() - 1);
        for (int i = 0; i < colors.size(); i++) {
            gradient.setColorAt((qreal) i * step, colors[i]);
        }
        pen.setBrush(gradient);
    }

    p.setPen(pen);
}

static inline void fix_point(QPointF &point, QRectF &rect)
{
    if (point.x() < rect.x()) {
        point.setX(rect.x());
    } else if (point.x() > rect.x() + rect.width()) {
        point.setX(rect.x() + rect.width());
    }

    if (point.y() < rect.y()) {
        point.setY(rect.y());
    } else if (point.y() > rect.y() + rect.height()) {
        point.setY(rect.y() + rect.height());
    }
}

void paint_line_graph(QPainter &p, QRectF &rect, int points, float *values, double tension, int fill)
{
    double width = rect.width();
    double height = rect.height();
    double pixelsPerPoint = width / (double) (points - 1);

    // Calculate cubic control points
    QVector<QPointF> controlPoints((points - 1) * 2);
    int cpi = 0;
    // First control point is equal to first point
    controlPoints[cpi++] = QPointF(rect.x(), rect.y() + height - values[0] * height);

    // Calculate control points between data points
    // Based on ideas from:
    // http://scaledinnovation.com/analytics/splines/aboutSplines.html
    for (int i = 0; i < (points - 2); i++) {
        double x0 = rect.x() + (double) i * pixelsPerPoint;
        double x1 = rect.x() + (double) (i + 1) * pixelsPerPoint;
        double x2 = rect.x() + (double) (i + 2) * pixelsPerPoint;
        double y0 = rect.y() + height - values[i] * height;
        double y1 = rect.y() + height - values[i + 1] * height;
        double y2 = rect.y() + height - values[i + 2] * height;
        double d01 = sqrt(pow(x1 - x0, 2) + pow(y1 - y0, 2));
        double d12 = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
        double fa = tension * d01 / (d01 + d12);
        double fb = tension * d12 / (d01 + d12);
        double p1x = x1 - fa * (x2 - x0);
        double p1y = y1 - fa * (y2 - y0);
        QPointF c1(p1x, p1y);
        fix_point(c1, rect);
        double p2x = x1 + fb * (x2 - x0);
        double p2y = y1 + fb * (y2 - y0);
        QPointF c2(p2x, p2y);
        fix_point(c2, rect);
        controlPoints[cpi++] = c1;
        controlPoints[cpi++] = c2;
    }

    // Last control point is equal to last point
    controlPoints[cpi++] = QPointF(rect.x() + width,
                                   rect.y() + height - values[points - 1] * height);

    // Draw a sequence of bezier curves to interpolate between points.
    QPainterPath curvePath;

    // Begin at the first data point.
    curvePath.moveTo(rect.x(), rect.y() + height - values[0] * height);
    cpi = 0;
    for (int i = 1; i < points; i++) {
        QPointF c1 = controlPoints[cpi++];
        QPointF c2 = controlPoints[cpi++];
        double endX = rect.x() + (double) i * pixelsPerPoint;
        double endY = rect.y() + height - values[i] * height;
        QPointF end(endX, endY);
        curvePath.cubicTo(c1, c2, end);
    }

    if (fill) {
        curvePath.lineTo(rect.x() + width, rect.y() + height);
        curvePath.lineTo(rect.x(), rect.y() + height);
        curvePath.closeSubpath();
        p.fillPath(curvePath, p.pen().brush());
    } else {
        p.drawPath(curvePath);
    }
}

void paint_bar_graph(QPainter &p, QRectF &rect, int points, float *values)
{
    double width = rect.width();
    double height = rect.height();
    double pixelsPerPoint = width / (double) points;
    QPointF bottomPoint(rect.x() + pixelsPerPoint / 2, rect.y() + height);
    QPointF topPoint(rect.x() + pixelsPerPoint / 2, rect.y());

    for (int i = 0; i < points; i++) {
        topPoint.setY(rect.y() + height - values[i] * height);
        p.drawLine(bottomPoint, topPoint);
        bottomPoint.setX(bottomPoint.x() + pixelsPerPoint);
        topPoint.setX(bottomPoint.x());
    }
}

void paint_segment_graph(QPainter &p,
                         const QRectF &rect,
                         int points,
                         const float *values,
                         const QVector<QColor> &colors,
                         int segments,
                         int segment_gap,
                         int segment_width)
{
    double pixelsPerPoint = rect.width() / (double) points;
    if (segment_width > pixelsPerPoint) {
        segment_width = pixelsPerPoint;
    }
    double segment_space = pixelsPerPoint - segment_width;
    double segmentSize = rect.height() / (double) segments;
    if (segment_gap >= segmentSize) {
        segment_gap = segmentSize - 1;
    }
    segmentSize = (rect.height() - ((double) segment_gap * (double) (segments - 1)))
                  / (double) segments;
    for (int i = 0; i < points; i++) {
        QPointF bottomPoint(rect.x() + (segment_space / 2.0) + (pixelsPerPoint * (double) i),
                            rect.y() + rect.height());
        QPointF topPoint(bottomPoint.x() + segment_width, bottomPoint.y() - segmentSize);
        qreal segmentRatio = 1.0 / (qreal) segments;
        for (int s = 0; s < segments; s++) {
            int colorIndex = colors.size()
                             - qRound((qreal) s / (qreal) segments * (qreal) colors.size()) - 1;
            colorIndex = qBound(0, colorIndex, colors.size());
            QColor segmentColor = colors[colorIndex];
            qreal minSegmentValue = segmentRatio * (qreal) s;
            qreal maxSegmentValue = segmentRatio * (qreal) (s + 1);
            if (values[i] < minSegmentValue) {
                break;
            } else if (values[i] < maxSegmentValue) {
                qreal opacity = (values[i] - minSegmentValue) / segmentRatio;
                segmentColor.setAlphaF(opacity);
            }
            p.fillRect(QRectF(topPoint, bottomPoint), segmentColor);
            bottomPoint.setY(topPoint.y() - segment_gap);
            topPoint.setY(bottomPoint.y() - segmentSize);
        }
    }
}
