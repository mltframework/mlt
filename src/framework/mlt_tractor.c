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
#include "mlt_field.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Forward references to static methods.
*/

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int track );
static void mlt_tractor_listener( mlt_multitrack tracks, mlt_tractor this );

/** Constructor for the tractor.
*/

mlt_tractor mlt_tractor_init( )
{
	mlt_tractor this = calloc( sizeof( struct mlt_tractor_s ), 1 );
	if ( this != NULL )
	{
		mlt_producer producer = &this->parent;
		if ( mlt_producer_init( producer, this ) == 0 )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			mlt_properties_set( properties, "resource", "<tractor>" );
			mlt_properties_set( properties, "mlt_type", "mlt_producer" );
			mlt_properties_set( properties, "mlt_service", "tractor" );
			mlt_properties_set_int( properties, "in", 0 );
			mlt_properties_set_int( properties, "out", -1 );
			mlt_properties_set_int( properties, "length", 0 );

			producer->get_frame = producer_get_frame;
			producer->close = ( mlt_destructor )mlt_tractor_close;
			producer->close_object = this;
		}
		else
		{
			free( this );
			this = NULL;
		}
	}
	return this;
}

mlt_tractor mlt_tractor_new( )
{
	mlt_tractor this = calloc( sizeof( struct mlt_tractor_s ), 1 );
	if ( this != NULL )
	{
		mlt_producer producer = &this->parent;
		if ( mlt_producer_init( producer, this ) == 0 )
		{
			mlt_multitrack multitrack = mlt_multitrack_init( );
			mlt_field field = mlt_field_new( multitrack, this );
			mlt_properties props = MLT_PRODUCER_PROPERTIES( producer );

			mlt_properties_set( props, "resource", "<tractor>" );
			mlt_properties_set( props, "mlt_type", "mlt_producer" );
			mlt_properties_set( props, "mlt_service", "tractor" );
			mlt_properties_set_position( props, "in", 0 );
			mlt_properties_set_position( props, "out", 0 );
			mlt_properties_set_position( props, "length", 0 );
			mlt_properties_set_data( props, "multitrack", multitrack, 0, ( mlt_destructor )mlt_multitrack_close, NULL );
			mlt_properties_set_data( props, "field", field, 0, ( mlt_destructor )mlt_field_close, NULL );

			mlt_events_listen( MLT_MULTITRACK_PROPERTIES( multitrack ), this, "producer-changed", ( mlt_listener )mlt_tractor_listener );

			producer->get_frame = producer_get_frame;
			producer->close = ( mlt_destructor )mlt_tractor_close;
			producer->close_object = this;
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
	return MLT_PRODUCER_SERVICE( &this->parent );
}

/** Get the producer object associated to the tractor.
*/

mlt_producer mlt_tractor_producer( mlt_tractor this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Get the properties object associated to the tractor.
*/

mlt_properties mlt_tractor_properties( mlt_tractor this )
{
	return MLT_PRODUCER_PROPERTIES( &this->parent );
}

/** Get the field this tractor is harvesting.
*/

mlt_field mlt_tractor_field( mlt_tractor this )
{
	return mlt_properties_get_data( MLT_TRACTOR_PROPERTIES( this ), "field", NULL );
}

/** Get the multitrack this tractor is pulling.
*/

mlt_multitrack mlt_tractor_multitrack( mlt_tractor this )
{
	return mlt_properties_get_data( MLT_TRACTOR_PROPERTIES( this ), "multitrack", NULL );
}

/** Ensure the tractors in/out points match the multitrack.
*/

void mlt_tractor_refresh( mlt_tractor this )
{
	mlt_multitrack multitrack = mlt_tractor_multitrack( this );
	mlt_properties properties = MLT_MULTITRACK_PROPERTIES( multitrack );
	mlt_properties self = MLT_TRACTOR_PROPERTIES( this );
	mlt_events_block( properties, self );
	mlt_events_block( self, self );
	mlt_multitrack_refresh( multitrack );
	mlt_properties_set_position( self, "in", 0 );
	mlt_properties_set_position( self, "out", mlt_properties_get_position( properties, "out" ) );
	mlt_events_unblock( self, self );
	mlt_events_unblock( properties, self );
	mlt_properties_set_position( self, "length", mlt_properties_get_position( properties, "length" ) );
}

static void mlt_tractor_listener( mlt_multitrack tracks, mlt_tractor this )
{
	mlt_tractor_refresh( this );
}

/** Connect the tractor.
*/

int mlt_tractor_connect( mlt_tractor this, mlt_service producer )
{
	int ret = mlt_service_connect_producer( MLT_TRACTOR_SERVICE( this ), producer, 0 );

	// This is the producer we're going to connect to
	if ( ret == 0 )
		this->producer = producer;

	return ret;
}

/** Set the producer for a specific track.
*/

int mlt_tractor_set_track( mlt_tractor this, mlt_producer producer, int index )
{
	return mlt_multitrack_connect( mlt_tractor_multitrack( this ), producer, index );
}

/** Get the producer for a specific track.
*/

mlt_producer mlt_tractor_get_track( mlt_tractor this, int index )
{
	return mlt_multitrack_track( mlt_tractor_multitrack( this ), index );
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	uint8_t *data = NULL;
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );
	mlt_frame frame = mlt_frame_pop_service( this );
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_properties_set_int( frame_properties, "width", mlt_properties_get_int( properties, "width" ) );
	mlt_properties_set_int( frame_properties, "height", mlt_properties_get_int( properties, "height" ) );
	mlt_properties_set( frame_properties, "rescale.interp", mlt_properties_get( properties, "rescale.interp" ) );
	mlt_properties_set_int( frame_properties, "distort", mlt_properties_get_int( properties, "distort" ) );
	mlt_properties_set_double( frame_properties, "consumer_aspect_ratio", mlt_properties_get_double( properties, "consumer_aspect_ratio" ) );
	mlt_properties_set_int( frame_properties, "consumer_deinterlace", mlt_properties_get_double( properties, "consumer_deinterlace" ) );
	mlt_properties_set( frame_properties, "deinterlace_method", mlt_properties_get( properties, "deinterlace_method" ) );
	mlt_properties_set_int( frame_properties, "normalised_width", mlt_properties_get_double( properties, "normalised_width" ) );
	mlt_properties_set_int( frame_properties, "normalised_height", mlt_properties_get_double( properties, "normalised_height" ) );
	mlt_frame_get_image( frame, buffer, format, width, height, writable );
	mlt_properties_set_data( properties, "image", *buffer, *width * *height * 2, NULL, NULL );
	mlt_properties_set_int( properties, "width", *width );
	mlt_properties_set_int( properties, "height", *height );
	mlt_properties_set_double( properties, "aspect_ratio", mlt_frame_get_aspect_ratio( frame ) );
	mlt_properties_set_int( properties, "progressive", mlt_properties_get_int( frame_properties, "progressive" ) );
	mlt_properties_set_int( properties, "distort", mlt_properties_get_int( frame_properties, "distort" ) );
	data = mlt_frame_get_alpha_mask( frame );
	mlt_properties_set_data( properties, "alpha", data, 0, NULL, NULL );
	return 0;
}

static int producer_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );
	mlt_frame frame = mlt_frame_pop_audio( this );
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	mlt_properties_set_data( properties, "audio", *buffer, 0, NULL, NULL );
	mlt_properties_set_int( properties, "frequency", *frequency );
	mlt_properties_set_int( properties, "channels", *channels );
	return 0;
}

static void destroy_data_queue( void *arg )
{
	if ( arg != NULL )
	{
		// Assign the correct type
		mlt_deque queue = arg;

		// Iterate through each item and destroy them
		while ( mlt_deque_peek_front( queue ) != NULL )
			mlt_properties_close( mlt_deque_pop_back( queue ) );

		// Close the deque
		mlt_deque_close( queue );
	}
}

/** Get the next frame.
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int track )
{
	mlt_tractor this = parent->child;

	// We only respond to the first track requests
	if ( track == 0 && this->producer != NULL )
	{
		int i = 0;
		int done = 0;
		mlt_frame temp = NULL;
		int count = 0;

		// Get the properties of the parent producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( parent );

		// Try to obtain the multitrack associated to the tractor
		mlt_multitrack multitrack = mlt_properties_get_data( properties, "multitrack", NULL );

		// Or a specific producer
		mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );

		// The output frame will hold the 'global' data feeds (ie: those which are targetted for the final frame)
		mlt_deque data_queue = mlt_deque_init( );

		// Determine whether this tractor feeds to the consumer or stops here
		int global_feed = mlt_properties_get_int( properties, "global_feed" );

		// If we don't have one, we're in trouble... 
		if ( multitrack != NULL )
		{
			// Used to garbage collect all frames
			char label[ 30 ];

			// Get the id of the tractor
			char *id = mlt_properties_get( properties, "_unique_id" );

			// Will be used to store the frame properties object
			mlt_properties frame_properties = NULL;

			// We'll store audio and video frames to use here
			mlt_frame audio = NULL;
			mlt_frame video = NULL;

			// Temporary properties
			mlt_properties temp_properties = NULL;

			// Get the multitrack's producer
			mlt_producer target = MLT_MULTITRACK_PRODUCER( multitrack );
			mlt_producer_seek( target, mlt_producer_frame( parent ) );
			mlt_producer_set_speed( target, mlt_producer_get_speed( parent ) );

			// We will create one frame and attach everything to it
			*frame = mlt_frame_init( );

			// Get the properties of the frame
			frame_properties = MLT_FRAME_PROPERTIES( *frame );

			// Loop through each of the tracks we're harvesting
			for ( i = 0; !done; i ++ )
			{
				// Get a frame from the producer
				mlt_service_get_frame( this->producer, &temp, i );

				// Get the temporary properties
				temp_properties = MLT_FRAME_PROPERTIES( temp );

				// Check for last track
				done = mlt_properties_get_int( temp_properties, "last_track" );

				// We store all frames with a destructor on the output frame
				sprintf( label, "_%s_%d", id, count ++ );
				mlt_properties_set_data( frame_properties, label, temp, 0, ( mlt_destructor )mlt_frame_close, NULL );

				// We want to append all 'final' feeds to the global queue
				if ( !done && mlt_properties_get_data( temp_properties, "data_queue", NULL ) != NULL )
				{
					// Move the contents of this queue on to the output frames data queue
					mlt_deque sub_queue = mlt_properties_get_data( MLT_FRAME_PROPERTIES( temp ), "data_queue", NULL );
					mlt_deque temp = mlt_deque_init( );
					while ( global_feed && mlt_deque_count( sub_queue ) )
					{
						mlt_properties p = mlt_deque_pop_back( sub_queue );
						if ( mlt_properties_get_int( p, "final" ) )
							mlt_deque_push_back( data_queue, p );
						else
							mlt_deque_push_back( temp, p );
					}
					while( mlt_deque_count( temp ) )
						mlt_deque_push_front( sub_queue, mlt_deque_pop_back( temp ) );
					mlt_deque_close( temp );
				}

				// Now do the same with the global queue but without the conditional behaviour
				if ( mlt_properties_get_data( temp_properties, "global_queue", NULL ) != NULL )
				{
					mlt_deque sub_queue = mlt_properties_get_data( MLT_FRAME_PROPERTIES( temp ), "global_queue", NULL );
					while ( mlt_deque_count( sub_queue ) )
					{
						mlt_properties p = mlt_deque_pop_back( sub_queue );
						mlt_deque_push_back( data_queue, p );
					}
				}

				// Pick up first video and audio frames
				if ( !done && !mlt_frame_is_test_audio( temp ) && !( mlt_properties_get_int( temp_properties, "hide" ) & 2 ) )
					audio = temp;
				if ( !done && !mlt_frame_is_test_card( temp ) && !( mlt_properties_get_int( temp_properties, "hide" ) & 1 ) )
					video = temp;
			}
	
			// Now stack callbacks
			if ( audio != NULL )
			{
				mlt_frame_push_audio( *frame, audio );
				mlt_frame_push_audio( *frame, producer_get_audio );
			}

			if ( video != NULL )
			{
				mlt_properties video_properties = MLT_FRAME_PROPERTIES( video );
				mlt_frame_push_service( *frame, video );
				mlt_frame_push_service( *frame, producer_get_image );
				if ( global_feed )
					mlt_properties_set_data( frame_properties, "data_queue", data_queue, 0, NULL, NULL );
				mlt_properties_set_data( video_properties, "global_queue", data_queue, 0, destroy_data_queue, NULL );
				mlt_properties_set_int( frame_properties, "width", mlt_properties_get_int( video_properties, "width" ) );
				mlt_properties_set_int( frame_properties, "height", mlt_properties_get_int( video_properties, "height" ) );
				mlt_properties_set_int( frame_properties, "real_width", mlt_properties_get_int( video_properties, "real_width" ) );
				mlt_properties_set_int( frame_properties, "real_height", mlt_properties_get_int( video_properties, "real_height" ) );
				mlt_properties_set_int( frame_properties, "progressive", mlt_properties_get_int( video_properties, "progressive" ) );
				mlt_properties_set_double( frame_properties, "aspect_ratio", mlt_properties_get_double( video_properties, "aspect_ratio" ) );
			}
			else
			{
				destroy_data_queue( data_queue );
			}

			mlt_frame_set_position( *frame, mlt_producer_frame( parent ) );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_audio", audio == NULL );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_image", video == NULL );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( *frame ), "consumer_lock_service", this, 0, NULL, NULL );
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
	if ( this != NULL && mlt_properties_dec_ref( MLT_TRACTOR_PROPERTIES( this ) ) <= 0 )
	{
		this->parent.close = NULL;
		mlt_producer_close( &this->parent );
		free( this );
	}
}

