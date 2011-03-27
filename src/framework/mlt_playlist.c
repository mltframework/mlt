/**
 * \file mlt_playlist.c
 * \brief playlist service class
 * \see mlt_playlist_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
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

#include "mlt_playlist.h"
#include "mlt_tractor.h"
#include "mlt_multitrack.h"
#include "mlt_field.h"
#include "mlt_frame.h"
#include "mlt_transition.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** \brief Virtual playlist entry used by mlt_playlist_s
*/

struct playlist_entry_s
{
	mlt_producer producer;
	mlt_position frame_in;
	mlt_position frame_out;
	mlt_position frame_count;
	int repeat;
	mlt_position producer_length;
	mlt_event event;
	int preservation_hack;
};

/* Forward declarations
*/

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static int mlt_playlist_unmix( mlt_playlist self, int clip );
static int mlt_playlist_resize_mix( mlt_playlist self, int clip, int in, int out );

/** Construct a playlist.
 *
 * Sets the resource property to "<playlist>".
 * Set the mlt_type to property to "mlt_producer".
 * \public \memberof mlt_playlist_s
 * \return a new playlist
 */

mlt_playlist mlt_playlist_init( )
{
	mlt_playlist self = calloc( sizeof( struct mlt_playlist_s ), 1 );
	if ( self != NULL )
	{
		mlt_producer producer = &self->parent;

		// Construct the producer
		mlt_producer_init( producer, self );

		// Override the producer get_frame
		producer->get_frame = producer_get_frame;

		// Define the destructor
		producer->close = ( mlt_destructor )mlt_playlist_close;
		producer->close_object = self;

		// Initialise blank
		mlt_producer_init( &self->blank, NULL );
		mlt_properties_set( MLT_PRODUCER_PROPERTIES( &self->blank ), "mlt_service", "blank" );
		mlt_properties_set( MLT_PRODUCER_PROPERTIES( &self->blank ), "resource", "blank" );

		// Indicate that this producer is a playlist
		mlt_properties_set_data( MLT_PLAYLIST_PROPERTIES( self ), "playlist", self, 0, NULL, NULL );

		// Specify the eof condition
		mlt_properties_set( MLT_PLAYLIST_PROPERTIES( self ), "eof", "pause" );
		mlt_properties_set( MLT_PLAYLIST_PROPERTIES( self ), "resource", "<playlist>" );
		mlt_properties_set( MLT_PLAYLIST_PROPERTIES( self ), "mlt_type", "mlt_producer" );
		mlt_properties_set_position( MLT_PLAYLIST_PROPERTIES( self ), "in", 0 );
		mlt_properties_set_position( MLT_PLAYLIST_PROPERTIES( self ), "out", -1 );
		mlt_properties_set_position( MLT_PLAYLIST_PROPERTIES( self ), "length", 0 );

		self->size = 10;
		self->list = malloc( self->size * sizeof( playlist_entry * ) );
	}

	return self;
}

/** Get the producer associated to this playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return the producer interface
 * \see MLT_PLAYLIST_PRODUCER
 */

mlt_producer mlt_playlist_producer( mlt_playlist self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Get the service associated to this playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return the service interface
 * \see MLT_PLAYLIST_SERVICE
 */

mlt_service mlt_playlist_service( mlt_playlist self )
{
	return MLT_PRODUCER_SERVICE( &self->parent );
}

/** Get the properties associated to this playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return the playlist's properties list
 * \see MLT_PLAYLIST_PROPERTIES
 */

mlt_properties mlt_playlist_properties( mlt_playlist self )
{
	return MLT_PRODUCER_PROPERTIES( &self->parent );
}

/** Refresh the playlist after a clip has been changed.
 *
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \return false
 */

static int mlt_playlist_virtual_refresh( mlt_playlist self )
{
	// Obtain the properties
	mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );
	int i = 0;
	mlt_position frame_count = 0;

	for ( i = 0; i < self->count; i ++ )
	{
		// Get the producer
		mlt_producer producer = self->list[ i ]->producer;
		if ( producer )
		{
			int current_length = mlt_producer_get_playtime( producer );

			// Check if the length of the producer has changed
			if ( self->list[ i ]->frame_in != mlt_producer_get_in( producer ) ||
				self->list[ i ]->frame_out != mlt_producer_get_out( producer ) )
			{
				// This clip should be removed...
				if ( current_length < 1 )
				{
					self->list[ i ]->frame_in = 0;
					self->list[ i ]->frame_out = -1;
					self->list[ i ]->frame_count = 0;
				}
				else
				{
					self->list[ i ]->frame_in = mlt_producer_get_in( producer );
					self->list[ i ]->frame_out = mlt_producer_get_out( producer );
					self->list[ i ]->frame_count = current_length;
				}

				// Update the producer_length
				self->list[ i ]->producer_length = current_length;
			}
		}

		// Calculate the frame_count
		self->list[ i ]->frame_count = ( self->list[ i ]->frame_out - self->list[ i ]->frame_in + 1 ) * self->list[ i ]->repeat;

		// Update the frame_count for self clip
		frame_count += self->list[ i ]->frame_count;
	}

	// Refresh all properties
	mlt_events_block( properties, properties );
	mlt_properties_set_position( properties, "length", frame_count );
	mlt_events_unblock( properties, properties );
	mlt_properties_set_position( properties, "out", frame_count - 1 );

	return 0;
}

/** Listener for producers on the playlist.
 *
 * Refreshes the playlist whenever an entry receives producer-changed.
 * \private \memberof mlt_playlist_s
 * \param producer a producer
 * \param self a playlist
 */

static void mlt_playlist_listener( mlt_producer producer, mlt_playlist self )
{
	mlt_playlist_virtual_refresh( self );
}

/** Append to the virtual playlist.
 *
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \param source a producer
 * \param in the producer's starting time
 * \param out the producer's ending time
 * \return true if there was an error
 */

static int mlt_playlist_virtual_append( mlt_playlist self, mlt_producer source, mlt_position in, mlt_position out )
{
	mlt_producer producer = NULL;
	mlt_properties properties = NULL;
	mlt_properties parent = NULL;

	// If we have a cut, then use the in/out points from the cut
	if ( mlt_producer_is_blank( source )  )
	{
		// Make sure the blank is long enough to accomodate the length specified
		if ( out - in + 1 > mlt_producer_get_length( &self->blank ) )
		{
			mlt_properties blank_props = MLT_PRODUCER_PROPERTIES( &self->blank );
			mlt_events_block( blank_props, blank_props );
			mlt_producer_set_in_and_out( &self->blank, in, out );
			mlt_events_unblock( blank_props, blank_props );
		}

		// Now make sure the cut comes from this self->blank
		if ( source == NULL )
		{
			producer = mlt_producer_cut( &self->blank, in, out );
		}
		else if ( !mlt_producer_is_cut( source ) || mlt_producer_cut_parent( source ) != &self->blank )
		{
			producer = mlt_producer_cut( &self->blank, in, out );
		}
		else
		{
			producer = source;
			mlt_properties_inc_ref( MLT_PRODUCER_PROPERTIES( producer ) );
		}

		properties = MLT_PRODUCER_PROPERTIES( producer );
	}
	else if ( mlt_producer_is_cut( source ) )
	{
		producer = source;
		if ( in < 0 )
			in = mlt_producer_get_in( producer );
		if ( out < 0 || out > mlt_producer_get_out( producer ) )
			out = mlt_producer_get_out( producer );
		properties = MLT_PRODUCER_PROPERTIES( producer );
		mlt_properties_inc_ref( properties );
	}
	else
	{
		producer = mlt_producer_cut( source, in, out );
		if ( in < 0 || in < mlt_producer_get_in( producer ) )
			in = mlt_producer_get_in( producer );
		if ( out < 0 || out > mlt_producer_get_out( producer ) )
			out = mlt_producer_get_out( producer );
		properties = MLT_PRODUCER_PROPERTIES( producer );
	}

	// Fetch the cuts parent properties
	parent = MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( producer ) );

	// Remove loader normalisers for fx cuts
	if ( mlt_properties_get_int( parent, "meta.fx_cut" ) )
	{
		mlt_service service = MLT_PRODUCER_SERVICE( mlt_producer_cut_parent( producer ) );
		mlt_filter filter = mlt_service_filter( service, 0 );
		while ( filter != NULL && mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "_loader" ) )
		{
			mlt_service_detach( service, filter );
			filter = mlt_service_filter( service, 0 );
		}
		mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( producer ), "meta.fx_cut", 1 );
	}

	// Check that we have room
	if ( self->count >= self->size )
	{
		int i;
		self->list = realloc( self->list, ( self->size + 10 ) * sizeof( playlist_entry * ) );
		for ( i = self->size; i < self->size + 10; i ++ ) self->list[ i ] = NULL;
		self->size += 10;
	}

	// Create the entry
	self->list[ self->count ] = calloc( sizeof( playlist_entry ), 1 );
	if ( self->list[ self->count ] != NULL )
	{
		self->list[ self->count ]->producer = producer;
		self->list[ self->count ]->frame_in = in;
		self->list[ self->count ]->frame_out = out;
		self->list[ self->count ]->frame_count = out - in + 1;
		self->list[ self->count ]->repeat = 1;
		self->list[ self->count ]->producer_length = mlt_producer_get_playtime( producer );
		self->list[ self->count ]->event = mlt_events_listen( parent, self, "producer-changed", ( mlt_listener )mlt_playlist_listener );
		mlt_event_inc_ref( self->list[ self->count ]->event );
		mlt_properties_set( properties, "eof", "pause" );
		mlt_producer_set_speed( producer, 0 );
		self->count ++;
	}

	return mlt_playlist_virtual_refresh( self );
}

/** Locate a producer by index.
 *
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \param[in, out] position the time at which to locate the producer, returns the time relative to the producer's starting point
 * \param[out] clip the index of the playlist entry
 * \param[out] total the duration of the playlist up to and including this producer
 * \return a producer or NULL if not found
 */

static mlt_producer mlt_playlist_locate( mlt_playlist self, mlt_position *position, int *clip, int *total )
{
	// Default producer to NULL
	mlt_producer producer = NULL;

	// Loop for each producer until found
	for ( *clip = 0; *clip < self->count; *clip += 1 )
	{
		// Increment the total
		*total += self->list[ *clip ]->frame_count;

		// Check if the position indicates that we have found the clip
		// Note that 0 length clips get skipped automatically
		if ( *position < self->list[ *clip ]->frame_count )
		{
			// Found it, now break
			producer = self->list[ *clip ]->producer;
			break;
		}
		else
		{
			// Decrement position by length of self entry
			*position -= self->list[ *clip ]->frame_count;
		}
	}

	return producer;
}

/** Seek in the virtual playlist.
 *
 * This gets the producer at the current position and seeks on the producer
 * while doing repeat and end-of-file handling. This is also responsible for
 * closing producers previous to the preceding playlist if the autoclose
 * property is set.
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \param[out] progressive true if the producer should be displayed progressively
 * \return the service interface of the producer at the play head
 * \see producer_get_frame
 */

static mlt_service mlt_playlist_virtual_seek( mlt_playlist self, int *progressive )
{
	// Map playlist position to real producer in virtual playlist
	mlt_position position = mlt_producer_frame( &self->parent );

	// Keep the original position since we change it while iterating through the list
	mlt_position original = position;

	// Clip index and total
	int i = 0;
	int total = 0;

	// Locate the producer for the position
	mlt_producer producer = mlt_playlist_locate( self, &position, &i, &total );

	// Get the properties
	mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );

	// Automatically close previous producers if requested
	if ( i > 1 // keep immediate previous in case app wants to get info about what just finished
		&& position < 2 // tolerate off-by-one error on going to next clip
		&& mlt_properties_get_int( properties, "autoclose" ) )
	{
		int j;
		// They might have jumped ahead!
		for ( j = 0; j < i - 1; j++ )
		{
			mlt_service_lock( MLT_PRODUCER_SERVICE( self->list[ j ]->producer ) );
			mlt_producer p = self->list[ j ]->producer;
			if ( p )
			{
				self->list[ j ]->producer = NULL;
				mlt_service_unlock( MLT_PRODUCER_SERVICE( p ) );
				mlt_producer_close( p );
			}
			// If p is null, the lock will not have been "taken"
		}
	}

	// Get the eof handling
	char *eof = mlt_properties_get( properties, "eof" );

	// Seek in real producer to relative position
	if ( producer != NULL )
	{
		int count = self->list[ i ]->frame_count / self->list[ i ]->repeat;
		*progressive = count == 1;
		mlt_producer_seek( producer, (int)position % count );
	}
	else if ( !strcmp( eof, "pause" ) && total > 0 )
	{
		playlist_entry *entry = self->list[ self->count - 1 ];
		int count = entry->frame_count / entry->repeat;
		mlt_producer self_producer = MLT_PLAYLIST_PRODUCER( self );
		mlt_producer_seek( self_producer, original - 1 );
		producer = entry->producer;
		mlt_producer_seek( producer, (int)entry->frame_out % count );
		mlt_producer_set_speed( self_producer, 0 );
		mlt_producer_set_speed( producer, 0 );
		*progressive = count == 1;
	}
	else if ( !strcmp( eof, "loop" ) && total > 0 )
	{
		playlist_entry *entry = self->list[ 0 ];
		mlt_producer self_producer = MLT_PLAYLIST_PRODUCER( self );
		mlt_producer_seek( self_producer, 0 );
		producer = entry->producer;
		mlt_producer_seek( producer, 0 );
	}
	else
	{
		producer = &self->blank;
	}

	return MLT_PRODUCER_SERVICE( producer );
}

/** Invoked when a producer indicates that it has prematurely reached its end.
 *
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \return a producer
 * \see producer_get_frame
 */

static mlt_producer mlt_playlist_virtual_set_out( mlt_playlist self )
{
	// Default producer to blank
	mlt_producer producer = &self->blank;

	// Map playlist position to real producer in virtual playlist
	mlt_position position = mlt_producer_frame( &self->parent );

	// Loop through the virtual playlist
	int i = 0;

	for ( i = 0; i < self->count; i ++ )
	{
		if ( position < self->list[ i ]->frame_count )
		{
			// Found it, now break
			producer = self->list[ i ]->producer;
			break;
		}
		else
		{
			// Decrement position by length of this entry
			position -= self->list[ i ]->frame_count;
		}
	}

	// Seek in real producer to relative position
	if ( i < self->count && self->list[ i ]->frame_out != position )
	{
		// Update the frame_count for the changed clip (hmmm)
		self->list[ i ]->frame_out = position;
		self->list[ i ]->frame_count = self->list[ i ]->frame_out - self->list[ i ]->frame_in + 1;

		// Refresh the playlist
		mlt_playlist_virtual_refresh( self );
	}

	return producer;
}

/** Obtain the current clips index.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return the index of the playlist entry at the current position
 */

int mlt_playlist_current_clip( mlt_playlist self )
{
	// Map playlist position to real producer in virtual playlist
	mlt_position position = mlt_producer_frame( &self->parent );

	// Loop through the virtual playlist
	int i = 0;

	for ( i = 0; i < self->count; i ++ )
	{
		if ( position < self->list[ i ]->frame_count )
		{
			// Found it, now break
			break;
		}
		else
		{
			// Decrement position by length of this entry
			position -= self->list[ i ]->frame_count;
		}
	}

	return i;
}

/** Obtain the current clips producer.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return the producer at the current position
 */

mlt_producer mlt_playlist_current( mlt_playlist self )
{
	int i = mlt_playlist_current_clip( self );
	if ( i < self->count )
		return self->list[ i ]->producer;
	else
		return &self->blank;
}

/** Get the position which corresponds to the start of the next clip.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param whence the location from which to make the index relative:
 * start of playlist, end of playlist, or current position
 * \param index the playlist entry index relative to whence
 * \return the time at which the referenced clip starts
 */

mlt_position mlt_playlist_clip( mlt_playlist self, mlt_whence whence, int index )
{
	mlt_position position = 0;
	int absolute_clip = index;
	int i = 0;

	// Determine the absolute clip
	switch ( whence )
	{
		case mlt_whence_relative_start:
			absolute_clip = index;
			break;

		case mlt_whence_relative_current:
			absolute_clip = mlt_playlist_current_clip( self ) + index;
			break;

		case mlt_whence_relative_end:
			absolute_clip = self->count - index;
			break;
	}

	// Check that we're in a valid range
	if ( absolute_clip < 0 )
		absolute_clip = 0;
	else if ( absolute_clip > self->count )
		absolute_clip = self->count;

	// Now determine the position
	for ( i = 0; i < absolute_clip; i ++ )
		position += self->list[ i ]->frame_count;

	return position;
}

/** Get all the info about the clip specified.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param info a clip info struct
 * \param index a playlist entry index
 * \return true if there was an error
 */

int mlt_playlist_get_clip_info( mlt_playlist self, mlt_playlist_clip_info *info, int index )
{
	int error = index < 0 || index >= self->count || self->list[ index ]->producer == NULL;
	memset( info, 0, sizeof( mlt_playlist_clip_info ) );
	if ( !error )
	{
		mlt_producer producer = mlt_producer_cut_parent( self->list[ index ]->producer );
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		info->clip = index;
		info->producer = producer;
		info->cut = self->list[ index ]->producer;
		info->start = mlt_playlist_clip( self, mlt_whence_relative_start, index );
		info->resource = mlt_properties_get( properties, "resource" );
		info->frame_in = self->list[ index ]->frame_in;
		info->frame_out = self->list[ index ]->frame_out;
		info->frame_count = self->list[ index ]->frame_count;
		info->repeat = self->list[ index ]->repeat;
		info->length = mlt_producer_get_length( producer );
		info->fps = mlt_producer_get_fps( producer );
	}

	return error;
}

/** Get number of clips in the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return the number of playlist entries
 */

int mlt_playlist_count( mlt_playlist self )
{
	return self->count;
}

/** Clear the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \return true if there was an error
 */

int mlt_playlist_clear( mlt_playlist self )
{
	int i;
	for ( i = 0; i < self->count; i ++ )
	{
		mlt_event_close( self->list[ i ]->event );
		mlt_producer_close( self->list[ i ]->producer );
	}
	self->count = 0;
	return mlt_playlist_virtual_refresh( self );
}

/** Append a producer to the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param producer the producer to append
 * \return true if there was an error
 */

int mlt_playlist_append( mlt_playlist self, mlt_producer producer )
{
	// Append to virtual list
	return mlt_playlist_virtual_append( self, producer, 0, mlt_producer_get_playtime( producer ) - 1 );
}

/** Append a producer to the playlist with in/out points.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param producer the producer to append
 * \param in the starting point on the producer; a negative value is the same as 0
 * \param out the ending point on the producer; a negative value is the same as producer length - 1
 * \return true if there was an error
 */

int mlt_playlist_append_io( mlt_playlist self, mlt_producer producer, mlt_position in, mlt_position out )
{
	// Append to virtual list
	if ( in < 0 && out < 0 )
		return mlt_playlist_append( self, producer );
	else
		return mlt_playlist_virtual_append( self, producer, in, out );
}

/** Append a blank to the playlist of a given length.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param length the ending time of the blank entry, not its duration
 * \return true if there was an error
 */

int mlt_playlist_blank( mlt_playlist self, mlt_position length )
{
	// Append to the virtual list
	if (length >= 0)
		return mlt_playlist_virtual_append( self, &self->blank, 0, length );
	else
		return 1;
}

/** Insert a producer into the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param producer the producer to insert
 * \param where the producer's playlist entry index
 * \param in the starting point on the producer
 * \param out the ending point on the producer
 * \return true if there was an error
 */

int mlt_playlist_insert( mlt_playlist self, mlt_producer producer, int where, mlt_position in, mlt_position out )
{
	// Append to end
	mlt_events_block( MLT_PLAYLIST_PROPERTIES( self ), self );
	mlt_playlist_append_io( self, producer, in, out );

	// Move to the position specified
	mlt_playlist_move( self, self->count - 1, where );
	mlt_events_unblock( MLT_PLAYLIST_PROPERTIES( self ), self );

	return mlt_playlist_virtual_refresh( self );
}

/** Remove an entry in the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param where the playlist entry index
 * \return true if there was an error
 */

int mlt_playlist_remove( mlt_playlist self, int where )
{
	int error = where < 0 || where >= self->count;
	if ( error == 0 && mlt_playlist_unmix( self, where ) != 0 )
	{
		// We need to know the current clip and the position within the playlist
		int current = mlt_playlist_current_clip( self );
		mlt_position position = mlt_producer_position( MLT_PLAYLIST_PRODUCER( self ) );

		// We need all the details about the clip we're removing
		mlt_playlist_clip_info where_info;
		playlist_entry *entry = self->list[ where ];
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( entry->producer );

		// Loop variable
		int i = 0;

		// Get the clip info
		mlt_playlist_get_clip_info( self, &where_info, where );

		// Make sure the clip to be removed is valid and correct if necessary
		if ( where < 0 )
			where = 0;
		if ( where >= self->count )
			where = self->count - 1;

		// Reorganise the list
		for ( i = where + 1; i < self->count; i ++ )
			self->list[ i - 1 ] = self->list[ i ];
		self->count --;

		if ( entry->preservation_hack == 0 )
		{
			// Decouple from mix_in/out if necessary
			if ( mlt_properties_get_data( properties, "mix_in", NULL ) != NULL )
			{
				mlt_properties mix = mlt_properties_get_data( properties, "mix_in", NULL );
				mlt_properties_set_data( mix, "mix_out", NULL, 0, NULL, NULL );
			}
			if ( mlt_properties_get_data( properties, "mix_out", NULL ) != NULL )
			{
				mlt_properties mix = mlt_properties_get_data( properties, "mix_out", NULL );
				mlt_properties_set_data( mix, "mix_in", NULL, 0, NULL, NULL );
			}

			if ( mlt_properties_ref_count( MLT_PRODUCER_PROPERTIES( entry->producer ) ) == 1 )
				mlt_producer_clear( entry->producer );
		}

		// Close the producer associated to the clip info
		mlt_event_close( entry->event );
		mlt_producer_close( entry->producer );

		// Correct position
		if ( where == current )
			mlt_producer_seek( MLT_PLAYLIST_PRODUCER( self ), where_info.start );
		else if ( where < current && self->count > 0 )
			mlt_producer_seek( MLT_PLAYLIST_PRODUCER( self ), position - where_info.frame_count );
		else if ( self->count == 0 )
			mlt_producer_seek( MLT_PLAYLIST_PRODUCER( self ), 0 );

		// Free the entry
		free( entry );

		// Refresh the playlist
		mlt_playlist_virtual_refresh( self );
	}

	return error;
}

/** Move an entry in the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param src an entry index
 * \param dest an entry index
 * \return false
 */

int mlt_playlist_move( mlt_playlist self, int src, int dest )
{
	int i;

	/* We need to ensure that the requested indexes are valid and correct it as necessary */
	if ( src < 0 )
		src = 0;
	if ( src >= self->count )
		src = self->count - 1;

	if ( dest < 0 )
		dest = 0;
	if ( dest >= self->count )
		dest = self->count - 1;

	if ( src != dest && self->count > 1 )
	{
		int current = mlt_playlist_current_clip( self );
		mlt_position position = mlt_producer_position( MLT_PLAYLIST_PRODUCER( self ) );
		playlist_entry *src_entry = NULL;

		// We need all the details about the current clip
		mlt_playlist_clip_info current_info;

		mlt_playlist_get_clip_info( self, &current_info, current );
		position -= current_info.start;

		if ( current == src )
			current = dest;
		else if ( current > src && current < dest )
			current ++;
		else if ( current == dest )
			current = src;

		src_entry = self->list[ src ];
		if ( src > dest )
		{
			for ( i = src; i > dest; i -- )
				self->list[ i ] = self->list[ i - 1 ];
		}
		else
		{
			for ( i = src; i < dest; i ++ )
				self->list[ i ] = self->list[ i + 1 ];
		}
		self->list[ dest ] = src_entry;

		mlt_playlist_get_clip_info( self, &current_info, current );
		mlt_producer_seek( MLT_PLAYLIST_PRODUCER( self ), current_info.start + position );
		mlt_playlist_virtual_refresh( self );
	}

	return 0;
}

/** Repeat the specified clip n times.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip a playlist entry index
 * \param repeat the number of times to repeat the clip
 * \return true if there was an error
 */

int mlt_playlist_repeat_clip( mlt_playlist self, int clip, int repeat )
{
	int error = repeat < 1 || clip < 0 || clip >= self->count;
	if ( error == 0 )
	{
		playlist_entry *entry = self->list[ clip ];
		entry->repeat = repeat;
		mlt_playlist_virtual_refresh( self );
	}
	return error;
}

/** Resize the specified clip.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \param in the new starting time on the clip's producer;  a negative value is the same as 0
 * \param out the new ending time on the clip's producer;  a negative value is the same as length - 1
 * \return true if there was an error
 */

int mlt_playlist_resize_clip( mlt_playlist self, int clip, mlt_position in, mlt_position out )
{
	int error = clip < 0 || clip >= self->count;
	if ( error == 0 && mlt_playlist_resize_mix( self, clip, in, out ) != 0 )
	{
		playlist_entry *entry = self->list[ clip ];
		mlt_producer producer = entry->producer;
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );

		mlt_events_block( properties, properties );

		if ( mlt_producer_is_blank( producer ) )
		{
			// Make sure the blank is long enough to accomodate the length specified
			if ( out - in + 1 > mlt_producer_get_length( &self->blank ) )
			{
				mlt_properties blank_props = MLT_PRODUCER_PROPERTIES( &self->blank );
				mlt_properties_set_int( blank_props, "length", out - in + 1 );
				mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( producer ), "length", out - in + 1 );
				mlt_producer_set_in_and_out( &self->blank, 0, out - in );
			}
		}

		if ( in < 0 )
			in = 0;
		if ( out < 0 || out >= mlt_producer_get_length( producer ) )
			out = mlt_producer_get_length( producer ) - 1;

		if ( out < in )
		{
			mlt_position t = in;
			in = out;
			out = t;
		}

		mlt_producer_set_in_and_out( producer, in, out );
		mlt_events_unblock( properties, properties );
		mlt_playlist_virtual_refresh( self );
	}
	return error;
}

/** Split a clip on the playlist at the given position.
 *
 * This splits after the specified frame.
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \param position the time at which to split relative to the beginning of the clip or its end if negative
 * \return true if there was an error
 */

int mlt_playlist_split( mlt_playlist self, int clip, mlt_position position )
{
	int error = clip < 0 || clip >= self->count;
	if ( error == 0 )
	{
		playlist_entry *entry = self->list[ clip ];
		position = position < 0 ? entry->frame_count + position - 1 : position;
		if ( position >= 0 && position < entry->frame_count - 1 )
		{
			int in = entry->frame_in;
			int out = entry->frame_out;
			mlt_events_block( MLT_PLAYLIST_PROPERTIES( self ), self );
			mlt_playlist_resize_clip( self, clip, in, in + position );
			if ( !mlt_producer_is_blank( entry->producer ) )
			{
				int i = 0;
				mlt_properties entry_properties = MLT_PRODUCER_PROPERTIES( entry->producer );
				mlt_producer split = mlt_producer_cut( entry->producer, in + position + 1, out );
				mlt_properties split_properties = MLT_PRODUCER_PROPERTIES( split );
				mlt_playlist_insert( self, split, clip + 1, 0, -1 );
				mlt_properties_lock( entry_properties );
				for ( i = 0; i < mlt_properties_count( entry_properties ); i ++ )
				{
					char *name = mlt_properties_get_name( entry_properties, i );
					if ( name != NULL && !strncmp( name, "meta.", 5 ) )
						mlt_properties_set( split_properties, name, mlt_properties_get_value( entry_properties, i ) );
				}
				mlt_properties_unlock( entry_properties );
				mlt_producer_close( split );
			}
			else
			{
				mlt_playlist_insert( self, &self->blank, clip + 1, 0, out - position - 1 );
			}
			mlt_events_unblock( MLT_PLAYLIST_PROPERTIES( self ), self );
			mlt_playlist_virtual_refresh( self );
		}
		else
		{
			error = 1;
		}
	}
	return error;
}

/** Split the playlist at the absolute position.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position the time at which to split relative to the beginning of the clip
 * \param left true to split before the frame starting at position
 * \return true if there was an error
 */

int mlt_playlist_split_at( mlt_playlist self, mlt_position position, int left )
{
	int result = self == NULL ? -1 : 0;
	if ( !result )
	{
		if ( position >= 0 && position < mlt_producer_get_playtime( MLT_PLAYLIST_PRODUCER( self ) ) )
		{
			int clip = mlt_playlist_get_clip_index_at( self, position );
			mlt_playlist_clip_info info;
			mlt_playlist_get_clip_info( self, &info, clip );
			if ( left && position != info.start )
				mlt_playlist_split( self, clip, position - info.start - 1 );
			else if ( !left )
				mlt_playlist_split( self, clip, position - info.start );
			result = position;
		}
		else if ( position <= 0 )
		{
			result = 0;
		}
		else
		{
			result = mlt_producer_get_playtime( MLT_PLAYLIST_PRODUCER( self ) );
		}
	}
	return result;
}

/** Join 1 or more consecutive clips.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the starting playlist entry index
 * \param count the number of entries to merge
 * \param merge ignored
 * \return true if there was an error
 */

int mlt_playlist_join( mlt_playlist self, int clip, int count, int merge )
{
	int error = clip < 0 || clip >= self->count;
	if ( error == 0 )
	{
		int i = clip;
		mlt_playlist new_clip = mlt_playlist_init( );
		mlt_events_block( MLT_PLAYLIST_PROPERTIES( self ), self );
		if ( clip + count >= self->count )
			count = self->count - clip - 1;
		for ( i = 0; i <= count; i ++ )
		{
			playlist_entry *entry = self->list[ clip ];
			mlt_playlist_append( new_clip, entry->producer );
			mlt_playlist_repeat_clip( new_clip, i, entry->repeat );
			entry->preservation_hack = 1;
			mlt_playlist_remove( self, clip );
		}
		mlt_events_unblock( MLT_PLAYLIST_PROPERTIES( self ), self );
		mlt_playlist_insert( self, MLT_PLAYLIST_PRODUCER( new_clip ), clip, 0, -1 );
		mlt_playlist_close( new_clip );
	}
	return error;
}

/** Mix consecutive clips for a specified length and apply transition if specified.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \param length the number of frames over which to create the mix
 * \param transition the transition to use for the mix
 * \return true if there was an error
 */

int mlt_playlist_mix( mlt_playlist self, int clip, int length, mlt_transition transition )
{
	int error = ( clip < 0 || clip + 1 >= self->count );
	if ( error == 0 )
	{
		playlist_entry *clip_a = self->list[ clip ];
		playlist_entry *clip_b = self->list[ clip + 1 ];
		mlt_producer track_a = NULL;
		mlt_producer track_b = NULL;
		mlt_tractor tractor = mlt_tractor_new( );
		mlt_events_block( MLT_PLAYLIST_PROPERTIES( self ), self );

		// Check length is valid for both clips and resize if necessary.
		int max_size = clip_a->frame_count > clip_b->frame_count ? clip_a->frame_count : clip_b->frame_count;
		length = length > max_size ? max_size : length;

		// Create the a and b tracks/cuts if necessary - note that no cuts are required if the length matches
		if ( length != clip_a->frame_count )
			track_a = mlt_producer_cut( clip_a->producer, clip_a->frame_out - length + 1, clip_a->frame_out );
		else
			track_a = clip_a->producer;

		if ( length != clip_b->frame_count )
			track_b = mlt_producer_cut( clip_b->producer, clip_b->frame_in, clip_b->frame_in + length - 1 );
		else
			track_b = clip_b->producer;

		// Set the tracks on the tractor
		mlt_tractor_set_track( tractor, track_a, 0 );
		mlt_tractor_set_track( tractor, track_b, 1 );

		// Insert the mix object into the playlist
		mlt_playlist_insert( self, MLT_TRACTOR_PRODUCER( tractor ), clip + 1, -1, -1 );
		mlt_properties_set_data( MLT_TRACTOR_PROPERTIES( tractor ), "mlt_mix", tractor, 0, NULL, NULL );

		// Attach the transition
		if ( transition != NULL )
		{
			mlt_field field = mlt_tractor_field( tractor );
			mlt_field_plant_transition( field, transition, 0, 1 );
			mlt_transition_set_in_and_out( transition, 0, length - 1 );
		}

		// Close our references to the tracks if we created new cuts above (the tracks can still be used here)
		if ( track_a != clip_a->producer )
			mlt_producer_close( track_a );
		if ( track_b != clip_b->producer )
			mlt_producer_close( track_b );

		// Check if we have anything left on the right hand clip
		if ( track_b == clip_b->producer )
		{
			clip_b->preservation_hack = 1;
			mlt_playlist_remove( self, clip + 2 );
		}
		else if ( clip_b->frame_out - clip_b->frame_in > length )
		{
			mlt_playlist_resize_clip( self, clip + 2, clip_b->frame_in + length, clip_b->frame_out );
			mlt_properties_set_data( MLT_PRODUCER_PROPERTIES( clip_b->producer ), "mix_in", tractor, 0, NULL, NULL );
			mlt_properties_set_data( MLT_TRACTOR_PROPERTIES( tractor ), "mix_out", clip_b->producer, 0, NULL, NULL );
		}
		else
		{
			mlt_producer_clear( clip_b->producer );
			mlt_playlist_remove( self, clip + 2 );
		}

		// Check if we have anything left on the left hand clip
		if ( track_a == clip_a->producer )
		{
			clip_a->preservation_hack = 1;
			mlt_playlist_remove( self, clip );
		}
		else if ( clip_a->frame_out - clip_a->frame_in > length )
		{
			mlt_playlist_resize_clip( self, clip, clip_a->frame_in, clip_a->frame_out - length );
			mlt_properties_set_data( MLT_PRODUCER_PROPERTIES( clip_a->producer ), "mix_out", tractor, 0, NULL, NULL );
			mlt_properties_set_data( MLT_TRACTOR_PROPERTIES( tractor ), "mix_in", clip_a->producer, 0, NULL, NULL );
		}
		else
		{
			mlt_producer_clear( clip_a->producer );
			mlt_playlist_remove( self, clip );
		}

		// Unblock and force a fire off of change events to listeners
		mlt_events_unblock( MLT_PLAYLIST_PROPERTIES( self ), self );
		mlt_playlist_virtual_refresh( self );
		mlt_tractor_close( tractor );
	}
	return error;
}

/** Add a transition to an existing mix.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \param transition a transition
 * \return true if there was an error
 */

int mlt_playlist_mix_add( mlt_playlist self, int clip, mlt_transition transition )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_playlist_get_clip( self, clip ) );
	mlt_properties properties = producer != NULL ? MLT_PRODUCER_PROPERTIES( producer ) : NULL;
	mlt_tractor tractor = properties != NULL ? mlt_properties_get_data( properties, "mlt_mix", NULL ) : NULL;
	int error = transition == NULL || tractor == NULL;
	if ( error == 0 )
	{
		mlt_field field = mlt_tractor_field( tractor );
		mlt_field_plant_transition( field, transition, 0, 1 );
		mlt_transition_set_in_and_out( transition, 0, self->list[ clip ]->frame_count - 1 );
	}
	return error;
}

/** Return the clip at the clip index.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of a playlist entry
 * \return a producer or NULL if there was an error
 */

mlt_producer mlt_playlist_get_clip( mlt_playlist self, int clip )
{
	if ( clip >= 0 && clip < self->count )
		return self->list[ clip ]->producer;
	return NULL;
}

/** Return the clip at the specified position.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position a time relative to the beginning of the playlist
 * \return a producer or NULL if not found
 */

mlt_producer mlt_playlist_get_clip_at( mlt_playlist self, mlt_position position )
{
	int index = 0, total = 0;
	return mlt_playlist_locate( self, &position, &index, &total );
}

/** Return the clip index of the specified position.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position a time relative to the beginning of the playlist
 * \return the index of the playlist entry
 */

int mlt_playlist_get_clip_index_at( mlt_playlist self, mlt_position position )
{
	int index = 0, total = 0;
	mlt_playlist_locate( self, &position, &index, &total );
	return index;
}

/** Determine if the clip is a mix.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \return true if the producer is a mix
 */

int mlt_playlist_clip_is_mix( mlt_playlist self, int clip )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_playlist_get_clip( self, clip ) );
	mlt_properties properties = producer != NULL ? MLT_PRODUCER_PROPERTIES( producer ) : NULL;
	mlt_tractor tractor = properties != NULL ? mlt_properties_get_data( properties, "mlt_mix", NULL ) : NULL;
	return tractor != NULL;
}

/** Remove a mixed clip - ensure that the cuts included in the mix find their way
 * back correctly on to the playlist.
 *
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \return true if there was an error
 */

static int mlt_playlist_unmix( mlt_playlist self, int clip )
{
	int error = ( clip < 0 || clip >= self->count );

	// Ensure that the clip request is actually a mix
	if ( error == 0 )
	{
		mlt_producer producer = mlt_producer_cut_parent( self->list[ clip ]->producer );
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		error = mlt_properties_get_data( properties, "mlt_mix", NULL ) == NULL ||
			    self->list[ clip ]->preservation_hack;
	}

	if ( error == 0 )
	{
		playlist_entry *mix = self->list[ clip ];
		mlt_tractor tractor = ( mlt_tractor )mlt_producer_cut_parent( mix->producer );
		mlt_properties properties = MLT_TRACTOR_PROPERTIES( tractor );
		mlt_producer clip_a = mlt_properties_get_data( properties, "mix_in", NULL );
		mlt_producer clip_b = mlt_properties_get_data( properties, "mix_out", NULL );
		int length = mlt_producer_get_playtime( MLT_TRACTOR_PRODUCER( tractor ) );
		mlt_events_block( MLT_PLAYLIST_PROPERTIES( self ), self );

		if ( clip_a != NULL )
		{
			mlt_producer_set_in_and_out( clip_a, mlt_producer_get_in( clip_a ), mlt_producer_get_out( clip_a ) + length );
		}
		else
		{
			mlt_producer cut = mlt_tractor_get_track( tractor, 0 );
			mlt_playlist_insert( self, cut, clip, -1, -1 );
			clip ++;
		}

		if ( clip_b != NULL )
		{
			mlt_producer_set_in_and_out( clip_b, mlt_producer_get_in( clip_b ) - length, mlt_producer_get_out( clip_b ) );
		}
		else
		{
			mlt_producer cut = mlt_tractor_get_track( tractor, 1 );
			mlt_playlist_insert( self, cut, clip + 1, -1, -1 );
		}

		mlt_properties_set_data( properties, "mlt_mix", NULL, 0, NULL, NULL );
		mlt_playlist_remove( self, clip );
		mlt_events_unblock( MLT_PLAYLIST_PROPERTIES( self ), self );
		mlt_playlist_virtual_refresh( self );
	}
	return error;
}

/** Resize a mix clip.
 *
 * \private \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \param in the new starting point
 * \param out the new ending point
 * \return true if there was an error
 */

static int mlt_playlist_resize_mix( mlt_playlist self, int clip, int in, int out )
{
	int error = ( clip < 0 || clip >= self->count );

	// Ensure that the clip request is actually a mix
	if ( error == 0 )
	{
		mlt_producer producer = mlt_producer_cut_parent( self->list[ clip ]->producer );
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		error = mlt_properties_get_data( properties, "mlt_mix", NULL ) == NULL;
	}

	if ( error == 0 )
	{
		playlist_entry *mix = self->list[ clip ];
		mlt_tractor tractor = ( mlt_tractor )mlt_producer_cut_parent( mix->producer );
		mlt_properties properties = MLT_TRACTOR_PROPERTIES( tractor );
		mlt_producer clip_a = mlt_properties_get_data( properties, "mix_in", NULL );
		mlt_producer clip_b = mlt_properties_get_data( properties, "mix_out", NULL );
		mlt_producer track_a = mlt_tractor_get_track( tractor, 0 );
		mlt_producer track_b = mlt_tractor_get_track( tractor, 1 );
		int length = out - in + 1;
		int length_diff = length - mlt_producer_get_playtime( MLT_TRACTOR_PRODUCER( tractor ) );
		mlt_events_block( MLT_PLAYLIST_PROPERTIES( self ), self );

		if ( clip_a != NULL )
			mlt_producer_set_in_and_out( clip_a, mlt_producer_get_in( clip_a ), mlt_producer_get_out( clip_a ) - length_diff );

		if ( clip_b != NULL )
			mlt_producer_set_in_and_out( clip_b, mlt_producer_get_in( clip_b ) + length_diff, mlt_producer_get_out( clip_b ) );

		mlt_producer_set_in_and_out( track_a, mlt_producer_get_in( track_a ) - length_diff, mlt_producer_get_out( track_a ) );
		mlt_producer_set_in_and_out( track_b, mlt_producer_get_in( track_b ), mlt_producer_get_out( track_b ) + length_diff );
		mlt_producer_set_in_and_out( MLT_MULTITRACK_PRODUCER( mlt_tractor_multitrack( tractor ) ), in, out );
		mlt_producer_set_in_and_out( MLT_TRACTOR_PRODUCER( tractor ), in, out );
		mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( mix->producer ), "length", out - in + 1 );
		mlt_producer_set_in_and_out( mix->producer, in, out );

		mlt_events_unblock( MLT_PLAYLIST_PROPERTIES( self ), self );
		mlt_playlist_virtual_refresh( self );
	}
	return error;
}

/** Consolidate adjacent blank producers.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param keep_length set false to remove the last entry if it is blank
 */

void mlt_playlist_consolidate_blanks( mlt_playlist self, int keep_length )
{
	if ( self != NULL )
	{
		int i = 0;
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );

		mlt_events_block( properties, properties );
		for ( i = 1; i < self->count; i ++ )
		{
			playlist_entry *left = self->list[ i - 1 ];
			playlist_entry *right = self->list[ i ];

			if ( mlt_producer_is_blank( left->producer ) && mlt_producer_is_blank( right->producer ) )
			{
				mlt_playlist_resize_clip( self, i - 1, 0, left->frame_count + right->frame_count - 1 );
				mlt_playlist_remove( self, i -- );
			}
		}

		if ( !keep_length && self->count > 0 )
		{
			playlist_entry *last = self->list[ self->count - 1 ];
			if ( mlt_producer_is_blank( last->producer ) )
				mlt_playlist_remove( self, self->count - 1 );
		}

		mlt_events_unblock( properties, properties );
		mlt_playlist_virtual_refresh( self );
	}
}

/** Determine if the specified clip index is a blank.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \return true if there was an error
 */

int mlt_playlist_is_blank( mlt_playlist self, int clip )
{
	return self == NULL || mlt_producer_is_blank( mlt_playlist_get_clip( self, clip ) );
}

/** Determine if the specified position is a blank.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position a time relative to the start or end (negative) of the playlist
 * \return true if there was an error
 */

int mlt_playlist_is_blank_at( mlt_playlist self, mlt_position position )
{
	return self == NULL || mlt_producer_is_blank( mlt_playlist_get_clip_at( self, position ) );
}

/** Replace the specified clip with a blank and return the clip.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \return a producer or NULL if there was an error
 */

mlt_producer mlt_playlist_replace_with_blank( mlt_playlist self, int clip )
{
	mlt_producer producer = NULL;
	if ( !mlt_playlist_is_blank( self, clip ) )
	{
		playlist_entry *entry = self->list[ clip ];
		int in = entry->frame_in;
		int out = entry->frame_out;
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );
		producer = entry->producer;
		mlt_properties_inc_ref( MLT_PRODUCER_PROPERTIES( producer ) );
		mlt_events_block( properties, properties );
		mlt_playlist_remove( self, clip );
		mlt_playlist_blank( self, out - in );
		mlt_playlist_move( self, self->count - 1, clip );
		mlt_events_unblock( properties, properties );
		mlt_playlist_virtual_refresh( self );
		mlt_producer_set_in_and_out( producer, in, out );
	}
	return producer;
}

/** Insert blank space.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the new blank section
 * \param length the ending time of the new blank section (duration - 1)
 */

void mlt_playlist_insert_blank( mlt_playlist self, int clip, int length )
{
	if ( self != NULL && length >= 0 )
	{
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );
		mlt_events_block( properties, properties );
		mlt_playlist_blank( self, length );
		mlt_playlist_move( self, self->count - 1, clip );
		mlt_events_unblock( properties, properties );
		mlt_playlist_virtual_refresh( self );
	}
}

/** Resize a blank entry.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position the time at which the blank entry exists relative to the start or end (negative) of the playlist.
 * \param length the additional amount of blank frames to add
 * \param find true to fist locate the blank after the clip at position
 */
void mlt_playlist_pad_blanks( mlt_playlist self, mlt_position position, int length, int find )
{
	if ( self != NULL && length != 0 )
	{
		int clip = mlt_playlist_get_clip_index_at( self, position );
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );
		mlt_events_block( properties, properties );
		if ( find && clip < self->count && !mlt_playlist_is_blank( self, clip ) )
			clip ++;
		if ( clip < self->count && mlt_playlist_is_blank( self, clip ) )
		{
			mlt_playlist_clip_info info;
			mlt_playlist_get_clip_info( self, &info, clip );
			if ( info.frame_out + length > info.frame_in )
				mlt_playlist_resize_clip( self, clip, info.frame_in, info.frame_out + length );
			else
				mlt_playlist_remove( self, clip );
		}
		else if ( find && clip < self->count && length > 0  )
		{
			mlt_playlist_insert_blank( self, clip, length );
		}
		mlt_events_unblock( properties, properties );
		mlt_playlist_virtual_refresh( self );
	}
}

/** Insert a clip at a specific time.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position the time at which to insert
 * \param producer the producer to insert
 * \param mode true if you want to overwrite any blank section
 * \return true if there was an error
 */

int mlt_playlist_insert_at( mlt_playlist self, mlt_position position, mlt_producer producer, int mode )
{
	int ret = self == NULL || position < 0 || producer == NULL;
	if ( ret == 0 )
	{
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );
		int length = mlt_producer_get_playtime( producer );
		int clip = mlt_playlist_get_clip_index_at( self, position );
		mlt_playlist_clip_info info;
		mlt_playlist_get_clip_info( self, &info, clip );
		mlt_events_block( properties, self );
		if ( clip < self->count && mlt_playlist_is_blank( self, clip ) )
		{
			// Split and move to new clip if need be
			if ( position != info.start && mlt_playlist_split( self, clip, position - info.start - 1 ) == 0 )
				mlt_playlist_get_clip_info( self, &info, ++ clip );

			// Split again if need be
			if ( length < info.frame_count )
				mlt_playlist_split( self, clip, length - 1 );

			// Remove
			mlt_playlist_remove( self, clip );

			// Insert
			mlt_playlist_insert( self, producer, clip, -1, -1 );
			ret = clip;
		}
		else if ( clip < self->count )
		{
			if ( position > info.start + info.frame_count / 2 )
				clip ++;
			if ( mode == 1 && clip < self->count && mlt_playlist_is_blank( self, clip ) )
			{
				mlt_playlist_get_clip_info( self, &info, clip );
				if ( length < info.frame_count )
					mlt_playlist_split( self, clip, length );
				mlt_playlist_remove( self, clip );
			}
			mlt_playlist_insert( self, producer, clip, -1, -1 );
			ret = clip;
		}
		else
		{
			if ( mode == 1 ) {
				if ( position == info.start )
					mlt_playlist_remove( self, clip );
				else
					mlt_playlist_blank( self, position - mlt_properties_get_int( properties, "length" ) - 1 );
			}
			mlt_playlist_append( self, producer );
			ret = self->count - 1;
		}
		mlt_events_unblock( properties, self );
		mlt_playlist_virtual_refresh( self );
	}
	else
	{
		ret = -1;
	}
	return ret;
}

/** Get the time at which the clip starts relative to the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \return the starting time
 */

int mlt_playlist_clip_start( mlt_playlist self, int clip )
{
	mlt_playlist_clip_info info;
	if ( mlt_playlist_get_clip_info( self, &info, clip ) == 0 )
		return info.start;
	return clip < 0 ? 0 : mlt_producer_get_playtime( MLT_PLAYLIST_PRODUCER( self ) );
}

/** Get the playable duration of the clip.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \return the duration of the playlist entry
 */

int mlt_playlist_clip_length( mlt_playlist self, int clip )
{
	mlt_playlist_clip_info info;
	if ( mlt_playlist_get_clip_info( self, &info, clip ) == 0 )
		return info.frame_count;
	return 0;
}

/** Get the duration of a blank space.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param clip the index of the playlist entry
 * \param bounded the maximum number of blank entries or 0 for all
 * \return the duration of a blank section
 */

int mlt_playlist_blanks_from( mlt_playlist self, int clip, int bounded )
{
	int count = 0;
	mlt_playlist_clip_info info;
	if ( self != NULL && clip < self->count )
	{
		mlt_playlist_get_clip_info( self, &info, clip );
		if ( mlt_playlist_is_blank( self, clip ) )
			count += info.frame_count;
		if ( bounded == 0 )
			bounded = self->count;
		for ( clip ++; clip < self->count && bounded >= 0; clip ++ )
		{
			mlt_playlist_get_clip_info( self, &info, clip );
			if ( mlt_playlist_is_blank( self, clip ) )
				count += info.frame_count;
			else
				bounded --;
		}
	}
	return count;
}

/** Remove a portion of the playlist by time.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 * \param position the starting time
 * \param length the duration of time to remove
 * \return the new entry index at the position
 */

int mlt_playlist_remove_region( mlt_playlist self, mlt_position position, int length )
{
	int index = mlt_playlist_get_clip_index_at( self, position );
	if ( index >= 0 && index < self->count )
	{
		mlt_properties properties = MLT_PLAYLIST_PROPERTIES( self );
		int clip_start = mlt_playlist_clip_start( self, index );
		int list_length = mlt_producer_get_playtime( MLT_PLAYLIST_PRODUCER( self ) );
		mlt_events_block( properties, self );

		if ( position + length > list_length )
			length -= ( position + length - list_length );

		if ( clip_start < position )
		{
			mlt_playlist_split( self, index ++, position - clip_start - 1 );
		}

		while( length > 0 )
		{
			if ( mlt_playlist_clip_length( self, index ) > length )
				mlt_playlist_split( self, index, length - 1 );
			length -= mlt_playlist_clip_length( self, index );
			mlt_playlist_remove( self, index );
		}

		mlt_playlist_consolidate_blanks( self, 0 );
		mlt_events_unblock( properties, self );
		mlt_playlist_virtual_refresh( self );

		// Just to be sure, we'll get the clip index again...
		index = mlt_playlist_get_clip_index_at( self, position );
	}
	return index;
}

/** Not implemented
 *
 * \deprecated not implemented
 * \public \memberof mlt_playlist_s
 * \param self
 * \param position
 * \param length
 * \param new_position
 * \return
 */

int mlt_playlist_move_region( mlt_playlist self, mlt_position position, int length, int new_position )
{
	if ( self != NULL )
	{
	}
	return 0;
}

/** Get the current frame.
 *
 * The implementation of the get_frame virtual function.
 * \private \memberof mlt_playlist_s
 * \param producer a producer
 * \param frame a frame by reference
 * \param index the time at which to get the frame
 * \return false
 */

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Check that we have a producer
	if ( producer == NULL )
	{
		*frame = NULL;
		return -1;
	}

	// Get this mlt_playlist
	mlt_playlist self = producer->child;

	// Need to ensure the frame is deinterlaced when repeating 1 frame
	int progressive = 0;

	// Get the real producer
	mlt_service real = mlt_playlist_virtual_seek( self, &progressive );

	// Check that we have a producer
	if ( real == NULL )
	{
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );
		return 0;
	}

	// Get the frame
	if ( !mlt_properties_get_int( MLT_SERVICE_PROPERTIES( real ), "meta.fx_cut" ) )
	{
		mlt_service_get_frame( real, frame, index );
	}
	else
	{
		mlt_producer parent = mlt_producer_cut_parent( ( mlt_producer )real );
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( parent ) );
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "fx_cut", 1 );
		mlt_frame_push_service( *frame, NULL );
		mlt_frame_push_audio( *frame, NULL );
		mlt_service_apply_filters( MLT_PRODUCER_SERVICE( parent ), *frame, 0 );
		mlt_service_apply_filters( real, *frame, 0 );
		mlt_deque_pop_front( MLT_FRAME_IMAGE_STACK( *frame ) );
		mlt_deque_pop_front( MLT_FRAME_AUDIO_STACK( *frame ) );
	}

	// Check if we're at the end of the clip
	mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );
	if ( mlt_properties_get_int( properties, "end_of_clip" ) )
		mlt_playlist_virtual_set_out( self );

	// Set the consumer progressive property
	if ( progressive )
	{
		mlt_properties_set_int( properties, "consumer_deinterlace", progressive );
		mlt_properties_set_int( properties, "test_audio", 1 );
	}

	// Check for notifier and call with appropriate argument
	mlt_properties playlist_properties = MLT_PRODUCER_PROPERTIES( producer );
	void ( *notifier )( void * ) = mlt_properties_get_data( playlist_properties, "notifier", NULL );
	if ( notifier != NULL )
	{
		void *argument = mlt_properties_get_data( playlist_properties, "notifier_arg", NULL );
		notifier( argument );
	}

	// Update position on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_frame( producer ) );

	// Position ourselves on the next frame
	mlt_producer_prepare_next( producer );

	return 0;
}

/** Close the playlist.
 *
 * \public \memberof mlt_playlist_s
 * \param self a playlist
 */

void mlt_playlist_close( mlt_playlist self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_PLAYLIST_PROPERTIES( self ) ) <= 0 )
	{
		int i = 0;
		self->parent.close = NULL;
		for ( i = 0; i < self->count; i ++ )
		{
			mlt_event_close( self->list[ i ]->event );
			mlt_producer_close( self->list[ i ]->producer );
			free( self->list[ i ] );
		}
		mlt_producer_close( &self->blank );
		mlt_producer_close( &self->parent );
		free( self->list );
		free( self );
	}
}
