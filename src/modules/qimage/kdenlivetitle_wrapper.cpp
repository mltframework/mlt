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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "kdenlivetitle_wrapper.h"
#include <framework/mlt_producer.h>
extern "C" {
void init_qt (const char* c){
    titleclass=new Title(QString(c));
}
void refresh_kdenlivetitle( void* buffer, int width, int height , double position){
   titleclass->drawKdenliveTitle(buffer,width,height,position);
}
}
Title::Title(const QString& filename){
    int argc=0;
    char* argv[1];
    argv[0]=0; 
    app=new QCoreApplication(argc,argv);
}
void Title::drawKdenliveTitle(void * buffer ,int width,int height,double position){
    //qDebug() << "ja" << width << height << buffer << position << endl;
    QImage img((uchar*)buffer,width,height,width*4,QImage::Format_ARGB32);
    img.fill(0x22);
    QPainter p;
    p.begin(&img);
    p.setFont(QFont("Arial",40));
    p.setPen(QPen(QColor(255,255,255)));
    p.drawText(width*.2+width*20*position,height/2,"test");
    p.end();
}
