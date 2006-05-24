/*
 * producer_image.c -- a QT/QImage based producer for MLT
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
 *         Charles Yates <charles.yates@gmail.com>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "producer_qimage.h"
#include "qimage_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_qimage_init( char *filename )
{
	producer_qimage this = calloc( sizeof( struct producer_qimage_s ), 1 );
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
	refresh_qimage( frame, *width, *height );

	// Get the image
	*buffer = mlt_properties_get_data( properties, "image", &image_size );
	alpha = mlt_properties_get_data( properties, "alpha", &alpha_size );

	// Get width and height (may have changed during the refresh)
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	// NB: Cloning is necessary with this producer (due to processing of images ahead of use)
	// The fault is not in the design of mlt, but in the implementation of the qimage producer...
	if ( *buffer != NULL )
	{
		if ( *format == mlt_image_yuv422 || *format == mlt_image_yuv420p )
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
		else if ( *format == mlt_image_rgb24a )
		{
			// Clone the image and the alpha
			image_size = *width * ( *height + 1 ) * 4;
			alpha_size = *width * ( *height + 1 );
			uint8_t *image_copy = mlt_pool_alloc( image_size );
			uint8_t *alpha_copy = mlt_pool_alloc( alpha_size );

			mlt_convert_yuv422_to_rgb24a(*buffer, image_copy, (*width)*(*height));

			// Now update properties so we free the copy after
			mlt_properties_set_data( properties, "image", image_copy, image_size, mlt_pool_release, NULL );
			mlt_properties_set_data( properties, "alpha", alpha_copy, alpha_size, mlt_pool_release, NULL );

			// We're going to pass the copy on
			*buffer = image_copy;
		}
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
	producer_qimage this = producer->child;

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
		mlt_properties_set_data( properties, "producer_qimage", this, 0, NULL, NULL );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Ensure that we have a way to obtain the position in the get_image
		mlt_properties_set_position( properties, "qimage_position", mlt_producer_position( producer ) );

		// Refresh the image
		refresh_qimage( *frame, 0, 0 );

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
	producer_qimage this = parent->child;
	parent->close = NULL;
	mlt_producer_close( parent );
	mlt_properties_close( this->filenames );
	free( this );
}
