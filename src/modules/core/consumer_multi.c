/*
 * Copyright (C) 2011-1024 Meltytech, LLC
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

// Forward references
static int start( mlt_consumer consumer );
static int stop( mlt_consumer consumer );
static int is_stopped( mlt_consumer consumer );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer consumer );
static void purge( mlt_consumer consumer );

static mlt_properties normalisers = NULL;

/** Initialise the consumer.
*/

mlt_consumer consumer_multi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_consumer consumer = mlt_consumer_new( profile );

	if ( consumer )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

		// Set defaults
		mlt_properties_set( properties, "resource", arg );
		mlt_properties_set_int( properties, "real_time", -1 );
		mlt_properties_set_int( properties, "terminate_on_pause", 1 );

		// Init state
		mlt_properties_set_int( properties, "joined", 1 );

		// Assign callbacks
		consumer->close = consumer_close;
		consumer->start = start;
		consumer->stop = stop;
		consumer->is_stopped = is_stopped;
		consumer->purge = purge;
	}

	return consumer;
}

static mlt_consumer create_consumer( mlt_profile profile, char *id, char *arg )
{
	char *myid = id ? strdup( id ) : NULL;
	char *myarg = ( myid && !arg ) ? strchr( myid, ':' ) : NULL;
	if ( myarg )
		*myarg ++ = '\0';
	else
		myarg = arg;
	mlt_consumer consumer = mlt_factory_consumer( profile, myid, myarg );
	free( myid );
	return consumer;
}

static void create_filter( mlt_profile profile, mlt_service service, char *effect, int *created )
{
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';

	// We cannot use GLSL-based filters here.
	if ( strncmp( effect, "movit.", 6 ) && strncmp( effect, "glsl.", 5 ) )
	{
		mlt_filter filter;
		// The swscale and avcolor_space filters require resolution as arg to test compatibility
		if ( strncmp( effect, "swscale", 7 ) == 0 || strncmp( effect, "avcolo", 6 ) == 0 )
		{
			int width = mlt_properties_get_int( MLT_SERVICE_PROPERTIES( service ), "meta.media.width" );
			filter = mlt_factory_filter( profile, id, &width );
		}
		else
		{
			filter = mlt_factory_filter( profile, id, arg );
		}
		if ( filter )
		{
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_loader", 1 );
			mlt_service_attach( service, filter );
			mlt_filter_close( filter );
			*created = 1;
		}
	}
	free( id );
}

static void attach_normalisers( mlt_profile profile, mlt_service service )
{
	// Loop variable
	int i;

	// Tokeniser
	mlt_tokeniser tokeniser = mlt_tokeniser_init( );

	// We only need to load the normalising properties once
	if ( normalisers == NULL )
	{
		char temp[ 1024 ];
		snprintf( temp, sizeof(temp), "%s/core/loader.ini", mlt_environment( "MLT_DATA" ) );
		normalisers = mlt_properties_load( temp );
		mlt_factory_register_for_clean_up( normalisers, ( mlt_destructor )mlt_properties_close );
	}

	// Apply normalisers
	for ( i = 0; i < mlt_properties_count( normalisers ); i ++ )
	{
		int j = 0;
		int created = 0;
		char *value = mlt_properties_get_value( normalisers, i );
		mlt_tokeniser_parse_new( tokeniser, value, "," );
		for ( j = 0; !created && j < mlt_tokeniser_count( tokeniser ); j ++ )
			create_filter( profile, service, mlt_tokeniser_get_string( tokeniser, j ), &created );
	}

	// Close the tokeniser
	mlt_tokeniser_close( tokeniser );

	// Attach the audio and video format converters
	int created = 0;
	// movit.convert skips setting the frame->convert_image pointer if GLSL cannot be used.
	mlt_filter filter = mlt_factory_filter( profile, "movit.convert", NULL );
	if ( filter != NULL )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_loader", 1 );
		mlt_service_attach( service, filter );
		mlt_filter_close( filter );
		created = 1;
	}
	// avcolor_space and imageconvert only set frame->convert_image if it has not been set.
	create_filter( profile, service, "avcolor_space", &created );
	if ( !created )
		create_filter( profile, service, "imageconvert", &created );
	create_filter( profile, service, "audioconvert", &created );
}

static void on_frame_show( void *dummy, mlt_properties properties, mlt_frame frame )
{
	mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
}

static mlt_consumer generate_consumer( mlt_consumer consumer, mlt_properties props, int index )
{
	mlt_profile profile = NULL;
	if ( mlt_properties_get( props, "mlt_profile" ) )
		profile = mlt_profile_init( mlt_properties_get( props, "mlt_profile" ) );
	if ( !profile )
		profile = mlt_profile_clone( mlt_service_profile( MLT_CONSUMER_SERVICE(consumer) ) );
	mlt_consumer nested = create_consumer( profile, mlt_properties_get( props, "mlt_service" ),
		mlt_properties_get( props, "target" ) );

	if ( nested )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
		mlt_properties nested_props = MLT_CONSUMER_PROPERTIES(nested);
		char key[30];

		snprintf( key, sizeof(key), "%d.consumer", index );
		mlt_properties_set_data( properties, key, nested, 0, (mlt_destructor) mlt_consumer_close, NULL );
		snprintf( key, sizeof(key), "%d.profile", index );
		mlt_properties_set_data( properties, key, profile, 0, (mlt_destructor) mlt_profile_close, NULL );

		mlt_properties_set_int( nested_props, "put_mode", 1 );
		mlt_properties_pass_list( nested_props, properties, "terminate_on_pause" );
		mlt_properties_set( props, "consumer", NULL );
		// set mlt_profile before other properties to facilitate presets
		mlt_properties_pass_list( nested_props, props, "mlt_profile" );
		mlt_properties_inherit( nested_props, props );

		attach_normalisers( profile, MLT_CONSUMER_SERVICE(nested) );

		// Relay the first available consumer-frame-show event
		mlt_event event = mlt_properties_get_data( properties, "frame-show-event", NULL );
		if ( !event )
		{
			event = mlt_events_listen( nested_props, properties, "consumer-frame-show", (mlt_listener) on_frame_show );
			mlt_properties_set_data( properties, "frame-show-event", event, 0, /*mlt_event_close*/ NULL, NULL );
		}
	}
	else
	{
		mlt_profile_close( profile );
	}
	return nested;
}

static void foreach_consumer_init( mlt_consumer consumer )
{
	const char *resource = mlt_properties_get( MLT_CONSUMER_PROPERTIES(consumer), "resource" );
	mlt_properties properties = mlt_properties_parse_yaml( resource );
	char key[20];
	int index = 0;

	if ( mlt_properties_get_data( MLT_CONSUMER_PROPERTIES(consumer), "0", NULL ) )
	{
		// Properties set directly by application
		mlt_properties p;

		if ( properties )
			mlt_properties_close( properties );
		properties = MLT_CONSUMER_PROPERTIES(consumer);
		do {
			snprintf( key, sizeof(key), "%d", index );
			if ( ( p = mlt_properties_get_data( properties, key, NULL ) ) )
				generate_consumer( consumer, p, index++ );
		} while ( p );
	}
	else if ( properties && mlt_properties_get_data( properties, "0", NULL ) )
	{
		// YAML file supplied
		mlt_properties p;

		do {
			snprintf( key, sizeof(key), "%d", index );
			if ( ( p = mlt_properties_get_data( properties, key, NULL ) ) )
				generate_consumer( consumer, p, index++ );
		} while ( p );
		mlt_properties_close( properties );
	}
	else
	{
		// properties file supplied or properties on this consumer
		const char *s;

		if ( properties )
			mlt_properties_close( properties );
		if ( resource )
			properties = mlt_properties_load( resource );
		else
			properties = MLT_CONSUMER_PROPERTIES( consumer );

		do {
			snprintf( key, sizeof(key), "%d", index );
			if ( ( s = mlt_properties_get( properties, key ) ) )
			{
				mlt_properties p = mlt_properties_new();
				if ( !p ) break;

				// Terminate mlt_service value at the argument delimiter if supplied.
				// Needed here instead of just relying upon create_consumer() so that
				// a properties preset is picked up correctly.
				char *service = strdup( mlt_properties_get( properties, key ) );
				char *arg = strchr( service, ':' );
				if ( arg ) {
					*arg ++ = '\0';
					mlt_properties_set( p, "target", arg );
				}
				mlt_properties_set( p, "mlt_service", service );
				free( service );

				snprintf( key, sizeof(key), "%d.", index );

				int i, count;
				count = mlt_properties_count( properties );
				for ( i = 0; i < count; i++ )
				{
					char *name = mlt_properties_get_name( properties, i );
					if ( !strncmp( name, key, strlen(key) ) )
						mlt_properties_set( p, name + strlen(key),
							mlt_properties_get_value( properties, i ) );
				}
				generate_consumer( consumer, p, index++ );
				mlt_properties_close( p );
			}
		} while ( s );
		if ( resource )
			mlt_properties_close( properties );
	}
}

static void foreach_consumer_start( mlt_consumer consumer )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested )
		{
			mlt_properties nested_props = MLT_CONSUMER_PROPERTIES(nested);
			mlt_properties_set_position( nested_props, "_multi_position", 0 );
			mlt_properties_set_data( nested_props, "_multi_audio", NULL, 0, NULL, NULL );
			mlt_properties_set_int( nested_props, "_multi_samples", 0 );
			mlt_consumer_start( nested );
		}
	} while ( nested );
}

static void foreach_consumer_refresh( mlt_consumer consumer )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested ) mlt_properties_set_int( MLT_CONSUMER_PROPERTIES(nested), "refresh", 1 );
	} while ( nested );
}

// Update certain properties on this consumer from the child consumers.
static void foreach_consumer_update( mlt_consumer consumer )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested )
			mlt_properties_pass_list( properties, MLT_CONSUMER_PROPERTIES(nested), "color_trc" );
	} while ( nested );
}

static void foreach_consumer_put( mlt_consumer consumer, mlt_frame frame )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested )
		{
			mlt_properties nested_props = MLT_CONSUMER_PROPERTIES(nested);
			double self_fps = mlt_properties_get_double( properties, "fps" );
			double nested_fps = mlt_properties_get_double( nested_props, "fps" );
			mlt_position nested_pos = mlt_properties_get_position( nested_props, "_multi_position" );
			mlt_position self_pos = mlt_frame_get_position( frame );
			double self_time = self_pos / self_fps;
			double nested_time = nested_pos / nested_fps;

			// get the audio for the current frame
			uint8_t *buffer = NULL;
			mlt_audio_format format = mlt_audio_s16;
			int channels = mlt_properties_get_int( properties, "channels" );
			int frequency = mlt_properties_get_int( properties, "frequency" );
			int current_samples = mlt_sample_calculator( self_fps, frequency, self_pos );
			mlt_frame_get_audio( frame, (void**) &buffer, &format, &frequency, &channels, &current_samples );
			int current_size = mlt_audio_format_size( format, current_samples, channels );

			// get any leftover audio
			int prev_size = 0;
			uint8_t *prev_buffer = mlt_properties_get_data( nested_props, "_multi_audio", &prev_size );
			uint8_t *new_buffer = NULL;
			if ( prev_size > 0 )
			{
				new_buffer = mlt_pool_alloc( prev_size + current_size );
				memcpy( new_buffer, prev_buffer, prev_size );
				memcpy( new_buffer + prev_size, buffer, current_size );
				buffer = new_buffer;
			}
			current_size += prev_size;
			current_samples += mlt_properties_get_int( nested_props, "_multi_samples" );

			while ( nested_time <= self_time )
			{
				// put ideal number of samples into cloned frame
				int deeply = index > 1 ? 1 : 0;
				mlt_frame clone_frame = mlt_frame_clone( frame, deeply );
				mlt_properties clone_props = MLT_FRAME_PROPERTIES( clone_frame );
				int nested_samples = mlt_sample_calculator( nested_fps, frequency, nested_pos );
				// -10 is an optimization to avoid tiny amounts of leftover samples
				nested_samples = nested_samples > current_samples - 10 ? current_samples : nested_samples;
				int nested_size = mlt_audio_format_size( format, nested_samples, channels );
				if ( nested_size > 0 )
				{
					prev_buffer = mlt_pool_alloc( nested_size );
					memcpy( prev_buffer, buffer, nested_size );
				}
				else
				{
					prev_buffer = NULL;
					nested_size = 0;
				}
				mlt_frame_set_audio( clone_frame, prev_buffer, format, nested_size, mlt_pool_release );
				mlt_properties_set_int( clone_props, "audio_samples", nested_samples );
				mlt_properties_set_int( clone_props, "audio_frequency", frequency );
				mlt_properties_set_int( clone_props, "audio_channels", channels );

				// chomp the audio
				current_samples -= nested_samples;
				current_size -= nested_size;
				buffer += nested_size;

				// Fix some things
				mlt_properties_set_int( clone_props, "meta.media.width",
					mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "width" ) );
				mlt_properties_set_int( clone_props, "meta.media.height",
					mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "height" ) );

				// send frame to nested consumer
				mlt_consumer_put_frame( nested, clone_frame );
				mlt_properties_set_position( nested_props, "_multi_position", ++nested_pos );
				nested_time = nested_pos / nested_fps;
			}

			// save any remaining audio
			if ( current_size > 0 )
			{
				prev_buffer = mlt_pool_alloc( current_size );
				memcpy( prev_buffer, buffer, current_size );
			}
			else
			{
				prev_buffer = NULL;
				current_size = 0;
			}
			mlt_pool_release( new_buffer );
			mlt_properties_set_data( nested_props, "_multi_audio", prev_buffer, current_size, mlt_pool_release, NULL );
			mlt_properties_set_int( nested_props, "_multi_samples", current_samples );
		}
	} while ( nested );
}

static void foreach_consumer_stop( mlt_consumer consumer )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;
	struct timespec tm = { 0, 1000 * 1000 };

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested )
		{
			// Let consumer with terminate_on_pause stop on their own
			if ( mlt_properties_get_int( MLT_CONSUMER_PROPERTIES(nested), "terminate_on_pause" ) )
			{
				// Send additional dummy frame to unlatch nested consumer's threads
				mlt_consumer_put_frame( nested, mlt_frame_init( MLT_CONSUMER_SERVICE(consumer) ) );
				// wait for stop
				while ( !mlt_consumer_is_stopped( nested ) )
					nanosleep( &tm, NULL );
			}
			else
			{
				mlt_consumer_stop( nested );
			}
		}
	} while ( nested );
}

/** Start the consumer.
*/

static int start( mlt_consumer consumer )
{
	// Check that we're not already running
	if ( is_stopped( consumer ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Assign the thread to properties with automatic dealloc
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );
		mlt_properties_set_int( properties, "joined", 0 );

		// Construct and start nested consumers
		if ( !mlt_properties_get_data( properties, "0.consumer", NULL ) )
			foreach_consumer_init( consumer );
		foreach_consumer_start( consumer );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, consumer );
	}
	return 0;
}

/** Stop the consumer.
*/

static int stop( mlt_consumer consumer )
{
	// Check that we're running
	if ( !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES(consumer), "joined" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
		pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );

		// Wait for termination
		if ( thread )
		{
			foreach_consumer_refresh( consumer );
			pthread_join( *thread, NULL );
		}
		mlt_properties_set_int( properties, "joined", 1 );

		// Stop nested consumers
		foreach_consumer_stop( consumer );
	}

	return 0;
}

/** Determine if the consumer is stopped.
*/

static int is_stopped( mlt_consumer consumer )
{
	return !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( consumer ), "running" );
}

/** Purge each of the child consumers.
*/

static void purge( mlt_consumer consumer )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	if ( mlt_properties_get_int( properties, "running" ) )
	{
		mlt_consumer nested = NULL;
		char key[30];
		int index = 0;

		do {
			snprintf( key, sizeof(key), "%d.consumer", index++ );
			nested = mlt_properties_get_data( properties, key, NULL );
			mlt_consumer_purge( nested );
		} while ( nested );
	}
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread( void *arg )
{
	mlt_consumer consumer = arg;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_frame frame = NULL;

	// Determine whether to stop at end-of-media
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	foreach_consumer_update( consumer );

	// Loop while running
	while ( !terminated && !is_stopped( consumer ) )
	{
		// Get the next frame
		frame = mlt_consumer_rt_frame( consumer );

		// Check for termination
		if ( terminate_on_pause && frame )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Check that we have a frame to work with
		if ( frame && !terminated && !is_stopped( consumer ) )
		{
			if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "rendered" ) )
			{
				if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "_speed" ) == 0 )
					foreach_consumer_refresh( consumer );
				foreach_consumer_put( consumer, frame );
			}
			else
			{
				int dropped = mlt_properties_get_int( properties, "_dropped" );
				mlt_log_info( MLT_CONSUMER_SERVICE(consumer), "dropped frame %d\n", ++dropped );
				mlt_properties_set_int( properties, "_dropped", dropped );
			}
			mlt_frame_close( frame );
		}
		else
		{
			if ( frame && terminated )
			{
				// Send this termination frame to nested consumers for their cancellation
				foreach_consumer_put( consumer, frame );
			}
			if ( frame )
				mlt_frame_close( frame );
			terminated = 1;
		}
	}

	// Indicate that the consumer is stopped
	mlt_consumer_stopped( consumer );

	return NULL;
}

/** Close the consumer.
*/

static void consumer_close( mlt_consumer consumer )
{
	mlt_consumer_stop( consumer );

	// Close the parent
	mlt_consumer_close( consumer );
	free( consumer );
}
