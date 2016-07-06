/*
 * producer_blipflash.c -- blip/flash generating producer
 * Copyright (C) 2013 Brian Matherly
 * Author: Brian Matherly <pez4brian@yahoo.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Fill an audio buffer with 1kHz "blip" samples.
*/

static void fill_blip( mlt_properties producer_properties, float* buffer, int frequency, int channels, int samples )
{
	int new_size = samples * channels * sizeof( float );
	int old_size = 0;
	float* blip = mlt_properties_get_data( producer_properties, "_blip", &old_size );

	if( !blip || new_size > old_size )
	{
		blip = mlt_pool_alloc( new_size );

		// Fill the blip buffer
		if ( blip != NULL )
		{
			int s = 0;
			int c = 0;

			for( s = 0; s < samples; s++ )
			{
				float f = 1000.0;
				float t = (float)s/(float)frequency;
				// Add 90 deg so the blip always starts at 1 for easy detection.
				float phase = M_PI / 2;
				float value = sin( 2*M_PI*f*t + phase );

				for( c = 0; c < channels; c++ )
				{
					float* sample_ptr = ((float*) blip) + (c * samples) + s;
					*sample_ptr = value;
				}
			}
		}
		// Cache the audio blip to save from regenerating it with every blip.
		mlt_properties_set_data( producer_properties, "_blip", blip, new_size, mlt_pool_release, NULL );
	};

	if( blip ) memcpy( buffer, blip, new_size );
}

static int producer_get_audio( mlt_frame frame, int16_t** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_producer producer = mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "_producer_blipflash", NULL );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	int size = *samples * *channels * sizeof( float );
	double fps = mlt_producer_get_fps( producer );
	int frames = mlt_frame_get_position( frame )  + mlt_properties_get_int( producer_properties, "offset" );
	int seconds = frames / fps;

	// Correct the returns if necessary
	*format = mlt_audio_float;
	*frequency = *frequency <= 0 ? 48000 : *frequency;
	*channels = *channels <= 0 ? 2 : *channels;
	*samples = *samples <= 0 ? mlt_sample_calculator( fps, *frequency, frames ) : *samples;

	// Allocate the buffer
	*buffer = mlt_pool_alloc( size );

	// Determine if this should be a blip or silence.
	frames = frames % lrint( fps );
	seconds = seconds % mlt_properties_get_int( producer_properties, "period" );
	if( seconds == 0 && frames == 0 )
	{
		fill_blip( producer_properties, (float*)*buffer, *frequency, *channels, *samples );
	}
	else
	{
		// Fill silence.
		memset( *buffer, 0, size );
	}

	// Set the buffer for destruction
	mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );

	return 0;
}

/** Fill an image buffer with either white (flash) or black as requested.
*/

static void fill_image( mlt_properties producer_properties, char* color, uint8_t* buffer, mlt_image_format format, int width, int height )
{

	int new_size = mlt_image_format_size( format, width, height, NULL );
	int old_size = 0;
	uint8_t* image = mlt_properties_get_data( producer_properties, color, &old_size );

	if( !image || new_size > old_size )
	{
		// Need to create a new cached image.
		image = mlt_pool_alloc( new_size );

		if ( image != NULL )
		{
			uint8_t r, g, b;
			uint8_t* p = image;

			if( !strcmp( color, "_flash" ) )
			{
				r = g = b = 255; // White
			}
			else
			{
				r = g = b = 0; // Black
			}

			switch( format )
			{
				default:
				case mlt_image_yuv422:
				{
					int uneven = width % 2;
					int count = ( width - uneven ) / 2 + 1;
					uint8_t y, u, v;

					RGB2YUV_601_SCALED( r, g, b, y, u, v );
					int i = height + 1;
					while ( --i )
					{
						int j = count;
						while ( --j )
						{
							*p ++ = y;
							*p ++ = u;
							*p ++ = y;
							*p ++ = v;
						}
						if ( uneven )
						{
							*p ++ = y;
							*p ++ = u;
						}
					}
					break;
				}
				case mlt_image_rgb24:
				{
					int i = width * height + 1;
					while ( --i )
					{
						*p ++ = r;
						*p ++ = g;
						*p ++ = b;
					}
					break;
				}
				case mlt_image_rgb24a:
				{
					int i = width * height + 1;
					while ( --i )
					{
						*p ++ = r;
						*p ++ = g;
						*p ++ = b;
						*p ++ = 255; // alpha
					}
					break;
				}
			}
			// Cache the image to save from regenerating it with every frame.
			mlt_properties_set_data( producer_properties, color, image, new_size, mlt_pool_release, NULL );
		}
	}

	if( image ) memcpy( buffer, image, new_size );
}

static int producer_get_image( mlt_frame frame, uint8_t** buffer, mlt_image_format* format, int* width, int* height, int writable )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_producer producer = mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "_producer_blipflash", NULL );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	int size = 0;
	double fps = mlt_producer_get_fps( producer );
	int frames = mlt_frame_get_position( frame );
	int seconds = frames / fps;

	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	// Correct the returns if necessary
	if( *format != mlt_image_yuv422 && *format != mlt_image_rgb24 && *format != mlt_image_rgb24a )
		*format = mlt_image_yuv422;
	if( *width <= 0 )
		*width = mlt_service_profile( MLT_PRODUCER_SERVICE(producer) )->width;
	if ( *height <= 0 )
		*height = mlt_service_profile( MLT_PRODUCER_SERVICE(producer) )->height;

	// Allocate the buffer
	size = mlt_image_format_size( *format, *width, *height, NULL );
	*buffer = mlt_pool_alloc( size );

	// Determine if this should be a flash or black.
	frames = frames % lrint( fps );
	seconds = seconds % mlt_properties_get_int( producer_properties, "period" );
	if( seconds == 0 && frames == 0 )
	{
		fill_image( producer_properties, "_flash", *buffer, *format, *width, *height );
	}
	else
	{
		fill_image( producer_properties, "_black", *buffer, *format, *width, *height );
	}

	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	// Create the alpha channel
	int alpha_size = *width * *height;
	uint8_t *alpha = mlt_pool_alloc( alpha_size );
	if ( alpha )
		memset( alpha, 255, alpha_size );

	// Update the frame
	mlt_frame_set_image( frame, *buffer, size, mlt_pool_release );
	mlt_frame_set_alpha( frame, alpha, alpha_size, mlt_pool_release );
	mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( producer_properties, "aspect_ratio" ) );
	mlt_properties_set_int( properties, "progressive", 1 );
	mlt_properties_set_int( properties, "meta.media.width", *width );
	mlt_properties_set_int( properties, "meta.media.height", *height );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Obtain properties of frame
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( *frame );

		// Save the producer to be used later
		mlt_properties_set_data( frame_properties, "_producer_blipflash", producer, 0, NULL, NULL );

		// Update time code on the frame
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Configure callbacks
		mlt_frame_push_get_image( *frame, producer_get_image );
		mlt_frame_push_audio( *frame, producer_get_audio );
	}

	// Calculate the next time code
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer this )
{
	this->close = NULL;
	mlt_producer_close( this );
	free( this );
}

/** Initialize.
*/

mlt_producer producer_blipflash_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Initialize the producer
	if ( producer )
	{
		mlt_properties_set_int( producer_properties, "period", 1 );
		mlt_properties_set_int( producer_properties, "offset", 0 );

		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
	}

	return producer;
}
