/**
 * \file mlt_transition.c
 * \brief abstraction for all transition services
 * \see mlt_transition_s
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

#include "mlt_transition.h"
#include "mlt_frame.h"
#include "mlt_log.h"
#include "mlt_producer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward references */

static int transition_get_frame( mlt_service self, mlt_frame_ptr frame, int index );

/** Initialize a new transition.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param child the object of a subclass
 * \return true on error
 */

int mlt_transition_init( mlt_transition self, void *child )
{
	mlt_service service = &self->parent;
	memset( self, 0, sizeof( struct mlt_transition_s ) );
	self->child = child;
	if ( mlt_service_init( service, self ) == 0 )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( self );

		service->get_frame = transition_get_frame;
		service->close = ( mlt_destructor )mlt_transition_close;
		service->close_object = self;

		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 0 );
		mlt_properties_set_int( properties, "a_track", 0 );
		mlt_properties_set_int( properties, "b_track", 1 );

		return 0;
	}
	return 1;
}

/** Create and initialize a new transition.
 *
 * \public \memberof mlt_transition_s
 * \return a new transition
 */

mlt_transition mlt_transition_new( )
{
	mlt_transition self = calloc( 1, sizeof( struct mlt_transition_s ) );
	if ( self != NULL )
		mlt_transition_init( self, NULL );
	return self;
}

/** Get the service class interface.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the service class
 * \see MLT_TRANSITION_SERVICE
 */

mlt_service mlt_transition_service( mlt_transition self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Get the properties interface.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the transition's properties
 * \see MLT_TRANSITION_PROPERTIES
 */

mlt_properties mlt_transition_properties( mlt_transition self )
{
	return MLT_TRANSITION_PROPERTIES( self );
}

/** Connect a transition with a producer's a and b tracks.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param producer a producer
 * \param a_track the track index of the first input
 * \param b_track the track index of the second index
 * \return true on error
 */

int mlt_transition_connect( mlt_transition self, mlt_service producer, int a_track, int b_track )
{
	int ret = mlt_service_connect_producer( &self->parent, producer, a_track );
	if ( ret == 0 )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( self );
		self->producer = producer;
		mlt_properties_set_int( properties, "a_track", a_track );
		mlt_properties_set_int( properties, "b_track", b_track );
	}
	return ret;
}

/** Set the starting and ending time for when the transition is active.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param in the starting time
 * \param out the ending time
 */

void mlt_transition_set_in_and_out( mlt_transition self, mlt_position in, mlt_position out )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( self );
	mlt_properties_set_position( properties, "in", in );
	mlt_properties_set_position( properties, "out", out );
}

/** Get the index of the a track.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the 0-based index of the track of the first producer
 */

int mlt_transition_get_a_track( mlt_transition self )
{
	return mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( self ), "a_track" );
}

/** Get the index of the b track.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the 0-based index of the track of the second producer
 */

int mlt_transition_get_b_track( mlt_transition self )
{
	return mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( self ), "b_track" );
}

/** Get the in point.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the starting time
 */

mlt_position mlt_transition_get_in( mlt_transition self )
{
	return mlt_properties_get_position( MLT_TRANSITION_PROPERTIES( self ), "in" );
}

/** Get the out point.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the ending time
 */

mlt_position mlt_transition_get_out( mlt_transition self )
{
	return mlt_properties_get_position( MLT_TRANSITION_PROPERTIES( self ), "out" );
}

/** Get the duration.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \return the duration or zero if unlimited
 */

mlt_position mlt_transition_get_length( mlt_transition self )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out = mlt_properties_get_position( properties, "out" );
	return ( out > 0 ) ? ( out - in + 1 ) : 0;
}

/** Get the position within the transition.
 *
 * The position is relative to the in point.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param frame a frame
 * \return the position
 */

mlt_position mlt_transition_get_position( mlt_transition self, mlt_frame frame )
{
	mlt_position in = mlt_transition_get_in( self );
	mlt_position position = mlt_frame_get_position( frame );
	return position - in;
}

/** Get the percent complete.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param frame a frame
 * \return the progress in the range 0.0 to 1.0
 */

double mlt_transition_get_progress( mlt_transition self, mlt_frame frame )
{
	double progress = 0;
	mlt_position in = mlt_transition_get_in( self );
	mlt_position out = mlt_transition_get_out( self );

	if ( out == 0 )
	{
		// If always active, use the frame's producer
		mlt_producer producer = mlt_frame_get_original_producer( frame );
		if ( producer )
		{
			in = mlt_producer_get_in( producer );
			out = mlt_producer_get_out( producer );
		}
	}
	if ( out != 0 )
	{
		mlt_position position = mlt_frame_get_position( frame );
		progress = ( double ) ( position - in ) / ( double ) ( out - in + 1 );
	}
	return progress;
}

/** Get the second field incremental progress.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param frame a frame
 * \return the progress increment in the range 0.0 to 1.0
 */

double mlt_transition_get_progress_delta( mlt_transition self, mlt_frame frame )
{
	double progress = 0;
	mlt_position in = mlt_transition_get_in( self );
	mlt_position out = mlt_transition_get_out( self );

	if ( out == 0 )
	{
		// If always active, use the frame's producer
		mlt_producer producer = mlt_frame_get_original_producer( frame );
		if ( producer )
		{
			in = mlt_producer_get_in( producer );
			out = mlt_producer_get_out( producer );
		}
	}
	if ( out != 0 )
	{
		mlt_position position = mlt_frame_get_position( frame );
		double length = out - in + 1;
		double x = ( double ) ( position - in ) / length;
		double y = ( double ) ( position + 1 - in ) / length;
		progress = length * ( y - x ) / 2.0;
	}
	return progress;
}

/** Process the frame.
 *
 * If we have no process method (unlikely), we simply return the a_frame unmolested.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 * \param a_frame a frame from the first producer
 * \param b_frame a frame from the second producer
 * \return a frame
 */

mlt_frame mlt_transition_process( mlt_transition self, mlt_frame a_frame, mlt_frame b_frame )
{
	if ( self->process == NULL )
		return a_frame;
	else
		return self->process( self, a_frame, b_frame );
}

static int get_image_a( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// All transitions get scaling
	const char *rescale = mlt_properties_get( a_props, "rescale.interp" );
	if ( !rescale || !strcmp( rescale, "none" ) )
		mlt_properties_set( a_props, "rescale.interp", "nearest" );

	// Ensure sane aspect ratio
	if ( mlt_properties_get_double( a_props, "aspect_ratio" ) == 0.0 )
		mlt_properties_set_double( a_props, "aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );

	return mlt_frame_get_image( a_frame, image, format, width, height, writable );
}

static int get_image_b( mlt_frame b_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_frame a_frame = mlt_frame_pop_frame( b_frame );
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// Set scaling from A frame if not already provided.
	if ( !mlt_properties_get( b_props, "rescale.interp" ) )
	{
		const char *rescale = mlt_properties_get( a_props, "rescale.interp" );
		if ( !rescale || !strcmp( rescale, "none" ) )
			rescale = "nearest";
		mlt_properties_set( b_props, "rescale.interp", rescale );
	}

	// Ensure sane aspect ratio
	if ( mlt_properties_get_double( b_props, "aspect_ratio" ) == 0.0 )
		mlt_properties_set_double( b_props, "aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );

	mlt_properties_pass_list( b_props, a_props,
		"consumer_deinterlace, deinterlace_method, consumer_aspect_ratio" );

	return mlt_frame_get_image( b_frame, image, format, width, height, writable );
}

/** Get a frame from a transition.

	The logic is complex here. A transition is typically applied to frames on the a and
	b tracks specified in the connect method above and only if both contain valid info
	for the transition type (this is either audio or image).

	However, the fixed a_track may not always contain data of the correct type, eg:
<pre>
	+---------+                               +-------+
	|c1       |                               |c5     | <-- A(0,1) <-- B(0,2) <-- get frame
	+---------+                     +---------+-+-----+        |          |
	                                |c4         |       <------+          |
	         +----------+-----------+-+---------+                         |
	         |c2        |c3           |                 <-----------------+
	         +----------+-------------+
</pre>
	During the overlap of c1 and c2, there is nothing for the A transition to do, so this
	results in a no operation, but B is triggered. During the overlap of c2 and c3, again,
	the A transition is inactive and because the B transition is pointing at track 0,
	it too would be inactive. This isn't an ideal situation - it's better if the B
	transition simply treats the frames from c3 as though they're the a track.

	For this to work, we cache all frames coming from all tracks between the a and b
	tracks.  Before we process, we determine that the b frame contains someting of the
	right type and then we determine which frame to use as the a frame (selecting a
	matching frame from a_track to b_track - 1). If both frames contain data of the
	correct type, we process the transition.

	This method is invoked for each track and we return the cached frames as needed.
	We clear the cache only when the requested frame is flagged as a 'last_track' frame.

 * \private \memberof mlt_transition_s
 * \param service a service
 * \param[out] frame a frame by reference
 * \param index 0-based track index
 * \return true on error
 */

static int transition_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	int error = 0;
	mlt_transition self = service->child;

	mlt_properties properties = MLT_TRANSITION_PROPERTIES( self );

	int accepts_blanks = mlt_properties_get_int( properties, "accepts_blanks" );
	int a_track = mlt_properties_get_int( properties, "a_track" );
	int b_track = mlt_properties_get_int( properties, "b_track" );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out = mlt_properties_get_position( properties, "out" );
	int always_active = mlt_properties_get_int( properties, "always_active" );
	int type = mlt_properties_get_int( properties, "_transition_type" );
	int reverse_order = 0;

	// Ensure that we have the correct order
	if ( a_track > b_track )
	{
		reverse_order = 1;
		a_track = b_track;
		b_track = mlt_properties_get_int( properties, "a_track" );
	}

	// Only act on this operation once per multitrack iteration from the tractor
	if ( !self->held )
	{
		int active = 0;
		int i = 0;
		int a_frame = a_track;
		int b_frame = b_track;
		mlt_position position;
		int ( *invalid )( mlt_frame ) = type == 1 ? mlt_frame_is_test_card : mlt_frame_is_test_audio;

		// Initialise temporary store
		if ( self->frames == NULL )
			self->frames = calloc( sizeof( mlt_frame ), b_track + 1 );

		// Get all frames between a and b
		for( i = a_track; i <= b_track; i ++ )
			mlt_service_get_frame( self->producer, &self->frames[ i ], i );

		// We're holding these frames until the last_track frame property is received
		self->held = 1;

		// When we need to locate the a_frame
		switch( type )
		{
			case 1:
			case 2:
				// Some transitions (esp. audio) may accept blank frames
				active = accepts_blanks;

				// If we're not active then...
				if ( !active )
				{
					// Hunt for the a_frame
					while( a_frame <= b_frame && invalid( self->frames[ a_frame ] ) )
						a_frame ++;

					// Determine if we're active now
					active = a_frame != b_frame && !invalid( self->frames[ b_frame ] );
				}
				break;

			default:
				mlt_log( service, MLT_LOG_ERROR, "invalid transition type\n" );
				break;
		}

		// Now handle the non-always active case
		if ( active && !always_active )
		{
			// For non-always-active transitions, we need the current position of the a frame
			position = mlt_frame_get_position( self->frames[ a_frame ] );

			// If a is in range, we're active
			active = position >= in && ( out == 0 || position <= out );
		}

		// Finally, process the a and b frames
		if ( active )
		{
			mlt_frame a_frame_ptr = self->frames[ !reverse_order ? a_frame : b_frame ];
			mlt_frame b_frame_ptr = self->frames[ !reverse_order ? b_frame : a_frame ];
			int a_hide = mlt_properties_get_int( MLT_FRAME_PROPERTIES( a_frame_ptr ), "hide" );
			int b_hide = mlt_properties_get_int( MLT_FRAME_PROPERTIES( b_frame_ptr ), "hide" );
			if ( !( a_hide & type ) && !( b_hide & type ) )
			{
				// Add hooks for pre-processing frames
				mlt_frame_push_get_image( a_frame_ptr, get_image_a );
				mlt_frame_push_frame( b_frame_ptr, a_frame_ptr );
				mlt_frame_push_get_image( b_frame_ptr, get_image_b );

				// Process the transition
				*frame = mlt_transition_process( self, a_frame_ptr, b_frame_ptr );

				// We need to ensure that the tractor doesn't consider this frame for output
				if ( *frame == a_frame_ptr )
					b_hide |= type;
				else
					a_hide |= type;

				mlt_properties_set_int( MLT_FRAME_PROPERTIES( a_frame_ptr ), "hide", a_hide );
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( b_frame_ptr ), "hide", b_hide );
			}
		}
	}

	// Obtain the frame from the cache or the producer we're attached to
	if ( index >= a_track && index <= b_track )
		*frame = self->frames[ index ];
	else
		error = mlt_service_get_frame( self->producer, frame, index );

	// Determine if that was the last track
	self->held = !mlt_properties_get_int( MLT_FRAME_PROPERTIES( *frame ), "last_track" );

	return error;
}

/** Close and destroy the transition.
 *
 * \public \memberof mlt_transition_s
 * \param self a transition
 */

void mlt_transition_close( mlt_transition self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_TRANSITION_PROPERTIES( self ) ) <= 0 )
	{
		self->parent.close = NULL;
		if ( self->close != NULL )
		{
			self->close( self );
		}
		else
		{
			mlt_service_close( &self->parent );
			free( self->frames );
			free( self );
		}
	}
}
