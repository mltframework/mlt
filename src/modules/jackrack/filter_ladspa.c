/*
 * filter_ladspa.c -- filter audio through LADSPA plugins
 * Copyright (C) 2004-2016 Meltytech, LLC
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
				plugin->enabled = TRUE;
				process_add_plugin( jackrack->procinfo, plugin );
				mlt_properties_set_int( properties, "instances", plugin->copies );
			}
			else
			{
				mlt_log_error( properties, "failed to load plugin %lu\n", id );
				return jackrack;
			}

			if ( plugin && plugin->desc && plugin->copies == 0 )
			{
				mlt_log_warning( properties, "Not compatible with %d channels. Requesting %lu channels instead.\n",
					channels, plugin->desc->channels );
				jackrack = initialise_jack_rack( properties, plugin->desc->channels );
			}
		}
	}
	return jackrack;
}


/** Get the audio.
*/

static int ladspa_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	int error = 0;

	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Initialise LADSPA if needed
	jack_rack_t *jackrack = mlt_properties_get_data( filter_properties, "jackrack", NULL );
	if ( jackrack == NULL )
	{
		sample_rate = *frequency; // global inside jack_rack
		jackrack = initialise_jack_rack( filter_properties, *channels );
	}

	if ( jackrack && jackrack->procinfo && jackrack->procinfo->chain &&
		 mlt_properties_get_int64( filter_properties, "_pluginid" ) )
	{
		plugin_t *plugin = jackrack->procinfo->chain;
		LADSPA_Data value;
		int i, c;
		mlt_position position = mlt_filter_get_position( filter, frame );
		mlt_position length = mlt_filter_get_length2( filter, frame );

		// Get the producer's audio
		*channels = jackrack->channels;
		*format = mlt_audio_float;
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

		for ( i = 0; i < plugin->desc->control_port_count; i++ )
		{
			// Apply the control port values
			char key[20];
			value = plugin_desc_get_default_control_value( plugin->desc, i, sample_rate );
			snprintf( key, sizeof(key), "%d", i );
			if ( mlt_properties_get( filter_properties, key ) )
				value = mlt_properties_anim_get_double( filter_properties, key, position, length );
			for ( c = 0; c < plugin->copies; c++ )
				plugin->holders[c].control_memory[i] = value;
		}
		plugin->wet_dry_enabled = mlt_properties_get( filter_properties, "wetness" ) != NULL;
		if ( plugin->wet_dry_enabled )
		{
			value = mlt_properties_anim_get_double( filter_properties, "wetness", position, length );
			for ( c = 0; c < *channels; c++ )
				plugin->wet_dry_values[c] = value;
		}

		// Configure the buffers
		LADSPA_Data **input_buffers  = mlt_pool_alloc( sizeof( LADSPA_Data* ) * *channels );
		LADSPA_Data **output_buffers = mlt_pool_alloc( sizeof( LADSPA_Data* ) * *channels );

		for ( i = 0; i < *channels; i++ )
		{
			input_buffers[i]  = (LADSPA_Data*) *buffer + i * *samples;
			output_buffers[i] = (LADSPA_Data*) *buffer + i * *samples;
		}

		// Do LADSPA processing
		error = process_ladspa( jackrack->procinfo, *samples, input_buffers, output_buffers );

		mlt_pool_release( input_buffers );
		mlt_pool_release( output_buffers );

		// read the status port values
		for ( i = 0; i < plugin->desc->status_port_count; i++ )
		{
			char key[20];
			int p = plugin->desc->status_port_indicies[i];
			for ( c = 0; c < plugin->copies; c++ )
			{
				snprintf( key, sizeof(key), "%d[%d]", p, c );
				value = plugin->holders[c].status_memory[i];
				mlt_properties_set_double( filter_properties, key, value );
			}
		}
	}
	else
	{
		// Nothing to do.
		error = mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
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
