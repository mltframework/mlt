/*
 * filter_qtext.cpp -- text overlay filter
 * Copyright (c) 2018 Meltytech, LLC
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
#include <QTextCodec>
#include <QString>

static QRectF get_text_path( QPainterPath* qpath, mlt_properties filter_properties, const char* text )
{
	int outline = mlt_properties_get_int( filter_properties, "outline" );
	char halign = mlt_properties_get( filter_properties, "halign" )[0];
	char style = mlt_properties_get( filter_properties, "style" )[0];
	int pad = mlt_properties_get_int( filter_properties, "pad" );
	int offset = pad + ( outline / 2 );
	int width = 0;
	int height = 0;

	qpath->setFillRule( Qt::WindingFill );

	// Get the strings to display
	QTextCodec *codec = QTextCodec::codecForName( mlt_properties_get( filter_properties, "encoding" ) );
	QTextDecoder *decoder = codec->makeDecoder();
	QString s = decoder->toUnicode( text );
	delete decoder;
	QStringList lines = s.split( "\n" );

	// Configure the font
	QFont font;
	font.setPixelSize( mlt_properties_get_int( filter_properties, "size" ) );
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

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *image_format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	char* argument = (char*)mlt_frame_pop_service( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	QString geom_str = QString::fromLatin1( mlt_properties_get( filter_properties, "geometry" ) );
	if( geom_str.isEmpty() )
	{
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "geometry property not set\n" );
		mlt_frame_get_image( frame, image, image_format, width, height, writable );
		free( argument );
		return 1;
	}
	mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "geometry", position, length );

	// Get the current image
	*image_format = mlt_image_rgb24a;
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "resize_alpha", 255 );
	error = mlt_frame_get_image( frame, image, image_format, width, height, writable );

	if( !error )
	{
		if ( geom_str.contains('%') )
		{
			rect.x *= *width;
			rect.w *= *width;
			rect.y *= *height;
			rect.h *= *height;
		}

		QImage qimg( *width, *height, QImage::Format_ARGB32 );
		convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );

		QPainterPath text_path;
		QRectF path_rect = get_text_path( &text_path, filter_properties, argument );
		QPainter painter( &qimg );
		painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );
		transform_painter( &painter, rect, path_rect, filter_properties, profile );
		paint_background( &painter, path_rect, filter_properties );
		paint_text( &painter, &text_path, filter_properties );
		painter.end();

		convert_qimage_to_mlt_rgba( &qimg, *image, *width, *height );
	}
	free( argument );

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	char* argument = mlt_properties_get( properties, "argument" );
	if ( !argument || !strcmp( "", argument ) )
		return frame;

	// Save the text to be used by get_image() to support parallel processing
	// when this filter is encapsulated by other filters.
	mlt_frame_push_service( frame, strdup( argument ) );

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
	mlt_properties_set( filter_properties, "argument", arg ? arg: "text" );
	mlt_properties_set( filter_properties, "geometry", "0%/0%:100%x100%:100%" );
	mlt_properties_set( filter_properties, "family", "Sans" );
	mlt_properties_set( filter_properties, "size", "48" );
	mlt_properties_set( filter_properties, "weight", "400" );
	mlt_properties_set( filter_properties, "style", "normal" );
	mlt_properties_set( filter_properties, "fgcolour", "0x000000ff" );
	mlt_properties_set( filter_properties, "bgcolour", "0x00000020" );
	mlt_properties_set( filter_properties, "olcolour", "0x00000000" );
	mlt_properties_set( filter_properties, "pad", "0" );
	mlt_properties_set( filter_properties, "halign", "left" );
	mlt_properties_set( filter_properties, "valign", "top" );
	mlt_properties_set( filter_properties, "outline", "0" );
	mlt_properties_set( filter_properties, "encoding", "UTF-8" );

	mlt_properties_set_int( filter_properties, "_filter_private", 1 );

	return filter;
}

}
