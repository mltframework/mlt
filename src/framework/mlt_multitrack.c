/*
 * mlt_multitrack.c -- multitrack service class
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

#include "mlt_multitrack.h"
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>

/** Private definition.
*/

struct mlt_multitrack_s
{
	// We're extending producer here
	struct mlt_producer_s parent;
	mlt_producer *list;
	int size;
	int count;
};

/** Forward reference.
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );

/** Constructor.
*/

mlt_multitrack mlt_multitrack_init( )
{
	// Allocate the multitrack object
	mlt_multitrack this = calloc( sizeof( struct mlt_multitrack_s ), 1 );

	if ( this != NULL )
	{
		mlt_producer producer = &this->parent;
		if ( mlt_producer_init( producer, this ) == 0 )
		{
			producer->get_frame = producer_get_frame;
		}
		else
		{
			free( this );
			this = NULL;
		}
	}
	
	return this;
}

/** Get the producer associated to this multitrack.
*/

mlt_producer mlt_multitrack_producer( mlt_multitrack this )
{
	return &this->parent;
}

/** Get the service associated this multitrack.
*/

mlt_service mlt_multitrack_service( mlt_multitrack this )
{
	return mlt_producer_service( mlt_multitrack_producer( this ) );
}

/** Get the properties associated this multitrack.
*/

mlt_properties mlt_multitrack_properties( mlt_multitrack this )
{
	return mlt_service_properties( mlt_multitrack_service( this ) );
}

/** Initialise timecode related information.
*/

static void mlt_multitrack_refresh( mlt_multitrack this )
{
	int i = 0;

	// Obtain the properties of this multitrack
	mlt_properties properties = mlt_multitrack_properties( this );

	// We need to ensure that the multitrack reports the longest track as its length
	mlt_timecode length = 0;

	// We need to ensure that fps are the same on all services
	double fps = 0;
	
	// Obtain stats on all connected services
	for ( i = 0; i < this->count; i ++ )
	{
		// Get the producer from this index
		mlt_producer producer = this->list[ i ];

		// If it's allocated then, update our stats
		if ( producer != NULL )
		{
			// Determine the longest length
			length = mlt_producer_get_length( producer ) > length ? mlt_producer_get_length( producer ) : length;
			
			// Handle fps
			if ( fps == 0 )
			{
				// This is the first producer, so it controls the fps
				fps = mlt_producer_get_fps( producer );
			}
			else if ( fps != mlt_producer_get_fps( producer ) )
			{
				// Generate a warning for now - the following attempt to fix may fail
				fprintf( stderr, "Warning: fps mismatch on track %d\n", i );

				// It should be safe to impose fps on an image producer, but not necessarily safe for video
				mlt_properties_set_double( mlt_producer_properties( producer ), "fps", fps );
			}
		}
	}

	// Update multitrack properties now - we'll not destroy the in point here
	mlt_properties_set_timecode( properties, "length", length );
	mlt_properties_set_timecode( properties, "out", length );
	mlt_properties_set_timecode( properties, "playtime", length - mlt_properties_get_timecode( properties, "in" ) );
}

/** Connect a producer to a given track.
*/

int mlt_multitrack_connect( mlt_multitrack this, mlt_producer producer, int track )
{
	// Connect to the producer to ourselves at the specified track
	int result = mlt_service_connect_producer( mlt_multitrack_service( this ), mlt_producer_service( producer ), track );

	if ( result == 0 )
	{
		// Resize the producer list if need be
		if ( track >= this->size )
		{
			int i;
			this->list = realloc( this->list, ( track + 10 ) * sizeof( mlt_producer ) );
			for ( i = this->size; i < track + 10; i ++ )
				this->list[ i ] = NULL;
			this->size = track + 10;
		}
		
		// Assign the track in our list here
		this->list[ track ] = producer;
		
		// Increment the track count if need be
		if ( track >= this->count )
			this->count = track + 1;
			
		// Refresh our stats
		mlt_multitrack_refresh( this );
	}

	return result;
}

/** Get frame method.

  	Special case here: The multitrack must be used in a conjunction with a downstream
	tractor-type service, ie:

	Producer1 \
	Producer2 - multitrack - { filters/transitions } - tractor - consumer
	Producer3 /

	The get_frame of a tractor pulls frames from it's connected service on all tracks and 
	will terminate as soon as it receives a test card with a last_track property. The 
	important case here is that the mulitrack does not move to the next frame until all
	tracks have been pulled. 

	Reasoning: In order to seek on a network such as above, the multitrack needs to ensure
	that all producers are positioned on the same frame. It uses the 'last track' logic
	to determine when to move to the next frame.

	Flaw: if a transition is configured to read from a b-track which happens to trigger
	the last frame logic (ie: it's configured incorrectly), then things are going to go
	out of sync.

	See playlist logic too.
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index )
{
	// Get the mutiltrack object
	mlt_multitrack this = parent->child;

	// Check if we have a track for this index
	if ( index < this->count && this->list[ index ] != NULL )
	{
		// Get the producer for this track
		mlt_producer producer = this->list[ index ];

		// Obtain the current timecode
		uint64_t position = mlt_producer_frame( parent );

		// Make sure we're at the same point
		mlt_producer_seek_frame( producer, position );

		// Get the frame from the producer
		mlt_service_get_frame( mlt_producer_service( producer ), frame, 0 );
	}
	else
	{
		// Generate a test frame
		*frame = mlt_frame_init( );

		// Let tractor know if we've reached the end
		mlt_properties_set_int( mlt_frame_properties( *frame ), "last_track", index >= this->count );

		// Update timecode on the frame we're creating
		mlt_frame_set_timecode( *frame, mlt_producer_position( parent ) );

		// Move on to the next frame
		if ( index >= this->count )
			mlt_producer_prepare_next( parent );
	}

	return 0;
}

/** Close this instance.
*/

void mlt_multitrack_close( mlt_multitrack this )
{
	// Close the producer
	mlt_producer_close( &this->parent );

	// Free the list
	free( this->list );

	// Free the object
	free( this );
}
