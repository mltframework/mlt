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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static void PreCompute(uint8_t *image, int32_t *rgb, int width, int height)
{
	register int x, y, z;
	int32_t pts[3];
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pts[0] = image[0];
			pts[1] = image[1];
			pts[2] = image[2];
			for (z = 0; z < 3; z++) 
			{
				if (x > 0) pts[z] += rgb[-3];
				if (y > 0) pts[z] += rgb[width * -3];
				if (x>0 && y>0) pts[z] -= rgb[(width + 1) * -3];
				*rgb++ = pts[z];
			}
			image += 3;
		}
	}
}

static int32_t GetRGB(int32_t *rgb, unsigned int w, unsigned int h, unsigned int x, int offsetx, unsigned int y, int offsety, unsigned int z)
{
	int xtheo = x * 1 + offsetx;
	int ytheo = y + offsety;
	if (xtheo < 0) xtheo = 0; else if (xtheo >= w) xtheo = w - 1;
	if (ytheo < 0) ytheo = 0; else if (ytheo >= h) ytheo = h - 1;
	return rgb[3*(xtheo+ytheo*w)+z];
}

static void DoBoxBlur(uint8_t *image, int32_t *rgb, unsigned int width, unsigned int height, unsigned int boxw, unsigned int boxh)
{
	register int x, y;
	float mul = 1.f / ((boxw*2) * (boxh*2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			*image++ = (GetRGB(rgb, width, height, x, +boxw, y, +boxh, 0)
			          + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 0)
			          - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 0)
			          - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 0)) * mul;
			*image++ = (GetRGB(rgb, width, height, x, +boxw, y, +boxh, 1)
			          + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 1)
			          - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 1)
			          - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 1)) * mul;
			*image++ = (GetRGB(rgb, width, height, x, +boxw, y, +boxh, 2)
			          + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 2)
			          - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 2)
			          - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 2)) * mul;
		}
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter =  (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	// Get the image
	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		short hori = mlt_properties_get_int( properties, "hori" );
		short vert = mlt_properties_get_int( properties, "vert" );

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
			mlt_position position = mlt_filter_get_position( filter, frame );
			mlt_position length = mlt_filter_get_length2( filter, frame );
			factor = mlt_properties_anim_get_double( properties, "blur", position, length );
		}

		if ( factor != 0)
		{
			int h = *height + 1;
			int32_t *rgb = mlt_pool_alloc( 3 * *width * h * sizeof(int32_t) );
			PreCompute( *image, rgb, *width, h );
			DoBoxBlur( *image, rgb, *width, h, (int) factor * hori, (int) factor * vert );
			mlt_pool_release( rgb );
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



