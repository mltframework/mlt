/*
 * mlt_field.c -- A field for planting multiple transitions and filters
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

#include "mlt_field.h"
#include "mlt_service.h"
#include "mlt_filter.h"
#include "mlt_transition.h"

#include <stdlib.h>
#include <string.h>

/** Private structures.
*/

struct mlt_field_s
{
	// We extending service here
	struct mlt_service_s parent;

	// This is the producer we're connected to
	mlt_service producer;
};

/** Forward declarations
*/

static int service_get_frame( mlt_service service, mlt_frame_ptr frame, int index );

/** Constructor. This service needs to know its producer at construction.

  	When the first filter or transition is planted, we connect it immediately to
	the producer of the field, and all subsequent plants are then connected to the
	previous plant. This immediate connection requires the producer prior to the first
	plant.
	
	It's a kind of arbitrary decsion - we could generate a dummy service here and 
	then connect the dummy in the normal connect manner. Behaviour may change in the
	future (say, constructing a dummy when producer service is NULL). However, the
	current solution is quite clean, if slightly out of sync with the rest of the
	mlt framework.
*/

mlt_field mlt_field_init( mlt_service producer )
{
	// Initialise the field
	mlt_field this = calloc( sizeof( struct mlt_field_s ), 1 );

	// Get the service
	mlt_service service = &this->parent;

	// Initialise the service
	mlt_service_init( service, this );

	// Override the get_frame method
	service->get_frame = service_get_frame;

	// Connect to the producer immediately
	if ( mlt_service_connect_producer( service, producer, 0 ) == 0 )
		this->producer = producer;

	// Return this
	return this;
}

/** Get the service associated to this field.
*/

mlt_service mlt_field_service( mlt_field this )
{
	return &this->parent;
}

/** Get the properties associated to this field.
*/

mlt_properties mlt_field_properties( mlt_field this )
{
	return mlt_service_properties( &this->parent );
}

/** Plant a filter.
*/

int mlt_field_plant_filter( mlt_field this, mlt_filter that, int track )
{
	// Connect the filter to the last producer
	int result = mlt_filter_connect( that, this->producer, track );

	// If sucessful, then we'll use this for connecting in the future
	if ( result == 0 )
	{
		// Update last
		this->producer = mlt_filter_service( that );
	}

	return result;
}

/** Plant a transition.
*/

int mlt_field_plant_transition( mlt_field this, mlt_transition that, int a_track, int b_track )
{
	// Connect the transition to the last producer
	int result = mlt_transition_connect( that, this->producer, a_track, b_track );

	// If sucessful, then we'll use this for connecting in the future
	if ( result == 0 )
	{
		// Update last
		this->producer = mlt_transition_service( that );
	}

	return 0;
}

/** Get a frame.
*/

static int service_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	mlt_field this = service->child;
	return mlt_service_get_frame( this->producer, frame, index );
}

/** Close the field.
*/

void mlt_field_close( mlt_field this )
{
	mlt_service_close( &this->parent );
	free( this );
}

