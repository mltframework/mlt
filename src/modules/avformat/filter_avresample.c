/*
 * filter_avresample.c -- adjust audio sample frequency
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ffmpeg Header files
#include <libavformat/avformat.h>

/** Get the audio.
*/

static int resample_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Get the resample information
	int output_rate = mlt_properties_get_int( filter_properties, "frequency" );
	int16_t *sample_buffer = mlt_properties_get_data( filter_properties, "buffer", NULL );

	// Obtain the resample context if it exists
	ReSampleContext *resample = mlt_properties_get_data( filter_properties, "audio_resample", NULL );

	// If no resample frequency is specified, default to requested value
	if ( output_rate == 0 )
		output_rate = *frequency;

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	if ( error ) return error;

	// Return now if no work to do
	if ( output_rate != *frequency )
	{
		// Will store number of samples created
		int used = 0;

		mlt_log_debug( MLT_FILTER_SERVICE(filter), "channels %d samples %d frequency %d -> %d\n",
			*channels, *samples, *frequency, output_rate );

		// Do not convert to s16 unless we need to change the rate
		if ( *format != mlt_audio_s16 )
		{
			*format = mlt_audio_s16;
			mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
		}

		// Create a resampler if nececessary
		if ( resample == NULL || *frequency != mlt_properties_get_int( filter_properties, "last_frequency" ) )
		{
			// Create the resampler
#if (LIBAVCODEC_VERSION_INT >= ((52<<16)+(15<<8)+0))
			resample = av_audio_resample_init( *channels, *channels, output_rate, *frequency,
				SAMPLE_FMT_S16, SAMPLE_FMT_S16, 16, 10, 0, 0.8 );
#else
			resample = audio_resample_init( *channels, *channels, output_rate, *frequency );
#endif

			// And store it on properties
			mlt_properties_set_data( filter_properties, "audio_resample", resample, 0, ( mlt_destructor )audio_resample_close, NULL );

			// And remember what it was created for
			mlt_properties_set_int( filter_properties, "last_frequency", *frequency );
		}

		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

		// Resample the audio
		used = audio_resample( resample, sample_buffer, *buffer, *samples );
		int size = used * *channels * sizeof( int16_t );

		// Resize if necessary
		if ( used > *samples )
		{
			*buffer = mlt_pool_realloc( *buffer, size );
			mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );
		}

		// Copy samples
		memcpy( *buffer, sample_buffer, size );

		// Update output variables
		*samples = used;
		*frequency = output_rate;
	}
	else
	{
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Only call this if we have a means to get audio
	if ( mlt_frame_is_test_audio( frame ) == 0 )
	{
		// Push the filter on to the stack
		mlt_frame_push_audio( frame, filter );

		// Assign our get_audio method
		mlt_frame_push_audio( frame, resample_get_audio );
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_avresample_init( char *arg )
{
	// Create a filter
	mlt_filter filter = mlt_filter_new( );

	// Initialise if successful
	if ( filter != NULL )
	{
		// Calculate size of the buffer
		int size = AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof( int16_t );

		// Allocate the buffer
		int16_t *buffer = mlt_pool_alloc( size );

		// Assign the process method
		filter->process = filter_process;

		// Deal with argument
		if ( arg != NULL )
			mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "frequency", arg );

		// Default to 2 channel output
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "channels", 2 );

		// Store the buffer
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( filter ), "buffer", buffer, size, mlt_pool_release, NULL );
	}

	return filter;
}
