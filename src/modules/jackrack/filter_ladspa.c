/*
 * filter_ladspa.c -- filter audio through LADSPA plugins
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
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

#include "filter_ladspa.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <string.h>

#include "jack_rack.h"

#define BUFFER_LEN 10000

static void initialise_jack_rack( mlt_properties properties, int channels )
{
	int i;
	char mlt_name[20];
	
	// Propogate these for the Jack processing callback
	mlt_properties_set_int( properties, "channels", channels );

	// Start JackRack
	if ( mlt_properties_get( properties, "src" ) )
	{
		// Create JackRack without Jack client name so that it only uses LADSPA
		jack_rack_t *jackrack = jack_rack_new( NULL, channels );
		mlt_properties_set_data( properties, "jackrack", jackrack, 0, NULL, NULL );
		jack_rack_open_file( jackrack, mlt_properties_get( properties, "src" ) );
	}
		
	// Allocate buffers
	LADSPA_Data **input_buffers = mlt_pool_alloc( sizeof( LADSPA_Data ) * channels );
	LADSPA_Data **output_buffers = mlt_pool_alloc( sizeof( LADSPA_Data ) * channels );

	// Set properties - released inside filter_close
	mlt_properties_set_data( properties, "input_buffers", input_buffers, sizeof( LADSPA_Data *) * channels, NULL, NULL );
	mlt_properties_set_data( properties, "output_buffers", output_buffers, sizeof( LADSPA_Data *) * channels, NULL, NULL );

	// Register Jack ports
	for ( i = 0; i < channels; i++ )
	{
		input_buffers[i] = mlt_pool_alloc( BUFFER_LEN * sizeof( LADSPA_Data ) );
		snprintf( mlt_name, sizeof( mlt_name ), "ibuf%d", i );
		mlt_properties_set_data( properties, mlt_name, input_buffers[i], BUFFER_LEN * sizeof( LADSPA_Data ), NULL, NULL );

		output_buffers[i] = mlt_pool_alloc( BUFFER_LEN * sizeof( LADSPA_Data ) );
		snprintf( mlt_name, sizeof( mlt_name ), "obuf%d", i );
		mlt_properties_set_data( properties, mlt_name, output_buffers[i], BUFFER_LEN * sizeof( LADSPA_Data ), NULL, NULL );
	}
}


/** Get the audio.
*/

static int ladspa_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	
	// Initialise LADSPA if needed
	jack_rack_t *jackrack = mlt_properties_get_data( filter_properties, "jackrack", NULL );
	if ( jackrack == NULL )
	{
		sample_rate = *frequency;
		initialise_jack_rack( filter_properties, *channels );
		jackrack = mlt_properties_get_data( filter_properties, "jackrack", NULL );
	}
		
	// Get the filter-specific properties
	LADSPA_Data **input_buffers = mlt_properties_get_data( filter_properties, "input_buffers", NULL );
	LADSPA_Data **output_buffers = mlt_properties_get_data( filter_properties, "output_buffers", NULL );
	
	// Process the audio
	int16_t *q = *buffer;
	int i, j;

	// Convert to floats and write into output ringbuffer
	for ( i = 0; i < *samples; i++ )
		for ( j = 0; j < *channels; j++ )
			input_buffers[ j ][ i ] = ( float )( *q ++ ) / 32768.0;

	// Do LADSPA processing
	if ( jackrack && process_ladspa( jackrack->procinfo, *samples, input_buffers, output_buffers) == 0 )
	{
		// Read from output buffer and convert from floats
		q = *buffer;
		for ( i = 0; i < *samples; i++ )
			for ( j = 0; j < *channels; j++ )
			{
				if ( output_buffers[ j ][ i ] > 1.0 )
					output_buffers[ j ][ i ] = 1.0;
				else if ( output_buffers[ j ][ i ] < -1.0 )
					output_buffers[ j ][ i ] = -1.0;
			
				if ( output_buffers[ j ][ i ] > 0 )
					*q ++ = 32767 * output_buffers[ j ][ i ];
				else
					*q ++ = 32768 * output_buffers[ j ][ i ];
			}
	}

	return 0;
}


/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	{
		mlt_frame_push_audio( frame, this );
		mlt_frame_push_audio( frame, ladspa_get_audio );
	}

	return frame;
}


static void filter_close( mlt_filter this )
{
	int i;
	char mlt_name[20];
	mlt_properties properties = MLT_FILTER_PROPERTIES( this );
	
	if ( mlt_properties_get_data( properties, "jackrack", NULL ) != NULL )
	{
		for ( i = 0; i < mlt_properties_get_int( properties, "channels" ); i++ )
		{
			snprintf( mlt_name, sizeof( mlt_name ), "obuf%d", i );
			mlt_pool_release( mlt_properties_get_data( properties, mlt_name, NULL ) );
			snprintf( mlt_name, sizeof( mlt_name ), "ibuf%d", i );
			mlt_pool_release( mlt_properties_get_data( properties, mlt_name, NULL ) );
		}
		mlt_pool_release( mlt_properties_get_data( properties, "output_buffers", NULL ) );
		mlt_pool_release( mlt_properties_get_data( properties, "input_buffers", NULL ) );
	
		jack_rack_t *jackrack = mlt_properties_get_data( properties, "jackrack", NULL );
		jack_rack_destroy( jackrack );
	}	
	this->parent.close = NULL;
	mlt_service_close( &this->parent );
}

/** Constructor for the filter.
*/

mlt_filter filter_ladspa_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );
		
		this->process = filter_process;
		this->close = filter_close;
		
		mlt_properties_set( properties, "src", arg );
	}
	return this;
}
