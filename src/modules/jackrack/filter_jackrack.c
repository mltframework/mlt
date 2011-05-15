/*
 * filter_jackrack.c -- filter audio through Jack and/or LADSPA plugins
 * Copyright (C) 2004 Ushodaya Enterprises Limited
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

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <string.h>

#include "jack_rack.h"

#define BUFFER_LEN 204800 * 6

static void jack_started_transmitter( mlt_listener listener, mlt_properties owner, mlt_service service, void **args )
{
	listener( owner, service, (mlt_position*) args[0] );
}

static void jack_stopped_transmitter( mlt_listener listener, mlt_properties owner, mlt_service service, void **args )
{
	listener( owner, service, (mlt_position*) args[0] );
}

static void jack_seek_transmitter( mlt_listener listener, mlt_properties owner, mlt_service service, void **args )
{
	listener( owner, service, (mlt_position*) args[0] );
}

#define JACKSTATE(x) (x==JackTransportStopped?"stopped":x==JackTransportStarting?"starting":x==JackTransportRolling?"rolling":"unknown")

static int jack_sync( jack_transport_state_t state, jack_position_t *jack_pos, void *arg )
{
	mlt_filter filter = (mlt_filter) arg;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
	mlt_position position = mlt_profile_fps( profile ) * jack_pos->frame / jack_pos->frame_rate + 0.5;
	int result = 1;

	mlt_log_debug( MLT_FILTER_SERVICE(filter), "%s frame %u rate %u pos %d last_pos %d\n",
		JACKSTATE(state), jack_pos->frame, jack_pos->frame_rate, position,
		mlt_properties_get_position( properties, "_last_pos" ) );
	if ( state == JackTransportStopped )
	{
		mlt_events_fire( properties, "jack-stopped", &position, NULL );
		mlt_properties_set_int( properties, "_sync_guard", 0 );
	}
	else if ( state == JackTransportStarting )
	{
		result = 0;
		if ( !mlt_properties_get_int( properties, "_sync_guard" ) )
		{
			mlt_properties_set_int( properties, "_sync_guard", 1 );
			mlt_events_fire( properties, "jack-started", &position, NULL );
		}
		else if ( position >= mlt_properties_get_position( properties, "_last_pos" ) - 2 )
		{
			mlt_properties_set_int( properties, "_sync_guard", 0 );
			result = 1;
		}
	}
	else
	{
		mlt_properties_set_int( properties, "_sync_guard", 0 );
	}

	return result;
}

static void on_jack_start( mlt_properties owner, mlt_properties properties )
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	jack_client_t *jack_client = mlt_properties_get_data( properties, "jack_client", NULL );
	jack_transport_start( jack_client );
}

static void on_jack_stop( mlt_properties owner, mlt_properties properties )
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	jack_client_t *jack_client = mlt_properties_get_data( properties, "jack_client", NULL );
	jack_transport_stop( jack_client );
}

static void on_jack_seek( mlt_properties owner, mlt_filter filter, mlt_position *position )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );


	mlt_properties_set_int( properties, "_sync_guard", 1 );
	mlt_properties_set_position( properties, "_jack_seek", *position );
	return;


	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	jack_client_t *jack_client = mlt_properties_get_data( properties, "jack_client", NULL );
	jack_nframes_t jack_frame = jack_get_sample_rate( jack_client );
	jack_frame *= *position / mlt_profile_fps( profile );

	fprintf(stderr, "%s: %d\n", __FUNCTION__, *position);
	jack_transport_locate( jack_client, jack_frame );
}

static void initialise_jack_ports( mlt_properties properties )
{
	int i;
	char mlt_name[20], rack_name[30];
	jack_port_t **port = NULL;
	jack_client_t *jack_client = mlt_properties_get_data( properties, "jack_client", NULL );
	jack_nframes_t jack_buffer_size = jack_get_buffer_size( jack_client );
	
	// Propogate these for the Jack processing callback
	int channels = mlt_properties_get_int( properties, "channels" );

	// Start JackRack
	if ( mlt_properties_get( properties, "src" ) )
	{
		snprintf( rack_name, sizeof( rack_name ), "jackrack%d", getpid() );
		jack_rack_t *jackrack = jack_rack_new( rack_name, mlt_properties_get_int( properties, "channels" ) );
		jack_rack_open_file( jackrack, mlt_properties_get( properties, "src" ) );		
		
		mlt_properties_set_data( properties, "jackrack", jackrack, 0,
			(mlt_destructor) jack_rack_destroy, NULL );
		mlt_properties_set( properties, "_rack_client_name", rack_name );
	}
	else
	{
		// We have to set this to something to prevent re-initialization.
		mlt_properties_set_data( properties, "jackrack", jack_client, 0, NULL, NULL );
	}
		
	// Allocate buffers and ports
	jack_ringbuffer_t **output_buffers = mlt_pool_alloc( sizeof( jack_ringbuffer_t *) * channels );
	jack_ringbuffer_t **input_buffers = mlt_pool_alloc( sizeof( jack_ringbuffer_t *) * channels );
	jack_port_t **jack_output_ports = mlt_pool_alloc( sizeof(jack_port_t *) * channels );
	jack_port_t **jack_input_ports = mlt_pool_alloc( sizeof(jack_port_t *) * channels );
	float **jack_output_buffers = mlt_pool_alloc( sizeof(float *) * jack_buffer_size );
	float **jack_input_buffers = mlt_pool_alloc( sizeof(float *) * jack_buffer_size );

	// Set properties - released inside filter_close
	mlt_properties_set_data( properties, "output_buffers", output_buffers,
		sizeof( jack_ringbuffer_t *) * channels, mlt_pool_release, NULL );
	mlt_properties_set_data( properties, "input_buffers", input_buffers,
		sizeof( jack_ringbuffer_t *) * channels, mlt_pool_release, NULL );
	mlt_properties_set_data( properties, "jack_output_ports", jack_output_ports,
		sizeof( jack_port_t *) * channels, mlt_pool_release, NULL );
	mlt_properties_set_data( properties, "jack_input_ports", jack_input_ports,
		sizeof( jack_port_t *) * channels, mlt_pool_release, NULL );
	mlt_properties_set_data( properties, "jack_output_buffers", jack_output_buffers,
		sizeof( float *) * channels, mlt_pool_release, NULL );
	mlt_properties_set_data( properties, "jack_input_buffers", jack_input_buffers,
		sizeof( float *) * channels, mlt_pool_release, NULL );

	// Start Jack processing - required before registering ports
	jack_activate( jack_client );
	
	// Register Jack ports
	for ( i = 0; i < channels; i++ )
	{
		int in;
		
		output_buffers[i] = jack_ringbuffer_create( BUFFER_LEN * sizeof(float) );
		input_buffers[i] = jack_ringbuffer_create( BUFFER_LEN * sizeof(float) );
		snprintf( mlt_name, sizeof( mlt_name ), "obuf%d", i );
		mlt_properties_set_data( properties, mlt_name, output_buffers[i],
			BUFFER_LEN * sizeof(float), (mlt_destructor) jack_ringbuffer_free, NULL );
		snprintf( mlt_name, sizeof( mlt_name ), "ibuf%d", i );
		mlt_properties_set_data( properties, mlt_name, input_buffers[i],
			BUFFER_LEN * sizeof(float), (mlt_destructor) jack_ringbuffer_free, NULL );
		
		for ( in = 0; in < 2; in++ )
		{
			snprintf( mlt_name, sizeof( mlt_name ), "%s_%d", in ? "in" : "out", i + 1);
			port = ( in ? &jack_input_ports[i] : &jack_output_ports[i] );
			
			*port =  jack_port_register( jack_client, mlt_name, JACK_DEFAULT_AUDIO_TYPE,
				( in ? JackPortIsInput : JackPortIsOutput ) | JackPortIsTerminal, 0 );
		}
	}
	
	// Establish connections
	for ( i = 0; i < channels; i++ )
	{
		int in;
		for ( in = 0; in < 2; in++ )
		{
			port = ( in ? &jack_input_ports[i] : &jack_output_ports[i] );
			snprintf( mlt_name, sizeof( mlt_name ), "%s", jack_port_name( *port ) );

			snprintf( rack_name, sizeof( rack_name ), "%s_%d", in ? "in" : "out", i + 1 );
			if ( mlt_properties_get( properties, "_rack_client_name" ) )
				snprintf( rack_name, sizeof( rack_name ), "%s:%s_%d", mlt_properties_get( properties, "_rack_client_name" ), in ? "out" : "in", i + 1);
			else if ( mlt_properties_get( properties, rack_name ) )
				snprintf( rack_name, sizeof( rack_name ), "%s", mlt_properties_get( properties, rack_name ) );
			else
				snprintf( rack_name, sizeof( rack_name ), "%s:%s_%d", mlt_properties_get( properties, "_client_name" ), in ? "out" : "in", i + 1);
			
			if ( in )
			{
				mlt_log_verbose( NULL, "JACK connect %s to %s\n", rack_name, mlt_name );
				jack_connect( jack_client, rack_name, mlt_name );
			}
			else
			{
				mlt_log_verbose( NULL, "JACK connect %s to %s\n", mlt_name, rack_name );
				jack_connect( jack_client, mlt_name, rack_name );
			}
		}
	}
}

static int jack_process (jack_nframes_t frames, void * data)
{
	mlt_filter filter = (mlt_filter) data;
 	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	int channels = mlt_properties_get_int( properties, "channels" );
	int frame_size = mlt_properties_get_int( properties, "_samples" ) * sizeof(float);
	int sync = mlt_properties_get_int( properties, "_sync" );
	int err = 0;
	int i;
	static int total_size = 0;
  
	jack_ringbuffer_t **output_buffers = mlt_properties_get_data( properties, "output_buffers", NULL );
	if ( output_buffers == NULL )
		return 0;
	jack_ringbuffer_t **input_buffers = mlt_properties_get_data( properties, "input_buffers", NULL );
	jack_port_t **jack_output_ports = mlt_properties_get_data( properties, "jack_output_ports", NULL );
	jack_port_t **jack_input_ports = mlt_properties_get_data( properties, "jack_input_ports", NULL );
	float **jack_output_buffers = mlt_properties_get_data( properties, "jack_output_buffers", NULL );
	float **jack_input_buffers = mlt_properties_get_data( properties, "jack_input_buffers", NULL );
	pthread_mutex_t *output_lock = mlt_properties_get_data( properties, "output_lock", NULL );
	pthread_cond_t *output_ready = mlt_properties_get_data( properties, "output_ready", NULL );
	
	for ( i = 0; i < channels; i++ )
	{
		size_t jack_size = ( frames * sizeof(float) );
		size_t ring_size;
		
		// Send audio through out port
		jack_output_buffers[i] = jack_port_get_buffer( jack_output_ports[i], frames );
		if ( ! jack_output_buffers[i] )
		{
			mlt_log_error( MLT_FILTER_SERVICE(filter), "no buffer for output port %d\n", i );
			err = 1;
			break;
		}
		ring_size = jack_ringbuffer_read_space( output_buffers[i] );
		jack_ringbuffer_read( output_buffers[i], ( char * )jack_output_buffers[i], ring_size < jack_size ? ring_size : jack_size );
		
		// Return audio through in port
		jack_input_buffers[i] = jack_port_get_buffer( jack_input_ports[i], frames );
		if ( ! jack_input_buffers[i] )
		{
			mlt_log_error( MLT_FILTER_SERVICE(filter), "no buffer for input port %d\n", i );
			err = 1;
			break;
		}
		
		// Do not start returning audio until we have sent first mlt frame
		if ( sync && i == 0 && frame_size > 0 )
			total_size += ring_size;
		mlt_log_debug( MLT_FILTER_SERVICE(filter), "sync %d frame_size %d ring_size %zu jack_size %zu\n", sync, frame_size, ring_size, jack_size );
		
		if ( ! sync || ( frame_size > 0  && total_size >= frame_size ) )
		{
			ring_size = jack_ringbuffer_write_space( input_buffers[i] );
			jack_ringbuffer_write( input_buffers[i], ( char * )jack_input_buffers[i], ring_size < jack_size ? ring_size : jack_size );
			
			if ( sync )
			{
				// Tell mlt that audio is available
				pthread_mutex_lock( output_lock);
				pthread_cond_signal( output_ready );
				pthread_mutex_unlock( output_lock);

				// Clear sync phase
				mlt_properties_set_int( properties, "_sync", 0 );
			}
		}
	}

	return err;
}


/** Get the audio.
*/

static int jackrack_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	int jack_frequency = mlt_properties_get_int( filter_properties, "_sample_rate" );

	// Get the producer's audio
	*format = mlt_audio_float;
	mlt_frame_get_audio( frame, buffer, format, &jack_frequency, channels, samples );
	
	// TODO: Deal with sample rate differences
	if ( *frequency != jack_frequency )
		mlt_log_error( MLT_FILTER_SERVICE( filter ), "mismatching frequencies JACK = %d actual = %d\n",
			jack_frequency, *frequency );
	*frequency = jack_frequency;

	// Initialise Jack ports and connections if needed
	if ( mlt_properties_get_int( filter_properties, "_samples" ) == 0 )
		mlt_properties_set_int( filter_properties, "_samples", *samples );
	
	// Get the filter-specific properties
	jack_ringbuffer_t **output_buffers = mlt_properties_get_data( filter_properties, "output_buffers", NULL );
	jack_ringbuffer_t **input_buffers = mlt_properties_get_data( filter_properties, "input_buffers", NULL );
//	pthread_mutex_t *output_lock = mlt_properties_get_data( filter_properties, "output_lock", NULL );
//	pthread_cond_t *output_ready = mlt_properties_get_data( filter_properties, "output_ready", NULL );
	
	// Process the audio
	float *q = (float*) *buffer;
	size_t size = *samples * sizeof(float);
	int j;
//	struct timespec tm = { 0, 0 };

	// Write into output ringbuffer
	for ( j = 0; j < *channels; j++ )
	{
		if ( jack_ringbuffer_write_space( output_buffers[j] ) >= size )
			jack_ringbuffer_write( output_buffers[j], (char*)( q + j * *samples ), size );
	}

	// Synchronization phase - wait for signal from Jack process
	while ( jack_ringbuffer_read_space( input_buffers[ *channels - 1 ] ) < size ) ;
		//pthread_cond_wait( output_ready, output_lock );
		
	// Read from input ringbuffer
	for ( j = 0; j < *channels; j++, q++ )
	{
		if ( jack_ringbuffer_read_space( input_buffers[j] ) >= size )
			jack_ringbuffer_read( input_buffers[j], (char*)( q + j * *samples ), size );
	}

	// Do JACK seeking if requested
	mlt_position pos = mlt_frame_get_position( frame );
	mlt_properties_set_position( filter_properties, "_last_pos", pos );
	if ( pos == mlt_properties_get_position( filter_properties, "_jack_seek" ) )
	{
		jack_client_t *jack_client = mlt_properties_get_data( filter_properties, "jack_client", NULL );
		jack_position_t jack_pos;
		jack_transport_query( jack_client, &jack_pos );
		double fps = mlt_profile_fps( mlt_service_profile( MLT_FILTER_SERVICE(filter) ) );
		jack_nframes_t jack_frame = jack_pos.frame_rate * pos / fps;
		jack_transport_locate( jack_client, jack_frame );
		mlt_properties_set_position( filter_properties, "_jack_seek", -1 );
	}

	return 0;
}


/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );
		mlt_frame_push_audio( frame, this );
		mlt_frame_push_audio( frame, jackrack_get_audio );
		
		if ( !mlt_properties_get_data( properties, "jackrack", NULL ) )
			initialise_jack_ports( properties );
	}

	return frame;
}


static void filter_close( mlt_filter this )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( this );
	jack_client_t *jack_client = mlt_properties_get_data( properties, "jack_client", NULL );
	jack_deactivate( jack_client );
	jack_client_close( jack_client );
	this->parent.close = NULL;
	mlt_service_close( &this->parent );
}

/** Constructor for the filter.
*/

mlt_filter filter_jackrack_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		char name[14];
		
		snprintf( name, sizeof( name ), "mlt%d", getpid() );
		jack_client_t *jack_client = jack_client_open( name, JackNullOption, NULL );
		if ( jack_client )
		{
			mlt_properties properties = MLT_FILTER_PROPERTIES( this );
			pthread_mutex_t *output_lock = mlt_pool_alloc( sizeof( pthread_mutex_t ) );
			pthread_cond_t  *output_ready = mlt_pool_alloc( sizeof( pthread_cond_t ) );
			
			jack_set_process_callback( jack_client, jack_process, this );
			jack_set_sync_callback( jack_client, jack_sync, this );
			jack_set_sync_timeout( jack_client, 5000000 );
			//TODO: jack_on_shutdown( jack_client, jack_shutdown_cb, this );
			this->process = filter_process;
			this->close = filter_close;
			pthread_mutex_init( output_lock, NULL );
			pthread_cond_init( output_ready, NULL );
			
			mlt_properties_set( properties, "src", arg );
			mlt_properties_set( properties, "_client_name", name );
			mlt_properties_set_data( properties, "jack_client", jack_client, 0, NULL, NULL );
			mlt_properties_set_int( properties, "_sample_rate", jack_get_sample_rate( jack_client ) );
			mlt_properties_set_data( properties, "output_lock", output_lock, 0, mlt_pool_release, NULL );
			mlt_properties_set_data( properties, "output_ready", output_ready, 0, mlt_pool_release, NULL );
			mlt_properties_set_int( properties, "_sync", 1 );
			mlt_properties_set_int( properties, "channels", 2 );

			mlt_events_register( properties, "jack-started", (mlt_transmitter) jack_started_transmitter );
			mlt_events_register( properties, "jack-stopped", (mlt_transmitter) jack_stopped_transmitter );
			mlt_events_register( properties, "jack-start", NULL );
			mlt_events_register( properties, "jack-stop", NULL );
			mlt_events_register( properties, "jack-seek", (mlt_transmitter) jack_seek_transmitter );
			mlt_events_listen( properties, properties, "jack-start", (mlt_listener) on_jack_start );
			mlt_events_listen( properties, properties, "jack-stop", (mlt_listener) on_jack_stop );
			mlt_events_listen( properties, this, "jack-seek", (mlt_listener) on_jack_seek );
			mlt_properties_set_position( properties, "_jack_seek", -1 );
		}
	}
	return this;
}
