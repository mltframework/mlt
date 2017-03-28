/**
 * \file mlt_field.c
 * \brief a field for planting multiple transitions and filters
 * \see mlt_field_s
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

#include "mlt_field.h"
#include "mlt_service.h"
#include "mlt_filter.h"
#include "mlt_transition.h"
#include "mlt_multitrack.h"
#include "mlt_tractor.h"

#include <stdlib.h>
#include <string.h>

/** \brief Field class
 *
 * The field is a convenience class that works with the tractor and multitrack classes to manage track filters and transitions.
 */

struct mlt_field_s
{
	/// This is the producer we're connected to
	mlt_service producer;

	/// Multitrack
	mlt_multitrack multitrack;

	/// Tractor
	mlt_tractor tractor;
};

/** Construct a field, mulitrack, and tractor.
 *
 * \public \memberof mlt_field_s
 * \return a new field
 */

mlt_field mlt_field_init( )
{
	// Initialise the field
	mlt_field self = calloc( 1, sizeof( struct mlt_field_s ) );

	// Initialise it
	if ( self != NULL )
	{
		// Construct a multitrack
		self->multitrack = mlt_multitrack_init( );

		// Construct a tractor
		self->tractor = mlt_tractor_init( );

		// The first plant will be connected to the mulitrack
		self->producer = MLT_MULTITRACK_SERVICE( self->multitrack );

		// Connect the tractor to the multitrack
		mlt_tractor_connect( self->tractor, self->producer );
	}

	// Return self
	return self;
}

/** Construct a field and initialize with supplied multitrack and tractor.
 *
 * \public \memberof mlt_field_s
 * \param multitrack a multitrack
 * \param tractor a tractor
 * \return a new field
 */

mlt_field mlt_field_new( mlt_multitrack multitrack, mlt_tractor tractor )
{
	// Initialise the field
	mlt_field self = calloc( 1, sizeof( struct mlt_field_s ) );

	// Initialise it
	if ( self != NULL )
	{
		// Construct a multitrack
		self->multitrack = multitrack;

		// Construct a tractor
		self->tractor = tractor;

		// The first plant will be connected to the mulitrack
		self->producer = MLT_MULTITRACK_SERVICE( self->multitrack );

		// Connect the tractor to the multitrack
		mlt_tractor_connect( self->tractor, self->producer );
	}

	// Return self
	return self;
}

/** Get the service associated to this field.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \return the tractor as a service
 */

mlt_service mlt_field_service( mlt_field self )
{
	return MLT_TRACTOR_SERVICE( self->tractor );
}

/** Get the multitrack.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \return the multitrack
 */

mlt_multitrack mlt_field_multitrack( mlt_field self )
{
	return self != NULL ? self->multitrack : NULL;
}

/** Get the tractor.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \return the tractor
 */

mlt_tractor mlt_field_tractor( mlt_field self )
{
	return self != NULL ? self->tractor : NULL;
}

/** Get the properties associated to this field.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \return a properties list
 */

mlt_properties mlt_field_properties( mlt_field self )
{
	return MLT_SERVICE_PROPERTIES( mlt_field_service( self ) );
}

/** Plant a filter.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \param that a filter
 * \param track the track index
 * \return true if there was an error
 */

int mlt_field_plant_filter( mlt_field self, mlt_filter that, int track )
{
	// Connect the filter to the last producer
	int result = mlt_filter_connect( that, self->producer, track );

	// If sucessful, then we'll use this for connecting in the future
	if ( result == 0 )
	{
		// This is now the new producer
		self->producer = MLT_FILTER_SERVICE( that );

		// Reconnect tractor to new producer
		mlt_tractor_connect( self->tractor, self->producer );

		// Fire an event
		mlt_events_fire( mlt_field_properties( self ), "service-changed", NULL );
	}

	return result;
}

/** Plant a transition.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \param that a transition
 * \param a_track input A's track index
 * \param b_track input B's track index
 * \return true if there was an error
 */

int mlt_field_plant_transition( mlt_field self, mlt_transition that, int a_track, int b_track )
{
	// Connect the transition to the last producer
	int result = mlt_transition_connect( that, self->producer, a_track, b_track );

	// If sucessful, then we'll use self for connecting in the future
	if ( result == 0 )
	{
		// This is now the new producer
		self->producer = MLT_TRANSITION_SERVICE( that );

		// Reconnect tractor to new producer
		mlt_tractor_connect( self->tractor, self->producer );

		// Fire an event
		mlt_events_fire( mlt_field_properties( self ), "service-changed", NULL );
	}

	return result;
}

/** Close the field.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 */

void mlt_field_close( mlt_field self )
{
	if ( self != NULL && mlt_properties_dec_ref( mlt_field_properties( self ) ) <= 0 )
	{
		//mlt_tractor_close( self->tractor );
		//mlt_multitrack_close( self->multitrack );
		free( self );
	}
}

/** Remove a filter or transition from the field.
 *
 * \public \memberof mlt_field_s
 * \param self a field
 * \param service the filter or transition to remove
 */

void mlt_field_disconnect_service( mlt_field self, mlt_service service )
{
	mlt_service p = mlt_service_producer( service );
	mlt_service c = mlt_service_consumer( service);
	int i;
	switch ( mlt_service_identify(c) )
	{
		case filter_type:
			i = mlt_filter_get_track( MLT_FILTER(c) );
			mlt_service_connect_producer( c, p, i );
			break;
		case transition_type:
			i = mlt_transition_get_a_track ( MLT_TRANSITION(c) );
			mlt_service_connect_producer( c, p, i );
			MLT_TRANSITION(c)->producer = p;
			break;
		case tractor_type:
			self->producer = p;
			mlt_tractor_connect( MLT_TRACTOR(c), p );
		default:
			break;
	}
	mlt_events_fire( mlt_field_properties( self ), "service-changed", NULL );
}
