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

/** Private definition.
*/

struct mlt_playlist_s
{
	struct mlt_producer_s parent;
	int count;
	int track;
};

/** Constructor.

	TODO: Override and implement all transport related method.
	TODO: Override and implement a time code normalising service get_frame
*/

mlt_playlist mlt_playlist_init( )
{
	mlt_playlist this = calloc( sizeof( struct mlt_playlist_s ), 1 );
	if ( this != NULL )
	{
		mlt_producer producer = &this->parent;
		if ( mlt_producer_init( producer, this ) != 0 )
		{
			free( this );
			this = NULL;
		}
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

/** Append a producer to the playlist.

	TODO: Implement
	TODO: Extract length information from the producer.
*/

int mlt_playlist_append( mlt_playlist this, mlt_producer producer )
{
	return 0;
}

/** Append a blank to the playlist of a given length.

	TODO: Implement
*/

int mlt_playlist_blank( mlt_playlist this, mlt_timecode length )
{
	return 0;
}

/** Close the playlist.
*/

mlt_playlist_close( mlt_playlist this )
{
	mlt_producer_close( &this->parent );
	free( this );
}
