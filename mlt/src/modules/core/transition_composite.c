/*
 * transition_composite.c -- compose one image over another using alpha channel
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "transition_composite.h"
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>

/** Composition class.
*/

typedef struct 
{
	struct mlt_transition_s parent;
}
transition_composite;

/** Get the image.
*/

static int transition_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties of the a frame
	mlt_properties a_props = mlt_frame_properties( this );

	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( this );

	if ( b_frame != NULL )
	{
		// Get the properties of the b frame
		mlt_properties b_props = mlt_frame_properties( b_frame );

		// Arbitrary composite defaults
		int x = 0;
		int y = 0;
		double mix = 1.0;

		// Override from b frame properties if provided
		if ( mlt_properties_get( b_props, "x" ) != NULL )
			x = mlt_properties_get_int( b_props, "x" );
		if ( mlt_properties_get( b_props, "y" ) != NULL )
			y = mlt_properties_get_int( b_props, "y" );
		if ( mlt_properties_get( b_props, "mix" ) != NULL )
			mix = mlt_properties_get_double( b_props, "mix" );

		// Composite the b_frame on the a_frame
		mlt_frame_composite_yuv( this, b_frame, x, y, mix );

		// Extract the a_frame image info
		*width = mlt_properties_get_int( a_props, "width" );
		*height = mlt_properties_get_int( a_props, "height" );
		*image = mlt_properties_get_data( a_props, "image", NULL );
	}
	else if ( a_props != NULL )
	{
		// Extract the a_frame image info
		*width = mlt_properties_get_int( a_props, "width" );
		*height = mlt_properties_get_int( a_props, "height" );
		*image = mlt_properties_get_data( a_props, "image", NULL );
	}

	return 0;
}

/** Composition transition processing.
*/

static mlt_frame composite_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_frame_push_get_image( a_frame, transition_get_image );
	mlt_frame_push_frame( a_frame, b_frame );
	
	// Propogate the transition properties to the b frame
	mlt_properties properties = mlt_transition_properties( this );
	mlt_properties b_props = mlt_frame_properties( b_frame );
	if ( mlt_properties_get( properties, "x" ) != NULL )
		mlt_properties_set_int( b_props, "x", mlt_properties_get_int( properties, "x" ) );
	if ( mlt_properties_get( properties, "y" ) != NULL )
		mlt_properties_set_int( b_props, "y", mlt_properties_get_int( properties, "y" ) );
	if ( mlt_properties_get( properties, "mix" ) != NULL )
		mlt_properties_set_double( b_props, "mix",  mlt_properties_get_double( properties, "mix" ) );

	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_composite_init( void *arg )
{
	transition_composite *this = calloc( sizeof( transition_composite ), 1 );
	if ( this != NULL )
	{
		mlt_transition transition = &this->parent;
		mlt_transition_init( transition, this );
		transition->process = composite_process;
		return &this->parent;
	}
	return NULL;
}

