/*
 * mlt_consumer.c -- abstraction for all consumer services
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

#include "config.h"
#include "mlt_consumer.h"
#include "mlt_factory.h"
#include "mlt_producer.h"
#include "mlt_frame.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

/** Public final methods
*/

int mlt_consumer_init( mlt_consumer this, void *child )
{
	int error = 0;
	memset( this, 0, sizeof( struct mlt_consumer_s ) );
	this->child = child;
	error = mlt_service_init( &this->parent, this );
	if ( error == 0 )
	{
		// Get the properties from the service
		mlt_properties properties = mlt_service_properties( &this->parent );

		// Get the normalisation preference
		char *normalisation = mlt_environment( "MLT_NORMALISATION" );

		// Deal with normalisation
		if ( normalisation == NULL || strcmp( normalisation, "NTSC" ) )
		{
			mlt_properties_set( properties, "normalisation", "PAL" );
			mlt_properties_set_double( properties, "fps", 25.0 );
			mlt_properties_set_int( properties, "width", 720 );
			mlt_properties_set_int( properties, "height", 576 );
			mlt_properties_set_int( properties, "progressive", 0 );
			mlt_properties_set_double( properties, "aspect_ratio", 128.0 / 117.0 );
		}
		else
		{
			mlt_properties_set( properties, "normalisation", "NTSC" );
			mlt_properties_set_double( properties, "fps", 30000.0 / 1001.0 );
			mlt_properties_set_int( properties, "width", 720 );
			mlt_properties_set_int( properties, "height", 480 );
			mlt_properties_set_int( properties, "progressive", 0 );
			mlt_properties_set_double( properties, "aspect_ratio", 72.0 / 79.0 );
		}

		// Default rescaler for all consumers
		mlt_properties_set( properties, "rescale", "bilinear" );

		// Default read ahead buffer size
		mlt_properties_set_int( properties, "buffer", 25 );

		// Default audio frequency and channels
		mlt_properties_set_int( properties, "frequency", 48000 );
		mlt_properties_set_int( properties, "channels", 2 );

		// Default of all consumers is real time
		mlt_properties_set_int( properties, "real_time", 1 );

		// Default to environment test card
		mlt_properties_set( properties, "test_card", mlt_environment( "MLT_TEST_CARD" ) );

		// Hmm - default all consumers to yuv422 :-/
		this->format = mlt_image_yuv422;
	}
	return error;
}

/** Create a new consumer.
*/

mlt_consumer mlt_consumer_new( )
{
	// Create the memory for the structure
	mlt_consumer this = malloc( sizeof( struct mlt_consumer_s ) );

	// Initialise it
	if ( this != NULL )
		mlt_consumer_init( this, NULL );

	// Return it
	return this;
}

/** Get the parent service object.
*/

mlt_service mlt_consumer_service( mlt_consumer this )
{
	return &this->parent;
}

/** Get the consumer properties.
*/

mlt_properties mlt_consumer_properties( mlt_consumer this )
{
	return mlt_service_properties( &this->parent );
}

/** Connect the consumer to the producer.
*/

int mlt_consumer_connect( mlt_consumer this, mlt_service producer )
{
	return mlt_service_connect_producer( &this->parent, producer, 0 );
}

/** Start the consumer.
*/

int mlt_consumer_start( mlt_consumer this )
{
	// Get the properies
	mlt_properties properties = mlt_consumer_properties( this );

	// Determine if there's a test card producer
	char *test_card = mlt_properties_get( properties, "test_card" );

	// Deal with it now.
	if ( test_card != NULL )
	{
		if ( mlt_properties_get_data( properties, "test_card_producer", NULL ) == NULL )
		{
			// Create a test card producer
			mlt_producer producer = mlt_factory_producer( NULL, test_card );

			// Do we have a producer
			if ( producer != NULL )
			{
				// Test card should loop I guess...
				mlt_properties_set( mlt_producer_properties( producer ), "eof", "loop" );

				// Set the test card on the consumer
				mlt_properties_set_data( properties, "test_card_producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
			}

			// Check and run an ante command
			if ( mlt_properties_get( properties, "ante" ) )
				system( mlt_properties_get( properties, "ante" ) );
		}
	}
	else
	{
		// Allow the hash table to speed things up
		mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );
	}

	// Set the real_time preference
	this->real_time = mlt_properties_get_int( properties, "real_time" );

	// Start the service
	if ( this->start != NULL )
		return this->start( this );

	return 0;
}

/** Protected method for consumer to get frames from connected service
*/

mlt_frame mlt_consumer_get_frame( mlt_consumer this )
{
	// Frame to return
	mlt_frame frame = NULL;

	// Get the service assoicated to the consumer
	mlt_service service = mlt_consumer_service( this );

	// Get the frame
	if ( mlt_service_get_frame( service, &frame, 0 ) == 0 )
	{
		// Get the consumer properties
		mlt_properties properties = mlt_consumer_properties( this );

		// Get the frame properties
		mlt_properties frame_properties = mlt_frame_properties( frame );

		// Get the test card producer
		mlt_producer test_card = mlt_properties_get_data( properties, "test_card_producer", NULL );

		// Attach the test frame producer to it.
		if ( test_card != NULL )
			mlt_properties_set_data( frame_properties, "test_card_producer", test_card, 0, NULL, NULL );

		// Attach the rescale property
		mlt_properties_set( frame_properties, "rescale.interp", mlt_properties_get( properties, "rescale" ) );

		// Aspect ratio and other jiggery pokery
		mlt_properties_set_double( frame_properties, "consumer_aspect_ratio", mlt_properties_get_double( properties, "aspect_ratio" ) );
		mlt_properties_set_int( frame_properties, "consumer_progressive", mlt_properties_get_int( properties, "progressive" ) );
		mlt_properties_set_int( frame_properties, "consumer_deinterlace", mlt_properties_get_int( properties, "deinterlace" ) );
	}

	// Return the frame
	return frame;
}

static inline long time_difference( struct timeval *time1 )
{
	struct timeval time2;
	time2.tv_sec = time1->tv_sec;
	time2.tv_usec = time1->tv_usec;
	gettimeofday( time1, NULL );
	return time1->tv_sec * 1000000 + time1->tv_usec - time2.tv_sec * 1000000 - time2.tv_usec;
}

static void *consumer_read_ahead_thread( void *arg )
{
	// The argument is the consumer
	mlt_consumer this = arg;

	// Get the properties of the consumer
	mlt_properties properties = mlt_consumer_properties( this );

	// Get the width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );

	// Get the maximum size of the buffer
	int buffer = mlt_properties_get_int( properties, "buffer" );

	// General frame variable
	mlt_frame frame = NULL;
	uint8_t *image = NULL;

	// Time structures
	struct timeval ante;

	// Average time for get_frame and get_image
	int count = 1;
	int skipped = 0;
	int64_t time_wait = 0;
	int64_t time_frame = 0;
	int64_t time_image = 0;

	// Get the first frame
	frame = mlt_consumer_get_frame( this );

	// Get the image of the first frame
	mlt_frame_get_image( frame, &image, &this->format, &width, &height, 0 );
	mlt_properties_set_int( mlt_frame_properties( frame ), "rendered", 1 );

	// Get the starting time (can ignore the times above)
	gettimeofday( &ante, NULL );

	// Continue to read ahead
	while ( this->ahead )
	{
		// Put the current frame into the queue
		pthread_mutex_lock( &this->mutex );
		while( this->ahead && mlt_deque_count( this->queue ) >= buffer )
			pthread_cond_wait( &this->cond, &this->mutex );
		mlt_deque_push_back( this->queue, frame );
		pthread_cond_broadcast( &this->cond );
		pthread_mutex_unlock( &this->mutex );
		time_wait += time_difference( &ante );

		// Get the next frame
		frame = mlt_consumer_get_frame( this );
		time_frame += time_difference( &ante );

		// Increment the count
		count ++;

		// Get the image
		if ( ( time_frame + time_image ) / count < ( 40000 - ( time_wait / count ) ) )
		{
			// Get the image, mark as rendered and time it
			mlt_frame_get_image( frame, &image, &this->format, &width, &height, 0 );
			mlt_properties_set_int( mlt_frame_properties( frame ), "rendered", 1 );
			time_image += time_difference( &ante );

			// Reset the skipped count
			skipped = 0;
		}
		else
		{
			// Increment the number of sequentially skipped frames
			skipped ++;

			// If we've reached an unacceptable level, reset everything
			if ( skipped > 10 )
			{
				skipped = 0;
				time_frame = 0;
				time_image = 0;
				time_wait = 0;
				count = 0;
			}
		}
	}

	// Remove the last frame
	mlt_frame_close( frame );

	return NULL;
}

static void consumer_read_ahead_start( mlt_consumer this )
{
	pthread_attr_t thread_attributes;
	
	// We're running now
	this->ahead = 1;

	// Create the frame queue
	this->queue = mlt_deque_init( );

	// Create the mutex
	pthread_mutex_init( &this->mutex, NULL );

	// Create the condition
	pthread_cond_init( &this->cond, NULL );

	// Inherit the scheduling priority
	pthread_attr_init( &thread_attributes );
	pthread_attr_setinheritsched( &thread_attributes, PTHREAD_INHERIT_SCHED );
	
	// Create the read ahead 
	pthread_create( &this->ahead_thread, &thread_attributes, consumer_read_ahead_thread, this );
}

static void consumer_read_ahead_stop( mlt_consumer this )
{
	// Make sure we're running
	if ( this->ahead )
	{
		// Inform thread to stop
		this->ahead = 0;

		// Broadcast to the condition in case it's waiting
		pthread_mutex_lock( &this->mutex );
		pthread_cond_broadcast( &this->cond );
		pthread_mutex_unlock( &this->mutex );

		// Join the thread
		pthread_join( this->ahead_thread, NULL );

		// Destroy the mutex
		pthread_mutex_destroy( &this->mutex );

		// Destroy the condition
		pthread_cond_destroy( &this->cond );

		// Wipe the queue
		while ( mlt_deque_count( this->queue ) )
			mlt_frame_close( mlt_deque_pop_back( this->queue ) );
	}
}

mlt_frame mlt_consumer_rt_frame( mlt_consumer this )
{
	// Frame to return
	mlt_frame frame = NULL;

	// Get the properties
	mlt_properties properties = mlt_consumer_properties( this );

	// Check if the user has requested real time or not
	if ( this->real_time )
	{
		int size = 1;

		// Is the read ahead running?
		if ( this->ahead == 0 )
		{
			int buffer = mlt_properties_get_int( properties, "buffer" );
			consumer_read_ahead_start( this );
			if ( buffer > 1 )
				size = buffer / 2;
		}
	
		// Get frame from queue
		pthread_mutex_lock( &this->mutex );
		while( this->ahead && mlt_deque_count( this->queue ) < size )
			pthread_cond_wait( &this->cond, &this->mutex );
		frame = mlt_deque_pop_front( this->queue );
		pthread_cond_broadcast( &this->cond );
		pthread_mutex_unlock( &this->mutex );
	}
	else
	{
		// Get the frame in non real time
		frame = mlt_consumer_get_frame( this );

		// This isn't true, but from the consumers perspective it is
		mlt_properties_set_int( mlt_frame_properties( frame ), "rendered", 1 );
	}

	return frame;
}

/** Stop the consumer.
*/

int mlt_consumer_stop( mlt_consumer this )
{
	// Get the properies
	mlt_properties properties = mlt_consumer_properties( this );

	// Stop the consumer
	if ( this->stop != NULL )
		this->stop( this );

	// Check if the user has requested real time or not and stop if necessary
	if ( mlt_properties_get_int( properties, "real_time" ) )
		consumer_read_ahead_stop( this );

	// Kill the test card
	mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );

	// Check and run a post command
	if ( mlt_properties_get( properties, "post" ) )
		system( mlt_properties_get( properties, "post" ) );

	return 0;
}

/** Determine if the consumer is stopped.
*/

int mlt_consumer_is_stopped( mlt_consumer this )
{
	// Check if the consumer is stopped
	if ( this->is_stopped != NULL )
		return this->is_stopped( this );

	return 0;
}

/** Close the consumer.
*/

void mlt_consumer_close( mlt_consumer this )
{
	// Get the childs close function
	void ( *consumer_close )( ) = this->close;

	// Make sure it only gets called once
	this->close = NULL;

	// Call the childs close if available
	if ( consumer_close != NULL )
		consumer_close( this );
	else
		mlt_service_close( &this->parent );
}
