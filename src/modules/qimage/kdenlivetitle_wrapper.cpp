/*
 * kdenlivetitle_wrapper.cpp -- kdenlivetitle wrapper
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

#include "kdenlivetitle_wrapper.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtCore/QMutex>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsTextItem>
#include <QtSvg/QGraphicsSvgItem>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QStyleOptionGraphicsItem>

#include <QtCore/QString>

#include <QtXml/QDomElement>
#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtGui/QWidget>
#include <framework/mlt_log.h>

#if QT_VERSION >= 0x040600
#include <QtGui/QGraphicsEffect>
#include <QtGui/QGraphicsBlurEffect>
#include <QtGui/QGraphicsDropShadowEffect>
#endif

static QApplication *app = NULL;

class ImageItem: public QGraphicsItem
{
public:
    ImageItem(QImage img)
    {
	m_img = img;
    }
    QImage m_img;


protected:

virtual QRectF boundingRect() const
{
    return QRectF(0, 0, m_img.width(), m_img.height());
}

virtual void paint( QPainter *painter,
                       const QStyleOptionGraphicsItem * /*option*/,
                       QWidget* )
{ 
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(QPoint(), m_img);
}
};


QString rectTransform( QString s, QTransform t )
{
	QStringList l = s.split( ',' );
	return QString::number(l.at(0).toDouble() * t.m11()) + ',' + QString::number(l.at(1).toDouble() * t.m22()) + ',' + QString::number(l.at(2).toDouble() * t.m11()) + ',' + QString::number(l.at(3).toDouble() * t.m22());
}

QString colorToString( const QColor& c )
{
	QString ret = "%1,%2,%3,%4";
	ret = ret.arg( c.red() ).arg( c.green() ).arg( c.blue() ).arg( c.alpha() );
	return ret;
}

QString rectFToString( const QRectF& c )
{
	QString ret = "%1,%2,%3,%4";
	ret = ret.arg( c.top() ).arg( c.left() ).arg( c.width() ).arg( c.height() );
	return ret;
}

QRectF stringToRect( const QString & s )
{

	QStringList l = s.split( ',' );
	if ( l.size() < 4 )
		return QRectF();
	return QRectF( l.at( 0 ).toDouble(), l.at( 1 ).toDouble(), l.at( 2 ).toDouble(), l.at( 3 ).toDouble() ).normalized();
}

QColor stringToColor( const QString & s )
{
	QStringList l = s.split( ',' );
	if ( l.size() < 4 )
		return QColor();
	return QColor( l.at( 0 ).toInt(), l.at( 1 ).toInt(), l.at( 2 ).toInt(), l.at( 3 ).toInt() );
	;
}
QTransform stringToTransform( const QString& s )
{
	QStringList l = s.split( ',' );
	if ( l.size() < 9 )
		return QTransform();
	return QTransform(
	           l.at( 0 ).toDouble(), l.at( 1 ).toDouble(), l.at( 2 ).toDouble(),
	           l.at( 3 ).toDouble(), l.at( 4 ).toDouble(), l.at( 5 ).toDouble(),
	           l.at( 6 ).toDouble(), l.at( 7 ).toDouble(), l.at( 8 ).toDouble()
	       );
}

static void qscene_delete( void *data )
{
	QGraphicsScene *scene = ( QGraphicsScene * )data;
	if (scene) delete scene;
	scene = NULL;
}


void loadFromXml( mlt_producer producer, QGraphicsScene *scene, const char *templateXml, const char *templateText )
{
	scene->clear();
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
	QDomDocument doc;
	QString data = QString::fromUtf8(templateXml);
	QString replacementText = QString::fromUtf8(templateText);
	doc.setContent(data);
	QDomElement title = doc.documentElement();
	
	// Check for invalid title
	if ( title.isNull() || title.tagName() != "kdenlivetitle" ) return;
	
        int originalWidth;
        int originalHeight;
	if ( title.hasAttribute("width") ) {
            originalWidth = title.attribute("width").toInt();
            originalHeight = title.attribute("height").toInt();
            scene->setSceneRect(0, 0, originalWidth, originalHeight);
        }
        else {
            originalWidth = scene->sceneRect().width();
            originalHeight = scene->sceneRect().height();
        }
        if ( title.hasAttribute( "out" ) ) {
            mlt_properties_set_position( producer_props, "_animation_out", title.attribute( "out" ).toDouble() );
        }
        else {
            mlt_properties_set_position( producer_props, "_animation_out", mlt_producer_get_out( producer ) );
        }
        
	mlt_properties_set_int( producer_props, "_original_width", originalWidth );
	mlt_properties_set_int( producer_props, "_original_height", originalHeight );

	QDomNodeList items = title.elementsByTagName("item");
        for ( int i = 0; i < items.count(); i++ )
	{
		QGraphicsItem *gitem = NULL;
		int zValue = items.item( i ).attributes().namedItem( "z-index" ).nodeValue().toInt();
		if ( zValue > -1000 )
		{
			if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsTextItem" )
			{
				QDomNamedNodeMap txtProperties = items.item( i ).namedItem( "content" ).attributes();
				QFont font( txtProperties.namedItem( "font" ).nodeValue() );
					QDomNode node = txtProperties.namedItem( "font-bold" );
				if ( !node.isNull() )
				{
				// Old: Bold/Not bold.
					font.setBold( node.nodeValue().toInt() );
				}
				else
				{
					// New: Font weight (QFont::)
					font.setWeight( txtProperties.namedItem( "font-weight" ).nodeValue().toInt() );
				}
					font.setItalic( txtProperties.namedItem( "font-italic" ).nodeValue().toInt() );
				font.setUnderline( txtProperties.namedItem( "font-underline" ).nodeValue().toInt() );
				// Older Kdenlive version did not store pixel size but point size
				if ( txtProperties.namedItem( "font-pixel-size" ).isNull() )
				{
					QFont f2;
					f2.setPointSize( txtProperties.namedItem( "font-size" ).nodeValue().toInt() );
					font.setPixelSize( QFontInfo( f2 ).pixelSize() );
				}
				else
					font.setPixelSize( txtProperties.namedItem( "font-pixel-size" ).nodeValue().toInt() );
				QColor col( stringToColor( txtProperties.namedItem( "font-color" ).nodeValue() ) );
				QString text = items.item( i ).namedItem( "content" ).firstChild().nodeValue();
				if ( !replacementText.isEmpty() )
				{
					text = text.replace( "%s", replacementText );
				}
				QGraphicsTextItem *txt = scene->addText(text, font);
				txt->setDefaultTextColor( col );
				
				// Effects
				if (!txtProperties.namedItem( "typewriter" ).isNull()) {
					// typewriter effect
					mlt_properties_set_int( producer_props, "_animated", 1 );
					QStringList effetData = QStringList() << "typewriter" << text << txtProperties.namedItem( "typewriter" ).nodeValue();
					txt->setData(0, effetData);
				}
				
				if ( txtProperties.namedItem( "alignment" ).isNull() == false )
				{
					txt->setTextWidth( txt->boundingRect().width() );
					QTextOption opt = txt->document()->defaultTextOption ();
					opt.setAlignment(( Qt::Alignment ) txtProperties.namedItem( "alignment" ).nodeValue().toInt() );
					txt->document()->setDefaultTextOption (opt);
				}
					if ( !txtProperties.namedItem( "kdenlive-axis-x-inverted" ).isNull() )
				{
					//txt->setData(OriginXLeft, txtProperties.namedItem("kdenlive-axis-x-inverted").nodeValue().toInt());
				}
				if ( !txtProperties.namedItem( "kdenlive-axis-y-inverted" ).isNull() )
				{
					//txt->setData(OriginYTop, txtProperties.namedItem("kdenlive-axis-y-inverted").nodeValue().toInt());
				}
					gitem = txt;
			}
			else if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsRectItem" )
			{
				QString rect = items.item( i ).namedItem( "content" ).attributes().namedItem( "rect" ).nodeValue();
				QString br_str = items.item( i ).namedItem( "content" ).attributes().namedItem( "brushcolor" ).nodeValue();
				QString pen_str = items.item( i ).namedItem( "content" ).attributes().namedItem( "pencolor" ).nodeValue();
				double penwidth = items.item( i ).namedItem( "content" ).attributes().namedItem( "penwidth") .nodeValue().toDouble();
				QGraphicsRectItem *rec = scene->addRect( stringToRect( rect ), QPen( QBrush( stringToColor( pen_str ) ), penwidth ), QBrush( stringToColor( br_str ) ) );
				gitem = rec;
			}
			else if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsPixmapItem" )
			{
				const QString url = items.item( i ).namedItem( "content" ).attributes().namedItem( "url" ).nodeValue();
				QImage img( url );
				ImageItem *rec = new ImageItem(img);
				scene->addItem( rec );
				gitem = rec;
			}
			else if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsSvgItem" )
			{
				const QString url = items.item( i ).namedItem( "content" ).attributes().namedItem( "url" ).nodeValue();
				QGraphicsSvgItem *rec = new QGraphicsSvgItem(url);
				scene->addItem(rec);
				gitem = rec;
			}
		}
		//pos and transform
		if ( gitem )
		{
			QPointF p( items.item( i ).namedItem( "position" ).attributes().namedItem( "x" ).nodeValue().toDouble(),
			           items.item( i ).namedItem( "position" ).attributes().namedItem( "y" ).nodeValue().toDouble() );
			gitem->setPos( p );
			gitem->setTransform( stringToTransform( items.item( i ).namedItem( "position" ).firstChild().firstChild().nodeValue() ) );
			int zValue = items.item( i ).attributes().namedItem( "z-index" ).nodeValue().toInt();
			gitem->setZValue( zValue );

#if QT_VERSION >= 0x040600
			// effects
			QDomNode eff = items.item(i).namedItem("effect");
			if (!eff.isNull()) {
				QDomElement e = eff.toElement();
				if (e.attribute("type") == "blur") {
					QGraphicsBlurEffect *blur = new QGraphicsBlurEffect();
					blur->setBlurRadius(e.attribute("blurradius").toInt());
					gitem->setGraphicsEffect(blur);
				}
				else if (e.attribute("type") == "shadow") {
					QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
					shadow->setBlurRadius(e.attribute("blurradius").toInt());
					shadow->setOffset(e.attribute("xoffset").toInt(), e.attribute("yoffset").toInt());
					gitem->setGraphicsEffect(shadow);
				}
			}
#endif
		}
	}

	QDomNode n = title.firstChildElement("background");
	if (!n.isNull()) {
		QColor color = QColor( stringToColor( n.attributes().namedItem( "color" ).nodeValue() ) );
                if (color.alpha() > 0) {
                        QGraphicsRectItem *rec = scene->addRect(0, 0, scene->width(), scene->height() , QPen( Qt::NoPen ), QBrush( color ) );
                        rec->setZValue(-1100);
                }
	  
	}

	QString startRect;
	n = title.firstChildElement( "startviewport" );
        // Check if node exists, if it has an x attribute, it is an old version title, don't use viewport
	if (!n.isNull() && !n.toElement().hasAttribute("x"))
	{
		startRect = n.attributes().namedItem( "rect" ).nodeValue();
	}
	n = title.firstChildElement( "endviewport" );
        // Check if node exists, if it has an x attribute, it is an old version title, don't use viewport
	if (!n.isNull() && !n.toElement().hasAttribute("x"))
	{
		QString rect = n.attributes().namedItem( "rect" ).nodeValue();
		if (startRect != rect)
			mlt_properties_set( producer_props, "_endrect", rect.toUtf8().data() );
	}
	if (!startRect.isEmpty()) {
	  	mlt_properties_set( producer_props, "_startrect", startRect.toUtf8().data() );
	}
	return;
}


void drawKdenliveTitle( producer_ktitle self, mlt_frame frame, int width, int height, double position, int force_refresh )
{
  	// Obtain the producer 
	mlt_producer producer = &self->parent;
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
        
        pthread_mutex_lock( &self->mutex );
	
	// Check if user wants us to reload the image
	if ( mlt_properties_get( producer_props, "_animated" ) != NULL || force_refresh == 1 || width != self->current_width || height != self->current_height || mlt_properties_get( producer_props, "_endrect" ) != NULL )
	{
		//mlt_cache_item_close( self->image_cache );
		self->current_image = NULL;
		mlt_properties_set_data( producer_props, "cached_image", NULL, 0, NULL, NULL );
		mlt_properties_set_int( producer_props, "force_reload", 0 );
	}
	
	if (self->current_image == NULL) {
		// restore QGraphicsScene
		QGraphicsScene *scene = static_cast<QGraphicsScene *> (mlt_properties_get_data( producer_props, "qscene", NULL ));

		if ( force_refresh == 1 && scene )
		{
			scene = NULL;
			mlt_properties_set_data( producer_props, "qscene", NULL, 0, NULL, NULL );
		}

		if ( scene == NULL )
		{
			int argc = 1;
			char* argv[1];
			argv[0] = (char*) "xxx";
			
			// Warning: all Qt graphic objects (QRect, ...) must be initialized AFTER 
			// the QApplication is created, otherwise their will be NULL
			
			if ( app == NULL ) {
				if ( qApp ) {
					app = qApp;
				}
				else {
#ifdef linux
					if ( getenv("DISPLAY") == 0 )
					{
						mlt_log_panic( MLT_PRODUCER_SERVICE( producer ), "Error, cannot render titles without an X11 environment.\nPlease either run melt from an X session or use a fake X server like xvfb:\nxvfb-run -a melt (...)\n" );
						pthread_mutex_unlock( &self->mutex );
						exit(1);
						return;
					}
#endif
					app = new QApplication( argc, argv );
					//fix to let the decimal point for every locale be: "."
					setlocale(LC_NUMERIC,"POSIX");
				}
			}
			scene = new QGraphicsScene();
			scene->setItemIndexMethod( QGraphicsScene::NoIndex );
                        scene->setSceneRect(0, 0, mlt_properties_get_int( properties, "width" ), mlt_properties_get_int( properties, "height" ));
			loadFromXml( producer, scene, mlt_properties_get( producer_props, "xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
			mlt_properties_set_data( producer_props, "qscene", scene, 0, ( mlt_destructor )qscene_delete, NULL );
		}
                
                QRectF start = stringToRect( QString( mlt_properties_get( producer_props, "_startrect" ) ) );
                QRectF end = stringToRect( QString( mlt_properties_get( producer_props, "_endrect" ) ) );
	
		int originalWidth = mlt_properties_get_int( producer_props, "_original_width" );
		int originalHeight= mlt_properties_get_int( producer_props, "_original_height" );
		const QRectF source( 0, 0, width, height );
		if (start.isNull()) {
		    start = QRectF( 0, 0, originalWidth, originalHeight );
		}

		// Effects
		QList <QGraphicsItem *> items = scene->items();
		QGraphicsTextItem *titem = NULL;
		for (int i = 0; i < items.count(); i++) {
		    titem = static_cast <QGraphicsTextItem*> ( items.at( i ) );
		    if (titem && !titem->data( 0 ).isNull()) {
			    QStringList params = titem->data( 0 ).toStringList();
			    if (params.at( 0 ) == "typewriter" ) {
				    // typewriter effect has 2 param values:
				    // the keystroke delay and a start offset, both in frames
				    QStringList values = params.at( 2 ).split( ";" );
				    int interval = qMax( 0, ( ( int ) position - values.at( 1 ).toInt()) / values.at( 0 ).toInt() );
				    QTextDocument *td = new QTextDocument( params.at( 1 ).left( interval ) );
				    td->setDefaultFont( titem->font() );
				    td->setDefaultTextOption( titem->document()->defaultTextOption() );
				    td->setTextWidth( titem->document()->textWidth() );
				    titem->setDocument( td );
			    }
		    }
		}

		//must be extracted from kdenlive title
		QImage img( width, height, QImage::Format_ARGB32 );
		img.fill( 0 );
		QPainter p1;
		p1.begin( &img );
		p1.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );
		//| QPainter::SmoothPixmapTransform );
                mlt_position anim_out = mlt_properties_get_position( producer_props, "_animation_out" );

		if (end.isNull())
		{
			scene->render( &p1, source, start, Qt::IgnoreAspectRatio );
		}
		else if ( position > anim_out ) {
                        scene->render( &p1, source, end, Qt::IgnoreAspectRatio );
                }
		else {
                        double percentage = position / anim_out;
			QPointF topleft = start.topLeft() + ( end.topLeft() - start.topLeft() ) * percentage;
			QPointF bottomRight = start.bottomRight() + ( end.bottomRight() - start.bottomRight() ) * percentage;
			const QRectF r1( topleft, bottomRight );
			scene->render( &p1, source, r1, Qt::IgnoreAspectRatio );
		}
		p1.end();

		int size = width * height * 4;
		uint8_t *pointer=img.bits();
		QRgb* src = ( QRgb* ) pointer;
		self->current_image = ( uint8_t * )mlt_pool_alloc( size );
		uint8_t *dst = self->current_image;
	
		for ( int i = 0; i < width * height * 4; i += 4 )
		{
			*dst++=qRed( *src );
			*dst++=qGreen( *src );
			*dst++=qBlue( *src );
			*dst++=qAlpha( *src );
			src++;
		}

		mlt_properties_set_data( producer_props, "cached_image", self->current_image, size, mlt_pool_release, NULL );
		self->current_width = width;
		self->current_height = height;
	}

	pthread_mutex_unlock( &self->mutex );
	mlt_properties_set_int( properties, "width", self->current_width );
	mlt_properties_set_int( properties, "height", self->current_height );
}


