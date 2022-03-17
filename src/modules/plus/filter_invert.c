/*
 * filter_invert.c -- invert filter
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
#include <string.h>

typedef struct {
	uint8_t* image;
	int height;
	int width;
	int full_range;
} slice_desc;

static int do_slice_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = (slice_desc*) data;
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
	int slice_line_end = slice_line_start + slice_height;
	int line_size = desc->width * 2;
	int min = desc->full_range ? 0 : 16;
	int max_luma = desc->full_range ? 255 : 235;
	int max_chroma = desc->full_range ? 255 : 240;
	int invert_luma = desc->full_range ? 255 : 251;
	int x,y;
	for ( y = slice_line_start; y < slice_line_end; y++)
	{
		uint8_t* p = desc->image + y * line_size;
		for ( x = 0; x < line_size; x += 2)
		{
			p[x] = CLAMP(invert_luma - p[x], min, max_luma);
			p[x+1] = CLAMP(256 - p[x+1], min, max_chroma);
		}
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	mlt_filter filter = mlt_frame_pop_service( frame );

	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );
	// Only process if we have no error and a valid color space
	if ( error == 0 && *format == mlt_image_yuv422)
	{
		slice_desc desc;
		desc.image = *image;
		desc.width = *width;
		desc.height = *height;
		desc.full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
		mlt_slices_run_normal(0, do_slice_proc, &desc);

		int mask = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "alpha" );
		if ( mask )
		{
			int size = *width * *height;
			uint8_t* alpha = mlt_pool_alloc( size );
			memset( alpha, mask, size );
			mlt_frame_set_alpha( frame, alpha, size, mlt_pool_release );
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

