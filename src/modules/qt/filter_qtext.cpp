/*
 * filter_qtext.cpp -- text overlay filter
 * Copyright (c) 2018-2020 Meltytech, LLC
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

#include "common.h"
#include <framework/mlt.h>
#include <framework/mlt_log.h>
#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <QFile>
#include <QTextDocument>
#include <QTextCodec>

static QRectF get_text_path( QPainterPath* qpath, mlt_properties filter_properties, const char* text, double scale )
{
	int outline = mlt_properties_get_int( filter_properties, "outline" ) * scale;
	char halign = mlt_properties_get( filter_properties, "halign" )[0];
	char style = mlt_properties_get( filter_properties, "style" )[0];
	int pad = mlt_properties_get_int( filter_properties, "pad" ) * scale;
	int offset = pad + ( outline / 2 );
	int width = 0;
	int height = 0;

	qpath->setFillRule( Qt::WindingFill );

	// Get the strings to display
	QString s = QString::fromUtf8(text);
	QStringList lines = s.split( "\n" );

	// Configure the font
	QFont font;
	font.setPixelSize( mlt_properties_get_int( filter_properties, "size" ) * scale );
	font.setFamily( mlt_properties_get( filter_properties, "family" ) );
	font.setWeight( ( mlt_properties_get_int( filter_properties, "weight" ) / 10 ) -1 );
	switch( style )
	{
	case 'i':
	case 'I':
		font.setStyle( QFont::StyleItalic );
		break;
	}
	QFontMetrics fm( font );

	// Determine the text rectangle size
	height = fm.lineSpacing() * lines.size();
	for( int i = 0; i < lines.size(); ++i )
	{
		const QString line = lines[i];
		int line_width = fm.width(line);
		int bearing = (line.size() > 0) ? fm.leftBearing(line.at(0)) : 0;
		if (bearing < 0)
			line_width -= bearing;
		bearing = (line.size() > 0) ? fm.rightBearing(line.at(line.size() - 1)) : 0;
		if (bearing < 0)
			line_width -= bearing;
		if (line_width > width)
			width = line_width;
	}

	// Lay out the text in the path
	int x = 0;
	int y = fm.ascent() + offset;
	for( int i = 0; i < lines.size(); ++i )
	{
		QString line = lines.at(i);
		x = offset;
		int line_width = fm.width(line);
		int bearing = (line.size() > 0)? fm.leftBearing(line.at(0)) : 0;

		if (bearing < 0) {
			line_width -= bearing;
			x -= bearing;
		}
		bearing = (line.size() > 0)? fm.rightBearing(line.at(line.size() - 1)) : 0;
		if (bearing < 0)
			line_width -= bearing;

		switch( halign )
		{
			default:
			case 'l':
			case 'L':
				break;
			case 'c':
			case 'C':
				x += ( width - line_width ) / 2;
				break;
			case 'r':
			case 'R':
				x += width - line_width;
				break;
		}
		qpath->addText( x, y, font, line );
		y += fm.lineSpacing();
	}

	// Account for outline and pad
	width += offset * 2;
	height += offset * 2;
	// Sanity check
	if( width == 0 ) width = 1;
	height += 2; // I found some fonts whose descenders get cut off.

	return QRectF( 0, 0, width, height );
}

static QColor get_qcolor( mlt_properties filter_properties, const char* name )
{
	mlt_color color = mlt_properties_get_color( filter_properties, name );
	return QColor( color.r, color.g, color.b, color.a );
}

static QPen get_qpen( mlt_properties filter_properties )
{
	QColor color;
	int outline = mlt_properties_get_int( filter_properties, "outline" );
	QPen pen;

	pen.setWidth( outline );
	if( outline )
	{
		color = get_qcolor( filter_properties, "olcolour" );
	}
	else
	{
		color = get_qcolor( filter_properties, "bgcolour" );
	}
	pen.setColor( color );

	return pen;
}

static QBrush get_qbrush( mlt_properties filter_properties )
{
	QColor color = get_qcolor( filter_properties, "fgcolour" );
	return QBrush ( color );
}

static void transform_painter( QPainter* painter, mlt_rect frame_rect, QRectF path_rect, mlt_properties filter_properties, mlt_profile profile )
{
	qreal sx = 1.0;
	qreal sy = mlt_profile_sar( profile );
	qreal path_width = path_rect.width() * sx;
	if( path_width > frame_rect.w )
	{
		sx *= frame_rect.w / path_width;
		sy *= frame_rect.w / path_width;
	}
	qreal path_height = path_rect.height() * sy;
	if( path_height > frame_rect.h )
	{
		sx *= frame_rect.h / path_height;
		sy *= frame_rect.h / path_height;
	}

	qreal dx = frame_rect.x;
	qreal dy = frame_rect.y;
	char halign = mlt_properties_get( filter_properties, "halign" )[0];
	switch( halign )
	{
		default:
		case 'l':
		case 'L':
			break;
		case 'c':
		case 'C':
			dx += ( frame_rect.w - ( sx * path_rect.width() ) ) / 2;
			break;
		case 'r':
		case 'R':
			dx += frame_rect.w - ( sx * path_rect.width() );
			break;
	}
	char valign = mlt_properties_get( filter_properties, "valign" )[0];
	switch( valign )
	{
		default:
		case 't':
		case 'T':
			break;
		case 'm':
		case 'M':
			dy += ( frame_rect.h - ( sy * path_rect.height() ) ) / 2;
			break;
		case 'b':
		case 'B':
			dy += frame_rect.h - ( sy * path_rect.height() );
			break;
	}

	QTransform transform;
	transform.translate( dx, dy );
	transform.scale( sx, sy );
	painter->setTransform( transform );
}

static void paint_background( QPainter* painter, QRectF path_rect, mlt_properties filter_properties )
{
	QColor bg_color = get_qcolor( filter_properties, "bgcolour" );
	painter->fillRect( path_rect, bg_color );
}

static void paint_text( QPainter* painter, QPainterPath* qpath, mlt_properties filter_properties )
{
	QPen pen = get_qpen( filter_properties );
	painter->setPen( pen );
	QBrush brush = get_qbrush( filter_properties );
	painter->setBrush( brush );
	painter->drawPath( *qpath );
}

static void close_qtextdoc(void* p)
{
	delete static_cast<QTextDocument*>(p);
}

static QTextDocument* get_rich_text(mlt_properties properties, double width, double height)
{
	QTextDocument* doc = (QTextDocument*) mlt_properties_get_data(properties, "QTextDocument", NULL);
	auto html = QString::fromUtf8(mlt_properties_get(properties, "html"));
	auto prevHtml = QString::fromUtf8(mlt_properties_get(properties, "_html"));
	auto resource = QString::fromUtf8(mlt_properties_get(properties, "resource"));
	auto prevResource = QString::fromUtf8(mlt_properties_get(properties, "_resource"));
	auto prevWidth = mlt_properties_get_double(properties, "_width");
	auto prevHeight = mlt_properties_get_double(properties, "_height");
	bool changed = !doc || qAbs(width - prevWidth) > 1 || qAbs(height- prevHeight) > 1;

	if (!resource.isEmpty() && (changed || resource != prevResource)) {
		QFile file(resource);
		if (file.open(QFile::ReadOnly)) {
			QByteArray data = file.readAll();
			QTextCodec *codec = QTextCodec::codecForHtml(data);
			doc = new QTextDocument;
			doc->setPageSize(QSizeF(width, height));
			doc->setHtml(codec->toUnicode(data));
			mlt_properties_set_data(properties, "QTextDocument", doc, 0, (mlt_destructor) close_qtextdoc, NULL);
			mlt_properties_set(properties, "_resource", resource.toUtf8().constData());
			mlt_properties_set_double(properties, "_width", width);
			mlt_properties_set_double(properties, "_height", height);
		}
	} else if (!html.isEmpty() && (changed || html != prevHtml)) {
//		fprintf(stderr, "%s\n", html.toUtf8().constData());
		doc = new QTextDocument;
		doc->setPageSize(QSizeF(width, height));
		doc->setHtml(html);
		mlt_properties_set_data(properties, "QTextDocument", doc, 0, (mlt_destructor) close_qtextdoc, NULL);
		mlt_properties_set(properties, "_html", html.toUtf8().constData());
		mlt_properties_set_double(properties, "_width", width);
		mlt_properties_set_double(properties, "_height", height);
	}

	return doc;
}

static mlt_properties get_filter_properties( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = mlt_frame_get_unique_properties( frame, MLT_FILTER_SERVICE(filter) );
	if ( !properties )
		properties = MLT_FILTER_PROPERTIES(filter);
	return properties;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *image_format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	char* argument = (char*)mlt_frame_pop_service( frame );
	mlt_properties filter_properties = get_filter_properties( filter, frame );
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	bool isRichText = qstrlen(mlt_properties_get(filter_properties, "html")) > 0 ||
					  qstrlen(mlt_properties_get(filter_properties, "resource")) > 0;
	QString geom_str = QString::fromLatin1( mlt_properties_get( filter_properties, "geometry" ) );
	if( geom_str.isEmpty() )
	{
		free( argument );
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "geometry property not set\n" );
		return mlt_frame_get_image( frame, image, image_format, width, height, writable );
	}
	mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "geometry", position, length );

	// Get the current image
	*image_format = mlt_image_rgba;
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "resize_alpha", 255 );
	mlt_service_lock(MLT_FILTER_SERVICE(filter));
	error = mlt_frame_get_image( frame, image, image_format, width, height, writable );

	if( !error )
	{
		double scale = mlt_profile_scale_width(profile, *width);
		double scale_height = mlt_profile_scale_height(profile, *height);
		if ( geom_str.contains('%') )
		{
			rect.x *= *width;
			rect.w *= *width;
			rect.y *= *height;
			rect.h *= *height;
		}
		else
		{
			rect.x *= scale;
			rect.y *= scale_height;
			rect.w *= scale;
			rect.h *= scale_height;
		}

		QImage qimg;
		convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );

		QPainterPath text_path;
#ifdef Q_OS_WIN
		auto pixel_ratio = mlt_properties_get_double(filter_properties, "pixel_ratio");
#else
		auto pixel_ratio = 1.0;
#endif
		QRectF path_rect(0, 0, rect.w / scale * pixel_ratio, rect.h / scale_height * pixel_ratio);
		QPainter painter( &qimg );
		painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );
		if (isRichText) {
			auto overflowY = mlt_properties_exists(filter_properties, "overflow-y")?
				!!mlt_properties_get_int(filter_properties, "overflow-y") :
				(path_rect.height() >= profile->height * pixel_ratio);
			auto drawRect = overflowY? QRectF() : path_rect;
			auto doc = get_rich_text(filter_properties, path_rect.width(), std::numeric_limits<qreal>::max());
			if (doc) {
				transform_painter(&painter, rect, path_rect, filter_properties, profile);
				if (overflowY) {
					path_rect.setHeight(qMax(path_rect.height(), doc->size().height()));
				}
				paint_background(&painter, path_rect, filter_properties);
				doc->drawContents(&painter, drawRect);
			}
		} else {
			path_rect = get_text_path(&text_path, filter_properties, argument, scale);
			transform_painter(&painter, rect, path_rect, filter_properties, profile);
			paint_background(&painter, path_rect, filter_properties);
			paint_text(&painter, &text_path, filter_properties);
		}
		painter.end();

		convert_qimage_to_mlt_rgba( &qimg, *image, *width, *height );
	}
	mlt_service_unlock(MLT_FILTER_SERVICE(filter));
	free( argument );

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = get_filter_properties( filter, frame );

	if (mlt_properties_get_int(properties, "_hide")) {
		return frame;
	}

	char* argument = mlt_properties_get(properties, "argument");
	char* html = mlt_properties_get(properties, "html");
	char* resource = mlt_properties_get(properties, "resource");

	// Save the text to be used by get_image() to support parallel processing
	// when this filter is encapsulated by other filters.
	if (qstrlen(resource)) {
		mlt_frame_push_service(frame, NULL);
	} else if (qstrlen(html)) {
		mlt_frame_push_service(frame, NULL);
	} else if (qstrlen(argument)) {
		mlt_frame_push_service(frame, strdup(argument));
	} else {
		return frame;
	}

	// Push the filter on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the get_image on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

extern "C" {

mlt_filter filter_qtext_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();

	if( !filter ) return NULL;

	if ( !createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )  {
		mlt_filter_close( filter );
		return NULL;
	}

	filter->process = filter_process;

	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	// Assign default values
	mlt_properties_set_string( filter_properties, "argument", arg ? arg: "text" );
	mlt_properties_set_string( filter_properties, "geometry", "0%/0%:100%x100%:100%" );
	mlt_properties_set_string( filter_properties, "family", "Sans" );
	mlt_properties_set_string( filter_properties, "size", "48" );
	mlt_properties_set_string( filter_properties, "weight", "400" );
	mlt_properties_set_string( filter_properties, "style", "normal" );
	mlt_properties_set_string( filter_properties, "fgcolour", "0x000000ff" );
	mlt_properties_set_string( filter_properties, "bgcolour", "0x00000020" );
	mlt_properties_set_string( filter_properties, "olcolour", "0x00000000" );
	mlt_properties_set_string( filter_properties, "pad", "0" );
	mlt_properties_set_string( filter_properties, "halign", "left" );
	mlt_properties_set_string( filter_properties, "valign", "top" );
	mlt_properties_set_string( filter_properties, "outline", "0" );
	mlt_properties_set_double( filter_properties, "pixel_ratio", 1.0 );
	mlt_properties_set_int( filter_properties, "_filter_private", 1 );

	return filter;
}

}
