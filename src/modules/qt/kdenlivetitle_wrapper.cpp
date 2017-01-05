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
#include <math.h>

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

// Private Constants
static const double PI = 3.14159265358979323846;

class ImageItem: public QGraphicsItem
{
public:
    ImageItem(QImage img)
    {
	m_img = img;
    }

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

private:
    QImage m_img;


};

void blur( QImage& image, int radius )
{
    int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
    int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

    int r1 = 0;
    int r2 = image.height() - 1;
    int c1 = 0;
    int c2 = image.width() - 1;

    int bpl = image.bytesPerLine();
    int rgba[4];
    unsigned char* p;

    int i1 = 0;
    int i2 = 3;

    for (int col = c1; col <= c2; col++) {
        p = image.scanLine(r1) + col * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p += bpl;
        for (int j = r1; j < r2; j++, p += bpl)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    for (int row = r1; row <= r2; row++) {
        p = image.scanLine(row) + c1 * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p += 4;
        for (int j = c1; j < c2; j++, p += 4)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    for (int col = c1; col <= c2; col++) {
        p = image.scanLine(r2) + col * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p -= bpl;
        for (int j = r1; j < r2; j++, p -= bpl)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    for (int row = r1; row <= r2; row++) {
        p = image.scanLine(row) + c2 * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p -= 4;
        for (int j = c1; j < c2; j++, p -= 4)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

}

class PlainTextItem: public QGraphicsItem
{
public:
    PlainTextItem(QString text, QFont font, double width, double height, QBrush brush, QColor outlineColor, double outline, int align, int lineSpacing)
    {
        m_boundingRect = QRectF(0, 0, width, height);
        m_brush = brush;
        m_outline = outline;
        m_pen = QPen(outlineColor);
        m_pen.setWidthF(outline);
        QFontMetrics metrics(font);
        lineSpacing += metrics.lineSpacing();

        // Calculate line width
        QStringList lines = text.split('\n');
        double linePos = metrics.ascent();
        foreach(const QString &line, lines)
        {
                QPainterPath linePath;
                linePath.addText(0, linePos, font, line);
                linePos += lineSpacing;
                if ( align == Qt::AlignHCenter )
                {
                        double offset = (width - metrics.width(line)) / 2;
                        linePath.translate(offset, 0);
                } else if ( align == Qt::AlignRight ) {
                        double offset = (width - metrics.width(line));
                        linePath.translate(offset, 0);
                }
                m_path = m_path.united(linePath);
        }
    }

    virtual QRectF boundingRect() const
    {
        return m_boundingRect;
    }

    virtual void paint( QPainter *painter,
                       const QStyleOptionGraphicsItem * option,
                       QWidget* w)
    {
        if ( !m_shadow.isNull() )
        {
                painter->drawImage(m_shadowOffset, m_shadow);
        }
        painter->fillPath(m_path, m_brush);
        if ( m_outline > 0 )
        {
                painter->strokePath(m_path, m_pen);
        }
    }

    void addShadow(QStringList params)
    {
        if (params.count() < 5 || params.at( 0 ).toInt() == false) 
        {
                // Invalid or no shadow wanted
                return;
        }
        // Build shadow image
        QColor shadowColor = QColor( params.at( 1 ) );
        int blurRadius = params.at( 2 ).toInt();
        int offsetX = params.at( 3 ).toInt();
        int offsetY = params.at( 4 ).toInt();
        m_shadow = QImage( m_boundingRect.width() + abs( offsetX ) + 4 * blurRadius, m_boundingRect.height() + abs( offsetY ) + 4 * blurRadius, QImage::Format_ARGB32_Premultiplied );
        m_shadow.fill( Qt::transparent );
        QPainterPath shadowPath = m_path;
        offsetX -= 2 * blurRadius;
        offsetY -= 2 * blurRadius;
        m_shadowOffset = QPoint( offsetX, offsetY );
        shadowPath.translate(2 * blurRadius, 2 * blurRadius);
        QPainter shadowPainter( &m_shadow );
        shadowPainter.fillPath( shadowPath, QBrush( shadowColor ) );
        shadowPainter.end();
        blur( m_shadow, blurRadius );
    }

private:
    QRectF m_boundingRect;
    QImage m_shadow;
    QPoint m_shadowOffset;
    QPainterPath m_path;
    QBrush m_brush;
    QPen m_pen;
    double m_outline;
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


void loadFromXml( producer_ktitle self, QGraphicsScene *scene, const char *templateXml, const char *templateText )
{
	scene->clear();
	mlt_producer producer = &self->parent;
	self->has_alpha = true;
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

                                int letterSpacing = txtProperties.namedItem( "font-spacing" ).nodeValue().toInt();
                                if ( letterSpacing != 0 ) {
                                    font.setLetterSpacing( QFont::AbsoluteSpacing, letterSpacing );
                                }
				// Older Kdenlive version did not store pixel size but point size
				if ( txtProperties.namedItem( "font-pixel-size" ).isNull() )
				{
					QFont f2;
					f2.setPointSize( txtProperties.namedItem( "font-size" ).nodeValue().toInt() );
					font.setPixelSize( QFontInfo( f2 ).pixelSize() );
				}
				else
                                {
					font.setPixelSize( txtProperties.namedItem( "font-pixel-size" ).nodeValue().toInt() );
                                }
                                if ( !txtProperties.namedItem( "letter-spacing" ).isNull() )
                                {
                                    font.setLetterSpacing(QFont::AbsoluteSpacing, txtProperties.namedItem( "letter-spacing" ).nodeValue().toInt());
                                }
				QColor col( stringToColor( txtProperties.namedItem( "font-color" ).nodeValue() ) );
				QString text = node.namedItem( "content" ).firstChild().nodeValue();
				if ( !replacementText.isEmpty() )
				{
					text = text.replace( "%s", replacementText );
				}
				QColor outlineColor(stringToColor( txtProperties.namedItem( "font-outline-color" ).nodeValue() ) );

                                int align = 1;
                                if ( txtProperties.namedItem( "alignment" ).isNull() == false )
				{
                                        align = txtProperties.namedItem( "alignment" ).nodeValue().toInt();
				}
				
				double boxWidth = 0;
                                double boxHeight = 0;
				if ( txtProperties.namedItem( "box-width" ).isNull() )
                                {
                                        // This is an old version title, find out dimensions from QGraphicsTextItem
                                        QGraphicsTextItem *txt = scene->addText(text, font);
                                        QRectF br = txt->boundingRect();
                                        boxWidth = br.width();
                                        boxHeight = br.height();
                                        scene->removeItem(txt);
                                        delete txt;
                                } else {
                                        boxWidth = txtProperties.namedItem( "box-width" ).nodeValue().toDouble();
                                        boxHeight = txtProperties.namedItem( "box-height" ).nodeValue().toDouble();
                                }
                                QBrush brush;
                                if ( txtProperties.namedItem( "gradient" ).isNull() == false )
				{
                                        // Calculate gradient
                                        QString gradientData = txtProperties.namedItem( "gradient" ).nodeValue();
                                        QStringList values = gradientData.split(";");
                                        if (values.count() < 5) {
                                            // invalid gradient, use default
                                            values = QStringList() << "#ff0000" << "#2e0046" << "0" << "100" << "90";
                                        }
                                        QLinearGradient gr;
                                        gr.setColorAt(values.at(2).toDouble() / 100, values.at(0));
                                        gr.setColorAt(values.at(3).toDouble() / 100, values.at(1));
                                        double angle = values.at(4).toDouble();
                                        if (angle <= 90) {
                                            gr.setStart(0, 0);
                                            gr.setFinalStop(boxWidth * cos( angle * PI / 180 ), boxHeight * sin( angle * PI / 180 ));
                                        } else {
                                            gr.setStart(boxWidth, 0);
                                            gr.setFinalStop(boxWidth + boxWidth * cos( angle * PI / 180 ), boxHeight * sin( angle * PI / 180 ));
                                        }
                                        brush = QBrush(gr);
				}
				else
                                {
                                    brush = QBrush(col);
                                }
				
				if ( txtProperties.namedItem( "compatibility" ).isNull() ) {
                                        // Workaround Qt5 crash in threaded drawing of QGraphicsTextItem, paint by ourselves
                                        PlainTextItem *txt = new PlainTextItem(text, font, boxWidth, boxHeight, brush, outlineColor, txtProperties.namedItem("font-outline").nodeValue().toDouble(), align, txtProperties.namedItem("line-spacing").nodeValue().toInt());
                                        if ( txtProperties.namedItem( "shadow" ).isNull() == false )
                                        {
                                                QStringList values = txtProperties.namedItem( "shadow" ).nodeValue().split(";");
                                                txt->addShadow(values);
                                        }
                                        scene->addItem( txt );
                                        gitem = txt;
                                } else {
                                        QGraphicsTextItem *txt = scene->addText(text, font);
                                        gitem = txt;
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
                                }
			}
			else if ( nodeAttributes.namedItem( "type" ).nodeValue() == "QGraphicsRectItem" )
			{
                                QDomNamedNodeMap rectProperties = node.namedItem( "content" ).attributes();
                                QRectF rect = stringToRect( rectProperties.namedItem( "rect" ).nodeValue() );
				QString pen_str = rectProperties.namedItem( "pencolor" ).nodeValue();
				double penwidth = rectProperties.namedItem( "penwidth") .nodeValue().toDouble();
                                QBrush brush;
                                if ( !rectProperties.namedItem( "gradient" ).isNull() )
				{
                                        // Calculate gradient
                                        QString gradientData = rectProperties.namedItem( "gradient" ).nodeValue();
                                        QStringList values = gradientData.split(";");
                                        if (values.count() < 5) {
                                            // invalid gradient, use default
                                            values = QStringList() << "#ff0000" << "#2e0046" << "0" << "100" << "90";
                                        }
                                        QLinearGradient gr;
                                        gr.setColorAt(values.at(2).toDouble() / 100, values.at(0));
                                        gr.setColorAt(values.at(3).toDouble() / 100, values.at(1));
                                        double angle = values.at(4).toDouble();
                                        if (angle <= 90) {
                                            gr.setStart(0, 0);
                                            gr.setFinalStop(rect.width() * cos( angle * PI / 180 ), rect.height() * sin( angle * PI / 180 ));
                                        } else {
                                            gr.setStart(rect.width(), 0);
                                            gr.setFinalStop(rect.width() + rect.width()* cos( angle * PI / 180 ), rect.height() * sin( angle * PI / 180 ));
                                        }
                                        brush = QBrush(gr);
				}
				else
                                {
                                    brush = QBrush(stringToColor( rectProperties.namedItem( "brushcolor" ).nodeValue() ) );
                                }
                                QPen pen;
                                if ( penwidth == 0 )
                                {
                                    pen = QPen( Qt::NoPen );
                                }
                                else
                                {
                                    pen = QPen( QBrush( stringToColor( pen_str ) ), penwidth, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin );
                                }
				QGraphicsRectItem *rec = scene->addRect( rect, pen, brush );
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
		self->has_alpha = color.alpha() != 255;
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


void drawKdenliveTitle( producer_ktitle self, mlt_frame frame, mlt_image_format format, int width, int height, double position, int force_refresh )
{
  	// Obtain the producer 
	mlt_producer producer = &self->parent;
	mlt_profile profile = mlt_service_profile ( MLT_PRODUCER_SERVICE( producer ) ) ;
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	pthread_mutex_lock( &self->mutex );

	// Check if user wants us to reload the image or if we need animation
	bool animated = mlt_properties_get( producer_props, "_endrect" ) != NULL;

	if ( mlt_properties_get( producer_props, "_animated" ) != NULL || force_refresh == 1 || width != self->current_width || height != self->current_height || animated )
	{
		if ( !animated )
		{
			// Cache image only if no animation
			self->current_image = NULL;
			mlt_properties_set_data( producer_props, "_cached_image", NULL, 0, NULL, NULL );
		}
		mlt_properties_set_int( producer_props, "force_reload", 0 );
	}
	int image_size = width * height * 4;
	if ( self->current_image == NULL || animated ) {
		// restore QGraphicsScene
		QGraphicsScene *scene = static_cast<QGraphicsScene *> (mlt_properties_get_data( producer_props, "qscene", NULL ));
		self->current_alpha = NULL;

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
				loadFromXml( self, scene, mlt_properties_get( producer_props, "_xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
			}
			else
			{
				// The title has no resource, all data should be serialized
				loadFromXml( self, scene, mlt_properties_get( producer_props, "xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
			  
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
		self->rgba_image = (uint8_t *) mlt_pool_alloc( image_size );
#if QT_VERSION >= 0x050200
		// QImage::Format_RGBA8888 was added in Qt5.2
		// Initialize the QImage with the MLT image because the data formats match.
		QImage img( self->rgba_image, width, height, QImage::Format_RGBA8888 );
#else
		QImage img( width, height, QImage::Format_ARGB32 );
#endif
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
		self->format = mlt_image_rgb24a;

		convert_qimage_to_mlt_rgba(&img, self->rgba_image, width, height);
		self->current_image = (uint8_t *) mlt_pool_alloc( image_size );
		memcpy( self->current_image, self->rgba_image, image_size );
		mlt_properties_set_data( producer_props, "_cached_buffer", self->rgba_image, image_size, mlt_pool_release, NULL );
		mlt_properties_set_data( producer_props, "_cached_image", self->current_image, image_size, mlt_pool_release, NULL );
		self->current_width = width;
		self->current_height = height;

		uint8_t *alpha = NULL;
		if ( self->has_alpha && ( alpha = mlt_frame_get_alpha_mask( frame ) ) )
		{
			self->current_alpha = (uint8_t*) mlt_pool_alloc( width * height );
			memcpy( self->current_alpha, alpha, width * height );
			mlt_properties_set_data( producer_props, "_cached_alpha", self->current_alpha, width * height, mlt_pool_release, NULL );
		}
	}

	// Convert image to requested format
	if ( format != mlt_image_none && format != mlt_image_glsl && format != self->format )
	{
		uint8_t *buffer = NULL;
		if ( self->format != mlt_image_rgb24a ) {
			// Image buffer was previously converted, revert to original rgba buffer
			self->current_image = (uint8_t *) mlt_pool_alloc( image_size );
			memcpy(self->current_image, self->rgba_image, image_size);
			mlt_properties_set_data( producer_props, "_cached_image", self->current_image, image_size, mlt_pool_release, NULL );
			self->format = mlt_image_rgb24a;
		}

		// First, set the image so it can be converted when we get it
		mlt_frame_replace_image( frame, self->current_image, self->format, width, height );
		mlt_frame_set_image( frame, self->current_image, image_size, NULL );
		self->format = format;

		// get_image will do the format conversion
		mlt_frame_get_image( frame, &buffer, &format, &width, &height, 0 );

		// cache copies of the image and alpha buffers
		if ( buffer )
		{
			image_size = mlt_image_format_size( format, width, height, NULL );
			self->current_image = (uint8_t*) mlt_pool_alloc( image_size );
			memcpy( self->current_image, buffer, image_size );
			mlt_properties_set_data( producer_props, "_cached_image", self->current_image, image_size, mlt_pool_release, NULL );
		}
		if ( ( buffer = mlt_frame_get_alpha( frame ) ) )
		{
			self->current_alpha = (uint8_t*) mlt_pool_alloc( width * height );
			memcpy( self->current_alpha, buffer, width * height );
			mlt_properties_set_data( producer_props, "_cached_alpha", self->current_alpha, width * height, mlt_pool_release, NULL );
		}
        }

	pthread_mutex_unlock( &self->mutex );
	mlt_properties_set_int( properties, "width", self->current_width );
	mlt_properties_set_int( properties, "height", self->current_height );
}


