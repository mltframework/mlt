/*
 * filter_charcoal.c -- charcoal filter
 * Copyright (C) 2003-2014 Meltytech, LLC
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

static inline int get_Y( uint8_t *pixels, int width, int height, int x, int y )
{
	if ( x < 0 || x >= width || y < 0 || y >= height )
	{
		return 235;
	}
	else
	{
		uint8_t *pixel = pixels + y * ( width << 1 ) + ( x << 1 );
		return *pixel;
	}
}

static inline int sqrti( int n )
{
	int p = 0;
	int q = 1;
	int r = n;
	int h = 0;

	while( q <= n )
		q = q << 2;

	while( q != 1 )
	{
		q = q >> 2;
		h = p + q;
		p = p >> 1;
		if ( r >= h )
		{
			p = p + q;
			r = r - h;
		}
	}

	return p;
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );

	// Get the image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		// Get the charcoal scatter value
		int x_scatter = mlt_properties_anim_get_double( properties, "x_scatter", position, length );
		int y_scatter = mlt_properties_anim_get_double( properties, "y_scatter", position, length );
		float scale = mlt_properties_anim_get_double( properties, "scale" ,position, length);
		float mix = mlt_properties_anim_get_double( properties, "mix", position, length);
		int invert = mlt_properties_anim_get_int( properties, "invert", position, length);

		// We'll process pixel by pixel
		int x = 0;
		int y = 0;

		// We need to create a new frame as this effect modifies the input
		uint8_t *temp = mlt_pool_alloc( *width * *height * 2 );
		uint8_t *p = temp;
		uint8_t *q = *image;

		// Calculations are carried out on a 3x3 matrix
		int matrix[ 3 ][ 3 ];

		// Used to carry out the matrix calculations
		int sum1;
		int sum2;
		float sum;
		int val;

		// Loop for each row
		for ( y = 0; y < *height; y ++ )
		{
			// Loop for each pixel
			for ( x = 0; x < *width; x ++ )
			{
				// Populate the matrix
				matrix[ 0 ][ 0 ] = get_Y( *image, *width, *height, x - x_scatter, y - y_scatter );
				matrix[ 0 ][ 1 ] = get_Y( *image, *width, *height, x            , y - y_scatter );
				matrix[ 0 ][ 2 ] = get_Y( *image, *width, *height, x + x_scatter, y - y_scatter );
				matrix[ 1 ][ 0 ] = get_Y( *image, *width, *height, x - x_scatter, y             );
				matrix[ 1 ][ 2 ] = get_Y( *image, *width, *height, x + x_scatter, y             );
				matrix[ 2 ][ 0 ] = get_Y( *image, *width, *height, x - x_scatter, y + y_scatter );
				matrix[ 2 ][ 1 ] = get_Y( *image, *width, *height, x            , y + y_scatter );
				matrix[ 2 ][ 2 ] = get_Y( *image, *width, *height, x + x_scatter, y + y_scatter );

				// Do calculations
				sum1 = (matrix[2][0] - matrix[0][0]) + ( (matrix[2][1] - matrix[0][1]) << 1 ) + (matrix[2][2] - matrix[2][0]);
				sum2 = (matrix[0][2] - matrix[0][0]) + ( (matrix[1][2] - matrix[1][0]) << 1 ) + (matrix[2][2] - matrix[2][0]);
				sum = scale * sqrti( sum1 * sum1 + sum2 * sum2 );

				// Assign value
				*p ++ = !invert ? ( sum >= 16 && sum <= 235 ? 251 - sum : sum < 16 ? 235 : 16 ) :
								  ( sum >= 16 && sum <= 235 ? sum : sum < 16 ? 16 : 235 );
				q ++;
				val = 128 + mix * ( *q ++ - 128 );
				val = val < 16 ? 16 : val > 240 ? 240 : val;
				*p ++ = val;
			}
		}

		// Return the created image
		*image = temp;

		// Store new and destroy old
		mlt_frame_set_image( frame, *image, *width * *height * 2, mlt_pool_release );
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

mlt_filter filter_charcoal_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "x_scatter", 1 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "y_scatter", 1 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "scale", 1.5 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "mix", 0.0 );
	}
	return filter;
}

