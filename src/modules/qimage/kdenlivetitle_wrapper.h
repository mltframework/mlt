/*
 * kdenlivetitle_wrapper.h -- kdenlivetitle wrapper
 * Copyright (c) 2009 Marco Gittler <g.marco@freenet.de>
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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <framework/mlt_frame.h>
#include <QtXml/QDomElement>
#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtGui/QWidget>

class QApplication;
class QGraphicsScene;
class QTransform;

class Title: public QObject
{

public:
	Title( const char *filename);
	virtual ~Title();
	void drawKdenliveTitle( uint8_t*, int, int, double, const char*, const char* );
	void loadFromXml( const char *templateXml, const QString templateText );

private:
	QString m_filename;
	QGraphicsScene *m_scene;
	QRectF m_start, m_end;
	QString colorToString( const QColor& );
	QString rectFToString( const QRectF& );
	QRectF stringToRect( const QString & );
	QColor stringToColor( const QString & );
	QTransform stringToTransform( const QString & );
};

