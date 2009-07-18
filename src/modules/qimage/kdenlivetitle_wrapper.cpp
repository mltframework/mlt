/*
 * kdenlivetitle_wrapper.cpp -- kdenlivetitle wrapper
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

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsTextItem>
#include <QtGui/QGraphicsPolygonItem>
#include <QtGui/QTextCursor>
#include "kdenlivetitle_wrapper.h"
#include <framework/mlt_producer.h>

static Title* titleclass;
static QApplication *app;

extern "C" {
    void init_qt (const char* c){
        titleclass=new Title(QString(c));
    }
    void refresh_kdenlivetitle( uint8_t* buffer, int width, int height , double position){
        titleclass->drawKdenliveTitle(buffer,width,height,position);
    }
}
Title::Title(const QString& filename):m_filename(filename),m_scene(NULL){
    //must be extracted from kdenlive title
    start =new QGraphicsPolygonItem(QPolygonF(QRectF(100, 100, 600, 600)));;
    end=new QGraphicsPolygonItem(QPolygonF(QRectF(0, 0, 300, 300)));;
}
void Title::drawKdenliveTitle(uint8_t * buffer ,int width,int height,double position){
    if (!m_scene){
        int argc=0;
        char* argv[1];
        argv[0]="xxx"; 
        if (! QApplication::activeWindow())
        //if (!app)
            app=new QApplication(argc,argv);
        m_scene=new QGraphicsScene;
        loadDocument(m_filename,start,end);
    }
    //must be extracted from kdenlive title
    
    QImage *img=new QImage(width,height,QImage::Format_ARGB32);
    QRectF rstart=start->boundingRect();
    QRectF rend=end->boundingRect();
    QPointF topleft=rstart.topLeft()+(rend.topLeft()-rstart.topLeft())*position;
    QPointF bottomRight=rstart.bottomRight()+(rend.bottomRight()-rstart.bottomRight())*position;
    QPainter p1;
    p1.begin(img);
    p1.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);//|QPainter::SmoothPixmapTransform );
    m_scene->render(&p1,QRect(0,0,width,height),QRectF(topleft,bottomRight));
    p1.end();
    uint8_t *pointer=img->bits();
    QRgb* src = (QRgb*) pointer;
    for (int i=0;i<width*height*4;i+=4){
        *buffer++=qRed(*src);
        *buffer++=qGreen(*src);
        *buffer++=qBlue(*src);
        *buffer++=qAlpha(*src);
        src++;
    }
    delete img;
}
int Title::loadDocument(const QString& url, QGraphicsPolygonItem* startv, QGraphicsPolygonItem* endv)
{
    QDomDocument doc;
    if (!m_scene)
        return -1;

        QFile file(url);
        if (file.open(QIODevice::ReadOnly)) {
            doc.setContent(&file, false);
            file.close();
        } 
        return loadFromXml(doc, startv, endv);
}
int Title::loadFromXml(QDomDocument doc, QGraphicsPolygonItem* startv, QGraphicsPolygonItem* endv)
{
    QDomNodeList titles = doc.elementsByTagName("kdenlivetitle");
    int maxZValue = 0;
    if (titles.size()) {

        QDomNodeList items = titles.item(0).childNodes();
        for (int i = 0; i < items.count(); i++) {
            QGraphicsItem *gitem = NULL;
            int zValue = items.item(i).attributes().namedItem("z-index").nodeValue().toInt();
            if (zValue > -1000) {
                if (items.item(i).attributes().namedItem("type").nodeValue() == "QGraphicsTextItem") {
                    QDomNamedNodeMap txtProperties = items.item(i).namedItem("content").attributes();
                    QFont font(txtProperties.namedItem("font").nodeValue());
                    font.setBold(txtProperties.namedItem("font-bold").nodeValue().toInt());
                    font.setItalic(txtProperties.namedItem("font-italic").nodeValue().toInt());
                    font.setUnderline(txtProperties.namedItem("font-underline").nodeValue().toInt());
                    // Older Kdenlive version did not store pixel size but point size
                    if (txtProperties.namedItem("font-pixel-size").isNull()) {
                        QFont f2;
                        f2.setPointSize(txtProperties.namedItem("font-size").nodeValue().toInt());
                        font.setPixelSize(QFontInfo(f2).pixelSize());
                    } else
                        font.setPixelSize(txtProperties.namedItem("font-pixel-size").nodeValue().toInt());
                    QColor col(stringToColor(txtProperties.namedItem("font-color").nodeValue()));
                    QGraphicsTextItem *txt = m_scene->addText(items.item(i).namedItem("content").firstChild().nodeValue(), font);
                    txt->setDefaultTextColor(col);
                    txt->setTextInteractionFlags(Qt::NoTextInteraction);
                    if (txtProperties.namedItem("alignment").isNull() == false) {
                        txt->setTextWidth(txt->boundingRect().width());
                        QTextCursor cur = txt->textCursor();
                        QTextBlockFormat format = cur.blockFormat();
                        format.setAlignment((Qt::Alignment) txtProperties.namedItem("alignment").nodeValue().toInt());
                        cur.select(QTextCursor::Document);
                        cur.setBlockFormat(format);
                        txt->setTextCursor(cur);
                        cur.clearSelection();
                        txt->setTextCursor(cur);
                    }

                    if (!txtProperties.namedItem("kdenlive-axis-x-inverted").isNull()) {
                        //txt->setData(OriginXLeft, txtProperties.namedItem("kdenlive-axis-x-inverted").nodeValue().toInt());
                    }
                    if (!txtProperties.namedItem("kdenlive-axis-y-inverted").isNull()) {
                        //txt->setData(OriginYTop, txtProperties.namedItem("kdenlive-axis-y-inverted").nodeValue().toInt());
                    }

                    gitem = txt;
                } else if (items.item(i).attributes().namedItem("type").nodeValue() == "QGraphicsRectItem") {
                    QString rect = items.item(i).namedItem("content").attributes().namedItem("rect").nodeValue();
                    QString br_str = items.item(i).namedItem("content").attributes().namedItem("brushcolor").nodeValue();
                    QString pen_str = items.item(i).namedItem("content").attributes().namedItem("pencolor").nodeValue();
                    double penwidth = items.item(i).namedItem("content").attributes().namedItem("penwidth").nodeValue().toDouble();
                    QGraphicsRectItem *rec = m_scene->addRect(stringToRect(rect), QPen(QBrush(stringToColor(pen_str)), penwidth), QBrush(stringToColor(br_str)));
                    gitem = rec;
                } else if (items.item(i).attributes().namedItem("type").nodeValue() == "QGraphicsPixmapItem") {
                    QString url = items.item(i).namedItem("content").attributes().namedItem("url").nodeValue();
                    QPixmap pix(url);
                    QGraphicsPixmapItem *rec = m_scene->addPixmap(pix);
                    rec->setData(Qt::UserRole, url);
                    gitem = rec;
                } else if (items.item(i).attributes().namedItem("type").nodeValue() == "QGraphicsSvgItem") {
                    QString url = items.item(i).namedItem("content").attributes().namedItem("url").nodeValue();
                    //QGraphicsSvgItem *rec = new QGraphicsSvgItem(url);
                    //m_scene->addItem(rec);
                    //rec->setData(Qt::UserRole, url);
                    //gitem = rec;
                }
            }
            //pos and transform
            if (gitem) {
                QPointF p(items.item(i).namedItem("position").attributes().namedItem("x").nodeValue().toDouble(),
                          items.item(i).namedItem("position").attributes().namedItem("y").nodeValue().toDouble());
                gitem->setPos(p);
                gitem->setTransform(stringToTransform(items.item(i).namedItem("position").firstChild().firstChild().nodeValue()));
                int zValue = items.item(i).attributes().namedItem("z-index").nodeValue().toInt();
                if (zValue > maxZValue) maxZValue = zValue;
                gitem->setZValue(zValue);
                //gitem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            }
            if (items.item(i).nodeName() == "background") {
                QColor color = QColor(stringToColor(items.item(i).attributes().namedItem("color").nodeValue()));
                //color.setAlpha(items.item(i).attributes().namedItem("alpha").nodeValue().toInt());
                QList<QGraphicsItem *> items = m_scene->items();
                for (int i = 0; i < items.size(); i++) {
                    if (items.at(i)->zValue() == -1100) {
                        ((QGraphicsRectItem *)items.at(i))->setBrush(QBrush(color));
                        break;
                    }
                }
            } else if (items.item(i).nodeName() == "startviewport" && startv) {
		    QString rect = items.item(i).attributes().namedItem("rect").nodeValue();
                    startv->setPolygon(stringToRect(rect));
                } else if (items.item(i).nodeName() == "endviewport" && endv) {
		    QString rect = items.item(i).attributes().namedItem("rect").nodeValue();
                    endv->setPolygon(stringToRect(rect));
                }
        }
    }
    return maxZValue;
}

QString Title::colorToString(const QColor& c)
{
    QString ret = "%1,%2,%3,%4";
    ret = ret.arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
    return ret;
}

QString Title::rectFToString(const QRectF& c)
{
    QString ret = "%1,%2,%3,%4";
    ret = ret.arg(c.top()).arg(c.left()).arg(c.width()).arg(c.height());
    return ret;
}

QRectF Title::stringToRect(const QString & s)
{

    QStringList l = s.split(',');
    if (l.size() < 4)
        return QRectF();
    return QRectF(l.at(0).toDouble(), l.at(1).toDouble(), l.at(2).toDouble(), l.at(3).toDouble()).normalized();
}

QColor Title::stringToColor(const QString & s)
{
    QStringList l = s.split(',');
    if (l.size() < 4)
        return QColor();
    return QColor(l.at(0).toInt(), l.at(1).toInt(), l.at(2).toInt(), l.at(3).toInt());;
}
QTransform Title::stringToTransform(const QString& s)
{
    QStringList l = s.split(',');
    if (l.size() < 9)
        return QTransform();
    return QTransform(
               l.at(0).toDouble(), l.at(1).toDouble(), l.at(2).toDouble(),
               l.at(3).toDouble(), l.at(4).toDouble(), l.at(5).toDouble(),
               l.at(6).toDouble(), l.at(7).toDouble(), l.at(8).toDouble()
           );
}
