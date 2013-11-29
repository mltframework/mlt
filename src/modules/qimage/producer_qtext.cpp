/*
 * producer_qtext.c -- text generating producer
 * Copyright (C) 2013 Brian Matherly
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QApplication>
#include <QImage>
#include <QColor>
#include <QLocale>
#include <QPainter>
#include <QFont>
#include <QString>
#include <QTextCodec>
#include <QTextDecoder>

/** Init QT (if necessary)
*/

static bool init_qt( mlt_producer producer )
{
	int argc = 1;
	char* argv[1];
	argv[0] = (char*) "xxx";

	if ( !qApp )
	{
#ifdef linux
		if ( getenv("DISPLAY") == 0 )
		{
			mlt_log_panic( MLT_PRODUCER_SERVICE( producer ), "Error, cannot render titles without an X11 environment.\nPlease either run melt from an X session or use a fake X server like xvfb:\nxvfb-run -a melt (...)\n" );
			return false;
		}
#endif
		new QApplication( argc, argv );
		const char *localename = mlt_properties_get_lcnumeric( MLT_SERVICE_PROPERTIES( MLT_PRODUCER_SERVICE( producer ) ) );
		QLocale::setDefault( QLocale( localename ) );
	}
	return true;
}

/** Check for change to property. Copy to private if changed. Return true if private changed.
*/

static bool update_private_property( mlt_properties producer_properties, const char* pub, const char* priv )
{
	char* pub_val = mlt_properties_get( producer_properties, pub );
	char* priv_val = mlt_properties_get( producer_properties, priv );

	if( pub_val == NULL )
	{
		// Can't use public value
		return false;
	}
	else if( priv_val == NULL || strcmp( pub_val, priv_val ) )
	{
		mlt_properties_set( producer_properties, priv, pub_val );
		return true;
	}

	return false;
}

/** Check if the qpath needs to be regenerated. Return true if regeneration is required.
*/

static bool check_qpath( mlt_properties producer_properties )
{
	bool stale = false;
	QPainterPath* qPath = static_cast<QPainterPath*>( mlt_properties_get_data( producer_properties, "_qpath", NULL ) );

	// Check if any properties changed.
	stale |= update_private_property( producer_properties, "text",     "_text" );
	stale |= update_private_property( producer_properties, "fgcolour", "_fgcolour" );
	stale |= update_private_property( producer_properties, "bgcolour", "_bgcolour" );
	stale |= update_private_property( producer_properties, "olcolour", "_olcolour" );
	stale |= update_private_property( producer_properties, "outline",  "_outline" );
	stale |= update_private_property( producer_properties, "align",    "_align" );
	stale |= update_private_property( producer_properties, "pad",      "_pad" );
	stale |= update_private_property( producer_properties, "size",     "_size" );
	stale |= update_private_property( producer_properties, "style",    "_style" );
	stale |= update_private_property( producer_properties, "weight",   "_weight" );
	stale |= update_private_property( producer_properties, "encoding", "_encoding" );

	// Make sure qPath is valid.
	stale |= qPath->isEmpty();

	return stale;
}

static void generate_qpath( mlt_properties producer_properties )
{
	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );
	QPainterPath* qPath = static_cast<QPainterPath*>( mlt_properties_get_data( producer_properties, "_qpath", NULL ) );
	int outline = mlt_properties_get_int( producer_properties, "_outline" );
	char* align = mlt_properties_get( producer_properties, "_align" );
	char* style = mlt_properties_get( producer_properties, "_style" );
	char* text = mlt_properties_get( producer_properties, "_text" );
	char* encoding = mlt_properties_get( producer_properties, "_encoding" );
	int pad = mlt_properties_get_int( producer_properties, "_pad" );
	int offset = pad + ( outline / 2 );
	int width = 0;
	int height = 0;

	// Make the image invalid. It must be regenerated from the new path.
	*qImg = QImage();
	// Make the path empty
	*qPath = QPainterPath();

	// Get the strings to display
	QTextCodec *codec = QTextCodec::codecForName( encoding );
	QTextDecoder *decoder = codec->makeDecoder();
	QString s = decoder->toUnicode( text );
	delete decoder;
	QStringList lines = s.split( "\n" );

	// Configure the font
	QFont font;
	font.setPixelSize( mlt_properties_get_int( producer_properties, "_size" ) );
	font.setFamily( mlt_properties_get( producer_properties, "_family" ) );
	font.setWeight( ( mlt_properties_get_int( producer_properties, "_weight" ) / 10 ) -1 );
	switch( style[0] )
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
		int line_width = fm.width( lines.at(i) );
		if( line_width > width ) width = line_width;
	}

	// Lay out the text in the path
	int x = 0;
	int y = fm.ascent() + 1 + offset;
	for( int i = 0; i < lines.size(); ++i )
	{
		QString line = lines.at(i);
		x = offset;
		switch( align[0] )
		{
			default:
			case 'l':
			case 'L':
				break;
			case 'c':
			case 'C':
				x += ( width - fm.width( line ) ) / 2;
				break;
			case 'r':
			case 'R':
				x += width - fm.width( line );
				break;
		}
		qPath->addText( x, y, font, line );
		y += fm.lineSpacing();
	}

	// Account for outline and pad
	width += offset * 2;
	height += offset * 2;
	// Sanity check
	if( width == 0 ) width = 1;
	if( height == 0 ) height = 1;
	mlt_properties_set_int( producer_properties, "meta.media.width", width );
	mlt_properties_set_int( producer_properties, "meta.media.height", height );
}

static void generate_qimage( mlt_properties producer_properties, QSize target_size )
{
	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );
	QPainterPath* qPath = static_cast<QPainterPath*>( mlt_properties_get_data( producer_properties, "_qpath", NULL ) );
	mlt_color bg_color = mlt_properties_get_color( producer_properties, "_bgcolour" );
	mlt_color fg_color = mlt_properties_get_color( producer_properties, "_fgcolour" );
	mlt_color ol_color = mlt_properties_get_color( producer_properties, "_olcolour" );
	int outline = mlt_properties_get_int( producer_properties, "_outline" );
	QSize native_size( mlt_properties_get_int( producer_properties, "meta.media.width" ),
					   mlt_properties_get_int( producer_properties, "meta.media.height" ) );
	qreal sx = 1.0;
	qreal sy = 1.0;

	// Create a new image and set up scaling
	if( !target_size.isEmpty() && target_size != native_size )
	{
		*qImg = QImage( target_size, QImage::Format_ARGB32 );
		sx = (qreal)target_size.width() / (qreal)native_size.width();
		sy = (qreal)target_size.height() / (qreal)native_size.height();
	}
	else
	{
		*qImg = QImage( native_size, QImage::Format_ARGB32 );
	}

	// Draw the text
	QPainter painter( qImg );
	// Scale the painter rather than the image for better looking results.
	painter.scale( sx, sy );
	painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );
	painter.fillRect ( 0, 0, qImg->width(), qImg->height(), QColor( bg_color.r, bg_color.g, bg_color.b, bg_color.a ) );
	QPen pen;
	pen.setWidth( outline );
	if( outline )
	{
		pen.setColor( QColor( ol_color.r, ol_color.g, ol_color.b, ol_color.a ) );
	}
	else
	{
		pen.setColor( QColor( bg_color.r, bg_color.g, bg_color.b, bg_color.a ) );
	}
	painter.setPen( pen );
	QBrush brush( QColor( fg_color.r, fg_color.g, fg_color.b, fg_color.a ) );
	painter.setBrush( brush );
	painter.drawPath( *qPath );
}

static void copy_qimage_to_mlt_image( QImage* qImg, uint8_t* mImg )
{
	int height = qImg->height();
	int width = qImg->width();
	int y = 0;

	// convert qimage to mlt
	y = height + 1;
	while ( --y )
	{
		QRgb* src = (QRgb*) qImg->scanLine( height - y );
		int x = width + 1;
		while ( --x )
		{
			*mImg++ = qRed( *src );
			*mImg++ = qGreen( *src );
			*mImg++ = qBlue( *src );
			*mImg++ = qAlpha( *src );
			src++;
		}
	}
}

static void copy_image_to_alpha( uint8_t* image, uint8_t* alpha, int width, int height )
{
	register int len = width * height;
	// Extract the alpha mask from the RGBA image using Duff's Device
	register uint8_t *s = image + 3; // start on the alpha component
	register uint8_t *d = alpha;
	register int n = ( len + 7 ) / 8;

	switch ( len % 8 )
	{
		case 0:	do { *d++ = *s; s += 4;
		case 7:		 *d++ = *s; s += 4;
		case 6:		 *d++ = *s; s += 4;
		case 5:		 *d++ = *s; s += 4;
		case 4:		 *d++ = *s; s += 4;
		case 3:		 *d++ = *s; s += 4;
		case 2:		 *d++ = *s; s += 4;
		case 1:		 *d++ = *s; s += 4;
				}
				while ( --n > 0 );
	}
}

static bool check_qimage( mlt_properties producer_properties, QSize target_size )
{
	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );

	if( qImg->isNull() )
	{
		return true;
	}

	QSize output_size = target_size;
	if( output_size.isEmpty() )
	{
		output_size.setWidth( mlt_properties_get_int( producer_properties, "meta.media.width" ) );
		output_size.setHeight( mlt_properties_get_int( producer_properties, "meta.media.height" ) );
	}

	if( output_size != qImg->size() )
	{
		return true;
	}

	return false;
}

static int producer_get_image( mlt_frame frame, uint8_t** buffer, mlt_image_format* format, int* width, int* height, int writable )
{
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = static_cast<mlt_producer>( mlt_properties_get_data( frame_properties, "_producer_qtext", NULL ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	int img_size = 0;
	int alpha_size = 0;
	uint8_t* alpha = NULL;
	QImage* qImg = NULL;

	// Detect rescale request
	QSize target_size( mlt_properties_get_int( frame_properties, "rescale_width" ),
					   mlt_properties_get_int( frame_properties, "rescale_height" ) );

	// Regenerate the qimage if necessary
	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	if( check_qimage( producer_properties, target_size ) == true )
	{
		generate_qimage( producer_properties, target_size );
	}

	qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );

	*format = mlt_image_rgb24a;
	*width = qImg->width();
	*height = qImg->height();

	// Allocate and fill the image buffer
	img_size = mlt_image_format_size( *format, *width, *height, NULL );
	*buffer = static_cast<uint8_t*>( mlt_pool_alloc( img_size ) );
	copy_qimage_to_mlt_image( qImg, *buffer );

	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	// Allocate and fill the alpha buffer
	alpha_size = *width * *height;
	alpha = static_cast<uint8_t*>( mlt_pool_alloc( alpha_size ) );
	copy_image_to_alpha( *buffer, alpha, *width, *height );

	// Update the frame
	mlt_frame_set_image( frame, *buffer, img_size, mlt_pool_release );
	mlt_frame_set_alpha( frame, alpha, alpha_size, mlt_pool_release );
	mlt_properties_set_int( frame_properties, "width", *width );
	mlt_properties_set_int( frame_properties, "height", *height );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Obtain properties of frame
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

		if( check_qpath( producer_properties ) )
		{
			generate_qpath( producer_properties );
		}

		// Save the producer to be used later
		mlt_properties_set_data( frame_properties, "_producer_qtext", static_cast<void*>( producer ), 0, NULL, NULL );

		// Set frame properties
		mlt_properties_set_int( frame_properties, "progressive", 1 );
		double force_ratio = mlt_properties_get_double( producer_properties, "force_aspect_ratio" );
		if ( force_ratio > 0.0 )
			mlt_properties_set_double( frame_properties, "aspect_ratio", force_ratio );
		else
			mlt_properties_set_double( frame_properties, "aspect_ratio", 1.0);

		// Update time code on the frame
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Configure callbacks
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Calculate the next time code
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );
	delete qImg;

	QPainterPath* qPath = static_cast<QPainterPath*>( mlt_properties_get_data( producer_properties, "_qpath", NULL ) );
	delete qPath;

	producer->close = NULL;

	mlt_producer_close( producer );
	free( producer );
}

/** Initialize.
*/
extern "C" {

mlt_producer producer_qtext_init( mlt_profile profile, mlt_service_type type, const char *id, char *filename )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Initialize the producer
	if ( producer )
	{
		if( init_qt( producer ) == false )
		{
			mlt_producer_close( producer );
			return NULL;
		}

		mlt_properties_set( producer_properties, "text",     "" );
		mlt_properties_set( producer_properties, "fgcolour", "0xffffffff" );
		mlt_properties_set( producer_properties, "bgcolour", "0x00000000" );
		mlt_properties_set( producer_properties, "olcolour", "0x00000000" );
		mlt_properties_set( producer_properties, "outline",  "0" );
		mlt_properties_set( producer_properties, "align",    "left" );
		mlt_properties_set( producer_properties, "pad",      "0" );
		mlt_properties_set( producer_properties, "family",   "Sans" );
		mlt_properties_set( producer_properties, "size",     "48" );
		mlt_properties_set( producer_properties, "style",    "normal" );
		mlt_properties_set( producer_properties, "weight",   "400" );
		mlt_properties_set( producer_properties, "encoding", "UTF-8" );

		// Parse the filename argument
		if ( filename == NULL ||
		     !strcmp( filename, "" ) ||
			 strstr( filename, "<producer>" ) )
		{
		}
		else if( filename[ 0 ] == '+' || strstr( filename, "/+" ) )
		{
			char *copy = strdup( filename + 1 );
			char *tmp = copy;
			if ( strstr( tmp, "/+" ) )
				tmp = strstr( tmp, "/+" ) + 2;
			if ( strrchr( tmp, '.' ) )
				( *strrchr( tmp, '.' ) ) = '\0';
			while ( strchr( tmp, '~' ) )
				( *strchr( tmp, '~' ) ) = '\n';
			mlt_properties_set( producer_properties, "text", tmp );
			mlt_properties_set( producer_properties, "resource", filename );
			free( copy );
		}
		else
		{
			FILE *f = fopen( filename, "r" );
			if ( f != NULL )
			{
				char line[81];
				char *tmp = NULL;
				size_t size = 0;
				line[80] = '\0';

				while ( fgets( line, 80, f ) )
				{
					size += strlen( line ) + 1;
					if ( tmp )
					{
						tmp = (char*)realloc( tmp, size );
						if ( tmp )
							strcat( tmp, line );
					}
					else
					{
						tmp = strdup( line );
					}
				}
				fclose( f );

				if ( tmp && tmp[ strlen( tmp ) - 1 ] == '\n' )
					tmp[ strlen( tmp ) - 1 ] = '\0';

				if ( tmp )
					mlt_properties_set( producer_properties, "text", tmp );
				mlt_properties_set( producer_properties, "resource", filename );
				free( tmp );
			}
		}

		// Create QT objects to be reused.
		QImage* qImg = new QImage();
		mlt_properties_set_data( producer_properties, "_qimg", static_cast<void*>( qImg ), 0, NULL, NULL );
		QPainterPath* qPath = new QPainterPath();
		mlt_properties_set_data( producer_properties, "_qpath", static_cast<void*>( qPath ), 0, NULL, NULL );

		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
	}

	return producer;
}

}
