/*
 * filter_gamma.c -- gamma filter
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

#include "filter_gamma.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );

	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		// Get the gamma value
		double gamma = mlt_properties_get_double( mlt_frame_properties( this ), "gamma" );

		if ( gamma != 1.0 )
		{
			uint8_t *p = *image;
			uint8_t *q = *image + *width * *height * 2;

			// Calculate the look up table
			double exp = 1 / gamma;
			uint8_t lookup[ 256 ];
			int i;

			for( i = 0; i < 256; i ++ )
				lookup[ i ] = ( uint8_t )( pow( ( double )i / 255.0, exp ) * 255 );

			while ( p != q )
			{
				*p = lookup[ *p ];
				p += 2;
			}
		}
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	double gamma = mlt_properties_get_double( mlt_filter_properties( this ), "gamma" );
	gamma = gamma <= 0 ? 1 : gamma;
	mlt_properties_set_double( mlt_frame_properties( frame ), "gamma", gamma );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_gamma_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( mlt_filter_properties( this ), "gamma", arg == NULL ? "1" : arg );
	}
	return this;
}
