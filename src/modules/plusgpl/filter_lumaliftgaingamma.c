/*
 * filter_lumaliftgaingamma.c -- Lift Gain Gamma filter for luma correction
 * Copyright (C) 2014 Janne Liljeblad
 * Author: Janne Liljeblad
 *
 * This filter is a port from Gimp and is distributed under a compatible license.
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static double clamp( double value, double low, double high)
{
	if (value < low)
		value = low;
	if (value > high)
		value = high;
	return value;
}

static double lut_value( double value, double lift, double gain, double gamma )
{
	double nvalue;
	double power;

	value = clamp( value + lift, 0, 1 );

	if( gain < 0.0)
		value = value * (1.0 + gain);
	else
		value = value + ((1.0 - value) * gain);

	if(gamma < 0.0)
	{
		if (value > 0.5)
			nvalue = 1.0 - value;
		else
			nvalue = value;

		if (nvalue < 0.0)
			nvalue = 0.0;

		nvalue = 0.5 * pow (nvalue * 2.0 , (double) (1.0 + gamma));
		
		if (value > 0.5)
			value = 1.0 - nvalue;
		else
			value = nvalue;
	}
	else
	{
		if (value > 0.5)
			nvalue = 1.0 - value;
		else
			nvalue = value;

		if (nvalue < 0.0)
			nvalue = 0.0;

		power = (gamma == 1.0) ? 127 : 1.0 / (1.0 - gamma);
		nvalue = 0.5 * pow (2.0 * nvalue, power);

		if (value > 0.5)
			value = 1.0 - nvalue;
		else
			value = nvalue;
	}

	return value;
}

static void fill_lgg_lut(int lgg_lut[], double lift, double gain, double gamma)
{
	int i;
	double val;
	for( i = 0; i < 256; i++ )
	{
		val = (double) i / 255.0;
		lgg_lut[ i ] = (int) (lut_value( val, lift, gain, gamma ) * 255.0);
	}
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

	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( frame, image, format, width, height, 0 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		// Get values and force accepted ranges
		double lift = mlt_properties_anim_get_double( properties, "lift", position, length );
		double gain = mlt_properties_anim_get_double( properties, "gain", position, length );
		double gamma = mlt_properties_anim_get_double( properties, "gamma", position, length );
		lift = clamp( lift, -0.5, 0.5 );
		gain = clamp( gain, -0.5, 0.5 );
		gamma = clamp( gamma, -1.0, 1.0 );

		// Build lut
		int lgg_lut[256];
		fill_lgg_lut( lgg_lut, lift, gain, gamma);

		// Filter
		int i = *width * *height + 1;
		uint8_t *p = *image;
		uint8_t *r = *image;
		while ( --i )
		{
			*p ++ = lgg_lut[ *r ++ ];
			*p ++ = lgg_lut[ *r ++ ];
			*p ++ = lgg_lut[ *r ++ ];
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
mlt_filter filter_lumaliftgaingamma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "lift", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "gain", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "gamma", "0" );
	}
	return filter;
}

