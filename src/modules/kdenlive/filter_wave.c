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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
	mlt_properties unique = mlt_frame_pop_service( frame );
	mlt_position position = mlt_frame_get_position( frame );

	// Get the image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		double factor = mlt_properties_get_int( unique, "wave" );
		int speed = mlt_properties_get_int( unique, "speed" );
		int deformX = mlt_properties_get_int( unique, "deformX" );
		int deformY = mlt_properties_get_int( unique, "deformY" );
		if (factor != 0) {
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
	// Get the starting wave level
	double wave = mlt_properties_get_double( MLT_FILTER_PROPERTIES( filter ), "start" );
	int speed = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "speed" );
	int deformX = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "deformX" );
	int deformY = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "deformY" );

	// If there is an end adjust gain to the range
	if ( mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "end" ) != NULL )
	{
		// Determine the time position of this frame in the transition duration
		mlt_position in = mlt_filter_get_in( filter );
		mlt_position out = mlt_filter_get_out( filter );
		mlt_position time = mlt_frame_get_position( frame );
		double position = ( double )( time - in ) / ( double )( out - in + 1 );
		double end = fabs( mlt_properties_get_double( MLT_FILTER_PROPERTIES( filter ), "end" ) );
		wave += ( end - wave ) * position;
	}

	// Push the frame filter
	mlt_properties unique = mlt_frame_unique_properties( frame, MLT_FILTER_SERVICE( filter ) );
	mlt_properties_set_double( unique, "wave", wave );
	mlt_properties_set_int( unique, "speed", speed );
	mlt_properties_set_int( unique, "deformX", deformX );
	mlt_properties_set_int( unique, "deformY", deformY );
	mlt_frame_push_service( frame, unique );
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
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "speed", arg == NULL ? "5" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "deformX", arg == NULL ? "1" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "deformY", arg == NULL ? "1" : arg);
	}
	return filter;
}



