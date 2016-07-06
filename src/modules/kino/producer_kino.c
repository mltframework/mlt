/*
 * producer_kino.c -- a DV file format parser
 * Copyright (C) 2005-2014 Meltytech, LLC
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include "kino_wrapper.h"

/* NB: This is an abstract producer - it provides no codec support whatsoever. */

#define FRAME_SIZE_525_60 	10 * 150 * 80
#define FRAME_SIZE_625_50 	12 * 150 * 80

typedef struct producer_kino_s *producer_kino;

struct producer_kino_s
{
	struct mlt_producer_s parent;
	kino_wrapper wrapper;
};

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_kino_init( mlt_profile profile, mlt_service_type type, const char *id, char *filename )
{
	kino_wrapper wrapper = kino_wrapper_init( );

	if ( kino_wrapper_open( wrapper, filename ) )
	{
		producer_kino this = calloc( 1, sizeof( struct producer_kino_s ) );

		if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
		{
			mlt_producer producer = &this->parent;
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
			double fps = kino_wrapper_is_pal( wrapper ) ? 25 : 30000.0 / 1001.0;
	
			// Assign the wrapper
			this->wrapper = wrapper;

			// Pass wrapper properties (frame rate, count etc)
			mlt_properties_set_position( properties, "length", kino_wrapper_get_frame_count( wrapper ) );
			mlt_properties_set_position( properties, "in", 0 );
			mlt_properties_set_position( properties, "out", kino_wrapper_get_frame_count( wrapper ) - 1 );
			mlt_properties_set_double( properties, "real_fps", fps );
			mlt_properties_set( properties, "resource", filename );

			// Register transport implementation with the producer
			producer->close = ( mlt_destructor )producer_close;
	
			// Register our get_frame implementation with the producer
			producer->get_frame = producer_get_frame;
	
			// Return the producer
			return producer;
		}
		free( this );
	}

	kino_wrapper_close( wrapper );

	return NULL;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_kino this = producer->child;
	uint8_t *data = mlt_pool_alloc( FRAME_SIZE_625_50 );
	
	// Obtain the current frame number
	uint64_t position = mlt_producer_frame( producer );
	
	// Create an empty frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	// Seek and fetch
	if ( kino_wrapper_get_frame( this->wrapper, data, position ) )
	{
		// Get the frames properties
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		// Determine if we're PAL or NTSC
		int is_pal = kino_wrapper_is_pal( this->wrapper );

		// Pass the dv data
		mlt_properties_set_data( properties, "dv_data", data, FRAME_SIZE_625_50, ( mlt_destructor )mlt_pool_release, NULL );

		// Update other info on the frame
		mlt_properties_set_int( properties, "width", 720 );
		mlt_properties_set_int( properties, "height", is_pal ? 576 : 480 );
		mlt_properties_set_int( properties, "top_field_first", is_pal ? 0 : ( data[ 5 ] & 0x07 ) == 0 ? 0 : 1 );
	}
	else
	{
		mlt_pool_release( data );
	}

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	if ( parent != NULL )
	{
		// Obtain this
		producer_kino this = parent->child;

		// Close the file
		if ( this != NULL )
			kino_wrapper_close( this->wrapper );

		// Close the parent
		parent->close = NULL;
		mlt_producer_close( parent );

		// Free the memory
		free( this );
	}
}
