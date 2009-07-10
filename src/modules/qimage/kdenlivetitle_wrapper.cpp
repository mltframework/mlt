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
#include <QtCore/QDebug>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include "kdenlivetitle_wrapper.h"
#include <framework/mlt_producer.h>
extern "C" {
void init_qt (const char* c){
    titleclass=new Title(QString(c));
}
void refresh_kdenlivetitle( void* buffer, int width, int height , double position){
   titleclass->drawKdenliveTitle(buffer,width,height,position);
   int i=0;
   unsigned char* pointer;
   //rotate bytes for correct order in mlt
   for (i=0;i<width*height*4;i+=4){
        pointer=(unsigned char*)buffer+i;
        pointer[0]=pointer[1];
        pointer[1]=pointer[2];
        pointer[2]=pointer[3];
        pointer[3]=pointer[0];
   }
}
}
Title::Title(const QString& filename){
    int argc=0;
    char* argv[1];
    argv[0]=0; 
    app=new QCoreApplication(argc,argv);
    //scene=new QGraphicsScene;
    //view=new QGraphicsView(scene);
    //view->show();
}
void Title::drawKdenliveTitle(void * buffer ,int width,int height,double position){
    //qDebug() << "ja" << width << height << buffer << position << endl;
    QImage img((uchar*)buffer,width,height,width*4,QImage::Format_ARGB32);
    img.fill(0);
    //scene->addText("hello");
    QPainter p;
    p.begin(&img);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
    p.setFont(QFont("Arial",60));
    p.setPen(QPen(QColor(255,255,255)));
    p.drawText(width*.2+width*20*position,height/2,"test");
    p.end();

    //QPainter p1;
    //p1.begin(&img);
    // scene->render(&p1);
    //p1.end();
}
