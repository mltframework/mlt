/*
 * filter_luma.c -- luma filter
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

#include "filter_luma.h"

#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_transition.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = mlt_frame_pop_service( this );
	mlt_properties properties = mlt_filter_properties( filter );
	mlt_transition luma = mlt_properties_get_data( properties, "luma", NULL );
	mlt_frame b_frame = mlt_properties_get_data( properties, "frame", NULL );
	int out = mlt_properties_get_int( properties, "period" );
	
	if ( out == 0 )
		out = 24;

	if ( luma == NULL )
	{
		char *resource = mlt_properties_get( properties, "resource" );
		luma = mlt_factory_transition( "luma", resource );
		if ( luma != NULL )
		{
			mlt_properties luma_properties = mlt_transition_properties( luma );
			mlt_properties_set_int( luma_properties, "in", 0 );
			mlt_properties_set_int( luma_properties, "out", out );
			mlt_properties_set_int( luma_properties, "reverse", 1 );
			mlt_properties_pass( luma_properties, properties, "luma." );
			mlt_properties_set_data( properties, "luma", luma, 0, ( mlt_destructor )mlt_transition_close, NULL );
		}
	}

	if ( b_frame == NULL )
	{
		b_frame = mlt_frame_init( );
		mlt_properties_set_data( properties, "frame", b_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
	}

	if ( luma != NULL && 
		( mlt_properties_get( properties, "blur" ) != NULL || 
		  mlt_frame_get_position( this ) % ( out + 1 ) != out ) )
		mlt_transition_process( luma, this, b_frame );

	error = mlt_frame_get_image( this, image, format, width, height, 1 );

	if ( error == 0 )
	{
		mlt_properties a_props = mlt_frame_properties( this );
		int size = 0;
		uint8_t *src = mlt_properties_get_data( a_props, "image", &size );
		uint8_t *dst = mlt_pool_alloc( size );

		if ( dst != NULL )
		{
			mlt_properties b_props = mlt_frame_properties( b_frame );
			memcpy( dst, src, size );
			mlt_properties_set_data( b_props, "image", dst, size, mlt_pool_release, NULL );
			mlt_properties_set_int( b_props, "width", *width );
			mlt_properties_set_int( b_props, "height", *height );
		}
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Push the filter on to the stack
	mlt_frame_push_service( frame, this );

	// Push the get_image on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_luma_init( void *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties properties = mlt_filter_properties( this );
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set( properties, "resource", arg );
	}
	return this;
}
