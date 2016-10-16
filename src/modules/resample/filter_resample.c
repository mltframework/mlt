/*
 * filter_resample.c -- adjust audio sample frequency
 * Copyright (C) 2003-2014 Meltytech, LLC
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <samplerate.h>
#include <string.h>

// BUFFER_LEN is based on a maximum of 96KHz, 5 fps, 8 channels
// TODO: dynamically allocate larger buffer size
#define BUFFER_LEN ((96000/5) * 8 * sizeof(float))
#define RESAMPLE_TYPE SRC_SINC_FASTEST

/** Get the audio.
*/

static int resample_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the resample information
	int output_rate = mlt_properties_get_int( filter_properties, "frequency" );

	// If no resample frequency is specified, default to requested value
	if ( output_rate == 0 )
		output_rate = *frequency;

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	if ( error ) return error;

	// Return now if no work to do
	if ( output_rate != *frequency && *frequency > 0 && *channels > 0 )
	{
		mlt_log_debug( MLT_FILTER_SERVICE(filter), "channels %d samples %d frequency %d -> %d\n",
			*channels, *samples, *frequency, output_rate );

		// Do not convert to float unless we need to change the rate
		if ( *format != mlt_audio_f32le )
			frame->convert_audio( frame, buffer, format, mlt_audio_f32le );

		mlt_service_lock( MLT_FILTER_SERVICE(filter) );

		SRC_DATA data;
		data.data_in = *buffer;
		data.data_out = mlt_properties_get_data( filter_properties, "output_buffer", NULL );
		data.src_ratio = ( float ) output_rate / ( float ) *frequency;
		data.input_frames = *samples;
		data.output_frames = BUFFER_LEN / sizeof(float) / *channels;
		data.end_of_input = 0;

		SRC_STATE *state = mlt_properties_get_data( filter_properties, "state", NULL );
		if ( !state || mlt_properties_get_int( filter_properties, "channels" ) != *channels )
		{
			// Recreate the resampler if the number of channels changed
			state = src_new( RESAMPLE_TYPE, *channels, &error );
			mlt_properties_set_data( filter_properties, "state", state, 0, (mlt_destructor) src_delete, NULL );
			mlt_properties_set_int( filter_properties, "channels", *channels );
		}

		// Resample the audio
		error = src_process( state, &data );
		if ( !error )
		{
			// Update output variables
			*samples = data.output_frames_gen;
			*frequency = output_rate;
			*buffer = data.data_out;
		}
		else
		{
			mlt_log_error( MLT_FILTER_SERVICE( filter ), "%s %d,%d,%d\n", src_strerror( error ), *frequency, *samples, output_rate );
		}
		mlt_service_unlock( MLT_FILTER_SERVICE(filter) );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	if ( mlt_frame_is_test_audio( frame ) == 0 )
	{
		mlt_frame_push_audio( frame, this );
		mlt_frame_push_audio( frame, resample_get_audio );
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_resample_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		int error;
		SRC_STATE *state = src_new( RESAMPLE_TYPE, 2 /* channels */, &error );
		if ( error == 0 )
		{
			void *output_buffer = mlt_pool_alloc( BUFFER_LEN );
			this->process = filter_process;
			if ( arg != NULL )
				mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "frequency", atoi( arg ) );
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "channels", 2 );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "state", state, 0, (mlt_destructor)src_delete, NULL );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "output_buffer", output_buffer, BUFFER_LEN, mlt_pool_release, NULL );
		}
		else
		{
			fprintf( stderr, "filter_resample_init: %s\n", src_strerror( error ) );
		}
	}
	return this;
}
