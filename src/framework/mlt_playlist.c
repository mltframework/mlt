/*
 * mlt_playlist.c -- playlist service class
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

#include "mlt_playlist.h"
#include "mlt_tractor.h"
#include "mlt_multitrack.h"
#include "mlt_field.h"
#include "mlt_frame.h"
#include "mlt_transition.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Virtual playlist entry.
*/

typedef struct playlist_entry_s
{
	mlt_producer producer;
	mlt_position frame_in;
	mlt_position frame_out;
	mlt_position frame_count;
	int repeat;
	mlt_position producer_length;
	mlt_event event;
}
playlist_entry;

/** Private definition.
*/

struct mlt_playlist_s
{
	struct mlt_producer_s parent;
	struct mlt_producer_s blank;

	int size;
	int count;
	playlist_entry **list;
};

/** Forward declarations
*/

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static int mlt_playlist_unmix( mlt_playlist this, int clip );
static int mlt_playlist_resize_mix( mlt_playlist this, int clip, int in, int out );

/** Constructor.
*/

mlt_playlist mlt_playlist_init( )
{
	mlt_playlist this = calloc( sizeof( struct mlt_playlist_s ), 1 );
	if ( this != NULL )
	{
		mlt_producer producer = &this->parent;

		// Construct the producer
		mlt_producer_init( producer, this );

		// Override the producer get_frame
		producer->get_frame = producer_get_frame;

		// Define the destructor
		producer->close = ( mlt_destructor )mlt_playlist_close;
		producer->close_object = this;

		// Initialise blank
		mlt_producer_init( &this->blank, NULL );
		mlt_properties_set( mlt_producer_properties( &this->blank ), "mlt_service", "blank" );
		mlt_properties_set( mlt_producer_properties( &this->blank ), "resource", "blank" );

		// Indicate that this producer is a playlist
		mlt_properties_set_data( mlt_playlist_properties( this ), "playlist", this, 0, NULL, NULL );

		// Specify the eof condition
		mlt_properties_set( mlt_playlist_properties( this ), "eof", "pause" );
		mlt_properties_set( mlt_playlist_properties( this ), "resource", "<playlist>" );
		mlt_properties_set( mlt_playlist_properties( this ), "mlt_type", "mlt_producer" );
		mlt_properties_set_position( mlt_playlist_properties( this ), "in", 0 );
		mlt_properties_set_position( mlt_playlist_properties( this ), "out", 0 );
		mlt_properties_set_position( mlt_playlist_properties( this ), "length", 0 );

		this->size = 10;
		this->list = malloc( this->size * sizeof( playlist_entry * ) );
	}
	
	return this;
}

/** Get the producer associated to this playlist.
*/

mlt_producer mlt_playlist_producer( mlt_playlist this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Get the service associated to this playlist.
*/

mlt_service mlt_playlist_service( mlt_playlist this )
{
	return mlt_producer_service( &this->parent );
}

/** Get the propertues associated to this playlist.
*/

mlt_properties mlt_playlist_properties( mlt_playlist this )
{
	return mlt_producer_properties( &this->parent );
}

/** Refresh the playlist after a clip has been changed.
*/

static int mlt_playlist_virtual_refresh( mlt_playlist this )
{
	// Obtain the properties
	mlt_properties properties = mlt_playlist_properties( this );

	// Get the fps of the first producer
	double fps = mlt_properties_get_double( properties, "first_fps" );
	int i = 0;
	mlt_position frame_count = 0;

	for ( i = 0; i < this->count; i ++ )
	{
		// Get the producer
		mlt_producer producer = this->list[ i ]->producer;
		int current_length = mlt_producer_get_out( producer ) - mlt_producer_get_in( producer ) + 1;

		// If fps is 0
		if ( fps == 0 )
		{
			// Inherit it from the producer
			fps = mlt_producer_get_fps( producer );
		}
		else if ( fps != mlt_properties_get_double( mlt_producer_properties( producer ), "fps" ) )
		{
			// Generate a warning for now - the following attempt to fix may fail
			fprintf( stderr, "Warning: fps mismatch on playlist producer %d\n", this->count );

			// It should be safe to impose fps on an image producer, but not necessarily safe for video
			mlt_properties_set_double( mlt_producer_properties( producer ), "fps", fps );
		}

		// Check if the length of the producer has changed
		if ( this->list[ i ]->producer != &this->blank &&
			 ( this->list[ i ]->frame_in != mlt_producer_get_in( producer ) ||
			   this->list[ i ]->frame_out != mlt_producer_get_out( producer ) ) )
		{
			// This clip should be removed...
			if ( current_length < 1 )
			{
				this->list[ i ]->frame_in = 0;
				this->list[ i ]->frame_out = -1;
				this->list[ i ]->frame_count = 0;
			}
			else 
			{
				this->list[ i ]->frame_in = mlt_producer_get_in( producer );
				this->list[ i ]->frame_out = mlt_producer_get_out( producer );
				this->list[ i ]->frame_count = current_length;
			}

			// Update the producer_length
			this->list[ i ]->producer_length = current_length;
		}

		// Calculate the frame_count
		this->list[ i ]->frame_count = ( this->list[ i ]->frame_out - this->list[ i ]->frame_in + 1 ) * this->list[ i ]->repeat;

		// Update the frame_count for this clip
		frame_count += this->list[ i ]->frame_count;
	}

	// Refresh all properties
	mlt_properties_set_double( properties, "first_fps", fps );
	mlt_properties_set_double( properties, "fps", fps == 0 ? 25 : fps );
	mlt_events_block( properties, properties );
	mlt_properties_set_position( properties, "length", frame_count );
	mlt_events_unblock( properties, properties );
	mlt_properties_set_position( properties, "out", frame_count - 1 );

	return 0;
}

/** Listener for producers on the playlist.
*/

static void mlt_playlist_listener( mlt_producer producer, mlt_playlist this )
{
	mlt_playlist_virtual_refresh( this );
}

/** Append to the virtual playlist.
*/

static int mlt_playlist_virtual_append( mlt_playlist this, mlt_producer source, mlt_position in, mlt_position out )
{
	char *resource = source != NULL ? mlt_properties_get( mlt_producer_properties( source ), "resource" ) : NULL;
	mlt_producer producer = NULL;
	mlt_properties properties = NULL;
	mlt_properties parent = NULL;

	// If we have a cut, then use the in/out points from the cut
	if ( resource == NULL || !strcmp( resource, "blank" ) )
	{
		producer = &this->blank;
		properties = mlt_producer_properties( producer );
		mlt_properties_inc_ref( properties );
	}
	else if ( mlt_producer_is_cut( source ) )
	{
		producer = source;
		if ( in == -1 )
			in = mlt_producer_get_in( producer );
		if ( out == -1 || out > mlt_producer_get_out( producer ) )
			out = mlt_producer_get_out( producer );
		properties = mlt_producer_properties( producer );
		mlt_properties_inc_ref( properties );
	}
	else
	{
		producer = mlt_producer_cut( source, in, out );
		if ( in == -1 || in < mlt_producer_get_in( producer ) )
			in = mlt_producer_get_in( producer );
		if ( out == -1 || out > mlt_producer_get_out( producer ) )
			out = mlt_producer_get_out( producer );
		properties = mlt_producer_properties( producer );
	}

	// Fetch the cuts parent properties
	parent = mlt_producer_properties( mlt_producer_cut_parent( producer ) );

	// Check that we have room
	if ( this->count >= this->size )
	{
		int i;
		this->list = realloc( this->list, ( this->size + 10 ) * sizeof( playlist_entry * ) );
		for ( i = this->size; i < this->size + 10; i ++ ) this->list[ i ] = NULL;
		this->size += 10;
	}

	// Create the entry
	this->list[ this->count ] = calloc( sizeof( playlist_entry ), 1 );
	if ( this->list[ this->count ] != NULL )
	{
		this->list[ this->count ]->producer = producer;
		this->list[ this->count ]->frame_in = in;
		this->list[ this->count ]->frame_out = out;
		this->list[ this->count ]->frame_count = out - in + 1;
		this->list[ this->count ]->repeat = 1;
		this->list[ this->count ]->producer_length = mlt_producer_get_out( producer ) - mlt_producer_get_in( producer ) + 1;
		this->list[ this->count ]->event = mlt_events_listen( parent, this, "producer-changed", ( mlt_listener )mlt_playlist_listener );
		mlt_event_inc_ref( this->list[ this->count ]->event );
		mlt_properties_set( properties, "eof", "pause" );
		mlt_producer_set_speed( producer, 0 );
		this->count ++;
	}

	return mlt_playlist_virtual_refresh( this );
}

/** Seek in the virtual playlist.
*/

static mlt_service mlt_playlist_virtual_seek( mlt_playlist this )
{
	// Default producer to blank
	mlt_producer producer = NULL;

	// Map playlist position to real producer in virtual playlist
	mlt_position position = mlt_producer_frame( &this->parent );

	mlt_position original = position;

	// Total number of frames
	int64_t total = 0;

	// Get the properties
	mlt_properties properties = mlt_playlist_properties( this );

	// Get the eof handling
	char *eof = mlt_properties_get( properties, "eof" );

	// Index for the main loop
	int i = 0;

	// Loop for each producer until found
	for ( i = 0; i < this->count; i ++ )
	{
		// Increment the total
		total += this->list[ i ]->frame_count;

		// Check if the position indicates that we have found the clip
		// Note that 0 length clips get skipped automatically
		if ( position < this->list[ i ]->frame_count )
		{
			// Found it, now break
			producer = this->list[ i ]->producer;
			break;
		}
		else
		{
			// Decrement position by length of this entry
			position -= this->list[ i ]->frame_count;
		}
	}

	// Seek in real producer to relative position
	if ( producer != NULL )
	{
		int count = this->list[ i ]->frame_count / this->list[ i ]->repeat;
		mlt_producer_seek( producer, position % count );
	}
	else if ( !strcmp( eof, "pause" ) && total > 0 )
	{
		playlist_entry *entry = this->list[ this->count - 1 ];
		mlt_producer this_producer = mlt_playlist_producer( this );
		mlt_producer_seek( this_producer, original - 1 );
		producer = entry->producer;
		mlt_producer_seek( producer, entry->frame_out );
		mlt_producer_set_speed( this_producer, 0 );
		mlt_producer_set_speed( producer, 0 );
	}
	else if ( !strcmp( eof, "loop" ) && total > 0 )
	{
		playlist_entry *entry = this->list[ 0 ];
		mlt_producer this_producer = mlt_playlist_producer( this );
		mlt_producer_seek( this_producer, 0 );
		producer = entry->producer;
		mlt_producer_seek( producer, 0 );
	}
	else
	{
		producer = &this->blank;
	}

	return mlt_producer_service( producer );
}

/** Invoked when a producer indicates that it has prematurely reached its end.
*/

static mlt_producer mlt_playlist_virtual_set_out( mlt_playlist this )
{
	// Default producer to blank
	mlt_producer producer = &this->blank;

	// Map playlist position to real producer in virtual playlist
	mlt_position position = mlt_producer_frame( &this->parent );

	// Loop through the virtual playlist
	int i = 0;

	for ( i = 0; i < this->count; i ++ )
	{
		if ( position < this->list[ i ]->frame_count )
		{
			// Found it, now break
			producer = this->list[ i ]->producer;
			break;
		}
		else
		{
			// Decrement position by length of this entry
			position -= this->list[ i ]->frame_count;
		}
	}

	// Seek in real producer to relative position
	if ( i < this->count && this->list[ i ]->frame_out != position )
	{
		// Update the frame_count for the changed clip (hmmm)
		this->list[ i ]->frame_out = position;
		this->list[ i ]->frame_count = this->list[ i ]->frame_out - this->list[ i ]->frame_in + 1;

		// Refresh the playlist
		mlt_playlist_virtual_refresh( this );
	}

	return producer;
}

/** Obtain the current clips index.
*/

int mlt_playlist_current_clip( mlt_playlist this )
{
	// Map playlist position to real producer in virtual playlist
	mlt_position position = mlt_producer_frame( &this->parent );

	// Loop through the virtual playlist
	int i = 0;

	for ( i = 0; i < this->count; i ++ )
	{
		if ( position < this->list[ i ]->frame_count )
		{
			// Found it, now break
			break;
		}
		else
		{
			// Decrement position by length of this entry
			position -= this->list[ i ]->frame_count;
		}
	}

	return i;
}

/** Obtain the current clips producer.
*/

mlt_producer mlt_playlist_current( mlt_playlist this )
{
	int i = mlt_playlist_current_clip( this );
	if ( i < this->count )
		return this->list[ i ]->producer;
	else
		return &this->blank;
}

/** Get the position which corresponds to the start of the next clip.
*/

mlt_position mlt_playlist_clip( mlt_playlist this, mlt_whence whence, int index )
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
			absolute_clip = mlt_playlist_current_clip( this ) + index;
			break;

		case mlt_whence_relative_end:
			absolute_clip = this->count - index;
			break;
	}

	// Check that we're in a valid range
	if ( absolute_clip < 0 )
		absolute_clip = 0;
	else if ( absolute_clip > this->count )
		absolute_clip = this->count;

	// Now determine the position
	for ( i = 0; i < absolute_clip; i ++ )
		position += this->list[ i ]->frame_count;

	return position;
}

/** Get all the info about the clip specified.
*/

int mlt_playlist_get_clip_info( mlt_playlist this, mlt_playlist_clip_info *info, int index )
{
	int error = index < 0 || index >= this->count;
	memset( info, 0, sizeof( mlt_playlist_clip_info ) );
	if ( !error )
	{
		mlt_producer producer = mlt_producer_cut_parent( this->list[ index ]->producer );
		mlt_properties properties = mlt_producer_properties( producer );
		info->clip = index;
		info->producer = producer;
		info->cut = this->list[ index ]->producer;
		info->start = mlt_playlist_clip( this, mlt_whence_relative_start, index );
		info->resource = mlt_properties_get( properties, "resource" );
		info->frame_in = this->list[ index ]->frame_in;
		info->frame_out = this->list[ index ]->frame_out;
		info->frame_count = this->list[ index ]->frame_count;
		info->repeat = this->list[ index ]->repeat;
		info->length = mlt_producer_get_length( producer );
		info->fps = mlt_producer_get_fps( producer );
	}

	return error;
}

/** Get number of clips in the playlist.
*/

int mlt_playlist_count( mlt_playlist this )
{
	return this->count;
}

/** Clear the playlist.
*/

int mlt_playlist_clear( mlt_playlist this )
{
	int i;
	for ( i = 0; i < this->count; i ++ )
	{
		mlt_event_close( this->list[ i ]->event );
		mlt_producer_close( this->list[ i ]->producer );
	}
	this->count = 0;
	mlt_properties_set_double( mlt_playlist_properties( this ), "first_fps", 0 );
	return mlt_playlist_virtual_refresh( this );
}

/** Append a producer to the playlist.
*/

int mlt_playlist_append( mlt_playlist this, mlt_producer producer )
{
	// Append to virtual list
	return mlt_playlist_virtual_append( this, producer, 0, mlt_producer_get_playtime( producer ) - 1 );
}

/** Append a producer to the playlist with in/out points.
*/

int mlt_playlist_append_io( mlt_playlist this, mlt_producer producer, mlt_position in, mlt_position out )
{
	// Append to virtual list
	if ( in != -1 && out != -1 )
		return mlt_playlist_virtual_append( this, producer, in, out );
	else
		return mlt_playlist_append( this, producer );
}

/** Append a blank to the playlist of a given length.
*/

int mlt_playlist_blank( mlt_playlist this, mlt_position length )
{
	// Append to the virtual list
	return mlt_playlist_virtual_append( this, &this->blank, 0, length );
}

/** Insert a producer into the playlist.
*/

int mlt_playlist_insert( mlt_playlist this, mlt_producer producer, int where, mlt_position in, mlt_position out )
{
	// Append to end
	mlt_events_block( mlt_playlist_properties( this ), this );
	mlt_playlist_append_io( this, producer, in, out );

	// Move to the position specified
	mlt_playlist_move( this, this->count - 1, where );
	mlt_events_unblock( mlt_playlist_properties( this ), this );

	return mlt_playlist_virtual_refresh( this );
}

/** Remove an entry in the playlist.
*/

int mlt_playlist_remove( mlt_playlist this, int where )
{
	int error = where < 0 || where >= this->count;
	if ( error == 0 && mlt_playlist_unmix( this, where ) != 0 )
	{
		// We need to know the current clip and the position within the playlist
		int current = mlt_playlist_current_clip( this );
		mlt_position position = mlt_producer_position( mlt_playlist_producer( this ) );

		// We need all the details about the clip we're removing
		mlt_playlist_clip_info where_info;
		playlist_entry *entry = this->list[ where ];
		mlt_properties properties = mlt_producer_properties( entry->producer );

		// Loop variable
		int i = 0;

		// Get the clip info 
		mlt_playlist_get_clip_info( this, &where_info, where );

		// Make sure the clip to be removed is valid and correct if necessary
		if ( where < 0 ) 
			where = 0;
		if ( where >= this->count )
			where = this->count - 1;

		// Reorganise the list
		for ( i = where + 1; i < this->count; i ++ )
			this->list[ i - 1 ] = this->list[ i ];
		this->count --;

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

		// Close the producer associated to the clip info
		mlt_event_close( entry->event );
		mlt_producer_clear( entry->producer );
		mlt_producer_close( entry->producer );

		// Correct position
		if ( where == current )
			mlt_producer_seek( mlt_playlist_producer( this ), where_info.start );
		else if ( where < current && this->count > 0 )
			mlt_producer_seek( mlt_playlist_producer( this ), position - where_info.frame_count );
		else if ( this->count == 0 )
			mlt_producer_seek( mlt_playlist_producer( this ), 0 );

		// Free the entry
		free( entry );

		// Refresh the playlist
		mlt_playlist_virtual_refresh( this );
	}

	return error;
}

/** Move an entry in the playlist.
*/

int mlt_playlist_move( mlt_playlist this, int src, int dest )
{
	int i;

	/* We need to ensure that the requested indexes are valid and correct it as necessary */
	if ( src < 0 ) 
		src = 0;
	if ( src >= this->count )
		src = this->count - 1;

	if ( dest < 0 ) 
		dest = 0;
	if ( dest >= this->count )
		dest = this->count - 1;
	
	if ( src != dest && this->count > 1 )
	{
		int current = mlt_playlist_current_clip( this );
		mlt_position position = mlt_producer_position( mlt_playlist_producer( this ) );
		playlist_entry *src_entry = NULL;

		// We need all the details about the current clip
		mlt_playlist_clip_info current_info;

		mlt_playlist_get_clip_info( this, &current_info, current );
		position -= current_info.start;

		if ( current == src )
			current = dest;
		else if ( current > src && current < dest )
			current ++;
		else if ( current == dest )
			current = src;

		src_entry = this->list[ src ];
		if ( src > dest )
		{
			for ( i = src; i > dest; i -- )
				this->list[ i ] = this->list[ i - 1 ];
		}
		else
		{
			for ( i = src; i < dest; i ++ )
				this->list[ i ] = this->list[ i + 1 ];
		}
		this->list[ dest ] = src_entry;

		mlt_playlist_get_clip_info( this, &current_info, current );
		mlt_producer_seek( mlt_playlist_producer( this ), current_info.start + position );
		mlt_playlist_virtual_refresh( this );
	}

	return 0;
}

/** Repeat the specified clip n times.
*/

int mlt_playlist_repeat_clip( mlt_playlist this, int clip, int repeat )
{
	int error = repeat < 1 || clip < 0 || clip >= this->count;
	if ( error == 0 )
	{
		playlist_entry *entry = this->list[ clip ];
		entry->repeat = repeat;
		mlt_playlist_virtual_refresh( this );
	}
	return error;
}

/** Resize the specified clip.
*/

int mlt_playlist_resize_clip( mlt_playlist this, int clip, mlt_position in, mlt_position out )
{
	int error = clip < 0 || clip >= this->count;
	if ( error == 0 && mlt_playlist_resize_mix( this, clip, in, out ) != 0 )
	{
		playlist_entry *entry = this->list[ clip ];
		mlt_producer producer = entry->producer;

		if ( in <= -1 )
			in = 0;
		if ( out <= -1 || out >= mlt_producer_get_length( producer ) )
			out = mlt_producer_get_length( producer ) - 1;

		if ( out < in )
		{
			mlt_position t = in;
			in = out;
			out = t;
		}

		mlt_producer_set_in_and_out( producer, in, out );
	}
	return error;
}

/** Split a clip on the playlist at the given position.
*/

int mlt_playlist_split( mlt_playlist this, int clip, mlt_position position )
{
	int error = clip < 0 || clip >= this->count;
	if ( error == 0 )
	{
		playlist_entry *entry = this->list[ clip ];
		if ( position >= 0 && position < entry->frame_count )
		{
			mlt_producer split = NULL;
			int in = entry->frame_in;
			int out = entry->frame_out;
			mlt_events_block( mlt_playlist_properties( this ), this );
			mlt_playlist_resize_clip( this, clip, in, in + position );
			split = mlt_producer_cut( entry->producer, in + position + 1, out );
			mlt_playlist_insert( this, split, clip + 1, 0, -1 );
			mlt_producer_close( split );
			mlt_events_unblock( mlt_playlist_properties( this ), this );
			mlt_events_fire( mlt_playlist_properties( this ), "producer-changed", NULL );
		}
		else
		{
			error = 1;
		}
	}
	return error;
}

/** Join 1 or more consecutive clips.
*/

int mlt_playlist_join( mlt_playlist this, int clip, int count, int merge )
{
	int error = clip < 0 || ( clip ) >= this->count;
	if ( error == 0 )
	{
		int i = clip;
		mlt_playlist new_clip = mlt_playlist_init( );
		mlt_events_block( mlt_playlist_properties( this ), this );
		if ( clip + count >= this->count )
			count = this->count - clip;
		for ( i = 0; i <= count; i ++ )
		{
			playlist_entry *entry = this->list[ clip ];
			mlt_playlist_append( new_clip, entry->producer );
			mlt_playlist_repeat_clip( new_clip, i, entry->repeat );
			mlt_playlist_remove( this, clip );
		}
		mlt_events_unblock( mlt_playlist_properties( this ), this );
		mlt_playlist_insert( this, mlt_playlist_producer( new_clip ), clip, 0, -1 );
		mlt_playlist_close( new_clip );
	}
	return error;
}

/** Mix consecutive clips for a specified length and apply transition if specified.
*/

int mlt_playlist_mix( mlt_playlist this, int clip, int length, mlt_transition transition )
{
	int error = ( clip < 0 || clip + 1 >= this->count );
	if ( error == 0 )
	{
		playlist_entry *clip_a = this->list[ clip ];
		playlist_entry *clip_b = this->list[ clip + 1 ];
		mlt_producer track_a;
		mlt_producer track_b;
		mlt_tractor tractor = mlt_tractor_new( );
		mlt_events_block( mlt_playlist_properties( this ), this );

		// TODO: Check length is valid for both clips and resize if necessary.

		// Create the a and b tracks/cuts
		track_a = mlt_producer_cut( clip_a->producer, clip_a->frame_out - length + 1, clip_a->frame_out );
		track_b = mlt_producer_cut( clip_b->producer, clip_b->frame_in, clip_b->frame_in + length - 1 );

		// Temporary - for the benefit of westley serialisation
		mlt_properties_set_int( mlt_producer_properties( track_a ), "cut", 1 );
		mlt_properties_set_int( mlt_producer_properties( track_b ), "cut", 1 );

		// Set the tracks on the tractor
		mlt_tractor_set_track( tractor, track_a, 0 );
		mlt_tractor_set_track( tractor, track_b, 1 );

		// Insert the mix object into the playlist
		mlt_playlist_insert( this, mlt_tractor_producer( tractor ), clip + 1, -1, -1 );
		mlt_properties_set_data( mlt_tractor_properties( tractor ), "mlt_mix", tractor, 0, NULL, NULL );

		// Attach the transition
		if ( transition != NULL )
		{
			mlt_field field = mlt_tractor_field( tractor );
			mlt_field_plant_transition( field, transition, 0, 1 );
			mlt_transition_set_in_and_out( transition, 0, length - 1 );
		}

		// Check if we have anything left on the right hand clip
		if ( clip_b->frame_out - clip_b->frame_in > length )
		{
			mlt_playlist_resize_clip( this, clip + 2, clip_b->frame_in + length, clip_b->frame_out );
			mlt_properties_set_data( mlt_producer_properties( clip_b->producer ), "mix_in", tractor, 0, NULL, NULL );
			mlt_properties_set_data( mlt_tractor_properties( tractor ), "mix_out", clip_b->producer, 0, NULL, NULL );
		}
		else
		{
			mlt_producer_clear( clip_b->producer );
			mlt_playlist_remove( this, clip + 2 );
		}

		// Check if we have anything left on the left hand clip
		if ( clip_a->frame_out - clip_a->frame_in > length )
		{
			mlt_playlist_resize_clip( this, clip, clip_a->frame_in, clip_a->frame_out - length );
			mlt_properties_set_data( mlt_producer_properties( clip_a->producer ), "mix_out", tractor, 0, NULL, NULL );
			mlt_properties_set_data( mlt_tractor_properties( tractor ), "mix_in", clip_a->producer, 0, NULL, NULL );
		}
		else
		{
			mlt_producer_clear( clip_a->producer );
			mlt_playlist_remove( this, clip );
		}

		mlt_events_unblock( mlt_playlist_properties( this ), this );
		mlt_events_fire( mlt_playlist_properties( this ), "producer-changed", NULL );
		mlt_producer_close( track_a );
		mlt_producer_close( track_b );
		mlt_tractor_close( tractor );
	}
	return error;
}

/** Remove a mixed clip - ensure that the cuts included in the mix find their way
	back correctly on to the playlist.
*/

static int mlt_playlist_unmix( mlt_playlist this, int clip )
{
	int error = ( clip < 0 || clip >= this->count ); 

	// Ensure that the clip request is actually a mix
	if ( error == 0 )
	{
		mlt_producer producer = mlt_producer_cut_parent( this->list[ clip ]->producer );
		mlt_properties properties = mlt_producer_properties( producer );
		error = mlt_properties_get_data( properties, "mlt_mix", NULL ) == NULL;
	}

	if ( error == 0 )
	{
		playlist_entry *mix = this->list[ clip ];
		mlt_tractor tractor = ( mlt_tractor )mlt_producer_cut_parent( mix->producer );
		mlt_properties properties = mlt_tractor_properties( tractor );
		mlt_producer clip_a = mlt_properties_get_data( properties, "mix_in", NULL );
		mlt_producer clip_b = mlt_properties_get_data( properties, "mix_out", NULL );
		int length = mlt_producer_get_playtime( mlt_tractor_producer( tractor ) );
		mlt_events_block( mlt_playlist_properties( this ), this );

		if ( clip_a != NULL )
		{
			mlt_producer_set_in_and_out( clip_a, mlt_producer_get_in( clip_a ), mlt_producer_get_out( clip_a ) + length );
		}
		else
		{
			mlt_producer cut = mlt_tractor_get_track( tractor, 0 );
			mlt_playlist_insert( this, cut, clip, -1, -1 );
			clip ++;
		}

		if ( clip_b != NULL )
		{
			mlt_producer_set_in_and_out( clip_b, mlt_producer_get_in( clip_b ) - length, mlt_producer_get_out( clip_b ) );
		}
		else
		{
			mlt_producer cut = mlt_tractor_get_track( tractor, 1 );
			mlt_playlist_insert( this, cut, clip + 1, -1, -1 );
		}

		mlt_properties_set_data( properties, "mlt_mix", NULL, 0, NULL, NULL );
		mlt_playlist_remove( this, clip );
		mlt_events_unblock( mlt_playlist_properties( this ), this );
		mlt_events_fire( mlt_playlist_properties( this ), "producer-changed", NULL );
	}
	return error;
}

static int mlt_playlist_resize_mix( mlt_playlist this, int clip, int in, int out )
{
	int error = ( clip < 0 || clip >= this->count ); 

	// Ensure that the clip request is actually a mix
	if ( error == 0 )
	{
		mlt_producer producer = mlt_producer_cut_parent( this->list[ clip ]->producer );
		mlt_properties properties = mlt_producer_properties( producer );
		error = mlt_properties_get_data( properties, "mlt_mix", NULL ) == NULL;
	}

	if ( error == 0 )
	{
		playlist_entry *mix = this->list[ clip ];
		mlt_tractor tractor = ( mlt_tractor )mlt_producer_cut_parent( mix->producer );
		mlt_properties properties = mlt_tractor_properties( tractor );
		mlt_producer clip_a = mlt_properties_get_data( properties, "mix_in", NULL );
		mlt_producer clip_b = mlt_properties_get_data( properties, "mix_out", NULL );
		mlt_producer track_a = mlt_tractor_get_track( tractor, 0 );
		mlt_producer track_b = mlt_tractor_get_track( tractor, 1 );
		int length = out - in + 1;
		int length_diff = length - mlt_producer_get_playtime( mlt_tractor_producer( tractor ) );
		mlt_events_block( mlt_playlist_properties( this ), this );

		if ( clip_a != NULL )
			mlt_producer_set_in_and_out( clip_a, mlt_producer_get_in( clip_a ), mlt_producer_get_out( clip_a ) - length_diff );

		if ( clip_b != NULL )
			mlt_producer_set_in_and_out( clip_b, mlt_producer_get_in( clip_b ) + length_diff, mlt_producer_get_out( clip_b ) );

		mlt_producer_set_in_and_out( track_a, mlt_producer_get_in( track_a ) - length_diff, mlt_producer_get_out( track_a ) );
		mlt_producer_set_in_and_out( track_b, mlt_producer_get_in( track_b ), mlt_producer_get_out( track_b ) + length_diff );
		mlt_producer_set_in_and_out( mlt_multitrack_producer( mlt_tractor_multitrack( tractor ) ), in, out );
		mlt_producer_set_in_and_out( mlt_tractor_producer( tractor ), in, out );
		mlt_properties_set_position( mlt_producer_properties( mix->producer ), "length", out - in + 1 );
		mlt_producer_set_in_and_out( mix->producer, in, out );

		mlt_events_unblock( mlt_playlist_properties( this ), this );
		mlt_playlist_virtual_refresh( this );
	}
	return error;
}

/** Get the current frame.
*/

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Get this mlt_playlist
	mlt_playlist this = producer->child;

	// Get the real producer
	mlt_service real = mlt_playlist_virtual_seek( this );

	// Get the frame
	mlt_service_get_frame( real, frame, index );

	// Check if we're at the end of the clip
	mlt_properties properties = mlt_frame_properties( *frame );
	if ( mlt_properties_get_int( properties, "end_of_clip" ) )
		mlt_playlist_virtual_set_out( this );

	// Check for notifier and call with appropriate argument
	mlt_properties playlist_properties = mlt_producer_properties( producer );
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
*/

void mlt_playlist_close( mlt_playlist this )
{
	if ( this != NULL && mlt_properties_dec_ref( mlt_playlist_properties( this ) ) <= 0 )
	{
		int i = 0;
		this->parent.close = NULL;
		for ( i = 0; i < this->count; i ++ )
		{
			mlt_event_close( this->list[ i ]->event );
			mlt_producer_close( this->list[ i ]->producer );
			free( this->list[ i ] );
		}
		mlt_producer_close( &this->blank );
		mlt_producer_close( &this->parent );
		free( this->list );
		free( this );
	}
}
