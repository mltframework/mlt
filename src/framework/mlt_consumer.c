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
		char *normalisation = getenv( "MLT_NORMALISATION" );

		// Deal with normalisation
		if ( normalisation == NULL || strcmp( normalisation, "NTSC" ) )
		{
			mlt_properties_set( properties, "normalisation", "PAL" );
			mlt_properties_set_double( properties, "fps", 25.0 );
			mlt_properties_set_int( properties, "width", 720 );
			mlt_properties_set_int( properties, "height", 576 );
			mlt_properties_set_int( properties, "progressive", 0 );
		}
		else
		{
			mlt_properties_set( properties, "normalisation", "NTSC" );
			mlt_properties_set_double( properties, "fps", 30000.0 / 1001.0 );
			mlt_properties_set_int( properties, "width", 720 );
			mlt_properties_set_int( properties, "height", 480 );
			mlt_properties_set_int( properties, "progressive", 0 );
		}
		mlt_properties_set_double( properties, "aspect_ratio", 4.0 / 3.0 );

		// Default rescaler for all consumers
		mlt_properties_set( properties, "rescale", "bilinear" );
	}
	return error;
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
		// Create a test card producer
		// TODO: do we want to use fezzik here?
		mlt_producer producer = mlt_factory_producer( "fezzik", test_card );

		// Do we have a producer
		if ( producer != NULL )
		{
			// Set the test card on the consumer
			mlt_properties_set_data( properties, "test_card_producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
		}
	}

	// Start the service
	if ( this->start != NULL )
		return this->start( this );

	return 0;
}

/** Protected method :-/ for consumer to get frames from connected service
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

		// Attach the test frame producer to it.
		mlt_producer test_card = mlt_properties_get_data( properties, "test_card_producer", NULL );
		mlt_properties_set_data( frame_properties, "test_card_producer", test_card, 0, NULL, NULL );

		// Attach the rescale property
		if ( mlt_properties_get( properties, "rescale" ) != NULL )
			mlt_properties_set( frame_properties, "rescale.interp", mlt_properties_get( properties, "rescale" ) );

		// TODO: Aspect ratio and other jiggery pokery
		mlt_properties_set_double( frame_properties, "consumer_aspect_ratio", mlt_properties_get_double( properties, "aspect_ratio" ) );
		
	}

	// Return the frame
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
		return this->stop( this );

	// Kill the test card
	mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );

	return 0;
}

/** Determine if the consumer is stopped.
*/

int mlt_consumer_is_stopped( mlt_consumer this )
{
	// Stop the consumer
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

