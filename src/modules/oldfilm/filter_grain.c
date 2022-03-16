/*
 * filter_grain.c -- grain filter
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

#include "common.h"
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_slices.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
	uint8_t *image;
	int width;
	int height;
	int noise;
	double contrast;
	double brightness;
	mlt_position pos;
	int min;
	int max_luma;
} slice_desc;

static int slice_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* d = (slice_desc*) data;
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
	uint8_t *p = d->image + slice_line_start * d->width * 2;

	oldfilm_rand_seed seed;
	oldfilm_init_seed(&seed, d->pos * jobs + index);
	for (int n = 0; n < slice_height * d->width; n++, p += 2) {
		if (p[0] > 20) {
			int pix = CLAMP(((double) p[0] - 127.0) * d->contrast + 127.0 + d->brightness, 0, 255);
			if (d->noise > 0) {
				pix -= oldfilm_fast_rand(&seed) % d->noise - d->noise;
			}
			p[0] = CLAMP(pix , d->min, d->max_luma);
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
		int noise = mlt_properties_anim_get_int( properties, "noise", pos, len );
		int full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
		slice_desc desc = {
			.image = *image,
			.width = *width,
			.height = *height,
			.noise = noise,
			.contrast = mlt_properties_anim_get_double( properties, "contrast", pos, len ) / 100.0,
			.brightness = 127.0 * (mlt_properties_anim_get_double( properties, "brightness", pos, len ) -100.0 ) / 100.0,
			.pos = pos,
			.min = full_range? 0 : 16,
			.max_luma = full_range? 255 : 235,
		};
		mlt_slices_run_normal(0, slice_proc, &desc);
	}

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_grain_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "noise", "40" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "contrast", "160" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "brightness", "70" );
	}
	return filter;
}

