/*
 * producer_pixbuf.c -- raster image loader based upon gdk-pixbuf
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_cache.h>
#include <framework/mlt_log.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "config.h"

#ifdef USE_EXIF
#include <libexif/exif-data.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct producer_pixbuf_s *producer_pixbuf;

struct producer_pixbuf_s
{
	struct mlt_producer_s parent;

	// File name list
	mlt_properties filenames;
	int count;
	int image_idx;
	int pixbuf_idx;
	int width;
	int height;
	int alpha;
	uint8_t *image;
	mlt_cache_item image_cache;
	pthread_mutex_t mutex;
};

static void load_filenames( producer_pixbuf this, mlt_properties producer_properties );
static void refresh_image( producer_pixbuf this, mlt_frame frame, int width, int height );
static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_pixbuf_init( char *filename )
{
	producer_pixbuf this = calloc( sizeof( struct producer_pixbuf_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		// Get the properties interface
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( &this->parent );
	
		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		// Set the default properties
		mlt_properties_set( properties, "resource", filename );
		mlt_properties_set_int( properties, "ttl", 25 );
		mlt_properties_set_int( properties, "aspect_ratio", 1 );
		mlt_properties_set_int( properties, "progressive", 1 );

		// Validate the resource
		if ( filename )
			load_filenames( this, properties );
		if ( this->count )
		{
			mlt_frame frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );
			if ( frame )
			{
				mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
				pthread_mutex_init( &this->mutex, NULL );
				mlt_properties_set_data( frame_properties, "producer_pixbuf", this, 0, NULL, NULL );
				mlt_frame_set_position( frame, mlt_producer_position( producer ) );
				mlt_properties_set_position( frame_properties, "pixbuf_position", mlt_producer_position( producer ) );
				refresh_image( this, frame, 0, 0 );
				mlt_frame_close( frame );
			}
		}
		if ( this->width == 0 )
		{
			producer_close( producer );
			producer = NULL;
		}
		return producer;
	}
	free( this );
	return NULL;
}

static void load_filenames( producer_pixbuf this, mlt_properties producer_properties )
{
	char *filename = mlt_properties_get( producer_properties, "resource" );
	this->filenames = mlt_properties_new( );

	// Read xml string
	if ( strstr( filename, "<svg" ) )
	{
		// Generate a temporary file for the svg
		char fullname[ 1024 ] = "/tmp/mlt.XXXXXX";
		int fd = g_mkstemp( fullname );

		if ( fd > -1 )
		{
			// Write the svg into the temp file
			ssize_t remaining_bytes;
			char *xml = filename;
			
			// Strip leading crap
			while ( xml[0] != '<' )
				xml++;
			
			remaining_bytes = strlen( xml );
			while ( remaining_bytes > 0 )
				remaining_bytes -= write( fd, xml + strlen( xml ) - remaining_bytes, remaining_bytes );
			close( fd );

			mlt_properties_set( this->filenames, "0", fullname );

			// Teehe - when the producer closes, delete the temp file and the space allo
			mlt_properties_set_data( producer_properties, "__temporary_file__", fullname, 0, ( mlt_destructor )unlink, NULL );
		}
	}
	// Obtain filenames
	else if ( strchr( filename, '%' ) != NULL )
	{
		// handle picture sequences
		int i = mlt_properties_get_int( producer_properties, "begin" );
		int gap = 0;
		char full[1024];
		int keyvalue = 0;
		char key[ 50 ];

		while ( gap < 100 )
		{
			struct stat buf;
			snprintf( full, 1023, filename, i ++ );
			if ( stat( full, &buf ) == 0 )
			{
				sprintf( key, "%d", keyvalue ++ );
				mlt_properties_set( this->filenames, key, full );
				gap = 0;
			}
			else
			{
				gap ++;
			}
		}
		if ( mlt_properties_count( this->filenames ) > 0 )
			mlt_properties_set_int( producer_properties, "ttl", 1 );
	}
	else if ( strstr( filename, "/.all." ) != NULL )
	{
		char wildcard[ 1024 ];
		char *dir_name = strdup( filename );
		char *extension = strrchr( dir_name, '.' );

		*( strstr( dir_name, "/.all." ) + 1 ) = '\0';
		sprintf( wildcard, "*%s", extension );

		mlt_properties_dir_list( this->filenames, dir_name, wildcard, 1 );

		free( dir_name );
	}
	else
	{
		mlt_properties_set( this->filenames, "0", filename );
	}

	this->count = mlt_properties_count( this->filenames );
}

static void refresh_image( producer_pixbuf this, mlt_frame frame, int width, int height )
{
	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the producer
	mlt_producer producer = &this->parent;

	// Obtain properties of producer
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Obtain the cache flag and structure
	int use_cache = mlt_properties_get_int( producer_props, "cache" );
	mlt_properties cache = mlt_properties_get_data( producer_props, "_cache", NULL );
	int update_cache = 0;

	// restore GdkPixbuf
	pthread_mutex_lock( &this->mutex );
	mlt_cache_item pixbuf_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "pixbuf.pixbuf" );
	GdkPixbuf *pixbuf = mlt_cache_item_data( pixbuf_cache, NULL );
	GError *error = NULL;

	// restore scaled image
	this->image_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "pixbuf.image" );
	this->image = mlt_cache_item_data( this->image_cache, NULL );

	// Check if user wants us to reload the image
	if ( mlt_properties_get_int( producer_props, "force_reload" ) )
	{
		pixbuf = NULL;
		this->image = NULL;
		mlt_properties_set_int( producer_props, "force_reload", 0 );
	}

	// Get the time to live for each frame
	double ttl = mlt_properties_get_int( producer_props, "ttl" );

	// Get the original position of this frame
	mlt_position position = mlt_properties_get_position( properties, "pixbuf_position" );
	position += mlt_producer_get_in( producer );

	// Image index
	int image_idx = ( int )floor( ( double )position / ttl ) % this->count;

	// Key for the cache
	char image_key[ 10 ];
	sprintf( image_key, "%d", image_idx );

	pthread_mutex_lock( &g_mutex );

	// Check if the frame is already loaded
	if ( use_cache )
	{
		if ( cache == NULL )
		{
			cache = mlt_properties_new( );
			mlt_properties_set_data( producer_props, "_cache", cache, 0, ( mlt_destructor )mlt_properties_close, NULL );
		}

		mlt_frame cached = mlt_properties_get_data( cache, image_key, NULL );

		if ( cached )
		{
			this->image_idx = image_idx;
			mlt_properties cached_props = MLT_FRAME_PROPERTIES( cached );
			this->width = mlt_properties_get_int( cached_props, "width" );
			this->height = mlt_properties_get_int( cached_props, "height" );
			mlt_properties_set_int( producer_props, "_real_width", mlt_properties_get_int( cached_props, "real_width" ) );
			mlt_properties_set_int( producer_props, "_real_height", mlt_properties_get_int( cached_props, "real_height" ) );
			this->image = mlt_properties_get_data( cached_props, "image", NULL );
			this->alpha = mlt_properties_get_int( cached_props, "alpha" );

			if ( width != 0 && ( width != this->width || height != this->height ) )
				this->image = NULL;
		}
	}
	int disable_exif = mlt_properties_get_int( producer_props, "disable_exif" );
	
    // optimization for subsequent iterations on single picture
	if ( width != 0 && ( image_idx != this->image_idx || width != this->width || height != this->height ) )
		this->image = NULL;
	if ( image_idx != this->pixbuf_idx )
		pixbuf = NULL;
	mlt_log_debug( MLT_PRODUCER_SERVICE( producer ), "image %p pixbuf %p idx %d image_idx %d pixbuf_idx %d width %d\n",
		this->image, pixbuf, image_idx, this->image_idx, this->pixbuf_idx, width );
	if ( !pixbuf || mlt_properties_get_int( producer_props, "_disable_exif" ) != disable_exif )
	{
		this->image = NULL;
		pixbuf = gdk_pixbuf_new_from_file( mlt_properties_get_value( this->filenames, image_idx ), &error );

		if ( pixbuf )
		{
#ifdef USE_EXIF
			// Read the exif value for this file
			if ( disable_exif == 0) {
				ExifData *d = exif_data_new_from_file( mlt_properties_get_value( this->filenames, image_idx ) );
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
					GdkPixbuf *processed = NULL;
					GdkPixbufRotation matrix = GDK_PIXBUF_ROTATE_NONE;

					// Rotate image according to exif data
					switch ( exif_orientation ) {
					  case 2:
					      processed = gdk_pixbuf_flip ( pixbuf, TRUE );
					      break;
					  case 3:
					      matrix = GDK_PIXBUF_ROTATE_UPSIDEDOWN;
					      processed = pixbuf;
					      break;
					  case 4:
					      processed = gdk_pixbuf_flip ( pixbuf, FALSE );
					      break;
					  case 5:
					      matrix = GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE;
					      processed = gdk_pixbuf_flip ( pixbuf, TRUE );
					      break;
					  case 6:
					      matrix = GDK_PIXBUF_ROTATE_CLOCKWISE;
					      processed = pixbuf;
					      break;
					  case 7:
					      matrix = GDK_PIXBUF_ROTATE_CLOCKWISE;
					      processed = gdk_pixbuf_flip ( pixbuf, TRUE );
					      break;
					  case 8:
					      matrix = GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE;
					      processed = pixbuf;
					      break;
					}
					if ( processed )
						pixbuf = gdk_pixbuf_rotate_simple( processed, matrix );
				}
			}
#endif

			// Register this pixbuf for destruction and reuse
			mlt_cache_item_close( pixbuf_cache );
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "pixbuf.pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref );
			pixbuf_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "pixbuf.pixbuf" );
			this->pixbuf_idx = image_idx;

			mlt_events_block( producer_props, NULL );
			mlt_properties_set_int( producer_props, "_real_width", gdk_pixbuf_get_width( pixbuf ) );
			mlt_properties_set_int( producer_props, "_real_height", gdk_pixbuf_get_height( pixbuf ) );
			mlt_properties_set_int( producer_props, "_disable_exif", disable_exif );
			mlt_events_unblock( producer_props, NULL );

			// Store the width/height of the pixbuf temporarily
			this->width = gdk_pixbuf_get_width( pixbuf );
			this->height = gdk_pixbuf_get_height( pixbuf );
		}
	}

	// If we have a pixbuf and we need an image
	if ( pixbuf && width > 0 && !this->image )
	{
		char *interps = mlt_properties_get( properties, "rescale.interp" );
		int interp = GDK_INTERP_BILINEAR;

		if ( strcmp( interps, "nearest" ) == 0 )
			interp = GDK_INTERP_NEAREST;
		else if ( strcmp( interps, "tiles" ) == 0 )
			interp = GDK_INTERP_TILES;
		else if ( strcmp( interps, "hyper" ) == 0 )
			interp = GDK_INTERP_HYPER;

		// Note - the original pixbuf is already safe and ready for destruction
		pixbuf = gdk_pixbuf_scale_simple( pixbuf, width, height, interp );

		// Store width and height
		this->width = width;
		this->height = height;
		
		// Allocate/define image
		this->alpha = gdk_pixbuf_get_has_alpha( pixbuf );
		int src_stride = gdk_pixbuf_get_rowstride( pixbuf );
		int dst_stride = this->width * ( this->alpha ? 4 : 3 );
		int image_size = dst_stride * ( height + 1 );
		this->image = mlt_pool_alloc( image_size );

		if ( src_stride != dst_stride )
		{
			int y = this->height;
			uint8_t *src = gdk_pixbuf_get_pixels( pixbuf );
			uint8_t *dst = this->image;
			while ( y-- )
			{
				memcpy( dst, src, dst_stride );
				dst += dst_stride;
				src += src_stride;
			}
		}
		else
		{
			memcpy( this->image, gdk_pixbuf_get_pixels( pixbuf ), src_stride * height );
		}
		if ( !use_cache )
			mlt_cache_item_close( this->image_cache );
		mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "pixbuf.image", this->image, image_size, mlt_pool_release );
		this->image_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "pixbuf.image" );
		this->image_idx = image_idx;

		// Finished with pixbuf now
		g_object_unref( pixbuf );

		// Ensure we update the cache when we need to
		update_cache = use_cache;
	}

	// release references no longer needed
	mlt_cache_item_close( pixbuf_cache );
	if ( width == 0 )
	{
		pthread_mutex_unlock( &this->mutex );
		mlt_cache_item_close( this->image_cache );
	}

	// Set width/height of frame
	mlt_properties_set_int( properties, "width", this->width );
	mlt_properties_set_int( properties, "height", this->height );
	mlt_properties_set_int( properties, "real_width", mlt_properties_get_int( producer_props, "_real_width" ) );
	mlt_properties_set_int( properties, "real_height", mlt_properties_get_int( producer_props, "_real_height" ) );

	if ( update_cache )
	{
		mlt_frame cached = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );
		mlt_properties cached_props = MLT_FRAME_PROPERTIES( cached );
		mlt_properties_set_int( cached_props, "width", this->width );
		mlt_properties_set_int( cached_props, "height", this->height );
		mlt_properties_set_int( cached_props, "real_width", mlt_properties_get_int( producer_props, "_real_width" ) );
		mlt_properties_set_int( cached_props, "real_height", mlt_properties_get_int( producer_props, "_real_height" ) );
		mlt_properties_set_data( cached_props, "image", this->image, this->width * ( this->alpha ? 4 : 3 ) * this->height, mlt_pool_release, NULL );
		mlt_properties_set_int( cached_props, "alpha", this->alpha );
		mlt_properties_set_data( cache, image_key, cached, 0, ( mlt_destructor )mlt_frame_close, NULL );
	}

	pthread_mutex_unlock( &g_mutex );
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	
	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the producer for this frame
	producer_pixbuf this = mlt_properties_get_data( properties, "producer_pixbuf", NULL );

	*width = mlt_properties_get_int( properties, "rescale_width" );
	*height = mlt_properties_get_int( properties, "rescale_height" );

	mlt_service_lock( MLT_PRODUCER_SERVICE( &this->parent ) );

	// Refresh the image
	refresh_image( this, frame, *width, *height );

	// Get width and height (may have changed during the refresh)
	*width = this->width;
	*height = this->height;

	// NB: Cloning is necessary with this producer (due to processing of images ahead of use)
	// The fault is not in the design of mlt, but in the implementation of the pixbuf producer...
	if ( this->image )
	{
		// Clone the image
		int image_size = this->width * this->height * ( this->alpha ? 4 :3 );
		uint8_t *image_copy = mlt_pool_alloc( image_size );
		memcpy( image_copy, this->image, image_size );
		// Now update properties so we free the copy after
		mlt_properties_set_data( properties, "image", image_copy, image_size, mlt_pool_release, NULL );
		// We're going to pass the copy on
		*buffer = image_copy;
		*format = this->alpha ? mlt_image_rgb24a : mlt_image_rgb24;
		mlt_log_debug( MLT_PRODUCER_SERVICE( &this->parent ), "%dx%d (%s)\n",
			this->width, this->height, mlt_image_format_name( *format ) );
	}
	else
	{
		error = 1;
	}

	// Release references and locks
	pthread_mutex_unlock( &this->mutex );
	mlt_cache_item_close( this->image_cache );
	mlt_service_unlock( MLT_PRODUCER_SERVICE( &this->parent ) );

	return error;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Get the real structure for this producer
	producer_pixbuf this = producer->child;

	// Fetch the producers properties
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	if ( this->filenames == NULL && mlt_properties_get( producer_properties, "resource" ) != NULL )
		load_filenames( this, producer_properties );

	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL && this->count > 0 )
	{
		// Obtain properties of frame and producer
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		// Set the producer on the frame properties
		mlt_properties_set_data( properties, "producer_pixbuf", this, 0, NULL, NULL );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Ensure that we have a way to obtain the position in the get_image
		mlt_properties_set_position( properties, "pixbuf_position", mlt_producer_position( producer ) );

		// Refresh the image
		refresh_image( this, *frame, 0, 0 );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", mlt_properties_get_int( producer_properties, "progressive" ) );
		
		double force_ratio = mlt_properties_get_double( producer_properties, "force_aspect_ratio" );
		if ( force_ratio > 0.0 )
			mlt_properties_set_double( properties, "aspect_ratio", force_ratio );
		else
			mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( producer_properties, "aspect_ratio" ) );

		// Push the get_image method
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_pixbuf this = parent->child;
	pthread_mutex_destroy( &this->mutex );
	parent->close = NULL;
	mlt_service_cache_purge( MLT_PRODUCER_SERVICE(parent) );
	mlt_producer_close( parent );
	mlt_properties_close( this->filenames );
	free( this );
}
