/*
 * qimage_wrapper.cpp -- a QT/QImage based producer for MLT
 * Copyright (C) 2006 Visual Media
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * NB: This module is designed to be functionally equivalent to the 
 * gtk2 image loading module so it can be used as replacement.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "qimage_wrapper.h"
#include "common.h"

#ifdef USE_KDE4
#include <kcomponentdata.h>
#endif

#include <QImage>
#include <QSysInfo>
#include <QMutex>
#include <QtEndian>
#include <QTemporaryFile>
#include <QImageReader>

#ifdef USE_EXIF
#include <libexif/exif-data.h>
#endif

#include <cmath>
#include <unistd.h>

extern "C" {

#include <framework/mlt_pool.h>
#include <framework/mlt_cache.h>

#ifdef USE_KDE4
static KComponentData *instance = 0L;
#endif

static void qimage_delete( void *data )
{
	QImage *image = ( QImage * )data;
	delete image;
	image = NULL;
#if defined(USE_KDE4)
	delete instance;
	instance = 0L;
#endif

}

/// Returns false if this is animated.
int init_qimage(const char *filename)
{
	QImageReader reader( filename );
	if ( reader.canRead() && reader.imageCount() > 1 ) {
		return 0;
	}
#ifdef USE_KDE4
	if ( !instance ) {
	    instance = new KComponentData( "qimage_prod" );
	}
#endif
	return 1;
}

static QImage* reorient_with_exif( producer_qimage self, int image_idx, QImage *qimage )
{
#ifdef USE_EXIF
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( &self->parent );
	ExifData *d = exif_data_new_from_file( mlt_properties_get_value( self->filenames, image_idx ) );
	ExifEntry *entry;
	int exif_orientation = 0;
	/* get orientation and rotate image accordingly if necessary */
	if (d) {
		if ( ( entry = exif_content_get_entry ( d->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION ) ) )
			exif_orientation = exif_get_short (entry->data, exif_data_get_byte_order (d));

		/* Free the EXIF data */
		exif_data_unref(d);
	}

	// Remember EXIF value, might be useful for someone
	mlt_properties_set_int( producer_props, "_exif_orientation" , exif_orientation );

	if ( exif_orientation > 1 )
	{
		  // Rotate image according to exif data
		  QImage processed;
		  QMatrix matrix;

		  switch ( exif_orientation ) {
		  case 2:
			  matrix.scale( -1, 1 );
			  break;
		  case 3:
			  matrix.rotate( 180 );
			  break;
		  case 4:
			  matrix.scale( 1, -1 );
			  break;
		  case 5:
			  matrix.rotate( 270 );
			  matrix.scale( -1, 1 );
			  break;
		  case 6:
			  matrix.rotate( 90 );
			  break;
		  case 7:
			  matrix.rotate( 90 );
			  matrix.scale( -1, 1 );
			  break;
		  case 8:
			  matrix.rotate( 270 );
			  break;
		  }
		  processed = qimage->transformed( matrix );
		  delete qimage;
		  qimage = new QImage( processed );
	}
#endif
	return qimage;
}

int refresh_qimage( producer_qimage self, mlt_frame frame )
{
	// Obtain properties of frame and producer
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = &self->parent;
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Check if user wants us to reload the image
	if ( mlt_properties_get_int( producer_props, "force_reload" ) )
	{
		self->qimage = NULL;
		self->current_image = NULL;
		mlt_properties_set_int( producer_props, "force_reload", 0 );
	}

	// Get the time to live for each frame
	double ttl = mlt_properties_get_int( producer_props, "ttl" );

	// Get the original position of this frame
	mlt_position position = mlt_frame_original_position( frame );
	position += mlt_producer_get_in( producer );

	// Image index
	int image_idx = ( int )floor( ( double )position / ttl ) % self->count;

	int disable_exif = mlt_properties_get_int( producer_props, "disable_exif" );

	if ( !createQApplicationIfNeeded( MLT_PRODUCER_SERVICE(producer) ) )
		return -1;

	if ( image_idx != self->qimage_idx )
		self->qimage = NULL;
	if ( !self->qimage || mlt_properties_get_int( producer_props, "_disable_exif" ) != disable_exif )
	{
		self->current_image = NULL;
		QImage *qimage = new QImage( QString::fromUtf8( mlt_properties_get_value( self->filenames, image_idx ) ) );
		self->qimage = qimage;

		if ( !qimage->isNull( ) )
		{
			// Read the exif value for this file
			if ( !disable_exif )
				qimage = reorient_with_exif( self, image_idx, qimage );

			// Register qimage for destruction and reuse
			mlt_cache_item_close( self->qimage_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage", qimage, 0, ( mlt_destructor )qimage_delete );
			self->qimage_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage" );
			self->qimage_idx = image_idx;

			// Store the width/height of the qimage
			self->current_width = qimage->width( );
			self->current_height = qimage->height( );

			mlt_events_block( producer_props, NULL );
			mlt_properties_set_int( producer_props, "meta.media.width", self->current_width );
			mlt_properties_set_int( producer_props, "meta.media.height", self->current_height );
			mlt_properties_set_int( producer_props, "_disable_exif", disable_exif );
			mlt_events_unblock( producer_props, NULL );
		}
		else
		{
			delete qimage;
			self->qimage = NULL;
		}
	}

	// Set width/height of frame
	mlt_properties_set_int( properties, "width", self->current_width );
	mlt_properties_set_int( properties, "height", self->current_height );

	return image_idx;
}

void refresh_image( producer_qimage self, mlt_frame frame, mlt_image_format format, int width, int height )
{
	// Obtain properties of frame and producer
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = &self->parent;

	// Get index and qimage
	int image_idx = refresh_qimage( self, frame );

	// optimization for subsequent iterations on single pictur
	if ( image_idx != self->image_idx || width != self->current_width || height != self->current_height )
		self->current_image = NULL;

	// If we have a qimage and need a new scaled image
	if ( self->qimage && ( !self->current_image || ( format != mlt_image_none && format != mlt_image_glsl && format != self->format ) ) )
	{
		QString interps = mlt_properties_get( properties, "rescale.interp" );
		bool interp = ( interps != "nearest" ) && ( interps != "none" );
		QImage *qimage = static_cast<QImage*>( self->qimage );

		// Note - the original qimage is already safe and ready for destruction
		if ( qimage->depth() == 1 )
		{
			QImage temp = qimage->convertToFormat( QImage::Format_RGB32 );
			delete qimage;
			qimage = new QImage( temp );
			self->qimage = qimage;
		}
		QImage scaled = interp? qimage->scaled( QSize( width, height ) ) :
			qimage->scaled( QSize(width, height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
		int has_alpha = scaled.hasAlphaChannel();

		// Store width and height
		self->current_width = width;
		self->current_height = height;

		// Allocate/define image
		int dst_stride = width * ( has_alpha ? 4 : 3 );
		int image_size = dst_stride * ( height + 1 );
		self->current_image = ( uint8_t * )mlt_pool_alloc( image_size );
		self->current_alpha = NULL;
		self->format = has_alpha ? mlt_image_rgb24a : mlt_image_rgb24;

		// Copy the image
		int y = self->current_height + 1;
		uint8_t *dst = self->current_image;
		while ( --y )
		{
			QRgb *src = (QRgb*) scaled.scanLine( self->current_height - y );
			int x = self->current_width + 1;
			while ( --x )
			{
				*dst++ = qRed(*src);
				*dst++ = qGreen(*src);
				*dst++ = qBlue(*src);
				if ( has_alpha ) *dst++ = qAlpha(*src);
				++src;
			}
		}

		// Convert image to requested format
		if ( format != mlt_image_none && format != mlt_image_glsl && format != self->format )
		{
			uint8_t *buffer = NULL;

			// First, set the image so it can be converted when we get it
			mlt_frame_replace_image( frame, self->current_image, self->format, width, height );
			mlt_frame_set_image( frame, self->current_image, image_size, mlt_pool_release );
			self->format = format;

			// get_image will do the format conversion
			mlt_frame_get_image( frame, &buffer, &format, &width, &height, 0 );

			// cache copies of the image and alpha buffers
			if ( buffer )
			{
				image_size = mlt_image_format_size( format, width, height, NULL );
				self->current_image = (uint8_t*) mlt_pool_alloc( image_size );
				memcpy( self->current_image, buffer, image_size );
			}
			if ( ( buffer = mlt_frame_get_alpha( frame ) ) )
			{
				self->current_alpha = (uint8_t*) mlt_pool_alloc( width * height );
				memcpy( self->current_alpha, buffer, width * height );
			}
		}

		// Update the cache
		mlt_cache_item_close( self->image_cache );
		mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.image", self->current_image, image_size, mlt_pool_release );
		self->image_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.image" );
		self->image_idx = image_idx;
		mlt_cache_item_close( self->alpha_cache );
		self->alpha_cache = NULL;
		if ( self->current_alpha )
		{
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.alpha", self->current_alpha, width * height, mlt_pool_release );
			self->alpha_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.alpha" );
		}
	}

	// Set width/height of frame
	mlt_properties_set_int( properties, "width", self->current_width );
	mlt_properties_set_int( properties, "height", self->current_height );
}

extern void make_tempfile( producer_qimage self, const char *xml )
{
	// Generate a temporary file for the svg
	QTemporaryFile tempFile( "mlt.XXXXXX" );

	tempFile.setAutoRemove( false );
	if ( tempFile.open() )
	{
		// Write the svg into the temp file
		char *fullname = tempFile.fileName().toUtf8().data();

		// Strip leading crap
		while ( xml[0] != '<' )
			xml++;

		qint64 remaining_bytes = strlen( xml );
		while ( remaining_bytes > 0 )
			remaining_bytes -= tempFile.write( xml + strlen( xml ) - remaining_bytes, remaining_bytes );
		tempFile.close();

		mlt_properties_set( self->filenames, "0", fullname );

		mlt_properties_set_data( MLT_PRODUCER_PROPERTIES( &self->parent ), "__temporary_file__",
			fullname, 0, ( mlt_destructor )unlink, NULL );
	}
}

} // extern "C"
