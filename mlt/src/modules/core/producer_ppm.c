/*
 * producer_ppm.c -- simple ppm test case
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "producer_ppm.h"
#include <framework/mlt_frame.h>
#include <stdlib.h>
#include <string.h>

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_ppm_init( void *command )
{
	producer_ppm this = calloc( sizeof( struct producer_ppm_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		producer->get_frame = producer_get_frame;
		producer->close = producer_close;

		if ( command == NULL )
			this->command = strdup( "image2raw -n -a -r 3 -ppm /usr/share/pixmaps/*.png" );
		else
			this->command = strdup( command );

		this->pipe = popen( this->command, "r" );

		return producer;
	}
	free( this );
	return NULL;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	// Get the RGB image
	uint8_t *rgb = mlt_properties_get_data( properties, "image", NULL );

	// Get width and height
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	// Convert to requested format
	if ( *format == mlt_image_yuv422 )
	{
		uint8_t *image = malloc( *width * *height * 2 );
		mlt_convert_rgb24_to_yuv422( rgb, *width, *height, *width * 3, image );
		mlt_properties_set_data( properties, "image", image, *width * *height * 2, free, NULL );
		*buffer = image;
	}
	else if ( *format == mlt_image_rgb24 )
	{
		*buffer = rgb;
	}

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_ppm this = producer->child;
	int width;
	int height;

	// Construct a test frame
	*frame = mlt_frame_init( );

	// Scan the header
	if ( fscanf( this->pipe, "P6\n%d %d\n255\n", &width, &height ) == 2 )
	{
		// Get the frames properties
		mlt_properties properties = mlt_frame_properties( *frame );

		// Allocate an image
		uint8_t *image = malloc( width * height * 3 );
		
		// Read it
		fread( image, width * height * 3, 1, this->pipe );

		// Pass the data on the frame properties
		mlt_properties_set_data( properties, "image", image, width * height * 3, free, NULL );
		mlt_properties_set_int( properties, "width", width );
		mlt_properties_set_int( properties, "height", height );

		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Update timecode on the frame we're creating
	mlt_frame_set_timecode( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_ppm this = parent->child;
	pclose( this->pipe );
	free( this->command );
	parent->close = NULL;
	mlt_producer_close( parent );
	free( this );
}

