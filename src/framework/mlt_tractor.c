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
#include "mlt_multitrack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Private structure.
*/

struct mlt_tractor_s
{
	struct mlt_producer_s parent;
	mlt_service producer;
};

/** Forward references to static methods.
*/

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int track );
static void producer_close( mlt_producer this );

/** Constructor for the tractor.

	TODO: thread this service...
*/

mlt_tractor mlt_tractor_init( )
{
	mlt_tractor this = calloc( sizeof( struct mlt_tractor_s ), 1 );
	if ( this != NULL )
	{
		mlt_producer producer = &this->parent;
		if ( mlt_producer_init( producer, this ) == 0 )
		{
			mlt_properties_set( mlt_producer_properties( producer ), "resource", "<tractor>" );
			mlt_properties_set( mlt_producer_properties( producer ), "mlt_type", "mlt_producer" );
			mlt_properties_set( mlt_producer_properties( producer ), "mlt_service", "tractor" );

			producer->get_frame = producer_get_frame;
			producer->close = producer_close;
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
	return mlt_producer_service( &this->parent );
}

/** Get the producer object associated to the tractor.
*/

mlt_producer mlt_tractor_producer( mlt_tractor this )
{
	return &this->parent;
}

/** Get the properties object associated to the tractor.
*/

mlt_properties mlt_tractor_properties( mlt_tractor this )
{
	return mlt_producer_properties( &this->parent );
}

/** Connect the tractor.
*/

int mlt_tractor_connect( mlt_tractor this, mlt_service producer )
{
	int ret = mlt_service_connect_producer( mlt_tractor_service( this ), producer, 0 );

	// This is the producer we're going to connect to
	if ( ret == 0 )
		this->producer = producer;

	return ret;
}

/** Get the next frame.

	TODO: This should be reading a pump being populated by the thread...
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int track )
{
	mlt_tractor this = parent->child;

	// We only respond to the first track requests
	if ( track == 0 && this->producer != NULL )
	{
		int i = 0;
		int looking = 1;
		int done = 0;
		mlt_frame temp = NULL;
		mlt_frame store[ 10 ];
		int count = 0;

		// Get the properties of the parent producer
		mlt_properties properties = mlt_producer_properties( parent );

		// Try to obtain the multitrack associated to the tractor
		mlt_multitrack multitrack = mlt_properties_get_data( properties, "multitrack", NULL );

		// Or a specific producer
		mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );

		// If we don't have one, we're in trouble... 
		if ( multitrack != NULL )
		{
			mlt_producer target = mlt_multitrack_producer( multitrack );
			mlt_producer_seek( target, mlt_producer_frame( parent ) );
			mlt_producer_set_speed( target, mlt_producer_get_speed( parent ) );

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
				else if ( ( !mlt_frame_is_test_card( temp ) || !mlt_frame_is_test_audio( temp ) ) && looking &&
					    	mlt_producer_frame( parent ) == mlt_frame_get_position( temp ) )
				{
					*frame = temp;
					looking = 0;
				}
				else
				{
					// We store all other frames for now
					store[ count ++ ] = temp;
				}
			}
	
			// Now place all the unused frames on to the properties (will be destroyed automatically)
			while ( count -- )
			{
				mlt_properties frame_properties = mlt_frame_properties( *frame );
				char label[ 30 ];
				sprintf( label, "tractor_%d", count );
				while ( mlt_properties_get_data( frame_properties, label, NULL ) != NULL )
					strcat( label, "+" );
				mlt_properties_set_data( frame_properties, label, store[ count ], 0, ( mlt_destructor )mlt_frame_close, NULL );
			}
		}
		else if ( producer != NULL )
		{
			mlt_producer_seek( producer, mlt_producer_frame( parent ) );
			mlt_producer_set_speed( producer, mlt_producer_get_speed( parent ) );
			mlt_service_get_frame( this->producer, frame, track );
		}
		else
		{
			fprintf( stderr, "tractor without a multitrack!!\n" );
			mlt_service_get_frame( this->producer, frame, track );
		}

		// Prepare the next frame
		mlt_producer_prepare_next( parent );

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
	this->parent.close = NULL;
	mlt_producer_close( &this->parent );
	free( this );
}

/** Close the producer.
*/

static void producer_close( mlt_producer this )
{
	mlt_tractor_close( this->child );
}

