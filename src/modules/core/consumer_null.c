/*
 * consumer_null.c -- a null consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

// Local header files
#include "consumer_null.h"

// mlt Header files
#include <framework/mlt_frame.h>

// System header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Forward references.
static int consumer_start( mlt_consumer this );
static int consumer_stop( mlt_consumer this );
static int consumer_is_stopped( mlt_consumer this );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer this );

/** Initialise the dv consumer.
*/

mlt_consumer consumer_null_init( char *arg )
{
	// Allocate the consumer
	mlt_consumer this = mlt_consumer_new( );

	// If memory allocated and initialises without error
	if ( this != NULL )
	{
		// Assign close callback
		this->close = consumer_close;

		// Set up start/stop/terminated callbacks
		this->start = consumer_start;
		this->stop = consumer_stop;
		this->is_stopped = consumer_is_stopped;
	}

	// Return this
	return this;
}

/** Start the consumer.
*/

static int consumer_start( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check that we're not already running
	if ( !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, this );
	}
	return 0;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check that we're running
	if ( mlt_properties_get_int( properties, "running" ) )
	{
		// Get the thread
		pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );

		// Wait for termination
		pthread_join( *thread, NULL );
	}

	return 0;
}

/** Determine if the consumer is stopped.
*/

static int consumer_is_stopped( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
	return !mlt_properties_get_int( properties, "running" );
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread( void *arg )
{
	// Map the argument to the object
	mlt_consumer this = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Frame and size
	mlt_frame frame = NULL;

	// Loop while running
	while( mlt_properties_get_int( properties, "running" ) )
	{
		// Get the frame
		frame = mlt_consumer_rt_frame( this );

		// Check that we have a frame to work with
		if ( frame != NULL )
		{
			// Close the frame
			mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			mlt_frame_close( frame );
		}
	}

	// Indicate that the consumer is stopped
	mlt_consumer_stopped( this );

	return NULL;
}

/** Close the consumer.
*/

static void consumer_close( mlt_consumer this )
{
	// Stop the consumer
	mlt_consumer_stop( this );

	// Close the parent
	mlt_consumer_close( this );

	// Free the memory
	free( this );
}
