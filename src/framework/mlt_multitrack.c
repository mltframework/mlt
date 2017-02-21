/**
 * \file mlt_multitrack.c
 * \brief multitrack service class
 * \see mlt_multitrack_s
 *
 * Copyright (C) 2003-2017 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mlt_multitrack.h"
#include "mlt_playlist.h"
#include "mlt_frame.h"
#include "mlt_factory.h"
#include "mlt_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward reference. */

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );

/** Construct and initialize a new multitrack.
 *
 * Sets the resource property to "<multitrack>".
 *
 * \public \memberof mlt_multitrack_s
 * \return a new multitrack
 */

mlt_multitrack mlt_multitrack_init( )
{
	// Allocate the multitrack object
	mlt_multitrack self = calloc( 1, sizeof( struct mlt_multitrack_s ) );

	if ( self != NULL )
	{
		mlt_producer producer = &self->parent;
		if ( mlt_producer_init( producer, self ) == 0 )
		{
			mlt_properties properties = MLT_MULTITRACK_PROPERTIES( self );
			producer->get_frame = producer_get_frame;
			mlt_properties_set_data( properties, "multitrack", self, 0, NULL, NULL );
			mlt_properties_set( properties, "log_id", "multitrack" );
			mlt_properties_set( properties, "resource", "<multitrack>" );
			mlt_properties_set_int( properties, "in", 0 );
			mlt_properties_set_int( properties, "out", -1 );
			mlt_properties_set_int( properties, "length", 0 );
			producer->close = ( mlt_destructor )mlt_multitrack_close;
		}
		else
		{
			free( self );
			self = NULL;
		}
	}

	return self;
}

/** Get the producer associated to this multitrack.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \return the producer object
 * \see MLT_MULTITRACK_PRODUCER
 */

mlt_producer mlt_multitrack_producer( mlt_multitrack self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Get the service associated this multitrack.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \return the service object
 * \see MLT_MULTITRACK_SERVICE
 */

mlt_service mlt_multitrack_service( mlt_multitrack self )
{
	return MLT_MULTITRACK_SERVICE( self );
}

/** Get the properties associated this multitrack.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \return the multitrack's property list
 * \see MLT_MULTITRACK_PROPERTIES
 */

mlt_properties mlt_multitrack_properties( mlt_multitrack self )
{
	return MLT_MULTITRACK_PROPERTIES( self );
}

/** Initialize position related information.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 */

void mlt_multitrack_refresh( mlt_multitrack self )
{
	int i = 0;

	// Obtain the properties of this multitrack
	mlt_properties properties = MLT_MULTITRACK_PROPERTIES( self );

	// We need to ensure that the multitrack reports the longest track as its length
	mlt_position length = 0;

	// Obtain stats on all connected services
	for ( i = 0; i < self->count; i ++ )
	{
		// Get the producer from this index
		mlt_track track = self->list[ i ];
		mlt_producer producer = track->producer;

		// If it's allocated then, update our stats
		if ( producer != NULL )
		{
			// If we have more than 1 track, we must be in continue mode
			if ( self->count > 1 )
				mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "continue" );

			// Determine the longest length
			//if ( !mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( producer ), "hide" ) )
				length = mlt_producer_get_playtime( producer ) > length ? mlt_producer_get_playtime( producer ) : length;
		}
	}

	// Update multitrack properties now - we'll not destroy the in point here
	mlt_events_block( properties, properties );
	mlt_properties_set_position( properties, "length", length );
	mlt_events_unblock( properties, properties );
	mlt_properties_set_position( properties, "out", length - 1 );
}

/** Listener for producers on the playlist.
 *
 * \private \memberof mlt_multitrack_s
 * \param producer a producer
 * \param self a multitrack
 */

static void mlt_multitrack_listener( mlt_producer producer, mlt_multitrack self )
{
	mlt_multitrack_refresh( self );
}

static void resize_service_caches( mlt_multitrack self )
{
	mlt_properties caches = mlt_properties_get_data( mlt_global_properties(), "caches", NULL );
	if ( caches )
	{
		int i;
		for ( i = 0; i < mlt_properties_count(caches); ++i )
		{
			mlt_cache cache = mlt_properties_get_data_at( caches, i, NULL );
			if ( self->count > mlt_cache_get_size(cache) )
				mlt_cache_set_size( cache, self->count + 1 );
		}
	}
}

/** Connect a producer to a given track.
 *
 * Note that any producer can be connected here, but see special case treatment
 * of playlist in clip point determination below.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \param producer the producer to connect to the multitrack producer
 * \param track the 0-based index of the track on which to connect the multitrack
 * \return true on error
 */

int mlt_multitrack_connect( mlt_multitrack self, mlt_producer producer, int track )
{
	// Connect to the producer to ourselves at the specified track
	int result = mlt_service_connect_producer( MLT_MULTITRACK_SERVICE( self ), MLT_PRODUCER_SERVICE( producer ), track );

	if ( result == 0 )
	{
		mlt_track current_track = ( track < self->count )? self->list[ track ] : NULL;
		// Resize the producer list if need be
		if ( track >= self->size )
		{
			int i;
			self->list = realloc( self->list, ( track + 10 ) * sizeof( mlt_track ) );
			for ( i = self->size; i < track + 10; i ++ )
				self->list[ i ] = NULL;
			self->size = track + 10;
		}

		if ( current_track )
		{
			mlt_event_close( current_track->event );
			mlt_producer_close( current_track->producer );
		}
		else
		{
			self->list[ track ] = malloc( sizeof( struct mlt_track_s ) );
		}

		// Assign the track in our list here
		self->list[ track ]->producer = producer;
		self->list[ track ]->event = mlt_events_listen( MLT_PRODUCER_PROPERTIES( producer ), self,
									 "producer-changed", ( mlt_listener )mlt_multitrack_listener );
		mlt_properties_inc_ref( MLT_PRODUCER_PROPERTIES( producer ) );
		mlt_event_inc_ref( self->list[ track ]->event );

		// Increment the track count if need be
		if ( track >= self->count )
		{
			self->count = track + 1;
			resize_service_caches( self );
		}

		// Refresh our stats
		mlt_multitrack_refresh( self );
	}

	return result;
}

/** Insert a producer to a given track.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \param producer the producer to connect to the multitrack producer
 * \param track the 0-based index of the track on which to connect the multitrack
 * \return true on error
 */

int mlt_multitrack_insert( mlt_multitrack self, mlt_producer producer, int track )
{
	if ( track >= self->count )
		return mlt_multitrack_connect( self, producer, track );

	// Connect to the producer to ourselves at the specified track
	int result = mlt_service_insert_producer( MLT_MULTITRACK_SERVICE( self ), MLT_PRODUCER_SERVICE( producer ), track );

	if ( result == 0 )
	{
		// Resize the producer list if needed.
		if ( self->count + 1 > self->size )
		{
			int new_size = self->size + 10;
			self->list = realloc( self->list, new_size * sizeof( mlt_track ) );
			if ( self->list )
			{
				memset( &self->list[ self->size ], 0, new_size - self->size );
				self->size = new_size;
			}
		}

		if ( self->list )
		{
			// Move all of the list elements following track N down by 1.
			memmove( &self->list[ track + 1 ], &self->list[ track ],
					( self->count - track ) * sizeof ( mlt_track ) );
			self->count ++;
			resize_service_caches( self );

			// Assign the track in our list.
			self->list[ track ] = malloc( sizeof( struct mlt_track_s ) );
			self->list[ track ]->producer = producer;
			self->list[ track ]->event = mlt_events_listen( MLT_PRODUCER_PROPERTIES( producer ), self,
										 "producer-changed", ( mlt_listener )mlt_multitrack_listener );
			mlt_properties_inc_ref( MLT_PRODUCER_PROPERTIES( producer ) );
			mlt_event_inc_ref( self->list[ track ]->event );

			// Refresh our stats
			mlt_multitrack_refresh( self );
		}
		else
		{
			result = -1;
		}
	}

	return result;
}

/** Remove the N-th track.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \param track the index of the track to remove
 * \return true if there was an error
 */

int mlt_multitrack_disconnect( mlt_multitrack self, int track )
{
	int error = -1;

	if ( self && self->list && track >= 0 && track < self->count )
	{
		// Disconnect the track producer.
		error = mlt_service_disconnect_producer( MLT_MULTITRACK_SERVICE(self), track );
		if ( !error )
		{
			// Release references on track.
			if ( self->list[ track ] )
			{
				mlt_producer_close( self->list[ track ]->producer );
				mlt_event_close( self->list[ track ]->event );
			}

			// Contract the list of tracks.
			for ( ; track + 1 < self->count; track ++ )
			{
				if ( self->list[ track ] && self->list[ track + 1 ] )
				{
					self->list[ track ]->producer = self->list[ track + 1 ]->producer;
					self->list[ track ]->event = self->list[ track + 1 ]->event;
				}
			}
			if ( self->list[ self->count - 1 ] )
			{
				free( self->list[ self->count - 1 ] );
				self->list[ self->count - 1 ] = NULL;
			}
			self->count --;

			// Recalculate the duration.
			mlt_multitrack_refresh( self );
		}
	}
	return error;
}

/** Get the number of tracks.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \return the number of tracks
 */

int mlt_multitrack_count( mlt_multitrack self )
{
	if ( self == NULL )
		return 0;
	else
		return self->count;
}

/** Get an individual track as a producer.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \param track the 0-based index of the producer to get
 * \return the producer or NULL if not valid
 */

mlt_producer mlt_multitrack_track( mlt_multitrack self, int track )
{
	mlt_producer producer = NULL;

	if ( self->list != NULL && track >= 0 && track < self->count )
		producer = self->list[ track ]->producer;

	return producer;
}

/** Position comparison function for sorting.
 *
 * \private \memberof mlt_multitrack_s
 * \param p1 a position
 * \param p2 another position
 * \return <0 if \p p1 is less than \p p2, 0 if equal, >0 if greater
 */

static int position_compare( const void *p1, const void *p2 )
{
	return *( const mlt_position * )p1 - *( const mlt_position * )p2;
}

/** Add a position to a set.
 *
 * \private \memberof mlt_multitrack_s
 * \param array an array of positions (the set)
 * \param size the current number of positions in the array (not the capacity of the array)
 * \param position the position to add
 * \return the new size of the array
 */

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

/** Increase the capacity of a set of mlt_position.
 *
 * \private \memberof mlt_multitrack_s
 * \param array an array of positions (the set)
 * \param count the current number of elements in the array (not the capacity)
 * \param[out] size the current capacity of the array
 * \return the new address of the array
 */

static mlt_position* resize_set( mlt_position *map, int count, int *size )
{
	mlt_position* result = map;
	// Resize only if needed.
	if ( count + 1 >= *size )
	{
		result = realloc( map, (*size + 1000) * sizeof(*map) );
		memset( map + *size, 0, 1000 * sizeof(*map) );
		*size += 1000;
	}
	return result;
}

/** Determine the clip point.
 *
 * <pre>
 * Special case here: a 'producer' has no concept of multiple clips - only the
 * playlist and multitrack producers have clip functionality. Further to that a
 * multitrack determines clip information from any connected tracks that happen
 * to be playlists.
 *
 * Additionally, it must locate clips in the correct order, for example, consider
 * the following track arrangement:
 *
 * playlist1 |0.0     |b0.0      |0.1          |0.1         |0.2           |
 * playlist2 |b1.0  |1.0           |b1.1     |1.1             |
 *
 * Note - b clips represent blanks. They are also reported as clip positions.
 *
 * When extracting clip positions from these playlists, we should get a sequence of:
 *
 * 0.0, 1.0, b0.0, 0.1, b1.1, 1.1, 0.1, 0.2, [out of playlist2], [out of playlist1]
 * </pre>
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 * \param whence from where to extract
 * \param index the 0-based index of which clip to extract
 * \return the position of clip \p index relative to \p whence
 */

mlt_position mlt_multitrack_clip( mlt_multitrack self, mlt_whence whence, int index )
{
	mlt_position position = 0;
	int i = 0;
	int j = 0;
	int size = 1000;
	mlt_position *map = calloc( size, sizeof(*map) );
	int count = 0;

	for ( i = 0; i < self->count; i ++ )
	{
		// Get the producer for this track
		mlt_producer producer = self->list[ i ]->producer;

		// If it's assigned and not a hidden track
		if ( producer != NULL )
		{
			// Get the properties of this producer
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			// Determine if it's a playlist
			mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );

			map = resize_set( map, count, &size );
			
			// Special case consideration of playlists
			if ( playlist != NULL )
			{
				for ( j = 0; j < mlt_playlist_count( playlist ); j ++ )
				{
					count = add_unique( map, count, mlt_playlist_clip( playlist, mlt_whence_relative_start, j ) );
					map = resize_set( map, count, &size );
				}
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
			position = mlt_producer_position( MLT_MULTITRACK_PRODUCER( self ) );
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
 *
 * <pre>
 * Special case here: The multitrack must be used in a conjunction with a downstream
 * tractor-type service, ie:
 *
 * Producer1 \
 * Producer2 - multitrack - { filters/transitions } - tractor - consumer
 * Producer3 /
 *
 * The get_frame of a tractor pulls frames from it's connected service on all tracks and
 * will terminate as soon as it receives a test card with a last_track property. The
 * important case here is that the mulitrack does not move to the next frame until all
 * tracks have been pulled.
 *
 * Reasoning: In order to seek on a network such as above, the multitrack needs to ensure
 * that all producers are positioned on the same frame. It uses the 'last track' logic
 * to determine when to move to the next frame.
 *
 * Flaw: if a transition is configured to read from a b-track which happens to trigger
 * the last frame logic (ie: it's configured incorrectly), then things are going to go
 * out of sync.
 *
 * See playlist logic too.
 * </pre>
 *
 * \private \memberof mlt_multitrack_s
 * \param parent the producer interface to a mulitrack
 * \param[out] frame a frame by reference
 * \param index the 0-based track index
 * \return true if there was an error
 */

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index )
{
	// Get the mutiltrack object
	mlt_multitrack self = parent->child;

	// Check if we have a track for this index
	if ( index >= 0 && index < self->count && self->list[ index ] != NULL )
	{
		// Get the producer for this track
		mlt_producer producer = self->list[ index ]->producer;

		// Get the track hide property
		int hide = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( producer ) ), "hide" );

		// Obtain the current position
		mlt_position position = mlt_producer_frame( parent );

		// Get the parent properties
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( parent );

		// Get the speed
		double speed = mlt_properties_get_double( producer_properties, "_speed" );

		// Make sure we're at the same point
		mlt_producer_seek( producer, position );

		// Get the frame from the producer
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), frame, 0 );

		// Indicate speed of this producer
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties_set_double( properties, "_speed", speed );
		mlt_frame_set_position( *frame, position );
		mlt_properties_set_int( properties, "hide", hide );
	}
	else
	{
		// Generate a test frame
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( parent ) );

		// Update position on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( parent ) );

		// Move on to the next frame
		if ( index >= self->count )
		{
			// Let tractor know if we've reached the end
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "last_track", 1 );

			// Move to the next frame
			mlt_producer_prepare_next( parent );
		}
	}

	return 0;
}

/** Close this instance and free its resources.
 *
 * \public \memberof mlt_multitrack_s
 * \param self a multitrack
 */

void mlt_multitrack_close( mlt_multitrack self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_MULTITRACK_PROPERTIES( self ) ) <= 0 )
	{
		int i = 0;
		for ( i = 0; i < self->count; i ++ )
		{
			if ( self->list[ i ] != NULL )
			{
				mlt_event_close( self->list[ i ]->event );
				mlt_producer_close( self->list[ i ]->producer );
				free( self->list[ i ] );
			}
		}

		// Close the producer
		self->parent.close = NULL;
		mlt_producer_close( &self->parent );

		// Free the list
		free( self->list );

		// Free the object
		free( self );
	}
}
