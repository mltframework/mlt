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
#include "transition_composite.h"

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int create_instance( mlt_filter this, char *name, char *value, int count )
{
	// Return from this function
	int error = 0;

	// Duplicate the value
	char *type = strdup( value );

	// Pointer to filter argument
	char *arg = type == NULL ? NULL : strchr( type, ':' );

	// New filter being created
	mlt_filter filter = NULL;

	// Cleanup type and arg
	if ( arg != NULL )
		*arg ++ = '\0';

	// Create the filter
	filter = mlt_factory_filter( type, arg );

	// If we have a filter, then initialise and store it
	if ( filter != NULL )
	{
		// Properties of this
		mlt_properties properties = mlt_filter_properties( this );

		// String to hold the property name
		char id[ 256 ];

		// String to hold the passdown key
		char key[ 256 ];

		// Construct id
		sprintf( id, "_filter_%d", count );

		// Counstruct key
		sprintf( key, "%s.", name );

		// Just in case, let's assume that the filter here has a composite
		mlt_properties_set( mlt_filter_properties( filter ), "composite.start", "0%,0%:100%x100%" );
		mlt_properties_set( mlt_filter_properties( filter ), "composite.fill", "true" );

		// Pass all the key properties on the filter down
		mlt_properties_pass( mlt_filter_properties( filter ), properties, key );

		// Ensure that filter is assigned
		mlt_properties_set_data( properties, id, filter, 0, ( mlt_destructor )mlt_filter_close, NULL );
	}
	else
	{
		// Indicate that an error has occurred
		error = 1;
	}

	// Cleanup
	free( type );

	// Return error condition
	return error;
}

static uint8_t *filter_get_alpha_mask( mlt_frame this )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// Return the alpha mask
	return mlt_properties_get_data( properties, "alpha", NULL );
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Error we will return
	int error = 0;

	// Get the watermark filter object
	mlt_filter this = mlt_frame_pop_service( frame );

	// Get the properties of the filter
	mlt_properties properties = mlt_filter_properties( this );

	// Get the composite from the filter
	mlt_transition composite = mlt_properties_get_data( properties, "composite", NULL );

	// Look for the first filter
	mlt_filter filter = mlt_properties_get_data( properties, "_filter_0", NULL );

	// Get the unique id of the filter (used to reacquire the producer position)
	char *name = mlt_properties_get( properties, "_unique_id" );

	// Get the original producer position
	mlt_position position = mlt_properties_get_position( mlt_frame_properties( frame ), name );

	// Create a composite if we don't have one
	if ( composite == NULL )
	{
		// Create composite via the factory
		composite = mlt_factory_transition( "composite", NULL );

		// If we have one
		if ( composite != NULL )
		{
			// Get the properties
			mlt_properties composite_properties = mlt_transition_properties( composite );

			// We want to ensure that we don't get a wobble...
			mlt_properties_set( composite_properties, "distort", "true" );
			mlt_properties_set( composite_properties, "progressive", "1" );

			// Pass all the composite. properties on the filter down
			mlt_properties_pass( composite_properties, properties, "composite." );

			// Register the composite for reuse/destruction
			mlt_properties_set_data( properties, "composite", composite, 0, ( mlt_destructor )mlt_transition_close, NULL );
		}
	}

	// Create filters
	if ( filter == NULL )
	{
		// Loop Variable
		int i = 0;

		// Number of filters created
		int count = 0;

		// Loop for all properties
		for ( i = 0; i < mlt_properties_count( properties ); i ++ )
		{
			// Get the name of this property
			char *name = mlt_properties_get_name( properties, i );

			// If the name does not contain a . and matches filter
			if ( strchr( name, '.' ) == NULL && !strncmp( name, "filter", 6 ) )
			{
				// Get the filter constructor
				char *value = mlt_properties_get_value( properties, i );

				// Create an instance
				if ( create_instance( this, name, value, count ) == 0 )
					count ++;
			}
		}
	}

	// Get the image
	error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only continue if we have both filter and composite
	if ( composite != NULL && filter != NULL )
	{
		// Get the resource of this filter (could be a shape [rectangle/circle] or an alpha provider of choice
		char *resource =  mlt_properties_get( properties, "resource" );

		// String to hold the filter to query on
		char id[ 256 ];

		// Index to hold the count
		int i = 0;

		// We will get the 'b frame' from the composite
		mlt_frame b_frame = composite_copy_region( composite, frame, position );

		// Make sure the filter is in the correct position
		while ( filter != NULL )
		{
			// Stack this filter
			mlt_filter_process( filter, b_frame );

			// Generate the key for the next
			sprintf( id, "_filter_%d", ++ i );

			// Get the next filter
			filter = mlt_properties_get_data( properties, id, NULL );
		}

		// Hmm - this is probably going to go wrong....
		mlt_frame_set_position( frame, position );

		// Get the b frame and process with composite if successful
		mlt_transition_process( composite, frame, b_frame );

		// If we have a shape producer copy the alpha mask from the shape frame to the b_frame
		if ( strcmp( resource, "rectangle" ) != 0 )
		{
			// Get the producer from the filter
			mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );

			// If We have no producer then create one
			if ( producer == NULL )
			{
				// Get the factory producer service
				char *factory = mlt_properties_get( properties, "factory" );

				// Special case circle resource
				if ( strcmp( resource, "circle" ) == 0 )
				{
					// Special case to ensure that fezzik produces a pixbuf with a NULL constructor
					resource = "pixbuf";

					// Specify the svg circle
					mlt_properties_set( properties, "producer.resource", "<svg width='100' height='100'><circle cx='50' cy='50' r='50' fill='black'/></svg>" );
				}

				// Create the producer 
				producer = mlt_factory_producer( factory, resource );

				// If we have one
				if ( producer != NULL )
				{
					// Get the producer properties
					mlt_properties producer_properties = mlt_producer_properties( producer );

					// Ensure that we loop
					mlt_properties_set( producer_properties, "eof", "loop" );

					// Now pass all producer. properties on the filter down
					mlt_properties_pass( producer_properties, properties, "producer." );

					// Register the producer for reuse/destruction
					mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
				}
			}

			// Now use the shape producer
			if ( producer != NULL )
			{
				// We will get the alpha frame from the producer
				mlt_frame shape_frame = NULL;

				// Make sure the producer is in the correct position
				mlt_producer_seek( producer, position );

				// Get the shape frame
				if ( mlt_service_get_frame( mlt_producer_service( producer ), &shape_frame, 0 ) == 0 )
				{
					int region_width = mlt_properties_get_int( mlt_frame_properties( b_frame ), "width" );
					int region_height = mlt_properties_get_int( mlt_frame_properties( b_frame ), "height" );
					
					// Get the shape image to trigger alpha creation
					mlt_properties_set( mlt_frame_properties( shape_frame ), "distort", "true" );
					error = mlt_frame_get_image( shape_frame, image, format, &region_width, &region_height, 1 );

					// Only override any existing b_frame alpha if the shape has an alpha
					if ( mlt_frame_get_alpha_mask( shape_frame ) != NULL )
					{
						// Set the b_frame alpha from the shape frame
						mlt_properties_set_data( mlt_frame_properties( b_frame ), "alpha", mlt_frame_get_alpha_mask( shape_frame ), 0 , NULL, NULL );
						b_frame->get_alpha_mask = filter_get_alpha_mask;
					}
					
					// Ensure that the shape frame will be closed
					mlt_properties_set_data( mlt_frame_properties( b_frame ), "shape_frame", shape_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
				}
			}
		}

		// Get the image
		error = mlt_frame_get_image( frame, image, format, width, height, 0 );

		// Close the b frame
		mlt_frame_close( b_frame );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Get the properties of the frame
	mlt_properties properties = mlt_frame_properties( frame );

	// Get a unique name to store the frame position
	char *name = mlt_properties_get( mlt_filter_properties( this ), "_unique_id" );

	// Assign the current position to the name
	mlt_properties_set_position( properties, name, mlt_frame_get_position( frame ) );

	// Push the filter on to the frame
	mlt_frame_push_service( frame, this );

	// Push the filter method
	mlt_frame_push_get_image( frame, filter_get_image );

	// Return the frame
	return frame;
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

		mlt_properties_set( properties, "factory", "fezzik" );
		
		// Resource defines the shape of the region
		mlt_properties_set( properties, "resource", arg == NULL ? "rectangle" : arg );
	}

	// Return the filter
	return this;
}

