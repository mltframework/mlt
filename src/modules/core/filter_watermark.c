/*
 * filter_watermark.c -- watermark filter
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

#include "filter_watermark.h"

#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_transition.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Error we will return
	int error = 0;

	// Get the watermark filter object
	mlt_filter this = mlt_frame_pop_service( frame );

	// Get the properties of the filter
	mlt_properties properties = MLT_FILTER_PROPERTIES( this );

	// Get the producer from the filter
	mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );

	// Get the composite from the filter
	mlt_transition composite = mlt_properties_get_data( properties, "composite", NULL );

	// Get the resource to use
	char *resource = mlt_properties_get( properties, "resource" );

	// Get the old resource
	char *old_resource = mlt_properties_get( properties, "_old_resource" );

	// Create a composite if we don't have one
	if ( composite == NULL )
	{
		// Create composite via the factory
		composite = mlt_factory_transition( "composite", NULL );

		// Register the composite for reuse/destruction
		if ( composite != NULL )
			mlt_properties_set_data( properties, "composite", composite, 0, ( mlt_destructor )mlt_transition_close, NULL );
	}

	// If we have one
	if ( composite != NULL )
	{
		// Get the properties
		mlt_properties composite_properties = MLT_TRANSITION_PROPERTIES( composite );

		// Pass all the composite. properties on the filter down
		mlt_properties_pass( composite_properties, properties, "composite." );

		// Force a refresh
		mlt_properties_set_int( composite_properties, "refresh", 1 );
	}

	// Create a producer if don't have one
	if ( producer == NULL || ( old_resource != NULL && strcmp( resource, old_resource ) ) )
	{
		// Get the factory producer service
		char *factory = mlt_properties_get( properties, "factory" );

		// Create the producer
		producer = mlt_factory_producer( factory, resource );

		// If we have one
		if ( producer != NULL )
		{
			// Register the producer for reuse/destruction
			mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

			// Ensure that we loop
			mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );

			// Set the old resource
			mlt_properties_set( properties, "_old_resource", resource );
		}
	}

	if ( producer != NULL )
	{
		// Get the producer properties
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

		// Now pass all producer. properties on the filter down
		mlt_properties_pass( producer_properties, properties, "producer." );
	}

	// Only continue if we have both producer and composite
	if ( composite != NULL && producer != NULL )
	{
		// Get the service of the producer
		mlt_service service = MLT_PRODUCER_SERVICE( producer );

		// We will get the 'b frame' from the producer
		mlt_frame b_frame = NULL;

		// Get the unique id of the filter (used to reacquire the producer position)
		char *name = mlt_properties_get( properties, "_unique_id" );

		// Get the original producer position
		mlt_position position = mlt_properties_get_position( MLT_FRAME_PROPERTIES( frame ), name );

		// Make sure the producer is in the correct position
		mlt_producer_seek( producer, position );

		// Resetting position to appease the composite transition
		mlt_frame_set_position( frame, position );

		// Get the b frame and process with composite if successful
		if ( mlt_service_get_frame( service, &b_frame, 0 ) == 0 )
		{
			// Get the a and b frame properties
			mlt_properties a_props = MLT_FRAME_PROPERTIES( frame );
			mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

			// Set the b frame to be in the same position and have same consumer requirements
			mlt_frame_set_position( b_frame, position );
			mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );
			mlt_properties_set_int( b_props, "consumer_deinterlace", mlt_properties_get_double( a_props, "consumer_deinterlace" ) );

			mlt_properties_set_int( b_props, "normalised_width", mlt_properties_get_int( a_props, "normalised_width" ) );
			mlt_properties_set_int( b_props, "normalised_height", mlt_properties_get_int( a_props, "normalised_height" ) );

			if ( mlt_properties_get_int( properties, "distort" ) )
			{
				mlt_properties_set( MLT_TRANSITION_PROPERTIES( composite ), "distort", "true" );
				mlt_properties_set( a_props, "distort", "true" );
				mlt_properties_set( b_props, "distort", "true" );
			}

			if ( mlt_properties_get_int( properties, "reverse" ) == 0 )
			{
				// Apply all filters that are attached to this filter to the b frame
				mlt_service_apply_filters( MLT_FILTER_SERVICE( this ), b_frame, 0 );

				// Process the frame
				mlt_transition_process( composite, frame, b_frame );

				// Get the image
				error = mlt_frame_get_image( frame, image, format, width, height, 1 );
			}
			else
			{
				char temp[ 132 ];
				int count = 0;
				uint8_t *alpha = NULL;
				mlt_transition_process( composite, b_frame, frame );
				mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_int( a_props, "consumer_aspect_ratio" ) );
				mlt_properties_set_int( a_props, "consumer_deinterlace", 1 );
				mlt_properties_set_int( b_props, "consumer_deinterlace", 1 );
				mlt_properties_set( a_props, "rescale.interp", "nearest" );
				mlt_properties_set( b_props, "rescale.interp", "nearest" );
				mlt_service_apply_filters( MLT_FILTER_SERVICE( this ), b_frame, 0 );
				error = mlt_frame_get_image( b_frame, image, format, width, height, 1 );
				alpha = mlt_frame_get_alpha_mask( b_frame );
				mlt_properties_set_data( a_props, "image", *image, *width * *height * 2, NULL, NULL );
				mlt_properties_set_data( a_props, "alpha", alpha, *width * *height, NULL, NULL );
				mlt_properties_set_int( a_props, "width", *width );
				mlt_properties_set_int( a_props, "height", *height );
				mlt_properties_set_int( a_props, "progressive", 1 );
				mlt_properties_inc_ref( b_props );
				strcpy( temp, "_b_frame" );
				while( mlt_properties_get_data( a_props, temp, NULL ) != NULL )
					sprintf( temp, "_b_frame%d", count ++ );
				mlt_properties_set_data( a_props, temp, b_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
			}
		}

		// Close the b frame
		mlt_frame_close( b_frame );
	}
	else
	{
		// Get the image from the frame without running fx
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Get the properties of the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Get a unique name to store the frame position
	char *name = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "_unique_id" );

	// Assign the current position to the name
	mlt_properties_set_position( properties, name, mlt_frame_get_position( frame ) - mlt_filter_get_in( this ) );

	// Push the filter on to the stack
	mlt_frame_push_service( frame, this );

	// Push the get_image on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_watermark_init( void *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );
		this->process = filter_process;
		mlt_properties_set( properties, "factory", "fezzik" );
		if ( arg != NULL )
			mlt_properties_set( properties, "resource", arg );
		// Ensure that attached filters are handled privately
		mlt_properties_set_int( properties, "_filter_private", 1 );
	}
	return this;
}

