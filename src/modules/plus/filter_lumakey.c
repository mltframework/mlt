/*
 * filter_lumakey.c -- Luma Key filter for luma based image compositing
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

#include <stdio.h>
#include <stdlib.h>

static int clamp( int value, int low, int high)
{
	if (value < low)
		value = low;
	if (value > high)
		value = high;
	return value;
}

/** Builds a lookup table that maps luma values to opacity values.
*/
static void fill_opa_lut(int lgg_lut[], int prelevel, int postlevel, int slope_start, int slope_end )
{
	int i;
	// Prelevel plateau
	for( i = 0; i < slope_start; i++ )
		lgg_lut[ i ] = prelevel;

	// Value transition
	if ( slope_start != slope_end )
	{
		double value = prelevel;
		double value_step = (double) (postlevel - prelevel) / (double) (slope_end - slope_start);
		for( i = slope_start; i < slope_end + 1; i++ )
		{
			lgg_lut[ i ] = (int)value;
			value += value_step;
		}
	}

	// Postlevel plateau
	for( i = slope_end; i < 256; i++ )
		lgg_lut[ i ] = postlevel;
}

/** Do image filtering.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );

	*format =  mlt_image_rgb24a;
	int error = mlt_frame_get_image( frame, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		// Get values and force accepted ranges
		int threshold = mlt_properties_anim_get_int( properties, "threshold", position, length );
		int slope = mlt_properties_anim_get_int( properties, "slope", position, length );
		int prelevel = mlt_properties_anim_get_int( properties, "prelevel", position, length );
		int postlevel = mlt_properties_anim_get_int( properties, "postlevel", position, length );

		threshold = clamp( threshold, 0, 255 );
		slope = clamp( slope, 0, 128 );
		prelevel = clamp( prelevel, 0, 255 );
		postlevel = clamp( postlevel, 0, 255 );

		int slope_start = clamp( threshold - slope, 0, 255 );
		int slope_end = clamp( threshold + slope, 0, 255 );

		// Build lut
		int opa_lut[256];
		fill_opa_lut( opa_lut, prelevel, postlevel, slope_start, slope_end );

		// Values for calculating visual luma from RGB
		double R = 0.3;
		double G = 0.59;
		double B = 0.11;

		// Filter
		int i = *width * *height + 1;
		uint8_t *sample = *image;
		uint8_t r, g, b;
		while ( --i )
		{
			r = *sample ++;
			g = *sample ++;
			b = *sample ++;
			*sample ++ = opa_lut[(int) (R * r + g * G + b * B) ];
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
mlt_filter filter_lumakey_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "threshold", "128" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "slope", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "prelevel", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "postlevel", "255" );
	}
	return filter;
}

