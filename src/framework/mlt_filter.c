/*
 * mlt_filter.c -- abstraction for all filter services
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "mlt_filter.h"
#include "mlt_frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int filter_get_frame( mlt_service this, mlt_frame_ptr frame, int index );

/** Initialize a new filter.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \param child the object of a subclass
 * \return true if there was an error
 */

int mlt_filter_init( mlt_filter this, void *child )
{
	mlt_service service = &this->parent;
	memset( this, 0, sizeof( struct mlt_filter_s ) );
	this->child = child;
	if ( mlt_service_init( service, this ) == 0 )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

		// Override the get_frame method
		service->get_frame = filter_get_frame;

		// Define the destructor
		service->close = ( mlt_destructor )mlt_filter_close;
		service->close_object = this;

		// Default in, out, track properties
		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 0 );
		mlt_properties_set_int( properties, "track", 0 );

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
	mlt_filter this = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( this != NULL )
		mlt_filter_init( this, NULL );
	return this;
}

/** Get the service class interface.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \return the service parent class
 * \see MLT_FILTER_SERVICE
 */

mlt_service mlt_filter_service( mlt_filter this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Get the filter properties.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \return the properties list for the filter
 * \see MLT_FILTER_PROPERTIES
 */

mlt_properties mlt_filter_properties( mlt_filter this )
{
	return MLT_SERVICE_PROPERTIES( MLT_FILTER_SERVICE( this ) );
}

/** Connect this filter to a producers track. Note that a filter only operates
 * on a single track, and by default it operates on the entirety of that track.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \param producer the producer to which to connect this filter
 * \param index which of potentially multiple producers to this service (0 based)
 */

int mlt_filter_connect( mlt_filter this, mlt_service producer, int index )
{
	int ret = mlt_service_connect_producer( &this->parent, producer, index );

	// If the connection was successful, grab the producer, track and reset in/out
	if ( ret == 0 )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( &this->parent );
		mlt_properties_set_position( properties, "in", 0 );
		mlt_properties_set_position( properties, "out", 0 );
		mlt_properties_set_int( properties, "track", index );
	}

	return ret;
}

/** Set the starting and ending time.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \param in the time relative to the producer at which start applying the filter
 * \param out the time relative to the producer at which to stop applying the filter
 */


void mlt_filter_set_in_and_out( mlt_filter this, mlt_position in, mlt_position out )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &this->parent );
	mlt_properties_set_position( properties, "in", in );
	mlt_properties_set_position( properties, "out", out );
}

/** Return the track that this filter is operating on.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \return true on error
 */


int mlt_filter_get_track( mlt_filter this )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &this->parent );
	return mlt_properties_get_int( properties, "track" );
}

/** Get the in point.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \return the start time for the filter relative to the producer
 */


mlt_position mlt_filter_get_in( mlt_filter this )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &this->parent );
	return mlt_properties_get_position( properties, "in" );
}

/** Get the out point.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \return the ending time for the filter relative to the producer
 */


mlt_position mlt_filter_get_out( mlt_filter this )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( &this->parent );
	return mlt_properties_get_position( properties, "out" );
}

/** Process the frame.
 *
 * \public \memberof mlt_filter_s
 * \param this a filter
 * \param frame a frame
 * \return a frame
 */


mlt_frame mlt_filter_process( mlt_filter this, mlt_frame frame )
{
	int disable = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "disable" );
	if ( disable || this->process == NULL )
		return frame;
	else
		return this->process( this, frame );
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
	mlt_filter this = service->child;

	// Get coords in/out/track
	int track = mlt_filter_get_track( this );
	int in = mlt_filter_get_in( this );
	int out = mlt_filter_get_out( this );

	// Get the producer this is connected to
	mlt_service producer = mlt_service_producer( &this->parent );

	// If the frame request is for this filters track, we need to process it
	if ( index == track || track == -1 )
	{
		int ret = mlt_service_get_frame( producer, frame, index );
		if ( ret == 0 )
		{
			mlt_position position = mlt_frame_get_position( *frame );
			if ( position >= in && ( out == 0 || position <= out ) )
				*frame = mlt_filter_process( this, *frame );
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
 * \param this a filter
 */


void mlt_filter_close( mlt_filter this )
{
	if ( this != NULL && mlt_properties_dec_ref( MLT_FILTER_PROPERTIES( this ) ) <= 0 )
	{
		if ( this->close != NULL )
		{
			this->close( this );
		}
		else
		{
			this->parent.close = NULL;
			mlt_service_close( &this->parent );
		}
		free( this );
	}
}
