/*
 * producer_noise.c -- noise generating producer
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

#include "producer_noise.h"
#include <framework/mlt_frame.h>
#include <framework/mlt_pool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Random number generator
*/

static unsigned int seed_x = 521288629;
static unsigned int seed_y = 362436069;

static unsigned inline int fast_rand( )
{
   static unsigned int a = 18000, b = 30903;
   seed_x = a * ( seed_x & 65535 ) + ( seed_x >> 16 );
   seed_y = b * ( seed_y & 65535 ) + ( seed_y >> 16 );
   return ( ( seed_x << 16 ) + ( seed_y & 65535 ) );
}

// Foward declarations
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

/** Initialise.
*/

mlt_producer producer_noise_init( void *arg )
{
	// Create a new producer object
	mlt_producer this = mlt_producer_new( );

	// Initialise the producer
	if ( this != NULL )
	{
		// Callback registration
		this->get_frame = producer_get_frame;
	}

	return this;
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( frame );

	// Calculate the size of the image
	int size = *width * *height * 2;

	// Set the format being returned
	*format = mlt_image_yuv422;

	// Allocate the image
	*buffer = mlt_pool_alloc( size );

	// Update the frame
	mlt_properties_set_data( properties, "image", *buffer, size, mlt_pool_release, NULL );
	mlt_properties_set_int( properties, "width", *width );
	mlt_properties_set_int( properties, "height", *height );

	// Before we write to the image, make sure we have one
	if ( *buffer != NULL )
	{
		// Calculate the end of the buffer
		uint8_t *p = *buffer + *width * *height * 2;

		// Value to hold a random number
		uint32_t value;

		// Generate random noise
		while ( p != *buffer )
		{
			value = fast_rand( ) & 0xff;
			*( -- p ) = 128;
			*( -- p ) = value < 16 ? 16 : value > 240 ? 240 : value;
		}
	}

	return 0;
}

static int producer_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the frame properties
	mlt_properties properties = mlt_frame_properties( frame );

	int size = 0;

	// Correct the returns if necessary
	*samples = *samples <= 0 ? 1920 : *samples;
	*channels = *channels <= 0 ? 2 : *channels;
	*frequency = *frequency <= 0 ? 48000 : *frequency;

	// Calculate the size of the buffer
	size = *samples * *channels * sizeof( int16_t );

	// Allocate the buffer
	*buffer = mlt_pool_alloc( size );

	// Make sure we got one and fill it
	if ( *buffer != NULL )
	{
		int16_t *p = *buffer + size / 2;
		while ( p != *buffer ) 
			*( -- p ) = fast_rand( ) & 0xff;
	}

	// Set the buffer for destruction
	mlt_properties_set_data( properties, "audio", *buffer, size, ( mlt_destructor )mlt_pool_release, NULL );

	return 0;
}

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( );

	// Check that we created a frame and initialise it
	if ( *frame != NULL )
	{
		// Obtain properties of frame
		mlt_properties properties = mlt_frame_properties( *frame );

		// Aspect ratio is 1?
		mlt_properties_set_double( properties, "aspect_ratio", 1.0 );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", 1 );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( this ) );

		// Push the get_image method
		mlt_frame_push_get_image( *frame, producer_get_image );

		// Specify the audio
		( *frame )->get_audio = producer_get_audio;
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( this );

	return 0;
}


