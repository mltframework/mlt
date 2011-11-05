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
	}

	return consumer;
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
			pthread_join( *thread, NULL );
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
	while ( !terminated && mlt_properties_get_int( properties, "running" ) )
	{
		// Get the next frame
		frame = mlt_consumer_rt_frame( consumer );

		// Check for termination
		if ( terminate_on_pause && frame )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Check that we have a frame to work with
		if ( frame )
		{
			// Close the frame
			mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			mlt_frame_close( frame );
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
