/*
 * mlt_transition.c -- abstraction for all transition services
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "mlt_transition.h"
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Forward references.
*/

static int transition_get_frame( mlt_service this, mlt_frame_ptr frame, int index );

/** Constructor.
*/

int mlt_transition_init( mlt_transition this, void *child )
{
	mlt_service service = &this->parent;
	memset( this, 0, sizeof( struct mlt_transition_s ) );
	this->child = child;
	if ( mlt_service_init( service, this ) == 0 )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

		service->get_frame = transition_get_frame;
		service->close = ( mlt_destructor )mlt_transition_close;
		service->close_object = this;

		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 0 );
		mlt_properties_set_int( properties, "a_track", 0 );
		mlt_properties_set_int( properties, "b_track", 1 );

		return 0;
	}
	return 1;
}

/** Create a new transition.
*/

mlt_transition mlt_transition_new( )
{
	mlt_transition this = calloc( 1, sizeof( struct mlt_transition_s ) );
	if ( this != NULL )
		mlt_transition_init( this, NULL );
	return this;
}

/** Get the service associated to the transition.
*/

mlt_service mlt_transition_service( mlt_transition this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Get the properties interface.
*/

mlt_properties mlt_transition_properties( mlt_transition this )
{
	return MLT_TRANSITION_PROPERTIES( this );
}

/** Connect this transition with a producers a and b tracks.
*/

int mlt_transition_connect( mlt_transition this, mlt_service producer, int a_track, int b_track )
{
	int ret = mlt_service_connect_producer( &this->parent, producer, a_track );
	if ( ret == 0 )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
		this->producer = producer;
		mlt_properties_set_int( properties, "a_track", a_track );
		mlt_properties_set_int( properties, "b_track", b_track );
	}
	return ret;
}

/** Set the in and out points.
*/

void mlt_transition_set_in_and_out( mlt_transition this, mlt_position in, mlt_position out )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
	mlt_properties_set_position( properties, "in", in );
	mlt_properties_set_position( properties, "out", out );
}

/** Get the index of the a track.
*/

int mlt_transition_get_a_track( mlt_transition this )
{
	return mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( this ), "a_track" );
}

/** Get the index of the b track.
*/

int mlt_transition_get_b_track( mlt_transition this )
{
	return mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( this ), "b_track" );
}

/** Get the in point.
*/

mlt_position mlt_transition_get_in( mlt_transition this )
{
	return mlt_properties_get_position( MLT_TRANSITION_PROPERTIES( this ), "in" );
}

/** Get the out point.
*/

mlt_position mlt_transition_get_out( mlt_transition this )
{
	return mlt_properties_get_position( MLT_TRANSITION_PROPERTIES( this ), "out" );
}

/** Process the frame.

  	If we have no process method (unlikely), we simply return the a_frame unmolested.
*/

mlt_frame mlt_transition_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	if ( this->process == NULL )
		return a_frame;
	else
		return this->process( this, a_frame, b_frame );
}

/** Get a frame from this transition.

	The logic is complex here. A transition is typically applied to frames on the a and 
	b tracks specified in the connect method above and only if both contain valid info
	for the transition type (this is either audio or image).

	However, the fixed a_track may not always contain data of the correct type, eg:

	+---------+                               +-------+
	|c1       |                               |c5     | <-- A(0,1) <-- B(0,2) <-- get frame
	+---------+                     +---------+-+-----+        |          |
	                                |c4         |       <------+          |
	         +----------+-----------+-+---------+                         |
	         |c2        |c3           |                 <-----------------+
	         +----------+-------------+

	During the overlap of c1 and c2, there is nothing for the A transition to do, so this
	results in a no operation, but B is triggered. During the overlap of c2 and c3, again, 
	the A transition is inactive and because the B transition is pointing at track 0, 
	it too would be inactive. This isn't an ideal situation - it's better if the B 
	transition simply treats the frames from c3 as though they're the a track.

	For this to work, we cache all frames coming from all tracks between the a and b 
	tracks.  Before we process, we determine that the b frame contains someting of the 
	right type and then we determine which frame to use as the a frame (selecting a
	matching frame from a_track to b_track - 1). If both frames contain data of the 
	correct type, we process the transition.

	This method is invoked for each track and we return the cached frames as needed.
	We clear the cache only when the requested frame is flagged as a 'last_track' frame.
*/

static int transition_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	int error = 0;
	mlt_transition this = service->child;

	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

	int accepts_blanks = mlt_properties_get_int( properties, "accepts_blanks" );
	int a_track = mlt_properties_get_int( properties, "a_track" );
	int b_track = mlt_properties_get_int( properties, "b_track" );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out = mlt_properties_get_position( properties, "out" );
	int always_active = mlt_properties_get_int( properties, "always_active" );
	int type = mlt_properties_get_int( properties, "_transition_type" );
	int reverse_order = 0;

	// Ensure that we have the correct order
	if ( a_track > b_track )
	{
		reverse_order = 1;
		a_track = b_track;
		b_track = mlt_properties_get_int( properties, "a_track" );
	}

	// Only act on this operation once per multitrack iteration from the tractor
	if ( !this->held )
	{
		int active = 0;
		int i = 0;
		int a_frame = a_track;
		int b_frame = b_track;
		mlt_position position;
		int ( *invalid )( mlt_frame ) = type == 1 ? mlt_frame_is_test_card : mlt_frame_is_test_audio;

		// Initialise temporary store
		if ( this->frames == NULL )
			this->frames = calloc( sizeof( mlt_frame ), b_track + 1 );

		// Get all frames between a and b
		for( i = a_track; i <= b_track; i ++ )
			mlt_service_get_frame( this->producer, &this->frames[ i ], i );

		// We're holding these frames until the last_track frame property is received
		this->held = 1;

		// When we need to locate the a_frame
		switch( type )
		{
			case 1:
			case 2:
				// Some transitions (esp. audio) may accept blank frames
				active = accepts_blanks;

				// If we're not active then...
				if ( !active )
				{
					// Hunt for the a_frame
					while( a_frame <= b_frame && invalid( this->frames[ a_frame ] ) )
						a_frame ++;

					// Determine if we're active now
					active = a_frame != b_frame && !invalid( this->frames[ b_frame ] );
				}
				break;

			default:
				fprintf( stderr, "invalid transition type\n" );
				break;
		}

		// Now handle the non-always active case
		if ( active && !always_active )
		{
			// For non-always-active transitions, we need the current position of the a frame
			position = mlt_frame_get_position( this->frames[ a_frame ] );

			// If a is in range, we're active
			active = position >= in && position <= out;
		}

		// Finally, process the a and b frames
		if ( active )
		{
			mlt_frame a_frame_ptr = this->frames[ !reverse_order ? a_frame : b_frame ];
			mlt_frame b_frame_ptr = this->frames[ !reverse_order ? b_frame : a_frame ];
			int a_hide = mlt_properties_get_int( MLT_FRAME_PROPERTIES( a_frame_ptr ), "hide" );
			int b_hide = mlt_properties_get_int( MLT_FRAME_PROPERTIES( b_frame_ptr ), "hide" );
			if ( !( a_hide & type ) && !( b_hide & type ) )
			{
				// Process the transition
				*frame = mlt_transition_process( this, a_frame_ptr, b_frame_ptr );
	
				// We need to ensure that the tractor doesn't consider this frame for output
				if ( *frame == a_frame_ptr )
					b_hide |= type;
				else
					a_hide |= type;
	
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( a_frame_ptr ), "hide", a_hide );
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( b_frame_ptr ), "hide", b_hide );
			}
		}
	}
	
	// Obtain the frame from the cache or the producer we're attached to
	if ( index >= a_track && index <= b_track )
		*frame = this->frames[ index ];
	else
		error = mlt_service_get_frame( this->producer, frame, index );

	// Determine if that was the last track
	this->held = !mlt_properties_get_int( MLT_FRAME_PROPERTIES( *frame ), "last_track" );

	return error;
}

/** Close the transition.
*/

void mlt_transition_close( mlt_transition this )
{
	if ( this != NULL && mlt_properties_dec_ref( MLT_TRANSITION_PROPERTIES( this ) ) <= 0 )
	{
		this->parent.close = NULL;
		if ( this->close != NULL )
		{
			this->close( this );
		}
		else
		{
			mlt_service_close( &this->parent );
			free( this->frames );
			free( this );
		}
	}
}
