/*
 * Copyright (c) 2015-2021 Meltytech, LLC
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

#ifndef GRAPH_H
#define GRAPH_H

#include <framework/mlt.h>
#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QVector>

QVector<QColor> get_graph_colors( mlt_properties filter_properties );
void setup_graph_painter( QPainter& p, QRectF& rect, mlt_properties filter_properties );
void setup_graph_pen( QPainter& p, QRectF& rect, mlt_properties filter_properties, double scale );
void paint_line_graph( QPainter& p, QRectF& rect, int points, float* values, double tension, int fill );
void paint_bar_graph( QPainter& p, QRectF& rect, int points, float* values );
void paint_segment_graph( QPainter& p, const QRectF& rect, int points, const float* values, double gap, const QVector<QColor>& colors, int segment_width );


#endif // GRAPH_H
