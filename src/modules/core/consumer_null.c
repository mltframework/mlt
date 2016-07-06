/*
 * consumer_null.c -- a null consumer
 * Copyright (C) 2003-2014 Meltytech, LLC
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

// mlt Header files
#include <framework/mlt_consumer.h>
#include <framework/mlt_frame.h>

// System header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Forward references.
static int consumer_start( mlt_consumer consumer );
static int consumer_stop( mlt_consumer consumer );
static int consumer_is_stopped( mlt_consumer consumer );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer consumer );

/** Initialise the dv consumer.
*/

mlt_consumer consumer_null_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the consumer
	mlt_consumer consumer = mlt_consumer_new( profile );

	// If memory allocated and initialises without error
	if ( consumer != NULL )
	{
		// Assign close callback
		consumer->close = consumer_close;

		// Set up start/stop/terminated callbacks
		consumer->start = consumer_start;
		consumer->stop = consumer_stop;
		consumer->is_stopped = consumer_is_stopped;
	}

	// Return consumer
	return consumer;
}

/** Start the consumer.
*/

static int consumer_start( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Check that we're not already running
	if ( !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );
		mlt_properties_set_int( properties, "joined", 0 );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, consumer );
	}
	return 0;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Check that we're running
	if ( !mlt_properties_get_int( properties, "joined" ) )
	{
		// Get the thread
		pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );
		mlt_properties_set_int( properties, "joined", 1 );

		// Wait for termination
		if ( thread )
			pthread_join( *thread, NULL );
	}

	return 0;
}

/** Determine if the consumer is stopped.
*/

static int consumer_is_stopped( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	return !mlt_properties_get_int( properties, "running" );
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread( void *arg )
{
	// Map the argument to the object
	mlt_consumer consumer = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Convenience functionality
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	// Frame and size
	mlt_frame frame = NULL;

	// Loop while running
	while( !terminated && mlt_properties_get_int( properties, "running" ) )
	{
		// Get the frame
		frame = mlt_consumer_rt_frame( consumer );

		// Check for termination
		if ( terminate_on_pause && frame != NULL )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Check that we have a frame to work with
		if ( frame != NULL )
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
	// Stop the consumer
	mlt_consumer_stop( consumer );

	// Close the parent
	mlt_consumer_close( consumer );

	// Free the memory
	free( consumer );
}
