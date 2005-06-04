/*
 * filter_deinterlace.c -- deinterlace filter
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "filter_deinterlace.h"
#include "deinterlace.h"

#include <framework/mlt_frame.h>

#include <string.h>
#include <stdlib.h>

/* Linear Blend filter - C version contributed by Rogerio Brito.
   This algorithm has the same interface as the other functions.

   The destination "screen" (pdst) is constructed from the source
   screen (psrc[0]) line by line.

   The i-th line of the destination screen is the average of 3 lines
   from the source screen: the (i-1)-th, i-th and (i+1)-th lines, with
   the i-th line having weight 2 in the computation.

   Remarks:
   * each line on pdst doesn't depend on previous lines;
   * due to the way the algorithm is defined, the first & last lines of the
     screen aren't deinterlaced.

*/
#if 0
static void deinterlace_yuv( uint8_t *pdst, uint8_t *psrc, int width, int height )
{
	register int x, y;
	register uint8_t *l0, *l1, *l2, *l3;

	l0 = pdst;			// target line
	l1 = psrc;			// 1st source line
	l2 = l1 + width;	// 2nd source line = line that follows l1
	l3 = l2 + width;	// 3rd source line = line that follows l2

	// Copy the first line
	memcpy(l0, l1, width);
	l0 += width;

	for (y = 1; y < height-1; ++y) 
	{
		// computes avg of: l1 + 2*l2 + l3
		for (x = 0; x < width; ++x)
			l0[x] = (l1[x] + (l2[x]<<1) + l3[x]) >> 2;

		// updates the line pointers
		l1 = l2; l2 = l3; l3 += width;
		l0 += width;
	}

	// Copy the last line
	memcpy(l0, l1, width);
}
#endif

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	int deinterlace = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "consumer_deinterlace" );
	
	// Pop the service off the stack
	mlt_filter filter = mlt_frame_pop_service( this );

	// Determine if we need a writable version or not
	if ( deinterlace && !writable )
		 writable = !mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "progressive" );

	// Get the input image
	error = mlt_frame_get_image( this, image, format, width, height, writable );

	// Check that we want progressive and we aren't already progressive
	if ( deinterlace && *format == mlt_image_yuv422 && *image != NULL )
	{
		// Determine deinterlace method
		char *method_str = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "method" );
		int method = DEINTERLACE_LINEARBLEND;
		char *frame_method_str = mlt_properties_get( MLT_FRAME_PROPERTIES( this ), "deinterlace_method" );
		
		if ( frame_method_str != NULL )
			method_str = frame_method_str;
		
		if ( method_str == NULL )
			method = DEINTERLACE_LINEARBLEND;
		else if ( strcmp( method_str, "bob" ) == 0 )
			method = DEINTERLACE_BOB;
		else if ( strcmp( method_str, "weave" ) == 0 )
			method = DEINTERLACE_BOB;
		else if ( strcmp( method_str, "greedy" ) == 0 )
			method = DEINTERLACE_GREEDY;
		else if ( strcmp( method_str, "onefield" ) == 0 )
			method = DEINTERLACE_ONEFIELD;
			
		// Deinterlace the image
		deinterlace_yuv( *image, image, *width * 2, *height, method );
		
		// Make sure that others know the frame is deinterlaced
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( this ), "progressive", 1 );
	}

	return error;
}

/** Deinterlace filter processing - this should be lazy evaluation here...
*/

static mlt_frame deinterlace_process( mlt_filter this, mlt_frame frame )
{
	// Push this on to the service stack
	mlt_frame_push_service( frame, this );

	// Push the get_image method on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );
	
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_deinterlace_init( void *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = deinterlace_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "method", arg );
	}
	return this;
}

