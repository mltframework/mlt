/**
 * \file mlt_filter.c
 * \brief abstraction for all filter services
 * \see mlt_filter_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mlt_filter.h"
#include "mlt_frame.h"
#include "mlt_producer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int filter_get_frame( mlt_service self, mlt_frame_ptr frame, int index );

/** Initialize a new filter.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param child the object of a subclass
 * \return true if there was an error
 */

int mlt_filter_init( mlt_filter self, void *child )
{
	mlt_service service = &self->parent;
	memset( self, 0, sizeof( struct mlt_filter_s ) );
	self->child = child;
	if ( mlt_service_init( service, self ) == 0 )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

		// Override the get_frame method
		service->get_frame = filter_get_frame;

		// Define the destructor
		service->close = ( mlt_destructor )mlt_filter_close;
		service->close_object = self;

		// Default in, out, track properties
		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 0 );

		return 0;
	}
	return 1;
}

/** Create a new filter and initialize it.
 *
 * \public \memberof mlt_filter_s
 * \return a new filter
 */

mlt_filter mlt_filter_new( )
{
	mlt_filter self = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( self != NULL && mlt_filter_init( self, NULL ) == 0 )
	{
		return self;
	}
	else
	{
		free(self);
		return NULL;
	}
}

/** Get the service class interface.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \return the service parent class
 * \see MLT_FILTER_SERVICE
 */

mlt_service mlt_filter_service( mlt_filter self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Get the filter properties.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \return the properties list for the filter
 * \see MLT_FILTER_PROPERTIES
 */

mlt_properties mlt_filter_properties( mlt_filter self )
{
	return MLT_SERVICE_PROPERTIES( MLT_FILTER_SERVICE( self ) );
}

/** Connect this filter to a producers track. Note that a filter only operates
 * on a single track, and by default it operates on the entirety of that track.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param producer the producer to which to connect this filter
 * \param index which of potentially multiple producers to this service (0 based)
 */

int mlt_filter_connect( mlt_filter self, mlt_service producer, int index )
{
	int ret = mlt_service_connect_producer( &self->parent, producer, index );

	// If the connection was successful, grab the producer, track and reset in/out
	if ( ret == 0 )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 0 );
		mlt_properties_set_int( properties, "track", index );
	}

	return ret;
}

/** Set the starting and ending time.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param in the time relative to the producer at which start applying the filter
 * \param out the time relative to the producer at which to stop applying the filter
 */


void mlt_filter_set_in_and_out( mlt_filter self, mlt_position in, mlt_position out )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	mlt_properties_set_position( properties, "in", in );
	mlt_properties_set_position( properties, "out", out );
}

/** Return the track that this filter is operating on.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \return true on error
 */


int mlt_filter_get_track( mlt_filter self )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	return mlt_properties_get_int( properties, "track" );
}

/** Get the in point.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \return the start time for the filter relative to the producer
 */


mlt_position mlt_filter_get_in( mlt_filter self )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	return mlt_properties_get_position( properties, "in" );
}

/** Get the out point.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \return the ending time for the filter relative to the producer
 */


mlt_position mlt_filter_get_out( mlt_filter self )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	return mlt_properties_get_position( properties, "out" );
}

/** Get the duration.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \return the duration or zero if unlimited
 */

mlt_position mlt_filter_get_length( mlt_filter self )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out = mlt_properties_get_position( properties, "out" );
	return ( out > 0 ) ? ( out - in + 1 ) : 0;
}

/** Get the duration.
 *
 * This version works with filters with no explicit in and out by getting the
 * length of the frame's producer.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param frame a frame from which to get its producer
 * \return the duration or zero if unlimited
 */

mlt_position mlt_filter_get_length2( mlt_filter self, mlt_frame frame )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out = mlt_properties_get_position( properties, "out" );

	if ( out == 0 && frame )
	{
		// If always active, use the frame's producer
		mlt_producer producer = mlt_frame_get_original_producer( frame );
		if ( producer )
		{
			producer = mlt_producer_cut_parent( producer );
			in = mlt_producer_get_in( producer );
			out = mlt_producer_get_out( producer );
		}
	}
	return ( out > 0 ) ? ( out - in + 1 ) : 0;
}

/** Get the position within the filter.
 *
 * The position is relative to the in point.
 * This will only be valid once mlt_filter_process is called.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param frame a frame
 * \return the position
 */

mlt_position mlt_filter_get_position( mlt_filter self, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( self );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	const char *unique_id = mlt_properties_get( properties, "_unique_id" );
	char name[20];

	// Make the properties key from unique id
	snprintf( name, 20, "pos.%s", unique_id );
	name[20 - 1] = '\0';

	return mlt_properties_get_position( MLT_FRAME_PROPERTIES( frame ), name ) - in;
}

/** Get the percent complete.
 *
 * This will only be valid once mlt_filter_process is called.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param frame a frame
 * \return the progress in the range 0.0 to 1.0
 */

double mlt_filter_get_progress( mlt_filter self, mlt_frame frame )
{
	double position = mlt_filter_get_position( self, frame );
	double length = mlt_filter_get_length2( self, frame );
	return position / length;
}

/** Process the frame.
 *
 * When fetching the frame position in a subclass process method, the frame's
 * position is relative to the filter's producer - not the filter's in point
 * or timeline.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 * \param frame a frame
 * \return a frame
 */


mlt_frame mlt_filter_process( mlt_filter self, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( self );
	int disable = mlt_properties_get_int( properties, "disable" );
	const char *unique_id = mlt_properties_get( properties, "_unique_id" );
	mlt_position position = mlt_frame_get_position( frame );
	char name[30];

	// Make the properties key from unique id
	snprintf( name, sizeof(name), "pos.%s", unique_id );
	name[sizeof(name) -1] = '\0';

	// Save the position on the frame
	mlt_properties_set_position( MLT_FRAME_PROPERTIES( frame ), name, position );

	if ( disable || self->process == NULL )
	{
		return frame;
	}
	else
	{
		// Add a reference to this filter on the frame
		mlt_properties_inc_ref( MLT_FILTER_PROPERTIES(self) );
		snprintf( name, sizeof(name), "filter.%s", unique_id );
		name[sizeof(name) -1] = '\0';
		mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), name, self, 0,
			(mlt_destructor) mlt_filter_close, NULL );

		return self->process( self, frame );
	}
}

/** Get a frame from this filter.
 *
 * \private \memberof mlt_filter_s
 * \param service a service
 * \param[out] frame a frame by reference
 * \param index as determined by the producer
 * \return true on error
 */


static int filter_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	mlt_filter self = service->child;

	// Get coords in/out/track
	int track = mlt_filter_get_track( self );
	int in = mlt_filter_get_in( self );
	int out = mlt_filter_get_out( self );

	// Get the producer this is connected to
	mlt_service producer = mlt_service_producer( &self->parent );

	// If the frame request is for this filters track, we need to process it
	if ( index == track || track == -1 )
	{
		int ret = mlt_service_get_frame( producer, frame, index );
		if ( ret == 0 )
		{
			mlt_position position = mlt_frame_get_position( *frame );
			if ( position >= in && ( out == 0 || position <= out ) )
				*frame = mlt_filter_process( self, *frame );
			return 0;
		}
		else
		{
			*frame = mlt_frame_init( service );
			return 0;
		}
	}
	else
	{
		return mlt_service_get_frame( producer, frame, index );
	}
}

/** Close and destroy the filter.
 *
 * \public \memberof mlt_filter_s
 * \param self a filter
 */


void mlt_filter_close( mlt_filter self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_FILTER_PROPERTIES( self ) ) <= 0 )
	{
		if ( self->close != NULL )
		{
			self->close( self );
		}
		else
		{
			self->parent.close = NULL;
			mlt_service_close( &self->parent );
		}
		free( self );
	}
}
