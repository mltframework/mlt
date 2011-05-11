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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <string.h>

#include "jack_rack.h"

#define BUFFER_LEN 10000

static jack_rack_t* initialise_jack_rack( mlt_properties properties, int channels )
{
	jack_rack_t *jackrack = NULL;
	char *resource = mlt_properties_get( properties, "resource" );
	if ( !resource && mlt_properties_get( properties, "src" ) )
		resource = mlt_properties_get( properties, "src" );

	// Start JackRack
	if ( resource || mlt_properties_get_int64( properties, "_pluginid" ) )
	{
		// Create JackRack without Jack client name so that it only uses LADSPA
		jackrack = jack_rack_new( NULL, channels );
		mlt_properties_set_data( properties, "jackrack", jackrack, 0,
			(mlt_destructor) jack_rack_destroy, NULL );

		if ( resource )
			// Load JACK Rack XML file
			jack_rack_open_file( jackrack, resource );
		else if ( mlt_properties_get_int64( properties, "_pluginid" ) )
		{
			// Load one LADSPA plugin by its UniqueID
			unsigned long id = mlt_properties_get_int64( properties, "_pluginid" );
			plugin_desc_t *desc = plugin_mgr_get_any_desc( jackrack->plugin_mgr, id );
			plugin_t *plugin;
			if ( desc && ( plugin = jack_rack_instantiate_plugin( jackrack, desc ) ) )
			{
				// TODO: move this into get_audio when keyframing is ready
				LADSPA_Data value;
				int index, c;

				plugin->enabled = TRUE;
				for ( index = 0; index < desc->control_port_count; index++ )
				{
					// Apply the control port values
					char key[20];
					value = plugin_desc_get_default_control_value( desc, index, sample_rate );
					snprintf( key, sizeof(key), "%d", index );
					if ( mlt_properties_get( properties, key ) )
						value = mlt_properties_get_double( properties, key );
					for ( c = 0; c < plugin->copies; c++ )
						plugin->holders[c].control_memory[index] = value;
				}
				plugin->wet_dry_enabled = mlt_properties_get( properties, "wetness" ) != NULL;
				if ( plugin->wet_dry_enabled )
				{
					value = mlt_properties_get_double( properties, "wetness" );
					for ( c = 0; c < channels; c++ )
						plugin->wet_dry_values[c] = value;
				}
				process_add_plugin( jackrack->procinfo, plugin );
			}
			else
				mlt_log_error( properties, "failed to load plugin %lu\n", id );
		}
	}
	return jackrack;
}


/** Get the audio.
*/

static int ladspa_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the producer's audio
	*format = mlt_audio_float;
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	
	// Initialise LADSPA if needed
	jack_rack_t *jackrack = mlt_properties_get_data( filter_properties, "jackrack", NULL );
	if ( jackrack == NULL )
	{
		sample_rate = *frequency; // global inside jack_rack
		jackrack = initialise_jack_rack( filter_properties, *channels );
	}
		
	// Get the filter-specific properties
	LADSPA_Data **input_buffers  = mlt_pool_alloc( sizeof( LADSPA_Data* ) * *channels );
	LADSPA_Data **output_buffers = mlt_pool_alloc( sizeof( LADSPA_Data* ) * *channels );

	int i;
	for ( i = 0; i < *channels; i++ )
	{
		input_buffers[i]  = (LADSPA_Data*) *buffer + i * *samples;
		output_buffers[i] = (LADSPA_Data*) *buffer + i * *samples;
	}

	// Do LADSPA processing
	int error = jackrack && process_ladspa( jackrack->procinfo, *samples, input_buffers, output_buffers );

	mlt_pool_release( input_buffers );
	mlt_pool_release( output_buffers );

	return error;
}


/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	if ( mlt_frame_is_test_audio( frame ) == 0 )
	{
		mlt_frame_push_audio( frame, this );
		mlt_frame_push_audio( frame, ladspa_get_audio );
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_ladspa_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );
		this->process = filter_process;
		mlt_properties_set( properties, "resource", arg );
		if ( !strncmp( id, "ladspa.", 7 ) )
			mlt_properties_set( properties, "_pluginid", id + 7 );
	}
	return this;
}
