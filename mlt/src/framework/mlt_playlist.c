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
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Virtual playlist entry.
*/

typedef struct
{
	mlt_producer producer;
	int64_t frame_in;
	int64_t frame_out;
	int64_t frame_count;
	mlt_timecode playtime;
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

		// Initialise blank
		mlt_producer_init( &this->blank, NULL );

		// Indicate that this producer is a playlist
		mlt_properties_set_data( mlt_playlist_properties( this ), "playlist", this, 0, NULL, NULL );
	}
	
	return this;
}

/** Get the producer associated to this playlist.
*/

mlt_producer mlt_playlist_producer( mlt_playlist this )
{
	return &this->parent;
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

static int mlt_playlist_virtual_refresh( mlt_playlist this )
{
	int i = 0;

	// Get the fps of the first producer
	double fps = mlt_properties_get_double( mlt_playlist_properties( this ), "first_fps" );
	mlt_timecode playtime = 0;

	for ( i = 0; i < this->count; i ++ )
	{
		// Get the producer
		mlt_producer producer = this->list[ i ]->producer;

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

		// Update the playtime for this clip
		playtime += this->list[ i ]->playtime;
	}

	// Refresh all properties
	mlt_properties_set_double( mlt_playlist_properties( this ), "first_fps", fps );
	mlt_properties_set_double( mlt_playlist_properties( this ), "fps", fps == 0 ? 25 : fps );
	mlt_properties_set_timecode( mlt_playlist_properties( this ), "length", playtime );
	mlt_properties_set_timecode( mlt_playlist_properties( this ), "out", playtime );

	return 0;
}

/** Append to the virtual playlist.
*/

static int mlt_playlist_virtual_append( mlt_playlist this, mlt_producer producer, int64_t in, int64_t out )
{
	double fps = mlt_properties_get_double( mlt_playlist_properties( this ), "fps" );
	double playtime = ( double )( out - in + 1 ) / fps;

	// Check that we have room
	if ( this->count >= this->size )
	{
		int i;
		this->list = realloc( this->list, ( this->size + 10 ) * sizeof( playlist_entry * ) );
		for ( i = this->size; i < this->size + 10; i ++ )
			this->list[ i ] = NULL;
		this->size += 10;
	}

	this->list[ this->count ] = calloc( sizeof( playlist_entry ), 1 );
	this->list[ this->count ]->producer = producer;
	this->list[ this->count ]->frame_in = in;
	this->list[ this->count ]->frame_out = out;
	this->list[ this->count ]->frame_count = out - in + 1;
	this->list[ this->count ]->playtime = playtime;

	mlt_producer_set_speed( producer, 0 );

	this->count ++;

	return mlt_playlist_virtual_refresh( this );
}

/** Seek in the virtual playlist.
*/

static mlt_producer mlt_playlist_virtual_seek( mlt_playlist this )
{
	// Default producer to blank
	mlt_producer producer = NULL;

	// Map playlist position to real producer in virtual playlist
	mlt_timecode pos = mlt_producer_position( &this->parent );
	int64_t position = mlt_producer_frame_position( &this->parent, pos );
	int64_t total = 0;

	// Loop through the virtual playlist
	int i = 0;

	for ( i = 0; i < this->count; i ++ )
	{
		// Increment the total
		total += this->list[ i ]->frame_count;

		if ( position < this->list[ i ]->frame_count )
		{
			// Found it, now break
			producer = this->list[ i ]->producer;
			position += this->list[ i ]->frame_in;
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
		mlt_producer_seek_frame( producer, position );
	}
	else if ( total > 0 )
	{
		playlist_entry *entry = this->list[ this->count - 1 ];
		mlt_producer this_producer = mlt_playlist_producer( this );
		mlt_producer_seek_frame( this_producer, total - 1 );
		producer = entry->producer;
		mlt_producer_seek_frame( producer, entry->frame_out );
	}
	else
	{
		mlt_producer_seek( mlt_playlist_producer( this ), 0 );
		producer = &this->blank;
	}

	return producer;
}

static mlt_producer mlt_playlist_virtual_set_out( mlt_playlist this )
{
	// Default producer to blank
	mlt_producer producer = &this->blank;

	// Map playlist position to real producer in virtual playlist
	mlt_timecode pos = mlt_producer_position( &this->parent );
	int64_t position = mlt_producer_frame_position( &this->parent, pos );

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
	if ( i < this->count )
	{
		// Update the playtime for the changed clip (hmmm)
		this->list[ i ]->frame_count = position + 1;
		this->list[ i ]->frame_out = position - this->list[ i ]->frame_in;

		// Refresh the playlist
		mlt_playlist_virtual_refresh( this );
	}

	return producer;
}

int mlt_playlist_current_clip( mlt_playlist this )
{
	// Map playlist position to real producer in virtual playlist
	mlt_timecode pos = mlt_producer_position( &this->parent );
	int64_t position = mlt_producer_frame_position( &this->parent, pos );

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

mlt_producer mlt_playlist_current( mlt_playlist this )
{
	int i = mlt_playlist_current_clip( this );
	if ( i < this->count )
		return this->list[ i ]->producer;
	else
		return &this->blank;
}

/** Get the timecode which corresponds to the start of the next clip.
*/

mlt_timecode mlt_playlist_clip( mlt_playlist this, mlt_whence whence, int index )
{
	int64_t position = 0;
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

	// Now determine the timecode
	for ( i = 0; i < absolute_clip; i ++ )
		position += this->list[ i ]->frame_count;

	return mlt_producer_time( &this->parent, position );
}

int mlt_playlist_get_clip_info( mlt_playlist this, mlt_playlist_clip_info *info, int index )
{
	int error = index < 0 || index >= this->count;
	memset( info, 0, sizeof( mlt_playlist_clip_info ) );
	if ( !error )
	{
		mlt_producer producer = this->list[ index ]->producer;
		mlt_properties properties = mlt_producer_properties( producer );
		info->producer = producer;
		info->start = mlt_playlist_clip( this, mlt_whence_relative_start, index );
		info->resource = mlt_properties_get( properties, "resource" );
		info->frame_in = this->list[ index ]->frame_in;
		info->in = mlt_producer_time( producer, info->frame_in );
		info->frame_out = this->list[ index ]->frame_out;
		info->out = mlt_producer_time( producer, info->frame_out );
		info->playtime = this->list[ index ]->playtime;
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
	this->count = 0;
	mlt_properties_set_double( mlt_playlist_properties( this ), "first_fps", 0 );
	return mlt_playlist_virtual_refresh( this );
}

/** Append a producer to the playlist.
*/

int mlt_playlist_append( mlt_playlist this, mlt_producer producer )
{
	// Append to virtual list
	int64_t out = mlt_producer_frame_position( producer, mlt_producer_get_out( producer ) );
	return mlt_playlist_virtual_append( this, producer, 0, out );
}

/** Append a producer to the playlist with in/out points.
*/

int mlt_playlist_append_io( mlt_playlist this, mlt_producer producer, double in, double out )
{
	// Append to virtual list
	if ( in != -1 && out != -1 )
	{
		int64_t fin = mlt_producer_frame_position( producer, in );
		int64_t fout = mlt_producer_frame_position( producer, out );
		return mlt_playlist_virtual_append( this, producer, fin, fout );
	}
	else
	{
		return mlt_playlist_append( this, producer );
	}
}

/** Append a blank to the playlist of a given length.
*/

int mlt_playlist_blank( mlt_playlist this, mlt_timecode length )
{
	// Append to the virtual list
	int64_t fout = mlt_producer_frame_position( &this->blank, length );
	return mlt_playlist_virtual_append( this, &this->blank, 0, fout );
}

/** Insert a producer into the playlist.
*/

int mlt_playlist_insert( mlt_playlist this, mlt_producer producer, int where, mlt_timecode in, mlt_timecode out )
{
	return 0;
}

int mlt_playlist_remove( mlt_playlist this, int where )
{
	return 0;
}

int mlt_playlist_move( mlt_playlist this, int from, int to )
{
	return 0;
}

int mlt_playlist_resize_clip( mlt_playlist this, int clip, mlt_timecode in, mlt_timecode out )
{
	int error = clip < 0 || clip >= this->count;
	if ( error == 0 )
	{
		playlist_entry *entry = this->list[ clip ];
		mlt_producer producer = entry->producer;

		if ( in <= -1 )
			in = 0;
		if ( out <= -1 || out >= mlt_producer_get_out( producer ) )
			out = mlt_producer_get_out( producer );

		if ( out < in )
		{
			mlt_timecode t = in;
			in = out;
			out = t;
		}

		int64_t fin = mlt_producer_frame_position( producer, in );
		int64_t fout = mlt_producer_frame_position( producer, out );

		entry->frame_in = fin;
		entry->frame_out = fout;
		entry->frame_count = fout - fin + 1;
		entry->playtime = out - in;
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
	mlt_producer real = mlt_playlist_virtual_seek( this );

	// Get the frame
	mlt_service_get_frame( mlt_producer_service( real ), frame, index );

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

	// Update timecode on the frame we're creating
	mlt_frame_set_timecode( *frame, mlt_producer_position( producer ) );

	// Position ourselves on the next frame
	mlt_producer_prepare_next( producer );

	return 0;
}

/** Close the playlist.
*/

void mlt_playlist_close( mlt_playlist this )
{
	mlt_producer_close( &this->parent );
	mlt_producer_close( &this->blank );
	free( this );
}
