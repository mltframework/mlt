/*
 * qimage_wrapper.cpp -- a Qt/QImage based producer for MLT
 * Copyright (C) 2006-2022 Meltytech, LLC
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
#include <QFileInfo>
#include <QDir>
#include <QMutex>
#include <QtEndian>
#include <QTemporaryFile>
#include <QImageReader>
#include <QMovie>

#ifdef USE_EXIF
#include <QTransform>
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

/// Returns frame count or 0 on error
int init_qimage(mlt_producer producer, const char *filename)
{
	if (!createQApplicationIfNeeded(MLT_PRODUCER_SERVICE(producer))) {
		return 0;
	}

	QImageReader reader;
	reader.setDecideFormatFromContent( true );
	reader.setFileName( filename );
	if ( reader.canRead() && reader.imageCount() > 1 ) {
		return reader.format() == "webp" ? reader.imageCount() : 0;
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
		  QTransform matrix;

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

int refresh_qimage( producer_qimage self, mlt_frame frame, int enable_caching )
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

	// Get the original position of this frame
	mlt_position position = mlt_frame_original_position( frame );
	position += mlt_producer_get_in( producer );

	// Image index
	int image_idx = ( int )floor( ( double )position / mlt_properties_get_int( producer_props, "ttl" ) ) % self->count;

	int disable_exif = mlt_properties_get_int( producer_props, "disable_exif" );

	if ( image_idx != self->qimage_idx )
	{
		self->qimage = NULL;
	}
	if ( !self->qimage || mlt_properties_get_int( producer_props, "_disable_exif" ) != disable_exif )
	{
		self->current_image = NULL;
		QImageReader reader;
		QImage *qimage;

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
		// Use Qt's orientation detection
		reader.setAutoTransform(!disable_exif);
#endif
		QString filename = QString::fromUtf8( mlt_properties_get_value( self->filenames, image_idx ) );
		if (filename.isEmpty()) {
			filename = QString::fromUtf8(mlt_properties_get(producer_props, "resource"));
		}

		// First try to detect the file type based on the content
		// in case the file extension is incorrect.
		reader.setDecideFormatFromContent( true );
		reader.setFileName( filename );
		if (reader.imageCount() > 1) {
			QMovie movie(filename);
			movie.setCacheMode(QMovie::CacheAll);
			movie.jumpToFrame(image_idx);
			qimage = new QImage(movie.currentImage());
		} else {
			qimage = new QImage(reader.read());
		}
		if ( qimage->isNull( ) )
		{
			mlt_log_info( MLT_PRODUCER_SERVICE( &self->parent ), "QImage retry: %d - %s\n",
					reader.error(), reader.errorString().toLatin1().data() );
			delete qimage;
			// If detection fails, try a more comprehensive detection including file extension
			reader.setDecideFormatFromContent( false );
			reader.setFileName( filename );
			qimage = new QImage( reader.read() );
			if ( qimage->isNull( ) )
			{
				mlt_log_info( MLT_PRODUCER_SERVICE( &self->parent ), "QImage fail: %d - %s\n",
						reader.error(), reader.errorString().toLatin1().data() );
			}
		}
		self->qimage = qimage;

		if ( !qimage->isNull( ) )
		{
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
			// Read the exif value for this file
			if ( !disable_exif )
			{
				qimage = reorient_with_exif( self, image_idx, qimage );
				self->qimage = qimage;
			}
#endif
			if ( enable_caching )
			{
				// Register qimage for destruction and reuse
				mlt_cache_item_close( self->qimage_cache );
				mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage", qimage, 0, ( mlt_destructor )qimage_delete );
				self->qimage_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage" );
			}
			else
			{
				// Ensure original image data will be deleted
				mlt_properties_set_data( producer_props, "qimage.qimage", qimage, 0, ( mlt_destructor )qimage_delete, NULL );
			}
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

void refresh_image( producer_qimage self, mlt_frame frame, mlt_image_format format, int width, int height, int enable_caching )
{
	// Obtain properties of frame and producer
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = &self->parent;

	// Get index and qimage
	int image_idx = refresh_qimage( self, frame, enable_caching );

	// optimization for subsequent iterations on single picture
	if (!enable_caching || image_idx != self->image_idx || width != self->current_width || height != self->current_height )
		self->current_image = NULL;

	// If we have a qimage and need a new scaled image
	if ( self->qimage && ( !self->current_image || ( format != mlt_image_none && format != mlt_image_movit && format != self->format ) ) )
	{
		QString interps = mlt_properties_get( properties, "consumer.rescale" );
		bool interp = ( interps != "nearest" ) && ( interps != "none" );
		QImage *qimage = static_cast<QImage*>( self->qimage );
		int has_alpha = qimage->hasAlphaChannel();
		QImage::Format qimageFormat = has_alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32;

		// Note - the original qimage is already safe and ready for destruction
		if ( enable_caching && qimage->format() != qimageFormat )
		{
			QImage temp = qimage->convertToFormat( qimageFormat );
			qimage = new QImage( temp );
			self->qimage = qimage;
			mlt_cache_item_close( self->qimage_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage", qimage, 0, ( mlt_destructor )qimage_delete );
			self->qimage_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.qimage" );
		}
		QImage scaled = interp? qimage->scaled( QSize( width, height ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) :
			qimage->scaled( QSize(width, height) );

		// Store width and height
		self->current_width = width;
		self->current_height = height;

		// Allocate/define image
		self->current_alpha = NULL;
		self->alpha_size = 0;

		// Convert scaled image to target format (it might be premultiplied after scaling).
		scaled = scaled.convertToFormat( qimageFormat );

		// Copy the image
		int image_size;
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
		if ( has_alpha )
		{
			self->format = mlt_image_rgba;
			scaled = scaled.convertToFormat( QImage::Format_RGBA8888 );
			image_size = mlt_image_format_size(self->format, width, height, NULL);
			self->current_image = ( uint8_t * )mlt_pool_alloc( image_size );
			memcpy( self->current_image, scaled.constBits(), scaled.sizeInBytes());
		}
		else
		{
			self->format = mlt_image_rgb;
			scaled = scaled.convertToFormat( QImage::Format_RGB888 );
			image_size = mlt_image_format_size(self->format, width, height, NULL);
			self->current_image = ( uint8_t * )mlt_pool_alloc( image_size );
			for (int y = 0; y < height; y++) {
				QRgb *values = reinterpret_cast<QRgb *>(scaled.scanLine(y));
				memcpy( &self->current_image[3 * y * width], values, 3 * width);
			}
		}
#else
		self->format = has_alpha? mlt_image_rgba : mlt_image_rgb;
		image_size = mlt_image_format_size( self->format, self->current_width, self->current_height, NULL );
		self->current_image = ( uint8_t * )mlt_pool_alloc( image_size );
		int y = self->current_height + 1;
		uint8_t *dst = self->current_image;
		if (has_alpha) {
			while ( --y )
			{
				QRgb *src = (QRgb*) scaled.scanLine( self->current_height - y );
				int x = self->current_width + 1;
				while ( --x )
				{
					*dst++ = qRed(*src);
					*dst++ = qGreen(*src);
					*dst++ = qBlue(*src);
					*dst++ = qAlpha(*src);
					++src;
				}
			}
		} else {
			while ( --y )
			{
				QRgb *src = (QRgb*) scaled.scanLine( self->current_height - y );
				int x = self->current_width + 1;
				while ( --x )
				{
					*dst++ = qRed(*src);
					*dst++ = qGreen(*src);
					*dst++ = qBlue(*src);
					++src;
				}
			}
		}
#endif

		// Convert image to requested format
		if ( format != mlt_image_none && format != mlt_image_movit && format != self->format && enable_caching )
		{
			uint8_t *buffer = NULL;

			// First, set the image so it can be converted when we get it
			mlt_frame_replace_image( frame, self->current_image, self->format, width, height );
			mlt_frame_set_image( frame, self->current_image, image_size, mlt_pool_release );

			// get_image will do the format conversion
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
			if ( ( buffer = (uint8_t*) mlt_frame_get_alpha_size(frame, &self->alpha_size) ) )
			{
                if ( !self->alpha_size )
                    self->alpha_size = self->current_width * self->current_height;
				self->current_alpha = (uint8_t*) mlt_pool_alloc( self->alpha_size );
				memcpy( self->current_alpha, buffer, self->alpha_size );
			}
		}

		self->image_idx = image_idx;
		if ( enable_caching )
		{
			// Update the cache
			mlt_cache_item_close( self->image_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qimage.image", self->current_image, image_size, mlt_pool_release );
			self->image_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qimage.image" );
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

int load_folder( producer_qimage self, const char *filename )
{
	int result = 0;

	// Obtain filenames within folder
	if ( strstr( filename, "/.all." ) != NULL )
	{
		mlt_properties filename_property = self->filenames;
		QFileInfo info( filename );
		QDir dir = info.absoluteDir();
		QStringList filters = {QString( "*.%1" ).arg( info.suffix() )};
		QStringList files = dir.entryList( filters, QDir::Files, QDir::Name );
		int key;
		for ( auto &f : files ) {
			key = mlt_properties_count( filename_property );
			mlt_properties_set_string( filename_property, QString::number( key ).toLatin1(), dir.absoluteFilePath( f ).toUtf8().constData() );
		}
		result = 1;
    }
	return result;
}

} // extern "C"
