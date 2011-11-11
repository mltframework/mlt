/*
 * Copyright (C) 2011 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Forward references
static int start( mlt_consumer consumer );
static int stop( mlt_consumer consumer );
static int is_stopped( mlt_consumer consumer );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer consumer );

static mlt_properties normalisers = NULL;

/** Initialise the consumer.
*/

mlt_consumer consumer_multi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_consumer consumer = mlt_consumer_new( profile );

	if ( consumer )
	{
		// Assign callbacks
		consumer->close = consumer_close;
		consumer->start = start;
		consumer->stop = stop;
		consumer->is_stopped = is_stopped;

		mlt_properties_set( MLT_CONSUMER_PROPERTIES(consumer), "resource", arg );
	}

	return consumer;
}

static mlt_consumer create_consumer( mlt_profile profile, char *id )
{
	char *myid = id ? strdup( id ) : NULL;
	char *arg = myid ? strchr( myid, ':' ) : NULL;
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_consumer consumer = mlt_factory_consumer( profile, myid, arg );
	if ( myid )
		free( myid );
	return consumer;
}

static void create_filter( mlt_profile profile, mlt_service service, char *effect, int *created )
{
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';

	// The swscale and avcolor_space filters require resolution as arg to test compatibility
	if ( strncmp( effect, "swscale", 7 ) == 0 || strncmp( effect, "avcolo", 6 ) == 0 )
		arg = (char*) mlt_properties_get_int( MLT_SERVICE_PROPERTIES( service ), "_real_width" );

	mlt_filter filter = mlt_factory_filter( profile, id, arg );
	if ( filter != NULL )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_loader", 1 );
		mlt_service_attach( service, filter );
		mlt_filter_close( filter );
		*created = 1;
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
	create_filter( profile, service, "avcolor_space", &created );
	if ( !created )
		create_filter( profile, service, "imageconvert", &created );
	create_filter( profile, service, "audioconvert", &created );
}

static mlt_consumer generate_consumer( mlt_consumer consumer, mlt_properties props, int index )
{
	mlt_profile profile = NULL;
	if ( mlt_properties_get( props, "mlt_profile" ) )
		profile = mlt_profile_init( mlt_properties_get( props, "mlt_profile" ) );
	if ( !profile )
		profile = mlt_profile_clone( mlt_service_profile( MLT_CONSUMER_SERVICE(consumer) ) );
	mlt_consumer nested = create_consumer( profile, mlt_properties_get( props, "consumer" ) );

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
		mlt_properties_set( props, "mlt_profile", NULL );
		mlt_properties_inherit( nested_props, props );

		attach_normalisers( profile, MLT_CONSUMER_SERVICE(nested) );
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

	if ( properties && mlt_properties_get_data( properties, "0", NULL ) )
	{
		mlt_properties p = NULL;
		char key[20];
		int index = 0;

		do {
			snprintf( key, sizeof(key), "%d", index );
			if ( ( p = mlt_properties_get_data( properties, key, NULL ) ) )
				generate_consumer( consumer, p, index++ );
		} while ( p );
		mlt_properties_close( properties );
	}
	else
	{
		const char *s = NULL;
		char key[20];
		int index = 0;

		if ( properties )
			mlt_properties_close( properties );
		properties = mlt_properties_load( resource );

		do {
			snprintf( key, sizeof(key), "%d", index );
			if ( ( s = mlt_properties_get( properties, key ) ) )
			{
				mlt_properties p = mlt_properties_new();
				int i, count;

				if ( !p ) break;
				mlt_properties_set( p, "consumer", mlt_properties_get( properties, key ) );
				snprintf( key, sizeof(key), "%d.", index );

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
		if ( nested ) mlt_consumer_start( nested );
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

static void foreach_consumer_put( mlt_consumer consumer, mlt_frame frame )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested ) mlt_consumer_put_frame( nested, mlt_frame_clone( frame ) );
	} while ( nested );
}

static void foreach_consumer_stop( mlt_consumer consumer )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_consumer nested = NULL;
	char key[30];
	int index = 0;

	do {
		snprintf( key, sizeof(key), "%d.consumer", index++ );
		nested = mlt_properties_get_data( properties, key, NULL );
		if ( nested ) mlt_consumer_stop( nested );
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
	if ( !is_stopped( consumer ) )
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
			if ( !mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "rendered" ) )
			{
				int dropped = mlt_properties_get_int( properties, "_dropped" );
				mlt_frame_close( frame );
				mlt_log_info( MLT_CONSUMER_SERVICE(consumer), "dropped frame %d\n", ++dropped );
				mlt_properties_set_int( properties, "_dropped", dropped );
				continue;
			}
			if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "_speed" ) == 0 )
				foreach_consumer_refresh( consumer );
			foreach_consumer_put( consumer, frame );
			mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			mlt_frame_close( frame );
		}
		else
		{
			if ( frame ) mlt_frame_close( frame );
			foreach_consumer_put( consumer, NULL );
			terminated = 1;
		}
	}

	// Indicate that the consumer is stopped
	mlt_properties_set_int( properties, "running", 0 );
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
