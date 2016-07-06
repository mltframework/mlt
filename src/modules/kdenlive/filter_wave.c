/*
 * wave.c -- wave filter
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// this is a utility function used by DoWave below
static uint8_t getPoint(uint8_t *src, int w, int h, int x, int y, int z)
{
	if (x<0) x+=-((-x)%w)+w; else if (x>=w) x=x%w;
	if (y<0) y+=-((-y)%h)+h; else if (y>=h) y=y%h;
	return src[(x+y*w)*4+z];
}

// the main meat of the algorithm lies here
static void DoWave(uint8_t *src, int src_w, int src_h, uint8_t *dst, mlt_position position, int speed, int factor, int deformX, int deformY)
{
	register int x, y;
	int decalY, decalX, z;
	float amplitude, phase, pulsation;
	register int uneven = src_w % 2;
	int w = (src_w - uneven ) / 2;
	amplitude = factor;
	pulsation = 0.5 / factor;   // smaller means bigger period
	phase = position * pulsation * speed / 10; // smaller means longer
	for (y=0;y<src_h;y++) {
		decalX = deformX ? sin(pulsation * y + phase) * amplitude : 0;
		for (x=0;x<w;x++) {
			decalY = deformY ? sin(pulsation * x * 2 + phase) * amplitude : 0;
			for (z=0; z<4; z++)
                		*dst++ = getPoint(src, w, src_h, (x+decalX), (y+decalY), z);
		}
		if (uneven) {
			decalY = sin(pulsation * x * 2 + phase) * amplitude;
			for (z=0; z<2; z++)
				*dst++ = getPoint(src, w, src_h, (x+decalX), (y+decalY), z);
		}
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_frame_get_position( frame );

	// Get the image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		double factor = mlt_properties_get_double( properties, "start" );

		mlt_position f_pos = mlt_filter_get_position( filter, frame );
		mlt_position f_len = mlt_filter_get_length2( filter, frame );
		int speed = mlt_properties_anim_get_int( properties, "speed", f_pos, f_len );
		int deformX = mlt_properties_anim_get_int( properties, "deformX", f_pos, f_len );
		int deformY = mlt_properties_anim_get_int( properties, "deformY", f_pos, f_len );

		if ( mlt_properties_get( properties, "end" ) )
		{
			// Determine the time position of this frame in the transition duration
			double end = fabs( mlt_properties_get_double( MLT_FILTER_PROPERTIES( filter ), "end" ) );
			factor += ( end - factor ) * mlt_filter_get_progress( filter, frame );
		}

		// If animated property "wave" is set, use its value. 
		char* wave_property = mlt_properties_get( properties, "wave" );
		if ( wave_property )
		{
			factor = mlt_properties_anim_get_double( properties, "wave", f_pos, f_len );
		}

		if (factor != 0) 
		{
			int image_size = *width * (*height) * 2;
			uint8_t *dst = mlt_pool_alloc (image_size);
			DoWave(*image, *width, (*height), dst, position, speed, factor, deformX, deformY);
			*image = dst;
			mlt_frame_set_image( frame, *image, image_size, mlt_pool_release );
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

mlt_filter filter_wave_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "start", arg == NULL ? "10" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "speed", "5");
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "deformX", "1");
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "deformY", "1");
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "wave", NULL);
	}
	return filter;
}

