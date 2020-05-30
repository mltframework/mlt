/*
 * qimage_wrapper.cpp -- a QT/QImage based producer for MLT
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
#include <QDebug>

#ifdef USE_EXIF
#include <libexif/exif-data.h>
#endif

#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {

#include <framework/mlt_pool.h>
#include <framework/mlt_cache.h>

#ifdef USE_KDE4
static KComponentData *instance = 0L;
#endif

static void qimage_delete( void *image )
{
	delete static_cast<QImage*>( image );
#if defined(USE_KDE4)
	delete instance;
	instance = 0L;
#endif

}

/// Returns false if this is animated.
int init_qimage(const char *filename, mlt_producer producer)
{
	// Ensure we have a Qt App
	if ( !createQApplicationIfNeeded( MLT_PRODUCER_SERVICE(producer) ) )
		return 0;

	QImageReader reader;
	reader.setDecideFormatFromContent( true );
	reader.setFileName( filename );

	// Ensure image format is readable
	if ( !reader.canRead() )
		return 0;

	// Animated images cannot be properly handled here
	if ( reader.imageCount() > 1 )
		return 0;

	// Get image size without reading image, should be supported in most formats
	QSize imageSize = reader.size();
	if ( !imageSize.isValid() )
	{
		// Cannot determine image size, fully load image
		QImage img = reader.read();
		imageSize = img.size();
	}
	if ( imageSize.width() <= 0)
	{
		// Invalid image
		return 0;
	}
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
	mlt_properties_set_int( producer_props, "meta.media.width", imageSize.width() );
	mlt_properties_set_int( producer_props, "meta.media.height", imageSize.height() );

#ifdef USE_KDE4
	if ( !instance ) {
	    instance = new KComponentData( "qimage_prod" );
	}
#endif
	return 1;
}

static QImage reorient_with_exif( producer_qimage self, int image_idx, QImage qimage )
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
		  return qimage.transformed( matrix );
	}
#endif
	return qimage;
}

void refresh_image( producer_qimage self, mlt_frame frame, mlt_image_format format, int width, int height )
{
	// Obtain properties of frame and producer
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = &self->parent;
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Check if user wants us to reload the image
	bool request_reload = false;
	if ( mlt_properties_get_int( producer_props, "force_reload" ) )
	{
		request_reload = true;
		mlt_properties_set_int( producer_props, "force_reload", 0 );
	}

	// Get the original position of this frame
	mlt_position position = mlt_frame_original_position( frame );
	position += mlt_producer_get_in( producer );

	// Image index
	int image_idx = ( int )floor( ( double )position / self->ttl ) % self->count;
	int disable_exif = mlt_properties_get_int( producer_props, "disable_exif" );

	// Check if source image should be reloaded
	if ( image_idx != self->qimage_idx || mlt_properties_get_int( producer_props, "_disable_exif" ) != disable_exif )
	{
		request_reload = true;
	}

	if ( request_reload )
	{
		if ( self->qimage )
		{
			self->qimage = NULL;
		}
	}
	else if ( self->current_image )
	{
		// Check if we can reuse the existing scaled image
		if ( image_idx != self->image_idx || width != self->current_width || height != self->current_height )
		{
			self->current_image = NULL;
		}
		else
		{
			// Image is ready, nothing else to do (just set width/height
			mlt_properties_set_int( properties, "width", self->current_width );
			mlt_properties_set_int( properties, "height", self->current_height );
			mlt_properties_set_int( properties, "format", self->format );
			return;
		}
	}

	// Holds the original image read from file
	QImage sourceImage;

	// Reload image from file
	if ( self->qimage == NULL )
	{
		self->current_image = NULL;
		QImageReader reader;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
		// Use Qt's orientation detection
		reader.setAutoTransform(!disable_exif);
#endif
		reader.setDecideFormatFromContent( true );
		reader.setFileName( QString::fromUtf8( mlt_properties_get_value( self->filenames, image_idx ) ) );
		sourceImage = reader.read();
		if ( sourceImage.isNull( ) )
		{
			return;
		}
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
		// Read the exif value for this file
		if ( !disable_exif )
		{
			sourceImage = reorient_with_exif( self, image_idx, sourceImage );
		}
#endif
		if (self->ttl > 1)
		{
			// Register qimage for destruction and reuse
			self->qimage = new QImage(sourceImage);
			mlt_cache_item_close( self->qimage_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage", self->qimage, 0, ( mlt_destructor )qimage_delete );
			self->qimage_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage" );
		}
		self->qimage_idx = image_idx;

		// Store the width/height of the qimage
		self->current_width = sourceImage.width( );
		self->current_height = sourceImage.height( );

		mlt_events_block( producer_props, NULL );
		mlt_properties_set_int( producer_props, "meta.media.width", self->current_width );
		mlt_properties_set_int( producer_props, "meta.media.height", self->current_height );
		mlt_properties_set_int( producer_props, "_disable_exif", disable_exif );
		mlt_events_unblock( producer_props, NULL );
	}
	else
	{
		sourceImage = *static_cast<QImage*>( self->qimage );
	}

	int has_alpha = sourceImage.hasAlphaChannel();
	if ( has_alpha )
	{
		format = mlt_image_rgb24a;
	}

	// If we have a qimage and need a new scaled image
	if ( !self->current_image || ( format != mlt_image_none && format != mlt_image_glsl && format != self->format ) )
	{
		QString interps = mlt_properties_get( properties, "rescale.interp" );
		bool interp = ( interps != "nearest" ) && ( interps != "none" );
		QImage::Format qimageFormat = has_alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32;

		// Note - the original qimage is already safe and ready for destruction
		if ( sourceImage.format() != qimageFormat )
		{
			sourceImage = sourceImage.convertToFormat( qimageFormat );
			mlt_cache_item_close( self->qimage_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage", new QImage(sourceImage), 0, ( mlt_destructor )qimage_delete );
			self->qimage_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage" );
		}

		// Scale image to requested size
		QImage scaled = interp? sourceImage.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) :
			sourceImage.scaled( width, height );

		// Store width and height
		self->current_width = width;
		self->current_height = height;

		// Allocate/define image
        int image_size;
#if QT_VERSION >= 0x050200
		if ( has_alpha )
		{
			image_size = 4 * width * height;
			self->format = mlt_image_rgb24a;
			scaled = scaled.convertToFormat( QImage::Format_RGBA8888 );
		}
		else
		{
			image_size = 3 * width * height;
			self->format = mlt_image_rgb24;
#if QT_VERSION >= 0x051300
			scaled.convertTo( QImage::Format_RGB888 );
#else
			scaled = scaled.convertToFormat( QImage::Format_RGB888 );
#endif
		}
		self->current_image = ( uint8_t * )mlt_pool_alloc( image_size );
		memcpy( self->current_image, scaled.constBits(), image_size);
#else
		self->format = has_alpha ? mlt_image_rgb24a : mlt_image_rgb24;
		image_size = mlt_image_format_size( self->format, self->current_width, self->current_height, NULL );
		self->current_image = ( uint8_t * )mlt_pool_alloc( image_size );

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
#endif
		self->current_alpha = NULL;
		self->alpha_size = 0;

		// Convert image to requested format - rgba images are kept in original format for faster compositing
		if ( self->ttl > 1 && format != mlt_image_none && format != mlt_image_glsl && format != self->format && !has_alpha )
		{
			uint8_t *buffer = NULL;
			// First, set the image so it can be converted when we get it
			mlt_frame_set_image( frame, self->current_image, image_size, mlt_pool_release );

			// Set width/height of frame
			mlt_properties_set_int( properties, "width", self->current_width );
			mlt_properties_set_int( properties, "height", self->current_height );
			mlt_properties_set_int( properties, "format", self->format );

			// Do the format conversion
			mlt_frame_get_image( frame, &buffer, &format, &width, &height, 0 );

			// cache copies of the image and alpha buffers
			if ( buffer )
			{
				self->current_width = width;
				self->current_height = height;
				self->format = format;
				image_size = mlt_image_format_size( format, width, height, NULL );
				self->current_image = (uint8_t*) mlt_pool_alloc( image_size );
				memcpy( self->current_image, buffer, image_size );
			}
			if ( ( buffer = (uint8_t*) mlt_properties_get_data( properties, "alpha", &self->alpha_size ) ) )
			{
                if ( !self->alpha_size )
                    self->alpha_size = self->current_width * self->current_height;
				self->current_alpha = (uint8_t*) mlt_pool_alloc( self->alpha_size );
				memcpy( self->current_alpha, buffer, self->alpha_size );
			}
		}

		if ( self->ttl > 1 )
		{
			// Update the cache
			mlt_cache_item_close( self->image_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.image", self->current_image, image_size, mlt_pool_release );
			self->image_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.image" );
			self->image_idx = image_idx;
			mlt_cache_item_close( self->alpha_cache );
			self->alpha_cache = NULL;
			if ( self->current_alpha )
			{
				mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.alpha", self->current_alpha, self->alpha_size, mlt_pool_release );
				self->alpha_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.alpha" );
			}
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
		QByteArray fullname = tempFile.fileName().toUtf8();

		// Strip leading crap
		while ( xml[0] != '<' )
			xml++;

		qint64 remaining_bytes = strlen( xml );
		while ( remaining_bytes > 0 )
			remaining_bytes -= tempFile.write( xml + strlen( xml ) - remaining_bytes, remaining_bytes );
		tempFile.close();

		mlt_properties_set( self->filenames, "0", fullname.data() );

		mlt_properties_set_data( MLT_PRODUCER_PROPERTIES( &self->parent ), "__temporary_file__",
			fullname.data(), 0, ( mlt_destructor )unlink, NULL );
	}
}

int load_sequence_sprintf(producer_qimage self, mlt_properties properties, const char *filename)
{
	int result = 0;

	// Obtain filenames with pattern
	if (filename && strchr(filename, '%')) {
		// handle picture sequences
		int i = mlt_properties_get_int( properties, "begin" );
		int keyvalue = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
		for (int gap = 0; gap < 100;) {
			QString full = QString::asprintf(filename, i++);
			if (QFile::exists(full)) {
				QString key = QString::asprintf("%d", keyvalue++);
				mlt_properties_set(self->filenames, key.toLatin1().constData(), full.toUtf8().constData());
				gap = 0;
			} else {
				gap ++;
			}
		}
#else
		char full[1024];
		char key[ 50 ];

		for (int gap = 0; gap < 100;) {
			struct stat buf;
			snprintf(full, 1023, filename, i++);
			if (stat(full, &buf ) == 0) {
				sprintf(key, "%d", keyvalue ++);
				mlt_properties_set(self->filenames, key, full);
				gap = 0;
			} else {
				gap ++;
			}
		}
#endif
		if (mlt_properties_count(self->filenames) > 0) {
			mlt_properties_set_int(properties, "ttl", 1);
			result = 1;
		}
	}
	return result;
}

} // extern "C"
