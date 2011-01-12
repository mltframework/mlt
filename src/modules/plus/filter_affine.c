/*
 * filter_affine.c -- affine filter
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

#include <framework/mlt_filter.h>
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter
	mlt_filter filter = mlt_frame_pop_service( this );

	// Get the properties
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	// Get the image
	int error = 0;
	*format = mlt_image_rgb24a;
	//mlt_frame_get_image( this, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );
		mlt_transition transition = mlt_properties_get_data( properties, "transition", NULL );
		mlt_frame a_frame = NULL;

		if ( producer == NULL )
		{
			char *background = mlt_properties_get( properties, "background" );
			mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
			producer = mlt_factory_producer( profile, NULL, background );
			mlt_properties_set_data( properties, "producer", producer, 0, (mlt_destructor)mlt_producer_close, NULL );
		}

		if ( transition == NULL )
		{
			mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
			transition = mlt_factory_transition( profile, "affine", NULL );
			mlt_properties_set_data( properties, "transition", transition, 0, (mlt_destructor)mlt_transition_close, NULL );
		}

		if ( producer != NULL && transition != NULL )
		{
			char *name = mlt_properties_get( properties, "_unique_id" );
			mlt_position position = mlt_properties_get_position( MLT_FRAME_PROPERTIES( this ), name );
			mlt_properties frame_properties = MLT_FRAME_PROPERTIES( this );
			mlt_position in = mlt_filter_get_in( filter );
			mlt_position out = mlt_filter_get_out( filter );
			double consumer_ar = mlt_properties_get_double( frame_properties, "consumer_aspect_ratio" );
			mlt_transition_set_in_and_out( transition, in, out );
			if ( out > 0 ) {
				mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( producer ), "length", out - in + 1 );
				mlt_producer_set_in_and_out( producer, in, out );
			}
			mlt_producer_seek( producer, position - in );
			mlt_frame_set_position( this, position );
			mlt_properties_pass( MLT_PRODUCER_PROPERTIES( producer ), properties, "producer." );
			mlt_properties_pass( MLT_TRANSITION_PROPERTIES( transition ), properties, "transition." );
			mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &a_frame, 0 );
			mlt_frame_set_position( a_frame, position );
//			mlt_properties_set_int( MLT_FRAME_PROPERTIES( a_frame ), "distort", 1 );

			// Special case - aspect_ratio = 0
			if ( mlt_properties_get_double( frame_properties, "aspect_ratio" ) == 0 )
				mlt_properties_set_double( frame_properties, "aspect_ratio", consumer_ar );
			if ( mlt_properties_get_double( MLT_FRAME_PROPERTIES( a_frame ), "aspect_ratio" ) == 0 )
				mlt_properties_set_double( MLT_FRAME_PROPERTIES( a_frame ), "aspect_ratio", consumer_ar );
			mlt_properties_set_double( MLT_FRAME_PROPERTIES( a_frame ), "consumer_aspect_ratio", consumer_ar );

			// Add the affine transition onto the frame stack
			mlt_transition_process( transition, a_frame, this );

			if (mlt_properties_get_int( properties, "use_normalised" ))
			{
				// Use the normalised width & height from the a_frame
				*width = mlt_properties_get_int( MLT_FRAME_PROPERTIES( a_frame ), "normalised_width" );
				*height = mlt_properties_get_int( MLT_FRAME_PROPERTIES( a_frame ), "normalised_height" );
			}
			
			mlt_frame_get_image( a_frame, image, format, width, height, writable );
			mlt_properties_set_data( frame_properties, "affine_frame", a_frame, 0, (mlt_destructor)mlt_frame_close, NULL );
			mlt_properties_set_data( frame_properties, "image", *image, *width * *height * 4, NULL, NULL );
			mlt_properties_set_data( frame_properties, "alpha", mlt_frame_get_alpha_mask( a_frame ), *width * *height, NULL, NULL );
		}
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
	mlt_properties_set_position( properties, name, mlt_frame_get_position( frame ) );

	// Push the frame filter
	mlt_frame_push_service( frame, this );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_affine_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "background", "colour:black" );
	}
	return this;
}


