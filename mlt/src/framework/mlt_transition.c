/*
 * mlt_transition.c -- abstraction for all transition services
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
		mlt_properties properties = mlt_transition_properties( this );

		service->get_frame = transition_get_frame;

		mlt_properties_set_timecode( properties, "in", 0 );
		mlt_properties_set_timecode( properties, "out", 0 );
		mlt_properties_set_int( properties, "a_track", 0 );
		mlt_properties_set_int( properties, "b_track", 1 );

		return 0;
	}
	return 1;
}

/** Get the service associated to the transition.
*/

mlt_service mlt_transition_service( mlt_transition this )
{
	return &this->parent;
}

/** Get the properties interface.
*/

mlt_properties mlt_transition_properties( mlt_transition this )
{
	return mlt_service_properties( mlt_transition_service( this ) );
}

/** Connect this transition with a producers a and b tracks.
*/

int mlt_transition_connect( mlt_transition this, mlt_service producer, int a_track, int b_track )
{
	int ret = mlt_service_connect_producer( &this->parent, producer, a_track );
	if ( ret == 0 )
	{
		mlt_properties properties = mlt_transition_properties( this );
		this->producer = producer;
		mlt_properties_set_int( properties, "a_track", a_track );
		mlt_properties_set_int( properties, "b_track", b_track );
	}
	return ret;
}

/** Set the in and out points.
*/

void mlt_transition_set_in_and_out( mlt_transition this, mlt_timecode in, mlt_timecode out )
{
	mlt_properties properties = mlt_transition_properties( this );
	mlt_properties_set_timecode( properties, "in", in );
	mlt_properties_set_timecode( properties, "out", out );
}

/** Get the index of the a track.
*/

int mlt_transition_get_a_track( mlt_transition this )
{
	mlt_properties properties = mlt_transition_properties( this );
	return mlt_properties_get_int( properties, "a_track" );
}

/** Get the index of the b track.
*/

int mlt_transition_get_b_track( mlt_transition this )
{
	mlt_properties properties = mlt_transition_properties( this );
	return mlt_properties_get_int( properties, "b_track" );
}

/** Get the in point.
*/

mlt_timecode mlt_transition_get_in( mlt_transition this )
{
	mlt_properties properties = mlt_transition_properties( this );
	return mlt_properties_get_timecode( properties, "in" );
}

/** Get the out point.
*/

mlt_timecode mlt_transition_get_out( mlt_transition this )
{
	mlt_properties properties = mlt_transition_properties( this );
	return mlt_properties_get_timecode( properties, "out" );
}

/** Process the frame.
*/

static mlt_frame transition_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	if ( this->process == NULL )
	{
		if ( !mlt_frame_is_test_card( a_frame ) )
		{
			mlt_frame_close( b_frame );
			return a_frame;
		}
		else
		{
			mlt_frame_close( a_frame );
			return b_frame;
		}
	}
	else
	{
		return this->process( this, a_frame, b_frame );
	}
}

/** Get a frame from this filter.

	The logic is complex here. A transition is applied to frames on the a and b tracks
	specified in the connect method above. Since all frames are obtained via this 
	method for all tracks, we have to take special care that we only obtain the a and
	b frames once - we do this on the first call to get a frame from either a or b.
	
	After that, we have 3 cases to resolve:
	
	1) 	if the track is the a_track and we're in the time zone, then we need to call the
		process method to do the effect on the frame (we assign NULL to the a_frame and
		b_frames here) otherwise, we pass on the a_frame unmolested;
	2)	if the track is the b_track and we're the in the time zone OR the b_frame is NULL,
		then we generate a test card frame, otherwise we pass on the b frame unmolested;
	3)	For all other tracks, we get the frames on demand.
*/

static int transition_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	mlt_transition this = service->child;

	mlt_properties properties = mlt_transition_properties( this );

	int a_track = mlt_properties_get_int( properties, "a_track" );
	int b_track = mlt_properties_get_int( properties, "b_track" );
	mlt_timecode in = mlt_properties_get_timecode( properties, "in" );
	mlt_timecode out = mlt_properties_get_timecode( properties, "out" );

	// Fetch a and b frames together...
	if ( ( index == a_track || index == b_track ) &&
		 ( this->a_frame == NULL && this->b_frame == NULL ) )
	{
		mlt_service_get_frame( this->producer, &this->a_frame, a_track );
		mlt_service_get_frame( this->producer, &this->b_frame, b_track );
	}
	
	// Special case track processing
	if ( index == a_track )
	{
		// Determine if we're in the right time zone
		mlt_timecode timecode = mlt_frame_get_timecode( this->a_frame );
		if ( timecode >= in && timecode < out )
		{
			// Process the transition
			*frame = transition_process( this, this->a_frame, this->b_frame );
			
			// Important - NULL both frames now so that we know they're done...
			this->a_frame = NULL;
			this->b_frame = NULL;
		}
		else
		{
			// Pass on the 'a frame' and remember that we've done it
			*frame = this->a_frame;
			this->a_frame = NULL;
		}			
		return 0;
	}
	if ( index == b_track )
	{
		if ( this->b_frame == NULL )
		{
			// We're *probably* in the zone and the a frame has been requested
			*frame = mlt_frame_init( );
		}
		else
		{
			mlt_timecode timecode = mlt_frame_get_timecode( this->b_frame );
			if ( timecode >= in && timecode < out )
			{
				// We're in the zone, but the 'a frame' has not been requested yet
				*frame = mlt_frame_init( );
			}
			else
			{
				// We're out of the zone, pass on b and remember that we've done it
				*frame = this->b_frame;
				this->b_frame = NULL;
			}
		}
		return 0;
	}
	else
	{
		// Pass through
		return mlt_service_get_frame( this->producer, frame, index );
	}
}

/** Close the transition.
*/

void mlt_transition_close( mlt_transition this )
{
	if ( this->close != NULL )
		this->close( this );
	else
		mlt_service_close( &this->parent );
}
