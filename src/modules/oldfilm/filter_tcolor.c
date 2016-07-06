/*
 * filter_tcolor.c -- tcolor filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
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


static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position pos = mlt_filter_get_position( filter, frame );
	mlt_position len = mlt_filter_get_length2( filter, frame );

	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if ( error == 0 && *image )
	{
		double over_cr = mlt_properties_anim_get_double( properties, "oversaturate_cr", pos, len )/100.0;
		double over_cb = mlt_properties_anim_get_double( properties, "oversaturate_cb", pos, len )/100.0;

		int video_width = *width;
		int video_height = *height;

		int x,y;
		
		for ( y = 0; y < video_height; y++)
		{
			for ( x = 0; x < video_width; x += 2)
			{
				uint8_t *pix = (*image + y * video_width * 2 + x * 2 + 1);
				uint8_t *pix1 = (*image + y * video_width * 2 + x * 2 + 3);
				*pix = MIN(MAX( ((double) * pix - 127.0) * over_cb + 127.0,0), 255);
				*pix1 = MIN(MAX( ((double) * pix1 - 127.0) * over_cr + 127.0,0), 255);
			}
		}
	}

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_tcolor_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "oversaturate_cr", "190" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "oversaturate_cb", "190" );
	}
	return filter;
}

