/*
 * producer_noise.c -- noise generating producer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
#include <framework/mlt_pool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Random number generator
*/

static unsigned int seed_x = 521288629;
static unsigned int seed_y = 362436069;

static inline unsigned int fast_rand( )
{
   static unsigned int a = 18000, b = 30903;
   seed_x = a * ( seed_x & 65535 ) + ( seed_x >> 16 );
   seed_y = b * ( seed_y & 65535 ) + ( seed_y >> 16 );
   return ( ( seed_x << 16 ) + ( seed_y & 65535 ) );
}

// Foward declarations
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer this );

/** Initialise.
*/

mlt_producer producer_noise_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer this = mlt_producer_new( profile );

	// Initialise the producer
	if ( this != NULL )
	{
		// Callback registration
		this->get_frame = producer_get_frame;
		this->close = ( mlt_destructor )producer_close;
	}

	return this;
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Calculate the size of the image
	int size = *width * *height * 2;

	// Set the format being returned
	*format = mlt_image_yuv422;

	// Allocate the image
	*buffer = mlt_pool_alloc( size );

	// Update the frame
	mlt_frame_set_image( frame, *buffer, size, mlt_pool_release );

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
			*( -- p ) = fast_rand( ) & 0x0f00;
	}

	// Set the buffer for destruction
	mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );

	return 0;
}

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( this ) );

	// Check that we created a frame and initialise it
	if ( *frame != NULL )
	{
		// Obtain properties of frame
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		// Aspect ratio is whatever it needs to be
		mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( MLT_PRODUCER_PROPERTIES( this ), "aspect_ratio" ) );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", 1 );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( this ) );

		// Push the get_image method
		mlt_frame_push_get_image( *frame, producer_get_image );

		// Specify the audio
		mlt_frame_push_audio( *frame, producer_get_audio );
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( this );

	return 0;
}

static void producer_close( mlt_producer this )
{
	this->close = NULL;
	mlt_producer_close( this );
	free( this );
}

