/*
 * filter_sepia.c -- sepia filter
 * Copyright (C) 2003-2022 Meltytech, LLC
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
	int height;
	int width;
	uint8_t u;
	uint8_t v;
} slice_desc;

static int do_slice_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = (slice_desc*) data;
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
	int slice_line_end = slice_line_start + slice_height;
	int line_size = desc->width * 2;
	int uneven = desc->width % 2;
	int x,y;
	for ( y = slice_line_start; y < slice_line_end; y++)
	{
		uint8_t* p = desc->image + y * line_size;
		for ( x = 0; x < line_size; x += 4)
		{
			p[x+1] = desc->u;
			p[x+3] = desc->v;
		}
		if ( uneven )
		{
			p[line_size - 1] = desc->u;
		}
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 && *image )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_position position = mlt_filter_get_position( filter, frame );
		mlt_position length = mlt_filter_get_length2( filter, frame );
		slice_desc desc;
		desc.image = *image;
		desc.height = *height;
		desc.width = *width;
		desc.u = mlt_properties_anim_get_int( properties, "u", position, length );
		desc.v = mlt_properties_anim_get_int( properties, "v", position, length );
		mlt_slices_run_normal(0, do_slice_proc, &desc);
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

mlt_filter filter_sepia_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "u", "75" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "v", "150" );
	}
	return filter;
}

