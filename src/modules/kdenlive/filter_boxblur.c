/*
 * filter_boxblur.c -- blur filter
 * Copyright (C) ?-2007 Leny Grisel <leny.grisel@laposte.net>
 * Copyright (C) 2007 Jean-Baptiste Mardelle <jb@ader.ch>
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
#include <framework/mlt_profile.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static void PreCompute(uint8_t *image, int32_t *rgba, int width, int height)
{
	register int x, y, z;
	int32_t pts[4];
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pts[0] = image[0];
			pts[1] = image[1];
			pts[2] = image[2];
			pts[3] = image[3];
			for (z = 0; z < 4; z++)
			{
				if (x > 0) pts[z] += rgba[-4];
				if (y > 0) pts[z] += rgba[width * -4];
				if (x>0 && y>0) pts[z] -= rgba[(width + 1) * -4];
				*rgba++ = pts[z];
			}
			image += 4;
		}
	}
}

static inline int32_t GetRGBA(int32_t *rgba, unsigned int w, unsigned int h, unsigned int x, int offsetx, unsigned int y, int offsety, unsigned int z)
{
	int xtheo = x + offsetx;
	int ytheo = y + offsety;
	return rgba[4 * (CLAMP(xtheo, 0, w-1) + CLAMP(ytheo, 0, h-1) * w) + z];
}

static void DoBoxBlur(uint8_t *image, int32_t *rgba, unsigned int width, unsigned int height, unsigned int boxw, unsigned int boxh)
{
	register int x, y;
	float mul = 1.f / ((boxw*2) * (boxh*2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			*image++ = (GetRGBA(rgba, width, height, x, +boxw, y, +boxh, 0)
			          + GetRGBA(rgba, width, height, x, -boxw, y, -boxh, 0)
			          - GetRGBA(rgba, width, height, x, -boxw, y, +boxh, 0)
			          - GetRGBA(rgba, width, height, x, +boxw, y, -boxh, 0)) * mul;
			*image++ = (GetRGBA(rgba, width, height, x, +boxw, y, +boxh, 1)
			          + GetRGBA(rgba, width, height, x, -boxw, y, -boxh, 1)
			          - GetRGBA(rgba, width, height, x, -boxw, y, +boxh, 1)
			          - GetRGBA(rgba, width, height, x, +boxw, y, -boxh, 1)) * mul;
			*image++ = (GetRGBA(rgba, width, height, x, +boxw, y, +boxh, 2)
			          + GetRGBA(rgba, width, height, x, -boxw, y, -boxh, 2)
			          - GetRGBA(rgba, width, height, x, -boxw, y, +boxh, 2)
			          - GetRGBA(rgba, width, height, x, +boxw, y, -boxh, 2)) * mul;
			*image++ = (GetRGBA(rgba, width, height, x, +boxw, y, +boxh, 3)
			          + GetRGBA(rgba, width, height, x, -boxw, y, -boxh, 3)
			          - GetRGBA(rgba, width, height, x, -boxw, y, +boxh, 3)
			          - GetRGBA(rgba, width, height, x, +boxw, y, -boxh, 3)) * mul;
		}
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter =  (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	unsigned int boxw = 0;
	unsigned int boxh = 0;
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	double hori = mlt_properties_anim_get_double( properties, "hori", position, length );
	double vert = mlt_properties_anim_get_double( properties, "vert", position, length );

	// Get blur factor
	double factor = mlt_properties_get_int( properties, "start" );
	if ( mlt_properties_get( properties, "end" ) )
	{
		double end = (double) mlt_properties_get_int( properties, "end" );
		factor += ( end - factor ) * mlt_filter_get_progress( filter, frame );
	}

	// If animated property "blur" is set, use its value.
	char* blur_property = mlt_properties_get( properties, "blur" );
	if ( blur_property )
	{
		factor = mlt_properties_anim_get_double( properties, "blur", position, length );
	}
	
	boxw = (unsigned int)(factor * hori);
	boxh = (unsigned int)(factor * vert);

	if ( boxw == 0 && boxh == 0 )
	{
		// Don't do anything
		error = mlt_frame_get_image( frame, image, format, width, height, writable );
	}
	else
	{
		// Get the image
		*format = mlt_image_rgba;
		int error = mlt_frame_get_image( frame, image, format, width, height, 1 );
		
		// Only process if we have no error and a valid colour space
		if ( error == 0 )
		{
			mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
			boxw *= mlt_profile_scale_width(profile, *width);
			boxh *= mlt_profile_scale_height(profile, *height);
			if (boxw || boxh) {
				int size = mlt_image_format_size( *format, *width, *height, NULL );
				int32_t *rgba = mlt_pool_alloc( 4 * size );
				PreCompute( *image, rgba, *width, *height );
				DoBoxBlur( *image, rgba, *width, *height, MAX(1, boxw), MAX(1, boxh) );
				mlt_pool_release( rgba );
			}
		}
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_boxblur_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "start", arg == NULL ? "2" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "hori", "1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "vert", "1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "blur", NULL );
	}
	return filter;
}



