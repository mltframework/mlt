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
#include "mlt_playlist.h"
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>

struct mlt_track_s
{
	mlt_producer producer;
	mlt_event event;
};

typedef struct mlt_track_s *mlt_track;

/** Private definition.
*/

struct mlt_multitrack_s
{
	// We're extending producer here
	struct mlt_producer_s parent;
	mlt_track *list;
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
			mlt_properties properties = mlt_multitrack_properties( this );
			producer->get_frame = producer_get_frame;
			mlt_properties_set_data( properties, "multitrack", this, 0, NULL, NULL );
			mlt_properties_set( properties, "log_id", "multitrack" );
			mlt_properties_set( properties, "resource", "<multitrack>" );
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
	return this != NULL ? &this->parent : NULL;
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

/** Initialise position related information.
*/

void mlt_multitrack_refresh( mlt_multitrack this )
{
	int i = 0;

	// Obtain the properties of this multitrack
	mlt_properties properties = mlt_multitrack_properties( this );

	// We need to ensure that the multitrack reports the longest track as its length
	mlt_position length = 0;

	// We need to ensure that fps are the same on all services
	double fps = 0;
	
	// Obtain stats on all connected services
	for ( i = 0; i < this->count; i ++ )
	{
		// Get the producer from this index
		mlt_track track = this->list[ i ];
		mlt_producer producer = track->producer;

		// If it's allocated then, update our stats
		if ( producer != NULL )
		{
			// If we have more than 1 track, we must be in continue mode
			if ( this->count > 1 )
				mlt_properties_set( mlt_producer_properties( producer ), "eof", "continue" );
			
			// Determine the longest length
			if ( !mlt_properties_get_int( mlt_producer_properties( producer ), "hide" ) )
				length = mlt_producer_get_playtime( producer ) > length ? mlt_producer_get_playtime( producer ) : length;
			
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
	mlt_events_block( properties, properties );
	mlt_properties_set_position( properties, "length", length );
	mlt_events_unblock( properties, properties );
	mlt_properties_set_position( properties, "out", length - 1 );
	mlt_properties_set_double( properties, "fps", fps );
}

/** Listener for producers on the playlist.
*/

static void mlt_multitrack_listener( mlt_producer producer, mlt_multitrack this )
{
	mlt_multitrack_refresh( this );
}

/** Connect a producer to a given track.

  	Note that any producer can be connected here, but see special case treatment
	of playlist in clip point determination below.
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
			this->list = realloc( this->list, ( track + 10 ) * sizeof( mlt_track ) );
			for ( i = this->size; i < track + 10; i ++ )
				this->list[ i ] = NULL;
			this->size = track + 10;
		}

		if ( this->list[ track ] != NULL )
		{
			mlt_event_close( this->list[ track ]->event );
			mlt_producer_close( this->list[ track ]->producer );
		}
		else
		{
			this->list[ track ] = malloc( sizeof( struct mlt_track_s ) );
		}

		// Assign the track in our list here
		this->list[ track ]->producer = producer;
		this->list[ track ]->event = mlt_events_listen( mlt_producer_properties( producer ), this, 
									 "producer-changed", ( mlt_listener )mlt_multitrack_listener );
		mlt_properties_inc_ref( mlt_producer_properties( producer ) );
		mlt_event_inc_ref( this->list[ track ]->event );
		
		// Increment the track count if need be
		if ( track >= this->count )
			this->count = track + 1;
			
		// Refresh our stats
		mlt_multitrack_refresh( this );
	}

	return result;
}

/** Get the number of tracks.
*/

int mlt_multitrack_count( mlt_multitrack this )
{
	return this->count;	
}

/** Get an individual track as a producer.
*/

mlt_producer mlt_multitrack_track( mlt_multitrack this, int track )
{
	mlt_producer producer = NULL;
	
	if ( this->list != NULL && track < this->count )
		producer = this->list[ track ]->producer;

	return producer;
}

static int position_compare( const void *p1, const void *p2 )
{
	return *( mlt_position * )p1 - *( mlt_position * )p2;
}

static int add_unique( mlt_position *array, int size, mlt_position position )
{
	int i = 0;
	for ( i = 0; i < size; i ++ )
		if ( array[ i ] == position )
			break;
	if ( i == size )
		array[ size ++ ] = position;
	return size;
}

/** Determine the clip point.

  	Special case here: a 'producer' has no concept of multiple clips - only the 
	playlist and multitrack producers have clip functionality. Further to that a 
	multitrack determines clip information from any connected tracks that happen 
	to be playlists.

	Additionally, it must locate clips in the correct order, for example, consider
	the following track arrangement:

	playlist1 |0.0     |b0.0      |0.1          |0.1         |0.2           |
	playlist2 |b1.0  |1.0           |b1.1     |1.1             |

	Note - b clips represent blanks. They are also reported as clip positions.

	When extracting clip positions from these playlists, we should get a sequence of:

	0.0, 1.0, b0.0, 0.1, b1.1, 1.1, 0.1, 0.2, [out of playlist2], [out of playlist1]
*/

mlt_position mlt_multitrack_clip( mlt_multitrack this, mlt_whence whence, int index )
{
	mlt_position position = 0;
	int i = 0;
	int j = 0;
	mlt_position *map = malloc( 1000 * sizeof( mlt_position ) );
	int count = 0;

	for ( i = 0; i < this->count; i ++ )
	{
		// Get the producer for this track
		mlt_producer producer = this->list[ i ]->producer;

		// If it's assigned and not a hidden track
		if ( producer != NULL )
		{
			// Get the properties of this producer
			mlt_properties properties = mlt_producer_properties( producer );

			// Determine if it's a playlist
			mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );

			// Special case consideration of playlists
			if ( playlist != NULL )
			{
				for ( j = 0; j < mlt_playlist_count( playlist ); j ++ )
					count = add_unique( map, count, mlt_playlist_clip( playlist, mlt_whence_relative_start, j ) );
				count = add_unique( map, count, mlt_producer_get_out( producer ) + 1 );
			}
			else
			{
				count = add_unique( map, count, 0 );
				count = add_unique( map, count, mlt_producer_get_out( producer ) + 1 );
			}
		}
	}

	// Now sort the map
	qsort( map, count, sizeof( mlt_position ), position_compare );

	// Now locate the requested index
	switch( whence )
	{
		case mlt_whence_relative_start:
			if ( index < count )
				position = map[ index ];
			else
				position = map[ count - 1 ];
			break;

		case mlt_whence_relative_current:
			position = mlt_producer_position( mlt_multitrack_producer( this ) );
			for ( i = 0; i < count - 2; i ++ ) 
				if ( position >= map[ i ] && position < map[ i + 1 ] )
					break;
			index += i;
			if ( index >= 0 && index < count )
				position = map[ index ];
			else if ( index < 0 )
				position = map[ 0 ];
			else
				position = map[ count - 1 ];
			break;

		case mlt_whence_relative_end:
			if ( index < count )
				position = map[ count - index - 1 ];
			else
				position = map[ 0 ];
			break;
	}

	// Free the map
	free( map );

	return position;
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

	This is not as clean as it should be... 
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index )
{
	// Get the mutiltrack object
	mlt_multitrack this = parent->child;

	// Check if we have a track for this index
	if ( index < this->count && this->list[ index ] != NULL )
	{
		// Determine the in point
		int in = mlt_producer_is_cut( this->list[ index ]->producer ) ? mlt_producer_get_in( this->list[ index ]->producer ) : 0;

		// Get the producer for this track
		mlt_producer producer = mlt_producer_cut_parent( this->list[ index ]->producer );

		// Obtain the current position
		mlt_position position = mlt_producer_frame( parent );

		// Get the clone index
		int clone_index = mlt_properties_get_int( mlt_producer_properties( this->list[ index ]->producer ), "_clone" );

		// Additionally, check if we're supposed to use a clone here
		if ( clone_index > 0 )
		{
			char key[ 25 ];
			sprintf( key, "_clone.%d", clone_index - 1 );
			producer = mlt_properties_get_data( mlt_producer_properties( producer ), key, NULL );
		}

		// Make sure we're at the same point
		mlt_producer_seek( producer, in + position );

		// Get the frame from the producer
		mlt_service_get_frame( mlt_producer_service( producer ), frame, 0 );

		// Indicate speed of this producer
		mlt_properties producer_properties = mlt_producer_properties( parent );
		double speed = mlt_properties_get_double( producer_properties, "_speed" );
		mlt_properties properties = mlt_frame_properties( *frame );
		mlt_properties_set_double( properties, "_speed", speed );
		mlt_properties_set_position( properties, "_position", position );
		mlt_properties_set_int( properties, "hide", mlt_properties_get_int( mlt_producer_properties( producer ), "hide" ) );
	}
	else
	{
		// Generate a test frame
		*frame = mlt_frame_init( );

		// Update position on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( parent ) );

		// Move on to the next frame
		if ( index >= this->count )
		{
			// Let tractor know if we've reached the end
			mlt_properties_set_int( mlt_frame_properties( *frame ), "last_track", 1 );

			// Move to the next frame
			mlt_producer_prepare_next( parent );
		}
	}

	return 0;
}

/** Close this instance.
*/

void mlt_multitrack_close( mlt_multitrack this )
{
	if ( this != NULL && mlt_properties_dec_ref( mlt_multitrack_properties( this ) ) <= 0 )
	{
		int i = 0;
		for ( i = 0; i < this->count; i ++ )
		{
			if ( this->list[ i ] != NULL )
			{
				mlt_event_close( this->list[ i ]->event );
				mlt_producer_close( this->list[ i ]->producer );
				free( this->list[ i ] );
			}
		}

		// Close the producer
		mlt_producer_close( &this->parent );

		// Free the list
		free( this->list );

		// Free the object
		free( this );
	}
}
