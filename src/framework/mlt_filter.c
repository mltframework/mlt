/*
 * mlt_filter.c -- abstraction for all filter services
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

#include "mlt_filter.h"
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int filter_get_frame( mlt_service this, mlt_frame_ptr frame, int index );

/** Constructor method.
*/

int mlt_filter_init( mlt_filter this, void *child )
{
	mlt_service service = &this->parent;
	memset( this, 0, sizeof( struct mlt_filter_s ) );
	this->child = child;
	if ( mlt_service_init( service, this ) == 0 )
	{
		service->get_frame = filter_get_frame;
		return 0;
	}
	return 1;
}

/** Get the service associated to this filter
*/

mlt_service mlt_filter_service( mlt_filter this )
{
	return &this->parent;
}

/** Connect this filter to a producers track. Note that a filter only operates
	on a single track, and by default it operates on the entirety of that track.
*/

int mlt_filter_connect( mlt_filter this, mlt_service producer, int index )
{
	int ret = mlt_service_connect_producer( &this->parent, producer, index );
	
	// If the connection was successful, grab the producer, track and reset in/out
	if ( ret == 0 )
	{
		this->producer = producer;
		this->track = index;
		this->in = 0;
		this->out = 0;
	}
	
	return ret;
}

/** Tune the in/out points.
*/

void mlt_filter_set_in_and_out( mlt_filter this, mlt_timecode in, mlt_timecode out )
{
	this->in = in;
	this->out = out;
}

/** Return the track that this filter is operating on.
*/

int mlt_filter_get_track( mlt_filter this )
{
	return this->track;
}

/** Get the in point.
*/

mlt_timecode mlt_filter_get_in( mlt_filter this )
{
	return this->in;
}

/** Get the out point.
*/

mlt_timecode mlt_filter_get_out( mlt_filter this )
{
	return this->out;
}

/** Process the frame.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	if ( this->process == NULL )
		return frame;
	else
		return this->process( this, frame );
}

/** Get a frame from this filter.
*/

static int filter_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	mlt_filter this = service->child;
	
	// If the frame request is for this filters track, we need to process it
	if ( index == this->track )
	{
		int ret = mlt_service_get_frame( this->producer, frame, index );
		if ( ret == 0 )
		{
			if ( !mlt_frame_is_test_card( *frame ) )
			{
				mlt_timecode timecode = mlt_frame_get_timecode( *frame );
				if ( timecode >= this->in && ( this->out == 0 || timecode < this->out ) )
					*frame = filter_process( this, *frame );
			}
			return 0;
		}
		else
		{
			*frame = mlt_frame_init( );
			return 0;
		}
	}
	else
	{
		return mlt_service_get_frame( this->producer, frame, index );
	}
}

/** Close the filter.
*/

void mlt_filter_close( mlt_filter this )
{
	if ( this->close != NULL )
		this->close( this );
	else
		mlt_service_close( &this->parent );
}

