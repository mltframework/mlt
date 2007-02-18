/*
 * wave.c -- wave filter
 * Author: Leny Grisel <leny.grisel@laposte.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * aint32_t with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "filter_wave.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// this is a utility function used by DoWave below
uint8_t getPoint(uint8_t *src, int w, int h, int x, int y, int z)
{
	if (x<0) x+=-((-x)%w)+w; else if (x>=w) x=x%w;
	if (y<0) y+=-((-y)%h)+h; else if (y>=h) y=y%h;
	return src[(x+y*w)*4+z];
}

// the main meat of the algorithm lies here
void DoWave(uint8_t *src, int src_w, int src_h, uint8_t *dst, mlt_position position, int speed, int factor, int deformX, int deformY)
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

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	mlt_position position = mlt_frame_get_position( this );

	// Only process if we have no error and a valid colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		double factor = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "wave" );
		int speed = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "speed" );
        	int deformX = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "deformX" );
        	int deformY = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "deformY" );
        	if (factor != 0) {
			int image_size = *width * (*height + 1) * 2;
            		int8_t *dest = mlt_pool_alloc (image_size);
            		DoWave(*image, *width, (*height + 1), dest, position, speed, factor, deformX, deformY);
            		memcpy(*image, dest, image_size);
            		mlt_pool_release(dest);
        	}
    	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Get the starting wave level
	double wave = mlt_properties_get_double( MLT_FILTER_PROPERTIES( this ), "start" );
	int speed = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "speed" );
	int deformX = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "deformX" );
	int deformY = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "deformY" );

	// If there is an end adjust gain to the range
	if ( mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "end" ) != NULL )
	{
		// Determine the time position of this frame in the transition duration
		mlt_position in = mlt_filter_get_in( this );
		mlt_position out = mlt_filter_get_out( this );
		mlt_position time = mlt_frame_get_position( frame );
		double position = ( double )( time - in ) / ( double )( out - in + 1 );
		double end = fabs( mlt_properties_get_double( MLT_FILTER_PROPERTIES( this ), "end" ) );
		wave += ( end - wave ) * position;
	}

	// Push the frame filter
	mlt_properties_set_double( MLT_FRAME_PROPERTIES( frame ), "wave", wave );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "speed", speed );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "deformX", deformX );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "deformY", deformY );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_wave_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "start", arg == NULL ? "10" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "speed", arg == NULL ? "5" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "deformX", arg == NULL ? "1" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "deformY", arg == NULL ? "1" : arg);
		}
	return this;
}



