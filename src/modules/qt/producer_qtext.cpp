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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "common.h"
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QFont>
#include <QString>
#include <QTextCodec>
#include <QTextDecoder>

static void close_qimg( void* qimg )
{
	delete static_cast<QImage*>( qimg );
}

static void close_qpath( void* qpath )
{
	delete static_cast<QPainterPath*>( qpath );
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

/** Check if the qpath needs to be regenerated. Return true if regeneration is required.
*/

static bool check_qpath( mlt_properties producer_properties )
{
	#define MAX_SIG 500
	char new_path_sig[MAX_SIG];

	// Generate a signature that represents the current properties
	snprintf( new_path_sig, MAX_SIG, "%s%s%s%s%s%s%s%s%s%s%s",
			mlt_properties_get( producer_properties, "text" ),
			mlt_properties_get( producer_properties, "fgcolour" ),
			mlt_properties_get( producer_properties, "bgcolour" ),
			mlt_properties_get( producer_properties, "olcolour" ),
			mlt_properties_get( producer_properties, "outline" ),
			mlt_properties_get( producer_properties, "align" ),
			mlt_properties_get( producer_properties, "pad" ),
			mlt_properties_get( producer_properties, "size" ),
			mlt_properties_get( producer_properties, "style" ),
			mlt_properties_get( producer_properties, "weight" ),
			mlt_properties_get( producer_properties, "encoding" ) );
	new_path_sig[ MAX_SIG - 1 ] = '\0';

	// Check if the properties have changed by comparing this signature with the
	// last one.
	char* last_path_sig = mlt_properties_get( producer_properties, "_path_sig" );

	if( !last_path_sig || strcmp( new_path_sig, last_path_sig ) )
	{
		mlt_properties_set( producer_properties, "_path_sig", new_path_sig );
		return true;
	}
	return false;
}

static void generate_qpath( mlt_properties producer_properties )
{
	QPainterPath* qPath = static_cast<QPainterPath*>( mlt_properties_get_data( producer_properties, "_qpath", NULL ) );
	int outline = mlt_properties_get_int( producer_properties, "outline" );
	char* align = mlt_properties_get( producer_properties, "align" );
	char* style = mlt_properties_get( producer_properties, "style" );
	char* text = mlt_properties_get( producer_properties, "text" );
	char* encoding = mlt_properties_get( producer_properties, "encoding" );
	int pad = mlt_properties_get_int( producer_properties, "pad" );
	int offset = pad + ( outline / 2 );
	int width = 0;
	int height = 0;

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
	font.setPixelSize( mlt_properties_get_int( producer_properties, "size" ) );
	font.setFamily( mlt_properties_get( producer_properties, "family" ) );
	font.setWeight( ( mlt_properties_get_int( producer_properties, "weight" ) / 10 ) -1 );
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

static bool check_qimage( mlt_properties frame_properties )
{
	mlt_producer producer = static_cast<mlt_producer>( mlt_properties_get_data( frame_properties, "_producer_qtext", NULL ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );
	QSize target_size( mlt_properties_get_int( frame_properties, "rescale_width" ),
					   mlt_properties_get_int( frame_properties, "rescale_height" ) );
	QSize native_size( mlt_properties_get_int( frame_properties, "meta.media.width" ),
					   mlt_properties_get_int( frame_properties, "meta.media.height" ) );

	// Check if the last image signature is different from the path signature
	// for this frame.
	char* last_img_sig = mlt_properties_get( producer_properties, "_img_sig" );
	char* path_sig = mlt_properties_get( frame_properties, "_path_sig" );

	if( !last_img_sig || strcmp( path_sig, last_img_sig ) )
	{
		mlt_properties_set( producer_properties, "_img_sig", path_sig );
		return true;
	}

	// Check if the last image size matches the requested image size
	QSize output_size = target_size;
	if( output_size.isEmpty() )
	{
		output_size = native_size;
	}
	if( output_size != qImg->size() )
	{
		return true;
	}

	return false;
}

static void generate_qimage( mlt_properties frame_properties )
{
	mlt_producer producer = static_cast<mlt_producer>( mlt_properties_get_data( frame_properties, "_producer_qtext", NULL ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );
	QSize target_size( mlt_properties_get_int( frame_properties, "rescale_width" ),
					   mlt_properties_get_int( frame_properties, "rescale_height" ) );
	QSize native_size( mlt_properties_get_int( frame_properties, "meta.media.width" ),
					   mlt_properties_get_int( frame_properties, "meta.media.height" ) );
	QPainterPath* qPath = static_cast<QPainterPath*>( mlt_properties_get_data( frame_properties, "_qpath", NULL ) );
	mlt_color bg_color = mlt_properties_get_color( frame_properties, "_bgcolour" );
	mlt_color fg_color = mlt_properties_get_color( frame_properties, "_fgcolour" );
	mlt_color ol_color = mlt_properties_get_color( frame_properties, "_olcolour" );
	int outline = mlt_properties_get_int( frame_properties, "_outline" );
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
	qImg->fill( QColor( bg_color.r, bg_color.g, bg_color.b, bg_color.a ).rgba() );

	// Draw the text
	QPainter painter( qImg );
	// Scale the painter rather than the image for better looking results.
	painter.scale( sx, sy );
	painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );

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

static int producer_get_image( mlt_frame frame, uint8_t** buffer, mlt_image_format* format, int* width, int* height, int writable )
{
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = static_cast<mlt_producer>( mlt_properties_get_data( frame_properties, "_producer_qtext", NULL ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	int img_size = 0;
	int alpha_size = 0;
	uint8_t* alpha = NULL;
	QImage* qImg = static_cast<QImage*>( mlt_properties_get_data( producer_properties, "_qimg", NULL ) );

	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	// Regenerate the qimage if necessary
	if( check_qimage( frame_properties ) == true )
	{
		generate_qimage( frame_properties );
	}

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
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

		// Regenerate the QPainterPath if necessary
		if( check_qpath( producer_properties ) )
		{
			generate_qpath( producer_properties );
		}

		// Give the frame a copy of the painter path
		QPainterPath* prodPath = static_cast<QPainterPath*>( mlt_properties_get_data( producer_properties, "_qpath", NULL ) );
		QPainterPath* framePath = new QPainterPath( *prodPath );
		mlt_properties_set_data( frame_properties, "_qpath", static_cast<void*>( framePath ), 0, close_qpath, NULL );

		// Pass properties to the frame that will be needed to render the path
		mlt_properties_set( frame_properties, "_path_sig", mlt_properties_get( producer_properties, "_path_sig" ) );
		mlt_properties_set( frame_properties, "_bgcolour", mlt_properties_get( producer_properties, "bgcolour" ) );
		mlt_properties_set( frame_properties, "_fgcolour", mlt_properties_get( producer_properties, "fgcolour" ) );
		mlt_properties_set( frame_properties, "_olcolour", mlt_properties_get( producer_properties, "olcolour" ) );
		mlt_properties_set( frame_properties, "_outline", mlt_properties_get( producer_properties, "outline" ) );
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
		if ( !createQApplicationIfNeeded( MLT_PRODUCER_SERVICE(producer) ) )
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
			// Convert file name string encoding.
			mlt_properties_set( producer_properties, "resource", filename );
			mlt_properties_from_utf8( producer_properties, "resource", "_resource" );
			filename = mlt_properties_get( producer_properties, "_resource" );

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
				free( tmp );
			}
		}

		// Create QT objects to be reused.
		mlt_properties_set_data( producer_properties, "_qimg", static_cast<void*>( new QImage() ), 0, close_qimg, NULL );
		mlt_properties_set_data( producer_properties, "_qpath", static_cast<void*>( new QPainterPath() ), 0, close_qpath, NULL );

		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
	}

	return producer;
}

}
