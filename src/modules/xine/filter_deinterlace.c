/*
 * filter_deinterlace.c -- deinterlace filter
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

#include <framework/mlt_filter.h>
#include <framework/mlt_log.h>
#include "deinterlace.h"

#include <framework/mlt_frame.h>

#include <string.h>
#include <stdlib.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	int deinterlace = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "consumer_deinterlace" );
	int progressive = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "progressive" );
	
	// Pop the service off the stack
	mlt_filter filter = mlt_frame_pop_service( this );

	// Determine if we need a writable version or not
	if ( deinterlace && !writable )
		 writable = !progressive;

	// Get the input image
	if ( deinterlace && !progressive )
		*format = mlt_image_yuv422;
	mlt_log_debug( MLT_FILTER_SERVICE( filter ), "xine.deinterlace %d prog %d format %s\n",
		deinterlace, progressive, mlt_image_format_name( *format ) );
	error = mlt_frame_get_image( this, image, format, width, height, writable );
	progressive = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "progressive" );

	// Check that we want progressive and we aren't already progressive
	if ( deinterlace && *format == mlt_image_yuv422 && *image && !progressive )
	{
		// Determine deinterlace method
		char *method_str = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "method" );
		int method = DEINTERLACE_LINEARBLEND;
		char *frame_method_str = mlt_properties_get( MLT_FRAME_PROPERTIES( this ), "deinterlace_method" );
		
		if ( frame_method_str != NULL )
			method_str = frame_method_str;
		
		if ( method_str == NULL )
			method = DEINTERLACE_LINEARBLEND;
		else if ( strcmp( method_str, "bob" ) == 0 )
			method = DEINTERLACE_BOB;
		else if ( strcmp( method_str, "weave" ) == 0 )
			method = DEINTERLACE_BOB;
		else if ( strcmp( method_str, "greedy" ) == 0 )
			method = DEINTERLACE_GREEDY;
		else if ( strcmp( method_str, "onefield" ) == 0 )
			method = DEINTERLACE_ONEFIELD;
			
		// Deinterlace the image
		deinterlace_yuv( *image, image, *width * 2, *height, method );
		
		// Make sure that others know the frame is deinterlaced
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( this ), "progressive", 1 );
	}

	return error;
}

/** Deinterlace filter processing - this should be lazy evaluation here...
*/

static mlt_frame deinterlace_process( mlt_filter this, mlt_frame frame )
{
	// Push this on to the service stack
	mlt_frame_push_service( frame, this );

	// Push the get_image method on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );
	
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_deinterlace_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = deinterlace_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "method", arg );
	}
	return this;
}

