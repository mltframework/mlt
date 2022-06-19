/*
 * filter_tcolor.c -- tcolor filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
 * Copyright (c) 2022 Meltytech, LLC
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
#include <framework/mlt_slices.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
	uint8_t* image;
	int width;
	int height;
	double over_cr;
	double over_cb;
} slice_desc;

static int do_slice_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = (slice_desc*) data;
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
	int slice_line_end = slice_line_start + slice_height;
	int line_size = desc->width * 2;
	int x,y;
	for ( y = slice_line_start; y < slice_line_end; y++)
	{
		uint8_t* p = desc->image + y * line_size;
		for ( x = 0; x < line_size; x += 4)
		{
			p[x+1] = CLAMP( ((double)p[x+1] - 127.0) * desc->over_cb + 127.0, 0, 255);
			p[x+3] = CLAMP( ((double)p[x+3] - 127.0) * desc->over_cr + 127.0, 0, 255);
		}
	}
	return 0;
}

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
		slice_desc desc;
		desc.over_cr = mlt_properties_anim_get_double( properties, "oversaturate_cr", pos, len )/100.0;
		desc.over_cb = mlt_properties_anim_get_double( properties, "oversaturate_cb", pos, len )/100.0;
		desc.image = *image;
		desc.width = *width;
		desc.height = *height;
		mlt_slices_run_normal(0, do_slice_proc, &desc);
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

