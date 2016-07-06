/*
 * filter_invert.c -- invert filter
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static inline int clamp( int v, int l, int u )
{
	return v < l ? l : ( v > u ? u : v );
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	mlt_filter filter = mlt_frame_pop_service( frame );

	int mask = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "alpha" );
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		uint8_t *p = *image;
		uint8_t *q = *image + *width * *height * 2;
		uint8_t *r = *image;

		while ( p != q )
		{
			*p ++ = clamp( 251 - *r ++, 16, 235 );
			*p ++ = clamp( 256 - *r ++, 16, 240 );
		}

		if ( mask )
		{
			uint8_t *alpha = mlt_frame_get_alpha_mask( frame );
			int size = *width * *height;
			memset( alpha, mask, size );
		}
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Push the frame filter
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_invert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
	}
	return filter;
}

