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

#include <stdlib.h>

/** Virtual playlist entry.
*/

typedef struct
{
	mlt_producer producer;
	mlt_timecode in;
	mlt_timecode playtime;
}
playlist_entry;

/** Private definition.
*/

struct mlt_playlist_s
{
	struct mlt_producer_s parent;
	int size;
	int count;
	mlt_producer *list;
	mlt_producer blank;

	int virtual_size;
	int virtual_count;
	playlist_entry **virtual_list;
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

		// Create a producer
		this->blank = calloc( sizeof( struct mlt_producer_s ), 1 );

		// Initialise it
		mlt_producer_init( this->blank, NULL );
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

/** Store a producer in the playlist.
*/

static int mlt_playlist_store( mlt_playlist this, mlt_producer producer )
{
	int i;

	// If it's already added, return the index
	for ( i = 0; i < this->count; i ++ )
	{
		if ( producer == this->list[ i ] )
			return i;
	}

	// Check that we have room
	if ( this->count >= this->size )
	{
		this->list = realloc( this->list, ( this->size + 10 ) * sizeof( mlt_producer ) );
		for ( i = this->size; i < this->size + 10; i ++ )
			this->list[ i ] = NULL;
		this->size += 10;
	}

	// Add this producer to the list
	this->list[ this->count ] = producer;

	return this->count ++;
}

/** Append to the virtual playlist.
*/

static int mlt_playlist_virtual_append( mlt_playlist this, int position, mlt_timecode in, mlt_timecode out )
{
	// Check that we have room
	if ( this->virtual_count >= this->virtual_size )
	{
		int i;
		this->virtual_list = realloc( this->virtual_list, ( this->virtual_size + 10 ) * sizeof( playlist_entry * ) );
		for ( i = this->virtual_size; i < this->virtual_size + 10; i ++ )
			this->virtual_list[ i ] = NULL;
		this->virtual_size += 10;
	}

	this->virtual_list[ this->virtual_count ] = malloc( sizeof( playlist_entry ) );
	this->virtual_list[ this->virtual_count ]->producer = this->list[ position ];
	this->virtual_list[ this->virtual_count ]->in = in;
	this->virtual_list[ this->virtual_count ]->playtime = out - in;

	this->virtual_count ++;

	return 0;
}

/** Seek in the virtual playlist.
*/

static mlt_producer mlt_playlist_virtual_seek( mlt_playlist this )
{
	// Default producer to blank
	mlt_producer producer = this->blank;

	// Map playlist position to real producer in virtual playlist
	mlt_timecode position = mlt_producer_position( &this->parent );

	// Loop through the virtual playlist
	int i = 0;

	for ( i = 0; i < this->virtual_count; i ++ )
	{
		if ( position < this->virtual_list[ i ]->playtime )
		{
			// Found it, now break
			producer = this->virtual_list[ i ]->producer;
			position += this->virtual_list[ i ]->in;
			break;
		}
		else
		{
			// Decrement position by length of this entry
			position -= this->virtual_list[ i ]->playtime;
		}
	}

	// Seek in real producer to relative position
	mlt_producer_seek( producer, position );

	return producer;
}

/** Append a producer to the playlist.
*/

int mlt_playlist_append( mlt_playlist this, mlt_producer producer )
{
	// Get the position of the producer in the list
	int position = mlt_playlist_store( this, producer );

	// Append to virtual list
	mlt_playlist_virtual_append( this, position, 0, mlt_producer_get_playtime( producer ) );

	return 0;
}

/** Append a blank to the playlist of a given length.
*/

int mlt_playlist_blank( mlt_playlist this, mlt_timecode length )
{
	// Get the position of the producer in the list
	int position = mlt_playlist_store( this, this->blank );

	// Append to the virtual list
	mlt_playlist_virtual_append( this, position, 0, length );

	return 0;
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
	mlt_producer_close( this->blank );
	free( this->blank );
	free( this );
}
