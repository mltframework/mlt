/*
 * filter_region.c -- region filter
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

#include "filter_region.h"
#include "transition_region.h"

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Get the properties of the filter
	mlt_properties properties = mlt_filter_properties( this );

	// Get the region transition
	mlt_transition transition = mlt_properties_get_data( properties, "_transition", NULL );

	// Create the transition if not available
	if ( transition == NULL )
	{
		// Create the transition
		transition = mlt_factory_transition( "region", NULL );

		// Register with the filter
		mlt_properties_set_data( properties, "_transition", transition, 0, ( mlt_destructor )mlt_transition_close, NULL );
	}

	// Pass all properties down
	mlt_properties_pass( mlt_transition_properties( transition ), properties, "" );

	// Process the frame
	return mlt_transition_process( transition, frame, NULL );
}

/** Constructor for the filter.
*/

mlt_filter filter_region_init( void *arg )
{
	// Create a new filter
	mlt_filter this = mlt_filter_new( );

	// Further initialisation
	if ( this != NULL )
	{
		// Get the properties from the filter
		mlt_properties properties = mlt_filter_properties( this );

		// Assign the filter process method
		this->process = filter_process;

		// Resource defines the shape of the region
		mlt_properties_set( properties, "resource", arg == NULL ? "rectangle" : arg );
	}

	// Return the filter
	return this;
}

