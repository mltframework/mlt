/*
 * producer_hold.c -- frame holding producer
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

#include "producer_hold.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

// Forward references
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

/** Constructor for the frame holding producer. Basically, all this producer does is
	provide a producer wrapper for the requested producer, allows the specifcation of
	the frame required and will then repeatedly obtain that frame for each get_frame
	and get_image requested.
*/

mlt_producer producer_hold_init( char *arg )
{
	// Construct a new holding producer
	mlt_producer this = mlt_producer_new( );

	// Construct the requested producer via fezzik
	mlt_producer producer = mlt_factory_producer( "fezzik", arg );

	// Initialise the frame holding capabilities
	if ( this != NULL && producer != NULL )
	{
		// Get the properties of this producer
		mlt_properties properties = mlt_producer_properties( this );

		// Store the producer
		mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Set frame, in, out and length for this producer
		mlt_properties_set_position( properties, "frame", 0 );
		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 25 );
		mlt_properties_set_position( properties, "length", 15000 );
		mlt_properties_set( properties, "resource", arg );

		// Override the get_frame method
		this->get_frame = producer_get_frame;
	}
	else
	{
		// Clean up (not sure which one failed, can't be bothered to find out, so close both)
		if ( this )
			mlt_producer_close( this );
		if ( producer )
			mlt_producer_close( producer );

		// Make sure we return NULL
		this = NULL;
	}

	// Return this producer
	return this;
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties of the frame
	mlt_properties properties = mlt_frame_properties( frame );

	// Obtain the real frame
	mlt_frame real_frame = mlt_frame_pop_service( frame );

	// We want distorted to ensure we don't hit the resize filter twice
	mlt_properties_set_int( properties, "distort", 1 );

	// Get the image from the real frame
	mlt_frame_get_image( real_frame, buffer, format, width, height, writable );

	// Set the values obtained on the frame
	mlt_properties_set_data( properties, "image", *buffer, *width * *height * 2, NULL, NULL );
	mlt_properties_set_int( properties, "width", *width );
	mlt_properties_set_int( properties, "height", *height );

	// We'll deinterlace on the downstream deinterlacer
	mlt_properties_set_int( mlt_frame_properties( real_frame ), "consumer_deinterlace", 1 );

	// All done
	return 0;
}

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Get the properties of this producer
	mlt_properties properties = mlt_producer_properties( this );

	// Construct a new frame
	*frame = mlt_frame_init( );

	// If we have a frame, then stack the producer itself and the get_image method
	if ( *frame != NULL )
	{
		// Define the real frame
		mlt_frame real_frame = NULL;

		// Get the producer
		mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );

		// Get the frame position requested
		mlt_position position = mlt_properties_get_position( properties, "frame" );

		// Seek the producer to the correct place
		mlt_producer_seek( producer, position );

		// Get the real frame
		mlt_service_get_frame( mlt_producer_service( producer ), &real_frame, index );

		// Ensure that the real frame gets wiped
		mlt_properties_set_data( mlt_frame_properties( *frame ), "real_frame", real_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );

		// Stack the real frame and method
		mlt_frame_push_service( *frame, real_frame );
		mlt_frame_push_service( *frame, producer_get_image );

		// Ensure that the consumer sees what the real frame has
		mlt_properties_pass( mlt_frame_properties( *frame ), mlt_frame_properties( real_frame ), "" );

		// Mirror the properties of the frame
		mlt_properties_mirror( mlt_frame_properties( *frame ), mlt_frame_properties( real_frame ) );
	}

	// Move to the next position
	mlt_producer_prepare_next( this );

	return 0;
}
