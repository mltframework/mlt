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
	// Get the properties of the a frame
	mlt_properties properties = mlt_frame_properties( frame );
	int output_rate = mlt_properties_get_int( properties, "resample.frequency" );
	SRC_STATE *state = mlt_properties_get_data( properties, "resample.state", NULL );
	SRC_DATA data;
	float *input_buffer = mlt_properties_get_data( properties, "resample.input_buffer", NULL );
	float *output_buffer = mlt_properties_get_data( properties, "resample.output_buffer", NULL );
	int i;

	// Restore the original get_audio
	frame->get_audio = mlt_properties_get_data( properties, "resample.get_audio", NULL );

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Convert to floating point
	for ( i = 0; i < *samples * *channels; ++i )
		input_buffer[ i ] = ( float )( (*buffer)[ i ] ) / 32768;

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
			*buffer = (int16_t*) malloc( data.output_frames_gen * *channels * 2 );
			mlt_properties_set_data( properties, "audio", *buffer, *channels * data.output_frames_gen * 2, free, NULL );
		}
		*samples = data.output_frames_gen;
		*frequency = output_rate;
		
		// Convert from floating back to signed 16bit
		for ( i = 0; i < *samples * *channels; ++i )
		{
			float sample = output_buffer[ i ];
			if ( sample > 1.0 )
				sample = 1.0;
			if ( sample < -1.0 )
				sample = -1.0;
			if ( sample >= 0 )
				(*buffer)[ i ] = lrint( 32767.0 * sample );
			else
				(*buffer)[ i ] = lrint( 32768.0 * sample );
		}
	}
	//else
		//fprintf( stderr, "resample_get_audio: %s\n", src_strerror( i ) );
	
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_properties properties = mlt_filter_properties( this );
	mlt_properties frame_props = mlt_frame_properties( frame );

	// Propogate the frequency property if supplied
	if ( mlt_properties_get( properties, "frequency" ) != NULL )
		mlt_properties_set_int( frame_props, "resample.frequency", mlt_properties_get_int( properties, "frequency" ) );

	// Propogate the other properties
	mlt_properties_set_int( frame_props, "resample.channels", mlt_properties_get_int( properties, "channels" ) );
	mlt_properties_set_data( frame_props, "resample.state", mlt_properties_get_data( properties, "state", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( frame_props, "resample.input_buffer", mlt_properties_get_data( properties, "input_buffer", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( frame_props, "resample.output_buffer", mlt_properties_get_data( properties, "output_buffer", NULL ), 0, NULL, NULL );
	
	// Backup the original get_audio (it's still needed)
	mlt_properties_set_data( frame_props, "resample.get_audio", frame->get_audio, 0, NULL, NULL );

	// Override the get_audio method
	frame->get_audio = resample_get_audio;

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_resample_init( char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( this != NULL && mlt_filter_init( this, NULL ) == 0 )
	{
		int error;
		SRC_STATE *state = src_new( RESAMPLE_TYPE, 2 /* channels */, &error );
		if ( error == 0 )
		{
			this->process = filter_process;
			if ( arg != NULL )
				mlt_properties_set_int( mlt_filter_properties( this ), "frequency", atoi( arg ) );
			mlt_properties_set_int( mlt_filter_properties( this ), "channels", 2 );
			mlt_properties_set_data( mlt_filter_properties( this ), "state", state, 0, (mlt_destructor)src_delete, NULL );
			mlt_properties_set_data( mlt_filter_properties( this ), "input_buffer",
				malloc( BUFFER_LEN ), BUFFER_LEN, free, NULL );
			mlt_properties_set_data( mlt_filter_properties( this ), "output_buffer",
				malloc( BUFFER_LEN ), BUFFER_LEN, free, NULL );
		}
		else
		{
			//fprintf( stderr, "filter_resample_init: %s\n", src_strerror( error ) );
		}
	}
	return this;
}

