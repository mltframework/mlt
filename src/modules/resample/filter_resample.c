/*
 * filter_resample.c -- adjust audio sample frequency
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

#include "filter_resample.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <samplerate.h>
#define __USE_ISOC99 1
#include <math.h>

#define BUFFER_LEN 20480
#define RESAMPLE_TYPE SRC_SINC_FASTEST

/** Get the audio.
*/

static int resample_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the resample information
	int output_rate = mlt_properties_get_int( filter_properties, "frequency" );
	SRC_STATE *state = mlt_properties_get_data( filter_properties, "state", NULL );
	float *input_buffer = mlt_properties_get_data( filter_properties, "input_buffer", NULL );
	float *output_buffer = mlt_properties_get_data( filter_properties, "output_buffer", NULL );
	int channels_avail = *channels;
	SRC_DATA data;
	int i;

	// If no resample frequency is specified, default to requested value
	if ( output_rate == 0 )
		output_rate = *frequency;

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, &channels_avail, samples );

	// Duplicate channels as necessary
	if ( channels_avail < *channels )
	{
		int size = *channels * *samples * sizeof( int16_t );
		int16_t *new_buffer = mlt_pool_alloc( size );
		int j, k = 0;
		
		// Duplicate the existing channels
		for ( i = 0; i < *samples; i++ )
		{
			for ( j = 0; j < *channels; j++ )
			{
				new_buffer[ ( i * *channels ) + j ] = (*buffer)[ ( i * channels_avail ) + k ];
				k = ( k + 1 ) % channels_avail;
			}
		}
		
		// Update the audio buffer now - destroys the old
		mlt_properties_set_data( properties, "audio", new_buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
		
		*buffer = new_buffer;
	}
	else if ( channels_avail == 6 && *channels == 2 )
	{
		// Nasty hack for ac3 5.1 audio - may be a cause of failure?
		int size = *channels * *samples * sizeof( int16_t );
		int16_t *new_buffer = mlt_pool_alloc( size );
		
		// Drop all but the first *channels
		for ( i = 0; i < *samples; i++ )
		{
			new_buffer[ ( i * *channels ) + 0 ] = (*buffer)[ ( i * channels_avail ) + 2 ];
			new_buffer[ ( i * *channels ) + 1 ] = (*buffer)[ ( i * channels_avail ) + 3 ];
		}

		// Update the audio buffer now - destroys the old
		mlt_properties_set_data( properties, "audio", new_buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
		
		*buffer = new_buffer;
	}

	// Return now if no work to do
	if ( output_rate != *frequency )
	{
		float *p = input_buffer;
		float *end = p + *samples * *channels;
		int16_t *q = *buffer;

		// Convert to floating point
		while( p != end )
			*p ++ = ( float )( *q ++ ) / 32768.0;

		// Resample
		data.data_in = input_buffer;
		data.data_out = output_buffer;
		data.src_ratio = ( float ) output_rate / ( float ) *frequency;
		data.input_frames = *samples;
		data.output_frames = BUFFER_LEN / *channels;
		data.end_of_input = 0;
		i = src_process( state, &data );
		if ( i == 0 )
		{
			if ( data.output_frames_gen > *samples )
			{
				*buffer = mlt_pool_realloc( *buffer, data.output_frames_gen * *channels * sizeof( int16_t ) );
				mlt_properties_set_data( properties, "audio", *buffer, *channels * data.output_frames_gen * 2, mlt_pool_release, NULL );
			}

			*samples = data.output_frames_gen;
			*frequency = output_rate;

			p = output_buffer;
			q = *buffer;
			end = p + *samples * *channels;
			
			// Convert from floating back to signed 16bit
			while( p != end )
			{
				if ( *p > 1.0 )
					*p = 1.0;
				if ( *p < -1.0 )
					*p = -1.0;
				if ( *p > 0 )
					*q ++ = 32767 * *p ++;
				else
					*q ++ = 32768 * *p ++;
			}
		}
		else
			fprintf( stderr, "resample_get_audio: %s %d,%d,%d\n", src_strerror( i ), *frequency, *samples, output_rate );
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	//if ( mlt_frame_is_test_audio( frame ) != 0 )
	{
		mlt_frame_push_audio( frame, this );
		mlt_frame_push_audio( frame, resample_get_audio );
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_resample_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		int error;
		SRC_STATE *state = src_new( RESAMPLE_TYPE, 2 /* channels */, &error );
		if ( error == 0 )
		{
			void *input_buffer = mlt_pool_alloc( BUFFER_LEN );
			void *output_buffer = mlt_pool_alloc( BUFFER_LEN );
			this->process = filter_process;
			if ( arg != NULL )
				mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "frequency", atoi( arg ) );
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "channels", 2 );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "state", state, 0, (mlt_destructor)src_delete, NULL );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "input_buffer", input_buffer, BUFFER_LEN, mlt_pool_release, NULL );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "output_buffer", output_buffer, BUFFER_LEN, mlt_pool_release, NULL );
		}
		else
		{
			fprintf( stderr, "filter_resample_init: %s\n", src_strerror( error ) );
		}
	}
	return this;
}
