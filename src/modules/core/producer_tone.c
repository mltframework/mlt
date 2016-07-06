/*
 * producer_tone.c -- audio tone generating producer
 * Copyright (C) 2014 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

static int producer_get_audio( mlt_frame frame, int16_t** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_producer producer = mlt_frame_pop_audio( frame );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	double fps = mlt_producer_get_fps( producer );
	mlt_position position = mlt_frame_get_position( frame );
	mlt_position length = mlt_producer_get_length( producer );

	// Correct the returns if necessary
	*format = mlt_audio_float;
	*frequency = *frequency <= 0 ? 48000 : *frequency;
	*channels = *channels <= 0 ? 2 : *channels;
	*samples = *samples <= 0 ? mlt_sample_calculator( fps, *frequency, position ) : *samples;

	// Allocate the buffer
	int size = *samples * *channels * sizeof( float );
	*buffer = mlt_pool_alloc( size );

	// Fill the buffer
	int s = 0;
	int c = 0;
	long double first_sample = mlt_sample_calculator_to_now( fps, *frequency, position );
	float a = mlt_properties_anim_get_double( producer_properties, "level", position, length );
	long double f = mlt_properties_anim_get_double( producer_properties, "frequency", position, length );
	long double p = mlt_properties_anim_get_double( producer_properties, "phase", position, length );
	p = (M_PI / 180) * p; // Convert from degrees to radians
	a = pow( 10, a / 20.0 ); // Convert from dB to amplitude

	for( s = 0; s < *samples; s++ )
	{
		long double t = (first_sample + s) / *frequency;

		float value = a * sin( 2*M_PI*f*t + p );

		for( c = 0; c < *channels; c++ )
		{
			float* sample_ptr = ((float*)*buffer) + (c * *samples) + s;
			*sample_ptr = value;
		}
	}

	// Set the buffer for destruction
	mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Update time code on the frame
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Configure callbacks
		mlt_frame_push_audio( *frame, producer );
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

mlt_producer producer_tone_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Initialize the producer
	if ( producer )
	{
		mlt_properties_set_double( producer_properties, "frequency", 1000.0 );
		mlt_properties_set_double( producer_properties, "phase", 0.0 );
		mlt_properties_set_double( producer_properties, "level", 0.0 );

		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
	}

	return producer;
}
