/*
 * mlt_producer.c -- abstraction for all producer services
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
#include "mlt_producer.h"
#include "mlt_frame.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/** Forward references.
*/

static int producer_get_frame( mlt_service this, mlt_frame_ptr frame, int index );

/** Constructor
*/

int mlt_producer_init( mlt_producer this, void *child )
{
	// Initialise the producer
	memset( this, 0, sizeof( struct mlt_producer_s ) );
	
	// Associate with the child
	this->child = child;

	// Initialise the service
	if ( mlt_service_init( &this->parent, this ) == 0 )
	{
		// The parent is the service
		mlt_service parent = &this->parent;

		// Get the properties of the parent
		mlt_properties properties = mlt_service_properties( parent );

		// Set the default properties
		mlt_properties_set( properties, "mlt_type", "mlt_producer" );
		mlt_properties_set_position( properties, "position", 0.0 );
		mlt_properties_set_double( properties, "frame", 0 );
		mlt_properties_set_double( properties, "fps", 25.0 );
		mlt_properties_set_double( properties, "speed", 1.0 );
		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 1799999 );
		mlt_properties_set_position( properties, "length", 1800000 );
		mlt_properties_set_double( properties, "aspect_ratio", 4.0 / 3.0 );
		mlt_properties_set( properties, "eof", "pause" );
		mlt_properties_set( properties, "resource", "<producer>" );

		// Override service get_frame
		parent->get_frame = producer_get_frame;
	}

	return 0;
}

/** Get the parent service object.
*/

mlt_service mlt_producer_service( mlt_producer this )
{
	return &this->parent;
}

/** Get the producer properties.
*/

mlt_properties mlt_producer_properties( mlt_producer this )
{
	return mlt_service_properties( &this->parent );
}

/** Seek to a specified position.
*/

int mlt_producer_seek( mlt_producer this, mlt_position position )
{
	// Determine eof handling
	char *eof = mlt_properties_get( mlt_producer_properties( this ), "eof" );

	// Check bounds
	if ( position < 0 )
		position = 0;
	else if ( !strcmp( eof, "pause" ) && position >= mlt_producer_get_playtime( this ) )
		position = mlt_producer_get_playtime( this ) - 1;

	// Set the position
	mlt_properties_set_position( mlt_producer_properties( this ), "position", position );

	// Calculate the absolute frame
	mlt_properties_set_position( mlt_producer_properties( this ), "frame", mlt_producer_get_in( this ) + position );

	return 0;
}

/** Get the current position (relative to in point).
*/

mlt_position mlt_producer_position( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "position" );
}

/** Get the current position (relative to start of producer).
*/

mlt_position mlt_producer_frame( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "frame" );
}

/** Set the playing speed.
*/

int mlt_producer_set_speed( mlt_producer this, double speed )
{
	return mlt_properties_set_double( mlt_producer_properties( this ), "speed", speed );
}

/** Get the playing speed.
*/

double mlt_producer_get_speed( mlt_producer this )
{
	return mlt_properties_get_double( mlt_producer_properties( this ), "speed" );
}

/** Get the frames per second.
*/

double mlt_producer_get_fps( mlt_producer this )
{
	return mlt_properties_get_double( mlt_producer_properties( this ), "fps" );
}

/** Set the in and out points.
*/

int mlt_producer_set_in_and_out( mlt_producer this, mlt_position in, mlt_position out )
{
	// Correct ins and outs if necessary
	if ( in < 0 )
		in = 0;
	else if ( in > mlt_producer_get_length( this ) )
		in = mlt_producer_get_length( this );

	if ( out < 0 )
		out = 0;
	else if ( out > mlt_producer_get_length( this ) )
		out = mlt_producer_get_length( this );

	// Swap ins and outs if wrong
	if ( out < in )
	{
		mlt_position t = in;
		in = out;
		out = t;
	}

	// Set the values
	mlt_properties_set_position( mlt_producer_properties( this ), "in", in );
	mlt_properties_set_position( mlt_producer_properties( this ), "out", out );

	return 0;
}

/** Get the in point.
*/

mlt_position mlt_producer_get_in( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "in" );
}

/** Get the out point.
*/

mlt_position mlt_producer_get_out( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "out" );
}

/** Get the total play time.
*/

mlt_position mlt_producer_get_playtime( mlt_producer this )
{
	return mlt_producer_get_out( this ) - mlt_producer_get_in( this ) + 1;
}

/** Get the total length of the producer.
*/

mlt_position mlt_producer_get_length( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "length" );
}

/** Prepare for next frame.
*/

void mlt_producer_prepare_next( mlt_producer this )
{
	mlt_producer_seek( this, mlt_producer_position( this ) + mlt_producer_get_speed( this ) );
}

/** Get a frame.
*/

static int producer_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	int result = 1;
	mlt_producer this = service->child;

	// Determine eof handling
	char *eof = mlt_properties_get( mlt_producer_properties( this ), "eof" );

	// A properly instatiated producer will have a get_frame method...
	if ( this->get_frame == NULL || ( !strcmp( eof, "continue" ) && mlt_producer_position( this ) > mlt_producer_get_out( this ) ) )
	{
		// Generate a test frame
		*frame = mlt_frame_init( );

		// Set the position
		result = mlt_frame_set_position( *frame, mlt_producer_position( this ) );

		// Calculate the next position
		mlt_producer_prepare_next( this );
	}
	else
	{
		// Get the frame from the implementation
		result = this->get_frame( this, frame, index );
	}

	// Copy the fps and speed of the producer onto the frame
	mlt_properties properties = mlt_frame_properties( *frame );
	mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( this ) );
	double speed = mlt_producer_get_speed( this );
	mlt_properties_set_double( properties, "speed", speed );

	return 0;
}

/** Close the producer.
*/

void mlt_producer_close( mlt_producer this )
{
	if ( this->close != NULL )
		this->close( this );
	else
		mlt_service_close( &this->parent );
}
