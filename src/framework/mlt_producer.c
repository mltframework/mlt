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
#include "mlt_factory.h"
#include "mlt_frame.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/** Forward references.
*/

static int producer_get_frame( mlt_service this, mlt_frame_ptr frame, int index );
static void mlt_producer_property_changed( mlt_service owner, mlt_producer this, char *name );
static void mlt_producer_service_changed( mlt_service owner, mlt_producer this );

/** Constructor
*/

int mlt_producer_init( mlt_producer this, void *child )
{
	// Check that we haven't received NULL
	int error = this == NULL;

	// Continue if no error
	if ( error == 0 )
	{
		// Initialise the producer
		memset( this, 0, sizeof( struct mlt_producer_s ) );
	
		// Associate with the child
		this->child = child;

		// Initialise the service
		if ( mlt_service_init( &this->parent, this ) == 0 )
		{
			// Get the normalisation preference
			char *normalisation = mlt_environment( "MLT_NORMALISATION" );

			// The parent is the service
			mlt_service parent = &this->parent;
	
			// Define the parent close
			parent->close = ( mlt_destructor )mlt_producer_close;
			parent->close_object = this;

			// For convenience, we'll assume the close_object is this
			this->close_object = this;

			// Get the properties of the parent
			mlt_properties properties = mlt_service_properties( parent );
	
			// Set the default properties
			mlt_properties_set( properties, "mlt_type", "mlt_producer" );
			mlt_properties_set_position( properties, "_position", 0.0 );
			mlt_properties_set_double( properties, "_frame", 0 );
			if ( normalisation == NULL || strcmp( normalisation, "NTSC" ) )
			{
				mlt_properties_set_double( properties, "fps", 25.0 );
				mlt_properties_set_double( properties, "aspect_ratio", 72.0 / 79.0 );
			}
			else
			{
				mlt_properties_set_double( properties, "fps", 30000.0 / 1001.0 );
				mlt_properties_set_double( properties, "aspect_ratio", 128.0 / 117.0 );
			}
			mlt_properties_set_double( properties, "_speed", 1.0 );
			mlt_properties_set_position( properties, "in", 0 );
			mlt_properties_set_position( properties, "out", 14999 );
			mlt_properties_set_position( properties, "length", 15000 );
			mlt_properties_set( properties, "eof", "pause" );
			mlt_properties_set( properties, "resource", "<producer>" );

			// Override service get_frame
			parent->get_frame = producer_get_frame;

			mlt_events_listen( properties, this, "service-changed", ( mlt_listener )mlt_producer_service_changed );
			mlt_events_listen( properties, this, "property-changed", ( mlt_listener )mlt_producer_property_changed );
			mlt_events_register( properties, "producer-changed", NULL );
		}
	}

	return error;
}

/** Listener for property changes.
*/

static void mlt_producer_property_changed( mlt_service owner, mlt_producer this, char *name )
{
	if ( !strcmp( name, "in" ) || !strcmp( name, "out" ) || !strcmp( name, "length" ) )
		mlt_events_fire( mlt_producer_properties( this ), "producer-changed", NULL );
}

/** Listener for service changes.
*/

static void mlt_producer_service_changed( mlt_service owner, mlt_producer this )
{
	mlt_events_fire( mlt_producer_properties( this ), "producer-changed", NULL );
}

/** Create a new producer.
*/

mlt_producer mlt_producer_new( )
{
	mlt_producer this = malloc( sizeof( struct mlt_producer_s ) );
	mlt_producer_init( this, NULL );
	return this;
}

/** Determine if producer is a cut.
*/

int mlt_producer_is_cut( mlt_producer this )
{
	return mlt_properties_get_int( mlt_producer_properties( this ), "_cut" );
}

/** Obtain the parent producer.
*/

mlt_producer mlt_producer_cut_parent( mlt_producer this )
{
	mlt_properties properties = mlt_producer_properties( this );
	if ( mlt_producer_is_cut( this ) )
		return mlt_properties_get_data( properties, "_cut_parent", NULL );
	else
		return this;
}

/** Create a cut of this producer
*/

mlt_producer mlt_producer_cut( mlt_producer this, int in, int out )
{
	mlt_producer result = mlt_producer_new( );
	mlt_producer parent = mlt_producer_cut_parent( this );
	mlt_properties properties = mlt_producer_properties( result );
	mlt_properties parent_props = mlt_producer_properties( parent );

	// Special case - allow for a cut of the entire producer (this will squeeze all other cuts to 0)
	if ( in <= 0 )
		in = 0;
	if ( out >= mlt_producer_get_playtime( parent ) )
		out = mlt_producer_get_playtime( parent ) - 1;

	mlt_properties_inc_ref( parent_props );
	mlt_properties_set_int( properties, "_cut", 1 );
	mlt_properties_set_data( properties, "_cut_parent", parent, 0, ( mlt_destructor )mlt_producer_close, NULL );
	mlt_properties_set_position( properties, "length", mlt_properties_get_position( parent_props, "length" ) );
	mlt_properties_set_position( properties, "in", 0 );
	mlt_properties_set_position( properties, "out", 0 );
	mlt_producer_set_in_and_out( result, in, out );

	return result;
}

/** Get the parent service object.
*/

mlt_service mlt_producer_service( mlt_producer this )
{
	return this != NULL ? &this->parent : NULL;
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
	mlt_properties properties = mlt_producer_properties( this );
	char *eof = mlt_properties_get( properties, "eof" );
	int use_points = 1 - mlt_properties_get_int( properties, "ignore_points" );

	// Recursive behaviour for cuts...
	if ( mlt_producer_is_cut( this ) )
		mlt_producer_seek( mlt_producer_cut_parent( this ), position + mlt_producer_get_in( this ) );

	// Check bounds
	if ( position < 0 )
	{
		position = 0;
	}
	else if ( use_points && !strcmp( eof, "pause" ) && position >= mlt_producer_get_playtime( this ) )
	{
		mlt_producer_set_speed( this, 0 );
		position = mlt_producer_get_playtime( this ) - 1;
	}
	else if ( use_points && !strcmp( eof, "loop" ) && position >= mlt_producer_get_playtime( this ) )
	{
		position = position % mlt_producer_get_playtime( this );
	}

	// Set the position
	mlt_properties_set_position( mlt_producer_properties( this ), "_position", position );

	// Calculate the absolute frame
	mlt_properties_set_position( mlt_producer_properties( this ), "_frame", use_points * mlt_producer_get_in( this ) + position );

	return 0;
}

/** Get the current position (relative to in point).
*/

mlt_position mlt_producer_position( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "_position" );
}

/** Get the current position (relative to start of producer).
*/

mlt_position mlt_producer_frame( mlt_producer this )
{
	return mlt_properties_get_position( mlt_producer_properties( this ), "_frame" );
}

/** Set the playing speed.
*/

int mlt_producer_set_speed( mlt_producer this, double speed )
{
	return mlt_properties_set_double( mlt_producer_properties( this ), "_speed", speed );
}

/** Get the playing speed.
*/

double mlt_producer_get_speed( mlt_producer this )
{
	return mlt_properties_get_double( mlt_producer_properties( this ), "_speed" );
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
	mlt_properties properties = mlt_producer_properties( this );

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
	mlt_events_block( properties, properties );
	mlt_properties_set_position( properties, "in", in );
	mlt_properties_set_position( properties, "out", out );
	mlt_events_unblock( properties, properties );
	mlt_events_fire( properties, "producer-changed", NULL );

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

	if ( !mlt_producer_is_cut( this ) )
	{
		// Determine eof handling
		char *eof = mlt_properties_get( mlt_producer_properties( this ), "eof" );

		// A properly instatiated producer will have a get_frame method...
		if ( this->get_frame == NULL || ( !strcmp( eof, "continue" ) && mlt_producer_position( this ) > mlt_producer_get_out( this ) ) )
		{
			// Generate a test frame
			*frame = mlt_frame_init( );

			// Set the position
			result = mlt_frame_set_position( *frame, mlt_producer_position( this ) );

			// Mark as a test card
			mlt_properties_set_int( mlt_frame_properties( *frame ), "test_image", 1 );
			mlt_properties_set_int( mlt_frame_properties( *frame ), "test_audio", 1 );

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
		double speed = mlt_producer_get_speed( this );
		mlt_properties_set_double( properties, "_speed", speed );
		mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( this ) );
		mlt_properties_set_int( properties, "test_audio", mlt_frame_is_test_audio( *frame ) );
		mlt_properties_set_int( properties, "test_image", mlt_frame_is_test_card( *frame ) );
	}
	else
	{
		mlt_properties properties = mlt_producer_properties( this );
		mlt_producer_seek( this, mlt_properties_get_int( properties, "_position" ) );
		result = producer_get_frame( mlt_producer_service( mlt_producer_cut_parent( this ) ), frame, index );
		double speed = mlt_producer_get_speed( this );
		mlt_properties_set_double( mlt_frame_properties( *frame ), "_speed", speed );
		mlt_producer_prepare_next( this );
	}

	return result;
}

/** Attach a filter.
*/

int mlt_producer_attach( mlt_producer this, mlt_filter filter )
{
	return mlt_service_attach( mlt_producer_service( this ), filter );
}

/** Detach a filter.
*/

int mlt_producer_detach( mlt_producer this, mlt_filter filter )
{
	return mlt_service_detach( mlt_producer_service( this ), filter );
}

/** Retrieve a filter.
*/

mlt_filter mlt_producer_filter( mlt_producer this, int index )
{
	return mlt_service_filter( mlt_producer_service( this ), index );
}

/** Close the producer.
*/

void mlt_producer_close( mlt_producer this )
{
	if ( this != NULL && mlt_properties_dec_ref( mlt_producer_properties( this ) ) <= 0 )
	{
		this->parent.close = NULL;

		if ( this->close != NULL )
			this->close( this->close_object );
		else
			mlt_service_close( &this->parent );
	}
}
