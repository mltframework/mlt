/*
 * filter_rgblut.c -- generic RGB look-up table filter with string interface
 * Copyright (C) 2014 Janne Liljeblad
 * Author: Janne Liljeblad
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
#include <framework/mlt_tokeniser.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/** Fill channel lut with integers parsed from property string.
*/
static void fill_channel_lut(int lut[], char* channel_table_str)
{
	mlt_tokeniser tokeniser = mlt_tokeniser_init();
	mlt_tokeniser_parse_new( tokeniser, channel_table_str, ";" );
	
	// Only create lut from string if tokens count exactly right 
	if ( tokeniser->count == 256 )
	{
		// Fill lut with token values
		int i;
		int val;
		for( i = 0; i < 256; i++ )
		{
			val = atoi(tokeniser->tokens[i]);
			lut[i] = val;
		}
	}
	else
	{
		// Fill lut with linear no-op table
		int i;
		for( i = 0; i < 256; i++ )
		{
			lut[i] = i;
		}
	}
	mlt_tokeniser_close( tokeniser );
}

/** Do it :-).
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	mlt_filter filter = mlt_frame_pop_service( frame );

	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( frame, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{

		// Create lut tables from properties for each RGB channel
		char* r_str = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "R_table" );
		int r_lut[256];
		fill_channel_lut( r_lut, r_str );

		char* g_str = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "G_table" );
		int g_lut[256];
		fill_channel_lut( g_lut, g_str );

		char* b_str = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "B_table" );
		int b_lut[256];
		fill_channel_lut( b_lut, b_str );

		// Apply look-up tables into image
		int i = *width * *height + 1;
		uint8_t *p = *image;
		uint8_t *r = *image;
		while ( --i )
		{
			*p ++ = r_lut[ *r ++ ];
			*p ++ = g_lut[ *r ++ ];
			*p ++ = b_lut[ *r ++ ];
		}
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

mlt_filter filter_rgblut_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
	}
	return filter;
}

