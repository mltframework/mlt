/*
 * filter_affine.c -- affine filter
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

#include "filter_affine.h"

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
	mlt_properties properties = mlt_filter_properties( filter );

	// Get the image
	int error = mlt_frame_get_image( this, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );
		mlt_transition transition = mlt_properties_get_data( properties, "transition", NULL );
		mlt_frame a_frame = NULL;

		if ( producer == NULL )
		{
			char *background = mlt_properties_get( properties, "background" );
			producer = mlt_factory_producer( "fezzik", background );
			mlt_properties_set_data( properties, "producer", producer, 0, (mlt_destructor)mlt_producer_close, NULL );
		}

		if ( transition == NULL )
		{
			transition = mlt_factory_transition( "affine", NULL );
			mlt_properties_set_data( properties, "transition", transition, 0, (mlt_destructor)mlt_transition_close, NULL );
		}

		if ( producer != NULL && transition != NULL )
		{
			mlt_properties frame_properties = mlt_frame_properties( this );
			mlt_properties_pass( mlt_producer_properties( producer ), properties, "producer." );
			mlt_properties_pass( mlt_transition_properties( transition ), properties, "transition." );
			mlt_service_get_frame( mlt_producer_service( producer ), &a_frame, 0 );
			mlt_properties_set( mlt_frame_properties( a_frame ), "rescale_interp", "nearest" );
			mlt_properties_set( mlt_frame_properties( a_frame ), "distort", "true" );
			mlt_properties_set_double( mlt_frame_properties( a_frame ), "consumer_aspect_ratio", 
									   mlt_properties_get_double( frame_properties, "consumer_aspect_ratio" ) );
			mlt_transition_process( transition, a_frame, this );
			mlt_frame_get_image( a_frame, image, format, width, height, writable );
			mlt_properties_set_data( frame_properties, "affine_frame", a_frame, 0, (mlt_destructor)mlt_frame_close, NULL );
			mlt_properties_set_data( frame_properties, "image", *image, *width * *height * 2, NULL, NULL );
		}
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Push the frame filter
	mlt_frame_push_service( frame, this );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_affine_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( mlt_filter_properties( this ), "background", "colour:black" );
		mlt_properties_set( mlt_filter_properties( this ), "transition.rotate", "10" );
	}
	return this;
}


