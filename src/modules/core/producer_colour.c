/*
 * producer_colour.c -- raster image loader based upon gdk-pixbuf
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

#include "producer_colour.h"
#include <framework/mlt_frame.h>
#include <framework/mlt_pool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
	uint8_t r, g, b, a;
} rgba_color;

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_colour_init( char *colour )
{
	mlt_producer producer = calloc( 1, sizeof( struct mlt_producer_s ) );
	if ( producer != NULL && mlt_producer_init( producer, NULL ) == 0 )
	{
		// Get the properties interface
		mlt_properties properties = mlt_producer_properties( producer );
	
		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		// Set the default properties
		mlt_properties_set( properties, "resource", colour == NULL ? "0x000000ff" : colour );
		
		return producer;
	}
	free( producer );
	return NULL;
}

rgba_color parse_color( char *color )
{
	rgba_color result = { 0xff, 0xff, 0xff, 0xff };

	if ( strchr( color, '/' ) )
		color = strrchr( color, '/' ) + 1;

	if ( !strncmp( color, "0x", 2 ) )
	{
		unsigned int temp = 0;
		sscanf( color + 2, "%x", &temp );
		result.r = ( temp >> 24 ) & 0xff;
		result.g = ( temp >> 16 ) & 0xff;
		result.b = ( temp >> 8 ) & 0xff;
		result.a = ( temp ) & 0xff;
	}
	else if ( !strcmp( color, "red" ) )
	{
		result.r = 0xff;
		result.g = 0x00;
		result.b = 0x00;
	}
	else if ( !strcmp( color, "green" ) )
	{
		result.r = 0x00;
		result.g = 0xff;
		result.b = 0x00;
	}
	else if ( !strcmp( color, "blue" ) )
	{
		result.r = 0x00;
		result.g = 0x00;
		result.b = 0xff;
	}
	else if ( strcmp( color, "white" ) )
	{
		unsigned int temp = 0;
		sscanf( color, "%d", &temp );
		result.r = ( temp >> 24 ) & 0xff;
		result.g = ( temp >> 16 ) & 0xff;
		result.b = ( temp >> 8 ) & 0xff;
		result.a = ( temp ) & 0xff;
	}

	return result;
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// May need to know the size of the image to clone it
	int size = 0;
	
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( frame );

	// Obtain the producer for this frame
	mlt_producer producer = mlt_properties_get_data( properties, "producer_colour", NULL );

	// Obtain properties of producer
	mlt_properties producer_props = mlt_producer_properties( producer );

	// Get the current image and dimensions cached in the producer
	uint8_t *image = mlt_properties_get_data( producer_props, "image", &size );
	int current_width = mlt_properties_get_int( producer_props, "width" );
	int current_height = mlt_properties_get_int( producer_props, "height" );

	// See if we need to regenerate
	if ( *width != current_width || *height != current_height )
	{
		// Allocate the image
		size = *width * *height * 2;
		image = mlt_pool_alloc( size );

		// Update the producer
		mlt_properties_set_data( producer_props, "image", image, size, mlt_pool_release, NULL );
		mlt_properties_set_int( producer_props, "width", *width );
		mlt_properties_set_int( producer_props, "height", *height );

		// Color the image
		rgba_color color = parse_color( mlt_properties_get( producer_props, "resource" ) );
		uint8_t y, u, v;
		int i = 0;
		RGB2YUV( color.r, color.g, color.b, y, u, v );

		while ( i < size )
		{
			image[ i ++ ] = y;
			image[ i ++ ] = u;
			image[ i ++ ] = y;
			image[ i ++ ] = v;
		}
	}

	// Update the frame
	mlt_properties_set_data( properties, "image", image, size, NULL, NULL );
	mlt_properties_set_int( properties, "width", *width );
	mlt_properties_set_int( properties, "height", *height );
	
	// Clone if necessary
	if ( writable )
	{
		// Clone our image
		uint8_t *copy = mlt_pool_alloc( size );
		memcpy( copy, image, size );

		// We're going to pass the copy on
		image = copy;

		// Now update properties so we free the copy after
		mlt_properties_set_data( properties, "image", copy, size, mlt_pool_release, NULL );
	}

	// Pass on the image
	*buffer = image;
	*format = mlt_image_yuv422;

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( );

	if ( *frame != NULL )
	{
		// Obtain properties of frame and producer
		mlt_properties properties = mlt_frame_properties( *frame );

		// Obtain properties of producer
		mlt_properties producer_props = mlt_producer_properties( producer );

		// Determine if we're producing PAL or NTSC
		int is_pal = mlt_properties_get_double( producer_props, "fps" ) == 25.0;

		// Set the producer on the frame properties
		mlt_properties_set_data( properties, "producer_colour", producer, 0, NULL, NULL );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", 1 );
		mlt_properties_set_double( properties, "aspect_ratio", is_pal ? 128.0/117.0 : 72.0/79.0 );

		// colour is an alias for resource
		if ( mlt_properties_get( producer_props, "colour" ) != NULL )
			mlt_properties_set( producer_props, "resource", mlt_properties_get( producer_props, "colour" ) );
		
		// Push the get_image method
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	producer->close = NULL;
	mlt_producer_close( producer );
	free( producer );
}
