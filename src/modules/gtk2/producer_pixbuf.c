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

pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct producer_pixbuf_s *producer_pixbuf;

struct producer_pixbuf_s
{
	struct mlt_producer_s parent;

	// File name list
	char **filenames;
	int count;
	int image_idx;

	int width;
	int height;
	uint8_t *image;
	uint8_t *alpha;
};

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

static int filter_files( const struct dirent *de )
{
	if ( de->d_name[ 0 ] != '.' )
		return 1;
	else
		return 0;
}

mlt_producer producer_pixbuf_init( char *filename )
{
	producer_pixbuf this = calloc( sizeof( struct producer_pixbuf_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		// Get the properties interface
		mlt_properties properties = mlt_producer_properties( &this->parent );
	
		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		// Set the default properties
		mlt_properties_set( properties, "resource", filename );
		mlt_properties_set_int( properties, "ttl", 25 );
		
		return producer;
	}
	free( this );
	return NULL;
}

static void refresh_image( mlt_frame frame, int width, int height )
{
	// Pixbuf 
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;

	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( frame );

	// Obtain the producer for this frame
	producer_pixbuf this = mlt_properties_get_data( properties, "producer_pixbuf", NULL );

	// Obtain the producer 
	mlt_producer producer = &this->parent;

	// Obtain properties of producer
	mlt_properties producer_props = mlt_producer_properties( producer );

	// Get the time to live for each frame
	double ttl = mlt_properties_get_int( producer_props, "ttl" );

	// Get the original position of this frame
	mlt_position position = mlt_properties_get_position( properties, "pixbuf_position" );

	// Image index
	int image_idx = ( int )floor( ( double )position / ttl ) % this->count;

	pthread_mutex_lock( &fastmutex );

    // optimization for subsequent iterations on single picture
	if ( width != 0 && this->image != NULL && image_idx == this->image_idx )
	{
		if ( width != this->width || height != this->height )
		{
			pixbuf = mlt_properties_get_data( producer_props, "pixbuf", NULL );
			mlt_pool_release( this->image );
			mlt_pool_release( this->alpha );
			this->image = NULL;
			this->alpha = NULL;
		}
	}
	else if ( this->image == NULL || image_idx != this->image_idx )
	{
		mlt_pool_release( this->image );
		mlt_pool_release( this->alpha );
		this->image = NULL;
		this->alpha = NULL;

		this->image_idx = image_idx;
		pixbuf = gdk_pixbuf_new_from_file( this->filenames[ image_idx ], &error );

		if ( pixbuf != NULL )
		{
			// Register this pixbuf for destruction and reuse
			mlt_properties_set_data( producer_props, "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );

			mlt_properties_set_int( producer_props, "real_width", gdk_pixbuf_get_width( pixbuf ) );
			mlt_properties_set_int( producer_props, "real_height", gdk_pixbuf_get_height( pixbuf ) );

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
	}

	// Set width/height of frame
	mlt_properties_set_int( properties, "width", this->width );
	mlt_properties_set_int( properties, "height", this->height );
	mlt_properties_set_int( properties, "real_width", mlt_properties_get_int( producer_props, "real_width" ) );
	mlt_properties_set_int( properties, "real_height", mlt_properties_get_int( producer_props, "real_height" ) );

	// pass the image data without destructor
	mlt_properties_set_data( properties, "image", this->image, this->width * ( this->height + 1 ) * 2, NULL, NULL );
	mlt_properties_set_data( properties, "alpha", this->alpha, this->width * this->height, NULL, NULL );

	pthread_mutex_unlock( &fastmutex );
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( frame );

	*width = mlt_properties_get_int( properties, "rescale_width" );
	*height = mlt_properties_get_int( properties, "rescale_height" );

	// Refresh the image
	refresh_image( frame, *width, *height );

	// May need to know the size of the image to clone it
	int size = 0;

	// Get the image
	uint8_t *image = mlt_properties_get_data( properties, "image", &size );

	// Get width and height
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	if ( size == 0 )
	{
		*width = mlt_properties_get_int( properties, "normalised_width" );
		*height = mlt_properties_get_int( properties, "normalised_height" );
		size = *width * ( *height + 1 );
	}

	// Clone if necessary
	// NB: Cloning is necessary with this producer (due to processing of images ahead of use)
	// The fault is not in the design of mlt, but in the implementation of pixbuf...
	//if ( writable )
	{
		// Clone our image
		uint8_t *copy = mlt_pool_alloc( size );
		if ( image != NULL )
			memcpy( copy, image, size );

		// We're going to pass the copy on
		image = copy;

		// Now update properties so we free the copy after
		mlt_properties_set_data( properties, "image", copy, size, mlt_pool_release, NULL );
	}

	// Pass on the image
	*buffer = image;

	return 0;
}

static uint8_t *producer_get_alpha_mask( mlt_frame this )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// Return the alpha mask
	return mlt_properties_get_data( properties, "alpha", NULL );
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Get the real structure for this producer
	producer_pixbuf this = producer->child;

	if ( this->count == 0 && mlt_properties_get( mlt_producer_properties( producer ), "resource" ) != NULL )
	{
		mlt_properties properties = mlt_producer_properties( producer );
		char *filename = mlt_properties_get( properties, "resource" );
		
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

				this->filenames = realloc( this->filenames, sizeof( char * ) * ( this->count + 1 ) );
				this->filenames[ this->count ++ ] = strdup( fullname );

				// Teehe - when the producer closes, delete the temp file and the space allo
				mlt_properties_set_data( properties, "__temporary_file__", this->filenames[ this->count - 1 ], 0, ( mlt_destructor )unlink, NULL );
			}
		}
		// Obtain filenames
		else if ( strchr( filename, '%' ) != NULL )
		{
			// handle picture sequences
			int i = 0;
			int gap = 0;
			char full[1024];

			while ( gap < 100 )
			{
				struct stat buf;
				snprintf( full, 1023, filename, i ++ );
				if ( stat( full, &buf ) == 0 )
				{
					this->filenames = realloc( this->filenames, sizeof( char * ) * ( this->count + 1 ) );
					this->filenames[ this->count ++ ] = strdup( full );
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
			char *dir_name = strdup( filename );
			char *extension = strrchr( filename, '.' );
			*( strstr( dir_name, "/.all." ) + 1 ) = '\0';
			char fullname[ 1024 ];
			strcpy( fullname, dir_name );
			struct dirent **de = NULL;
			int n = scandir( fullname, &de, filter_files, alphasort );
			int i;
			struct stat info;

			for (i = 0; i < n; i++ )
			{
				snprintf( fullname, 1023, "%s%s", dir_name, de[i]->d_name );

				if ( strstr( fullname, extension ) && lstat( fullname, &info ) == 0 &&
				 	( S_ISREG( info.st_mode ) || info.st_mode | S_IXUSR ) )
				{
					this->filenames = realloc( this->filenames, sizeof( char * ) * ( this->count + 1 ) );
					this->filenames[ this->count ++ ] = strdup( fullname );
				}
				free( de[ i ] );
			}

			free( de );
			free( dir_name );
		}
		else
		{
			this->filenames = realloc( this->filenames, sizeof( char * ) * ( this->count + 1 ) );
			this->filenames[ this->count ++ ] = strdup( filename );
		}
	}

	// Generate a frame
	*frame = mlt_frame_init( );

	if ( *frame != NULL && this->count > 0 )
	{
		// Obtain properties of frame and producer
		mlt_properties properties = mlt_frame_properties( *frame );

		// Determine if we're rendering for PAL or NTSC
		int is_pal = mlt_properties_get_int( properties, "normalised_height" ) == 576;

		// Set the producer on the frame properties
		mlt_properties_set_data( properties, "producer_pixbuf", this, 0, NULL, NULL );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Ensure that we have a way to obtain the position in the get_image
		mlt_properties_set_position( properties, "pixbuf_position", mlt_producer_position( producer ) );

		// Refresh the image
		refresh_image( *frame, 0, 0 );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", 1 );
		mlt_properties_set_double( properties, "aspect_ratio", is_pal ? 59.0/54.0 : 10.0/11.0 );

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
	mlt_pool_release( this->image );
	parent->close = NULL;
	mlt_producer_close( parent );
	while ( this->count -- )
		free( this->filenames[ this->count ] );
	free( this->filenames );
	free( this );
}
