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
#include "common.h"

#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QMutex>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QTextCursor>
#include <QTextDocument>
#include <QStyleOptionGraphicsItem>

#include <QString>

#include <QDomElement>
#include <QRectF>
#include <QColor>
#include <QWidget>
#include <framework/mlt_log.h>

#if QT_VERSION >= 0x040600
#include <QGraphicsEffect>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>
#endif

Q_DECLARE_METATYPE(QTextCursor);

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
	
	// Check title locale
	if ( title.hasAttribute( "LC_NUMERIC" ) ) {
	    QString locale = title.attribute( "LC_NUMERIC" );
	    QLocale::setDefault( locale );
	}
	
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
	mlt_properties_set_int( producer_props, "meta.media.width", originalWidth );
	mlt_properties_set_int( producer_props, "meta.media.height", originalHeight );

	QDomNode node;
	QDomNodeList items = title.elementsByTagName("item");
        for ( int i = 0; i < items.count(); i++ )
	{
		QGraphicsItem *gitem = NULL;
		node = items.item( i );
		QDomNamedNodeMap nodeAttributes = node.attributes();
		int zValue = nodeAttributes.namedItem( "z-index" ).nodeValue().toInt();
		if ( zValue > -1000 )
		{
			if ( nodeAttributes.namedItem( "type" ).nodeValue() == "QGraphicsTextItem" )
			{
				QDomNamedNodeMap txtProperties = node.namedItem( "content" ).attributes();
				QFont font( txtProperties.namedItem( "font" ).nodeValue() );
				QDomNode propsNode = txtProperties.namedItem( "font-bold" );
				if ( !propsNode.isNull() )
				{
					// Old: Bold/Not bold.
					font.setBold( propsNode.nodeValue().toInt() );
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
				QString text = node.namedItem( "content" ).firstChild().nodeValue();
				if ( !replacementText.isEmpty() )
				{
					text = text.replace( "%s", replacementText );
				}
				QGraphicsTextItem *txt = scene->addText(text, font);
				if (txtProperties.namedItem("font-outline").nodeValue().toDouble()>0.0){
					QTextDocument *doc = txt->document();
					// Make sure some that the text item does not request refresh by itself
					doc->blockSignals(true);
					QTextCursor cursor(doc);
					cursor.select(QTextCursor::Document);
					QTextCharFormat format;
					format.setTextOutline(
							QPen(QColor( stringToColor( txtProperties.namedItem( "font-outline-color" ).nodeValue() ) ),
							txtProperties.namedItem("font-outline").nodeValue().toDouble(),
							Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin)
					);
					format.setForeground(QBrush(col));

					cursor.mergeCharFormat(format);
				} else {
					txt->setDefaultTextColor( col );
				}
				
				// Effects
				if (!txtProperties.namedItem( "typewriter" ).isNull()) {
					// typewriter effect
					mlt_properties_set_int( producer_props, "_animated", 1 );
					QStringList effetData = QStringList() << "typewriter" << text << txtProperties.namedItem( "typewriter" ).nodeValue();
					txt->setData(0, effetData);
					if ( !txtProperties.namedItem( "textwidth" ).isNull() )
						txt->setData( 1, txtProperties.namedItem( "textwidth" ).nodeValue() );
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
				if ( !txtProperties.namedItem("preferred-width").isNull() )
				{
					txt->setTextWidth( txtProperties.namedItem("preferred-width").nodeValue().toInt() );
				}
					gitem = txt;
			}
			else if ( nodeAttributes.namedItem( "type" ).nodeValue() == "QGraphicsRectItem" )
			{
				QString rect = node.namedItem( "content" ).attributes().namedItem( "rect" ).nodeValue();
				QString br_str = node.namedItem( "content" ).attributes().namedItem( "brushcolor" ).nodeValue();
				QString pen_str = node.namedItem( "content" ).attributes().namedItem( "pencolor" ).nodeValue();
				double penwidth = node.namedItem( "content" ).attributes().namedItem( "penwidth") .nodeValue().toDouble();
				QGraphicsRectItem *rec = scene->addRect( stringToRect( rect ), QPen( QBrush( stringToColor( pen_str ) ), penwidth, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin ), QBrush( stringToColor( br_str ) ) );
				gitem = rec;
			}
			else if ( nodeAttributes.namedItem( "type" ).nodeValue() == "QGraphicsPixmapItem" )
			{
				const QString url = node.namedItem( "content" ).attributes().namedItem( "url" ).nodeValue();
				const QString base64 = items.item(i).namedItem("content").attributes().namedItem("base64").nodeValue();
				QImage img;
				if (base64.isEmpty()){
					img.load(url);
				}else{
					img.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
				}
				ImageItem *rec = new ImageItem(img);
				scene->addItem( rec );
				gitem = rec;
			}
			else if ( nodeAttributes.namedItem( "type" ).nodeValue() == "QGraphicsSvgItem" )
			{
				QString url = items.item(i).namedItem("content").attributes().namedItem("url").nodeValue();
				QString base64 = items.item(i).namedItem("content").attributes().namedItem("base64").nodeValue();
				QGraphicsSvgItem *rec = NULL;
				if (base64.isEmpty()){
					rec = new QGraphicsSvgItem(url);
				}else{
					rec = new QGraphicsSvgItem();
					QSvgRenderer *renderer= new QSvgRenderer(QByteArray::fromBase64(base64.toLatin1()), rec );
					rec->setSharedRenderer(renderer);
				}
				if (rec){
					scene->addItem(rec);
					gitem = rec;
				}
			}
		}
		//pos and transform
		if ( gitem )
		{
			QPointF p( node.namedItem( "position" ).attributes().namedItem( "x" ).nodeValue().toDouble(),
			           node.namedItem( "position" ).attributes().namedItem( "y" ).nodeValue().toDouble() );
			gitem->setPos( p );
			gitem->setTransform( stringToTransform( node.namedItem( "position" ).firstChild().firstChild().nodeValue() ) );
			int zValue = nodeAttributes.namedItem( "z-index" ).nodeValue().toInt();
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
	mlt_profile profile = mlt_service_profile ( MLT_PRODUCER_SERVICE( producer ) ) ;
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
			if ( !createQApplicationIfNeeded( MLT_PRODUCER_SERVICE(producer) ) ) {
				pthread_mutex_unlock( &self->mutex );
				return;
			}
			if ( !QMetaType::type("QTextCursor") )
				qRegisterMetaType<QTextCursor>( "QTextCursor" );
			scene = new QGraphicsScene();
			scene->setItemIndexMethod( QGraphicsScene::NoIndex );
                        scene->setSceneRect(0, 0, mlt_properties_get_int( properties, "width" ), mlt_properties_get_int( properties, "height" ));
			if ( mlt_properties_get( producer_props, "resource" ) && mlt_properties_get( producer_props, "resource" )[0] != '\0' )
			{
				// The title has a resource property, so we read all properties from the resource.
				// Do not serialize the xmldata
				loadFromXml( producer, scene, mlt_properties_get( producer_props, "_xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
			}
			else
			{
				// The title has no resource, all data should be serialized
				loadFromXml( producer, scene, mlt_properties_get( producer_props, "xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
			  
			}
			mlt_properties_set_data( producer_props, "qscene", scene, 0, ( mlt_destructor )qscene_delete, NULL );
		}
                
                QRectF start = stringToRect( QString( mlt_properties_get( producer_props, "_startrect" ) ) );
                QRectF end = stringToRect( QString( mlt_properties_get( producer_props, "_endrect" ) ) );	
		const QRectF source( 0, 0, width, height );

		if (start.isNull()) {
		    start = QRectF( 0, 0, mlt_properties_get_int( producer_props, "meta.media.width" ), mlt_properties_get_int( producer_props, "meta.media.height" ) );
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
				    QTextCursor cursor = titem->textCursor();
				    cursor.movePosition(QTextCursor::EndOfBlock);
				    // get the font format
				    QTextCharFormat format = cursor.charFormat();
				    cursor.select(QTextCursor::Document);
				    QString txt = params.at( 1 ).left( interval );
				    // If the string to insert is empty, insert a space / linebreak so that we don't loose
				    // formatting infos for the next iterations
				    int lines = params.at( 1 ).count( '\n' );
				    QString empty = " ";
				    for (int i = 0; i < lines; i++)
					    empty.append( "\n " );
				    cursor.insertText( txt.isEmpty() ? empty : txt, format );
				    if ( !titem->data( 1 ).isNull() )
					  titem->setTextWidth( titem->data( 1 ).toDouble() );
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
                        double percentage = 0;
			if ( position && anim_out )
				percentage = position / anim_out;
			QPointF topleft = start.topLeft() + ( end.topLeft() - start.topLeft() ) * percentage;
			QPointF bottomRight = start.bottomRight() + ( end.bottomRight() - start.bottomRight() ) * percentage;
			const QRectF r1( topleft, bottomRight );
			scene->render( &p1, source, r1, Qt::IgnoreAspectRatio );
			if ( profile && !profile->progressive ){
				int line=0;
				double percentage_next_filed	= ( position + 0.5 ) / anim_out;
				QPointF topleft_next_field = start.topLeft() + ( end.topLeft() - start.topLeft() ) * percentage_next_filed;
				QPointF bottomRight_next_field = start.bottomRight() + ( end.bottomRight() - start.bottomRight() ) * percentage_next_filed;
				const QRectF r2( topleft_next_field, bottomRight_next_field );
				QImage img1( width, height, QImage::Format_ARGB32 );
				img1.fill( 0 );
				QPainter p2;
				p2.begin(&img1);
				p2.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );
				scene->render(&p2,source,r2,  Qt::IgnoreAspectRatio );
				p2.end();
				int next_field_line = (  mlt_properties_get_int( producer_props, "top_field_first" ) ? 1 : 0 );
				for (line = next_field_line ;line<height;line+=2){
						memcpy(img.scanLine(line),img1.scanLine(line),img.bytesPerLine());
				}

			}
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


