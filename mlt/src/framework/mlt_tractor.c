/*
 * mlt_tractor.c -- tractor service class
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

#include "mlt_tractor.h"
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>

/** Private structure.
*/

struct mlt_tractor_s
{
	struct mlt_service_s parent;
	mlt_service producer;
};

/** Forward references to static methods.
*/

static int service_get_frame( mlt_service this, mlt_frame_ptr frame, int track );

/** Constructor for the tractor.

	TODO: thread this service...
*/

mlt_tractor mlt_tractor_init( )
{
	mlt_tractor this = calloc( sizeof( struct mlt_tractor_s ), 1 );
	if ( this != NULL )
	{
		mlt_service service = &this->parent;
		if ( mlt_service_init( service, this ) == 0 )
		{
			service->get_frame = service_get_frame;
		}
		else
		{
			free( this );
			this = NULL;
		}
	}
	return this;
}

/** Get the service object associated to the tractor.
*/

mlt_service mlt_tractor_service( mlt_tractor this )
{
	return &this->parent;
}

/** Connect the tractor.
*/

int mlt_tractor_connect( mlt_tractor this, mlt_service producer )
{
	int ret = mlt_service_connect_producer( &this->parent, producer, 0 );

	if ( ret == 0 )
	{
		// This is the producer we're going to connect to
		this->producer = producer;
	}

	return ret;
}

/** Get the next frame.

	TODO: This should be reading a pump being populated by the thread...
*/

static int service_get_frame( mlt_service parent, mlt_frame_ptr frame, int track )
{
	mlt_tractor this = parent->child;

	// We only respond to the first track requests
	if ( track == 0 && this->producer != NULL )
	{
		int i = 0;
		int looking = 1;
		int done = 0;
		mlt_frame temp;

		// Loop through each of the tracks we're harvesting
		for ( i = 0; !done; i ++ )
		{
			// Get a frame from the producer
			mlt_service_get_frame( this->producer, &temp, i );

			// Check for last track
			done = mlt_properties_get_int( mlt_frame_properties( temp ), "last_track" );

			// Handle the frame
			if ( done && looking )
			{
				// Use this as output if we don't have one already
				*frame = temp;
			}
			else if ( !mlt_frame_is_test_card( temp ) && looking )
			{
				// This is the one we want and we can stop looking
				*frame = temp;
				looking = 0;
			}
			else
			{
				// We discard all other frames
				mlt_frame_close( temp );
			}
		}

		// Indicate our found status
		return 0;
	}
	else
	{
		// Generate a test card
		*frame = mlt_frame_init( );
		return 0;
	}
}

/** Close the tractor.
*/

void mlt_tractor_close( mlt_tractor this )
{
	mlt_service_close( &this->parent );
	free( this );
}

