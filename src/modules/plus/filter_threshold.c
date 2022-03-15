/*
 * filter_threshold.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2008-2022 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <string.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_slices.h>

typedef struct {
	int midpoint;
	int use_alpha;
	int invert;
	int full_luma;
	uint8_t* image;
	uint8_t* alpha;
	int width;
	int height;
} slice_desc;

static int do_slice_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = (slice_desc*) data;
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
	int slice_line_end = slice_line_start + slice_height;
	int size = desc->width * slice_height * 2;
	uint8_t white = desc->full_luma? 255 : 235;
	uint8_t black = desc->full_luma? 0 : 16;
	uint8_t A = desc->invert? white : black;
	uint8_t B = desc->invert? black : white;
	uint8_t* p = desc->image + (slice_line_start * desc->width * 2);
	int i = 0;

	if ( !desc->use_alpha )
	{
		for (i = 0; i < size; i += 2)
		{
			if ( p[i] < desc->midpoint )
				p[i] = A;
			else
				p[i] = B;
			p[i+1] = 128;
		}
	}
	else
	{

		if ( desc->alpha )
		{
			uint8_t *alpha = desc->alpha + (slice_line_start * desc->width);
			for (i = 0; i < size; i += 2)
			{
				if ( alpha[i/2] < desc->midpoint )
					p[i] = A;
				else
					p[i] = B;
				p[i+1] = 128;
			}
		}
		else
		{
			for (i = 0; i < size; i += 2)
			{
				p[i] = B;
				p[i+1] = 128;
			}
		}
	}
}

/** Get the images and apply the luminance of the mask to the alpha of the frame.
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service(frame);

	// Render the frame
	*format = mlt_image_yuv422;
	if ( mlt_frame_get_image( frame, image, format, width, height, writable ) == 0 )
	{
		slice_desc desc;
		mlt_properties properties = mlt_filter_properties(filter);
		mlt_position position = mlt_filter_get_position(filter, frame);
		mlt_position length = mlt_filter_get_length2(filter, frame);
		desc.midpoint = mlt_properties_anim_get_int(properties, "midpoint", position, length);
		desc.use_alpha = mlt_properties_get_int(properties, "use_alpha");
		desc.invert = mlt_properties_get_int(properties, "invert");
		desc.full_luma = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_luma");
		desc.image = *image;
		desc.alpha = NULL;
		desc.width = *width;
		desc.height = *height;
		if ( desc.use_alpha )
		{
			desc.alpha = mlt_frame_get_alpha( frame );
		}
		mlt_slices_run_normal(0, do_slice_proc, &desc);
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service(frame, filter);
	mlt_frame_push_get_image(frame, filter_get_image);
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_threshold_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "midpoint", 128 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "use_alpha", 0 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "invert", 0 );
		filter->process = filter_process;
	}
	return filter;
}
