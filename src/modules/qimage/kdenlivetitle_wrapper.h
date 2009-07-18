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
class QGraphicsPolygonItem;
class QCoreApplication;
class QApplication;
class QObject;
class Title;
class QGraphicsView;
class QGraphicsScene;

class Title: public QObject {

    public:
            Title(const QString &);
            void drawKdenliveTitle(uint8_t*,int,int,double);
    private:
            QString m_filename;
            int  loadFromXml(QDomDocument doc, QGraphicsPolygonItem* /*startv*/, QGraphicsPolygonItem* /*endv*/);
            int loadDocument(const QString& url, QGraphicsPolygonItem* startv, QGraphicsPolygonItem* endv);
            QGraphicsView *view;
            QGraphicsScene *m_scene;
            QGraphicsPolygonItem *start,*end;
            QString colorToString(const QColor&);
            QString rectFToString(const QRectF&);
            QRectF stringToRect(const QString &);
            QColor stringToColor(const QString &);
            QTransform stringToTransform(const QString &);
};

