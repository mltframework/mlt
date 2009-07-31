/*
 * kdenlivetitle_wrapper.h -- kdenlivetitle wrapper
 * Copyright (c) 2009 Marco Gittler <g.marco@freenet.de>
 * Copyright (c) 2009 Jean-Baptiste Mardelle <jb@kdenlive.org>
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

#include <QtCore/QString>
#include <framework/mlt_frame.h>
#include <QtXml/QDomElement>
#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtGui/QWidget>

class QApplication;
class QGraphicsScene;
class QTransform;

void drawKdenliveTitle( mlt_producer producer, uint8_t*, int, int, double, int );
void loadFromXml( mlt_producer producer, QGraphicsScene *scene, const char *templateXml, const char *templateText,int width, int height );

QString rectTransform( QString s, QTransform t );
QString colorToString( const QColor& );
QString rectFToString( const QRectF& );
QRectF stringToRect( const QString & );
QColor stringToColor( const QString & );
QTransform stringToTransform( const QString & );


