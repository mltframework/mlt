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
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

typedef enum
{
	SIGNAL_FORMAT_PAL,
	SIGNAL_FORMAT_NTSC
} mlt_signal_format;

mlt_producer producer_pixbuf_init( const char *filename )
{
	producer_pixbuf this = calloc( sizeof( struct producer_pixbuf_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		producer->get_frame = producer_get_frame;
		producer->close = producer_close;

		// Get the properties interface
		mlt_properties properties = mlt_producer_properties( &this->parent );
	
		// Set the default properties
		mlt_properties_set_int( properties, "video_standard", mlt_video_standard_pal );
		mlt_properties_set_double( properties, "ttl", 5 );
		
		// Obtain filenames
		if ( strchr( filename, '%' ) != NULL )
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
		else
		{
			this->filenames = realloc( this->filenames, sizeof( char * ) * ( this->count + 1 ) );
			this->filenames[ this->count ++ ] = strdup( filename );
		}

		// Initialise gobject types
		g_type_init();

		return producer;
	}
	free( this );
	return NULL;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// May need to know the size of the image to clone it
	int size = 0;

	// Get the image
	uint8_t *image = mlt_properties_get_data( properties, "image", &size );

	// Get width and height
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	// Clone if necessary
	if ( writable )
	{
		// Clone our image
		uint8_t *copy = malloc( size );
		memcpy( copy, image, size );

		// We're going to pass the copy on
		image = copy;

		// Now update properties so we free the copy after
		mlt_properties_set_data( properties, "image", copy, size, free, NULL );
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
	producer_pixbuf this = producer->child;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;

	// Generate a frame
	*frame = mlt_frame_init( );

	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( *frame );

	// Get the time to live for each frame
	double ttl = mlt_properties_get_double( mlt_producer_properties( producer ), "ttl" );

	// Image index
	int image_idx = ( int )floor( mlt_producer_position( producer ) / ttl ) % this->count;

	// Update timecode on the frame we're creating
	mlt_frame_set_timecode( *frame, mlt_producer_position( producer ) );

    // optimization for subsequent iterations on single picture
	if ( this->image != NULL && image_idx == this->image_idx )
	{
		// Set width/height
		mlt_properties_set_int( properties, "width", this->width );
		mlt_properties_set_int( properties, "height", this->height );

		// if picture sequence pass the image and alpha data without destructor
		mlt_properties_set_data( properties, "image", this->image, 0, NULL, NULL );
		mlt_properties_set_data( properties, "alpha", this->alpha, 0, NULL, NULL );

		// Set alpha mask call back
        ( *frame )->get_alpha_mask = producer_get_alpha_mask;

		// Stack the get image callback
		mlt_frame_push_get_image( *frame, producer_get_image );

	}
	else 
	{
		free( this->image );
		free( this->alpha );
		this->image_idx = image_idx;
		pixbuf = gdk_pixbuf_new_from_file( this->filenames[ image_idx ], &error );
	}

	// If we have a pixbuf
	if ( pixbuf )
	{
		// Scale to adjust for sample aspect ratio
		if ( mlt_properties_get_int( properties, "video_standard" ) == mlt_video_standard_pal )
		{
			GdkPixbuf *temp = pixbuf;
			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf,
				(gint) ( (float) gdk_pixbuf_get_width( pixbuf ) * 54.0/59.0),
				gdk_pixbuf_get_height( pixbuf ), GDK_INTERP_HYPER );
			pixbuf = scaled;
			g_object_unref( temp );
		}
		else
		{
			GdkPixbuf *temp = pixbuf;
			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf,
				(gint) ( (float) gdk_pixbuf_get_width( pixbuf ) * 11.0/10.0 ),
				gdk_pixbuf_get_height( pixbuf ), GDK_INTERP_HYPER );
			pixbuf = scaled;
			g_object_unref( temp );
		}

		// Store width and height
		this->width = gdk_pixbuf_get_width( pixbuf );
		this->height = gdk_pixbuf_get_height( pixbuf );

		// Allocate/define image and alpha
		uint8_t *image = malloc( this->width * this->height * 2 );
		uint8_t *alpha = NULL;

		// Extract YUV422 and alpha
		if ( gdk_pixbuf_get_has_alpha( pixbuf ) )
		{
			// Allocate the alpha mask
			alpha = malloc( this->width * this->height );

			// Convert the image
			mlt_convert_rgb24a_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
										  this->width, this->height,
										  gdk_pixbuf_get_rowstride( pixbuf ),
										  image, alpha );
		}
		else
		{ 
			// No alpha to extract
			mlt_convert_rgb24_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
										 this->width, this->height,
										 gdk_pixbuf_get_rowstride( pixbuf ),
										 image );
		}

		// Finished with pixbuf now
		g_object_unref( pixbuf );

		// Set width/height of frame
		mlt_properties_set_int( properties, "width", this->width );
		mlt_properties_set_int( properties, "height", this->height );

		// Pass alpha and image on properties with or without destructor
		this->image = image;
		this->alpha = alpha;

		// pass the image and alpha data without destructor
		mlt_properties_set_data( properties, "image", image, 0, NULL, NULL );
		mlt_properties_set_data( properties, "alpha", alpha, 0, NULL, NULL );

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
	if ( this->image )
		free( this->image );
	if ( this->alpha )
		free( this->alpha );
	parent->close = NULL;
	mlt_producer_close( parent );
	free( this );
}

