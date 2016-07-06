/*
 * filter_audiowave.c -- display audio waveform
 * Copyright (C) 2010-2014 Meltytech, LLC
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

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int size = *width * *height * 2;
	*format = mlt_image_yuv422;
	*image = mlt_pool_alloc( size );
	mlt_frame_set_image( frame, *image, size, mlt_pool_release );
	uint8_t *wave = mlt_frame_get_waveform( frame, *width, *height );
	if ( wave )
	{
		uint8_t *p = *image;
		uint8_t *q = *image + *width * *height * 2;
		uint8_t *s = wave;

		while ( p != q )
		{
			*p ++ = *s ++;
			*p ++ = 128;
		}
	}
	return ( wave == NULL );
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_audiowave_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
		filter->process = filter_process;
	return filter;
}

