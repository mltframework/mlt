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
		mlt_properties_set_timecode( properties, "position", 0.0 );
		mlt_properties_set_double( properties, "frame", 0 );
		mlt_properties_set_double( properties, "fps", 25.0 );
		mlt_properties_set_double( properties, "speed", 1.0 );
		mlt_properties_set_timecode( properties, "in", 0.0 );
		mlt_properties_set_timecode( properties, "out", 36000.0 );
		mlt_properties_set_timecode( properties, "length", 36000.0 );
		mlt_properties_set_int( properties, "known_length", 1 );
		mlt_properties_set_double( properties, "aspect_ratio", 4.0 / 3.0 );
		mlt_properties_set( properties, "log_id", "multitrack" );

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

/** Convert frame position to timecode.
*/

mlt_timecode mlt_producer_time( mlt_producer this, int64_t frame )
{
	if ( frame < 0 )
		return -1;
	else
		return ( mlt_timecode )frame / mlt_producer_get_fps( this );
}

/** Convert timecode to frame position.
*/

int64_t mlt_producer_frame_position( mlt_producer this, mlt_timecode position )
{
	if ( position < 0 )
		return -1;
	else
		return ( int64_t )( floor( position * mlt_producer_get_fps( this ) + 0.5 ) );
}

/** Seek to a specified time code.
*/

int mlt_producer_seek( mlt_producer this, mlt_timecode timecode )
{
	// Check bounds
	if ( timecode < 0 )
		timecode = 0;
	if ( timecode > mlt_producer_get_playtime( this ) )
		timecode = mlt_producer_get_playtime( this );

	// Set the position
	mlt_properties_set_timecode( mlt_producer_properties( this ), "position", timecode );

	// Calculate the absolute frame
	double frame = ( mlt_producer_get_in( this ) + timecode ) * mlt_producer_get_fps( this );
	mlt_properties_set_double( mlt_producer_properties( this ), "frame", floor( frame + 0.5 ) );

	return 0;
}

/** Seek to a specified absolute frame.
*/

int mlt_producer_seek_frame( mlt_producer this, int64_t frame )
{
	// Calculate the time code
	double timecode = ( frame / mlt_producer_get_fps( this ) ) - mlt_producer_get_in( this );

	// If timecode is invalid, then seek on time
	if ( frame < 0 || timecode < 0 )
	{
		// Seek to the in point
		mlt_producer_seek( this, 0 );
	}
	else if ( timecode > mlt_producer_get_playtime( this ) )
	{
		// Seek to the out point
		mlt_producer_seek( this, mlt_producer_get_playtime( this ) );
	}
	else
	{
		// Set the position
		mlt_properties_set_timecode( mlt_producer_properties( this ), "position", timecode );

		// Set the absolute frame
		mlt_properties_set_double( mlt_producer_properties( this ), "frame", frame );
	}

	return 0;
}

/** Get the current time code.
*/

mlt_timecode mlt_producer_position( mlt_producer this )
{
	return mlt_properties_get_timecode( mlt_producer_properties( this ), "position" );
}

/** Get the current frame.
*/

uint64_t mlt_producer_frame( mlt_producer this )
{
	return mlt_properties_get_double( mlt_producer_properties( this ), "frame" );
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

int mlt_producer_set_in_and_out( mlt_producer this, mlt_timecode in, mlt_timecode out )
{
	// Correct ins and outs if necessary
	if ( in < 0 )
		in = 0;
	if ( in > mlt_producer_get_length( this ) )
		in = mlt_producer_get_length( this );
	if ( out < 0 )
		out = 0;
	if ( out > mlt_producer_get_length( this ) )
		out = mlt_producer_get_length( this );

	// Swap ins and outs if wrong
	if ( out < in )
	{
		mlt_timecode t = in;
		in = out;
		out = t;
	}

	// Set the values
	mlt_properties_set_timecode( mlt_producer_properties( this ), "in", in );
	mlt_properties_set_timecode( mlt_producer_properties( this ), "out", out );

	// Seek to the in point
	mlt_producer_seek( this, 0 );

	return 0;
}

/** Get the in point.
*/

mlt_timecode mlt_producer_get_in( mlt_producer this )
{
	return mlt_properties_get_timecode( mlt_producer_properties( this ), "in" );
}

/** Get the out point.
*/

mlt_timecode mlt_producer_get_out( mlt_producer this )
{
	return mlt_properties_get_timecode( mlt_producer_properties( this ), "out" );
}

/** Get the total play time.
*/

mlt_timecode mlt_producer_get_playtime( mlt_producer this )
{
	return mlt_producer_get_out( this ) - mlt_producer_get_in( this );
}

/** Get the total length of the producer.
*/

mlt_timecode mlt_producer_get_length( mlt_producer this )
{
	return mlt_properties_get_timecode( mlt_producer_properties( this ), "length" );
}

/** Prepare for next frame.
*/

void mlt_producer_prepare_next( mlt_producer this )
{
	mlt_producer_seek_frame( this, mlt_producer_frame( this ) + mlt_producer_get_speed( this ) );
}

/** Get a frame.
*/

static int producer_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	int result = 1;
	mlt_producer this = service->child;

	// A properly instatiated producer will have a get_frame method...
	if ( this->get_frame != NULL )
	{
		// Get the frame from the implementation
		result = this->get_frame( this, frame, index );

		mlt_properties frame_properties = mlt_frame_properties( *frame );
		double speed = mlt_producer_get_speed( this );
		mlt_properties_set_double( frame_properties, "speed", speed );
	}
	else
	{
		// Generate a test frame
		*frame = mlt_frame_init( );

		// Set the timecode
		result = mlt_frame_set_timecode( *frame, mlt_producer_position( this ) );

		// Calculate the next timecode
		mlt_producer_prepare_next( this );
	}

	// Copy the fps of the producer onto the frame
	mlt_properties properties = mlt_frame_properties( *frame );
	mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( this ) );

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
