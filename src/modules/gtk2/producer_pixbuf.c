
/*
 * producer_pixbuf.c -- raster image loader based upon gdk-pixbuf
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "producer_pixbuf.h"
#include <framework/mlt_frame.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

static pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct producer_pixbuf_s *producer_pixbuf;

struct producer_pixbuf_s
{
	struct mlt_producer_s parent;

	// File name list
	mlt_properties filenames;
	int count;
	int image_idx;

	int width;
	int height;
	uint8_t *image;
	uint8_t *alpha;
};

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
		
		return producer;
	}
	free( this );
	return NULL;
}

static void refresh_image( mlt_frame frame, int width, int height )
{
	// Pixbuf 
	GdkPixbuf *pixbuf = mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "pixbuf", NULL );
	GError *error = NULL;

	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the producer for this frame
	producer_pixbuf this = mlt_properties_get_data( properties, "producer_pixbuf", NULL );

	// Obtain the producer 
	mlt_producer producer = &this->parent;

	// Obtain properties of producer
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Obtain the cache flag and structure
	int use_cache = mlt_properties_get_int( producer_props, "cache" );
	mlt_properties cache = mlt_properties_get_data( producer_props, "_cache", NULL );
	int update_cache = 0;

	// Get the time to live for each frame
	double ttl = mlt_properties_get_int( producer_props, "ttl" );

	// Get the original position of this frame
	mlt_position position = mlt_properties_get_position( properties, "pixbuf_position" );

	// Image index
	int image_idx = ( int )floor( ( double )position / ttl ) % this->count;

	// Key for the cache
	char image_key[ 10 ];
	sprintf( image_key, "%d", image_idx );

	pthread_mutex_lock( &fastmutex );

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
			this->alpha = mlt_properties_get_data( cached_props, "alpha", NULL );

			if ( width != 0 && ( width != this->width || height != this->height ) )
				this->image = NULL;
		}
	}

    // optimization for subsequent iterations on single picture
	if ( width != 0 && this->image != NULL && image_idx == this->image_idx )
	{
		if ( width != this->width || height != this->height )
		{
			pixbuf = mlt_properties_get_data( producer_props, "_pixbuf", NULL );
			if ( !use_cache )
			{
				mlt_pool_release( this->image );
				mlt_pool_release( this->alpha );
			}
			this->image = NULL;
			this->alpha = NULL;
		}
	}
	else if ( pixbuf == NULL && ( this->image == NULL || image_idx != this->image_idx ) )
	{
		if ( !use_cache )
		{
			mlt_pool_release( this->image );
			mlt_pool_release( this->alpha );
		}
		this->image = NULL;
		this->alpha = NULL;

		this->image_idx = image_idx;
		pixbuf = gdk_pixbuf_new_from_file( mlt_properties_get_value( this->filenames, image_idx ), &error );

		if ( pixbuf != NULL )
		{
			// Register this pixbuf for destruction and reuse
			mlt_events_block( producer_props, NULL );
			mlt_properties_set_data( producer_props, "_pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );
			g_object_ref( pixbuf );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );

			mlt_properties_set_int( producer_props, "_real_width", gdk_pixbuf_get_width( pixbuf ) );
			mlt_properties_set_int( producer_props, "_real_height", gdk_pixbuf_get_height( pixbuf ) );
			mlt_events_unblock( producer_props, NULL );

			// Store the width/height of the pixbuf temporarily
			this->width = gdk_pixbuf_get_width( pixbuf );
			this->height = gdk_pixbuf_get_height( pixbuf );
		}
	}

	// If we have a pixbuf
	if ( pixbuf && width > 0 )
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
		this->image = mlt_pool_alloc( width * ( height + 1 ) * 2 );

		// Extract YUV422 and alpha
		if ( gdk_pixbuf_get_has_alpha( pixbuf ) )
		{
			// Allocate the alpha mask
			this->alpha = mlt_pool_alloc( this->width * this->height );

			// Convert the image
			mlt_convert_rgb24a_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
										  this->width, this->height,
										  gdk_pixbuf_get_rowstride( pixbuf ),
										  this->image, this->alpha );
		}
		else
		{ 
			// No alpha to extract
			mlt_convert_rgb24_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
										 this->width, this->height,
										 gdk_pixbuf_get_rowstride( pixbuf ),
										 this->image );
		}

		// Finished with pixbuf now
		g_object_unref( pixbuf );

		// Ensure we update the cache when we need to
		update_cache = use_cache;
	}

	// Set width/height of frame
	mlt_properties_set_int( properties, "width", this->width );
	mlt_properties_set_int( properties, "height", this->height );
	mlt_properties_set_int( properties, "real_width", mlt_properties_get_int( producer_props, "_real_width" ) );
	mlt_properties_set_int( properties, "real_height", mlt_properties_get_int( producer_props, "_real_height" ) );

	// pass the image data without destructor
	mlt_properties_set_data( properties, "image", this->image, this->width * ( this->height + 1 ) * 2, NULL, NULL );
	mlt_properties_set_data( properties, "alpha", this->alpha, this->width * this->height, NULL, NULL );

	if ( update_cache )
	{
		mlt_frame cached = mlt_frame_init( );
		mlt_properties cached_props = MLT_FRAME_PROPERTIES( cached );
		mlt_properties_set_int( cached_props, "width", this->width );
		mlt_properties_set_int( cached_props, "height", this->height );
		mlt_properties_set_int( cached_props, "real_width", mlt_properties_get_int( producer_props, "_real_width" ) );
		mlt_properties_set_int( cached_props, "real_height", mlt_properties_get_int( producer_props, "_real_height" ) );
		mlt_properties_set_data( cached_props, "image", this->image, this->width * ( this->height + 1 ) * 2, mlt_pool_release, NULL );
		mlt_properties_set_data( cached_props, "alpha", this->alpha, this->width * this->height, mlt_pool_release, NULL );
		mlt_properties_set_data( cache, image_key, cached, 0, ( mlt_destructor )mlt_frame_close, NULL );
	}

	pthread_mutex_unlock( &fastmutex );
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// We need to know the size of the image to clone it
	int image_size = 0;
	int alpha_size = 0;

	// Alpha channel
	uint8_t *alpha = NULL;

	*width = mlt_properties_get_int( properties, "rescale_width" );
	*height = mlt_properties_get_int( properties, "rescale_height" );

	// Refresh the image
	refresh_image( frame, *width, *height );

	// Get the image
	*buffer = mlt_properties_get_data( properties, "image", &image_size );
	alpha = mlt_properties_get_data( properties, "alpha", &alpha_size );

	// Get width and height (may have changed during the refresh)
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	// NB: Cloning is necessary with this producer (due to processing of images ahead of use)
	// The fault is not in the design of mlt, but in the implementation of the pixbuf producer...
	if ( *buffer != NULL )
	{
		// Clone the image and the alpha
		uint8_t *image_copy = mlt_pool_alloc( image_size );
		uint8_t *alpha_copy = mlt_pool_alloc( alpha_size );

		memcpy( image_copy, *buffer, image_size );

		// Copy or default the alpha
		if ( alpha != NULL )
			memcpy( alpha_copy, alpha, alpha_size );
		else
			memset( alpha_copy, 255, alpha_size );

		// Now update properties so we free the copy after
		mlt_properties_set_data( properties, "image", image_copy, image_size, mlt_pool_release, NULL );
		mlt_properties_set_data( properties, "alpha", alpha_copy, alpha_size, mlt_pool_release, NULL );

		// We're going to pass the copy on
		*buffer = image_copy;
	}
	else
	{
		// TODO: Review all cases of invalid images
		*buffer = mlt_pool_alloc( 50 * 50 * 2 );
		mlt_properties_set_data( properties, "image", *buffer, image_size, mlt_pool_release, NULL );
		*width = 50;
		*height = 50;
	}

	return 0;
}

static uint8_t *producer_get_alpha_mask( mlt_frame this )
{
	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Return the alpha mask
	return mlt_properties_get_data( properties, "alpha", NULL );
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Get the real structure for this producer
	producer_pixbuf this = producer->child;

	// Fetch the producers properties
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	if ( this->filenames == NULL && mlt_properties_get( producer_properties, "resource" ) != NULL )
	{
		char *filename = mlt_properties_get( producer_properties, "resource" );
		this->filenames = mlt_properties_new( );

		// Read xml string
		if ( strstr( filename, "<svg" ) )
		{
			// Generate a temporary file for the svg
			char fullname[ 1024 ] = "/tmp/mlt.XXXXXX";
			int fd = mkstemp( fullname );

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
					mlt_properties_set( this->filenames, "0", full );
					gap = 0;
				}
				else
				{
					gap ++;
				}
			}
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

	// Generate a frame
	*frame = mlt_frame_init( );

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
		refresh_image( *frame, 0, 0 );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", mlt_properties_get_int( producer_properties, "progressive" ) );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( producer_properties, "aspect_ratio" ) );

		// Set alpha call back
		( *frame )->get_alpha_mask = producer_get_alpha_mask;

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
	if ( !mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( parent ), "cache" ) )
	{
		mlt_pool_release( this->image );
		mlt_pool_release( this->alpha );
	}
	parent->close = NULL;
	mlt_producer_close( parent );
	mlt_properties_close( this->filenames );
	free( this );
}
