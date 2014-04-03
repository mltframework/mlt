/*
 * filter_lift_gamma_gain.cpp
 * Copyright (C) 2014 Brian Matherly <pez4brian@yahoo.com>
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

#include <framework/mlt.h>
#include <stdlib.h>
#include <math.h>

typedef struct
{
	uint8_t rlut[256];
	uint8_t glut[256];
	uint8_t blut[256];
	double rlift, glift, blift;
	double rgamma, ggamma, bgamma;
	double rgain, ggain, bgain;
} private_data;

static void refresh_lut( mlt_filter filter, mlt_frame frame )
{
	private_data* private = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	double rlift = mlt_properties_anim_get_double( properties, "lift_r", position, length );
	double glift = mlt_properties_anim_get_double( properties, "lift_g", position, length );
	double blift = mlt_properties_anim_get_double( properties, "lift_b", position, length );
	double rgamma = mlt_properties_anim_get_double( properties, "gamma_r", position, length );
	double ggamma = mlt_properties_anim_get_double( properties, "gamma_g", position, length );
	double bgamma = mlt_properties_anim_get_double( properties, "gamma_b", position, length );
	double rgain = mlt_properties_anim_get_double( properties, "gain_r", position, length );
	double ggain = mlt_properties_anim_get_double( properties, "gain_g", position, length );
	double bgain = mlt_properties_anim_get_double( properties, "gain_b", position, length );

	// Only regenerate the LUT if something changed.
	if( private->rlift != rlift || private->glift != glift || private->blift != blift ||
		private->rgamma != rgamma || private->ggamma != ggamma || private->bgamma != bgamma ||
		private->rgain != rgain || private->ggain != ggain || private->bgain != bgain )
	{
		int i = 0;
		for( i = 0; i < 256; i++ )
		{
			// Convert to gamma 2.2
			double gamma22 = pow( (double)i / 255.0, 1.0 / 2.2 );
			double r = gamma22;
			double g = gamma22;
			double b = gamma22;

			// Apply lift
			r += rlift * ( 1.0 - r );
			g += glift * ( 1.0 - g );
			b += blift * ( 1.0 - b );

			// Apply gamma
			r = pow( r, 2.2 / rgamma );
			g = pow( g, 2.2 / ggamma );
			b = pow( b, 2.2 / bgamma );

			// Apply gain
			r *= pow( rgain, 1.0 / rgamma );
			g *= pow( ggain, 1.0 / ggamma );
			b *= pow( bgain, 1.0 / bgamma );

			// Clamp values
			r = r < 0.0 ? 0.0 : r > 1.0 ? 1.0 : r;
			g = g < 0.0 ? 0.0 : g > 1.0 ? 1.0 : g;
			b = b < 0.0 ? 0.0 : b > 1.0 ? 1.0 : b;

			// Update LUT
			private->rlut[ i ] = (int)(r * 255.0);
			private->glut[ i ] = (int)(g * 255.0);
			private->blut[ i ] = (int)(b * 255.0);
		}

		// Store the values that created the LUT so that
		// changes can be detected.
		private->rlift = rlift;
		private->glift = glift;
		private->blift = blift;
		private->rgamma = rgamma;
		private->ggamma = ggamma;
		private->bgamma = bgamma;
		private->rgain = rgain;
		private->ggain = ggain;
		private->bgain = bgain;
	}
}

static void apply_lut( mlt_filter filter, uint8_t* image, mlt_image_format format, int width, int height )
{
	private_data* private = (private_data*)filter->child;
	int total = width * height + 1;
	uint8_t* sample = image;

	switch( format )
	{
	case mlt_image_rgb24:
		while( --total )
		{
			*sample = private->rlut[ *sample ];
			sample++;
			*sample = private->glut[ *sample ];
			sample++;
			*sample = private->blut[ *sample ];
			sample++;
		}
		break;
	case mlt_image_rgb24a:
		while( --total )
		{
			*sample = private->rlut[ *sample ];
			sample++;
			*sample = private->glut[ *sample ];
			sample++;
			*sample = private->blut[ *sample ];
			sample++;
			sample++; // Skip alpha
		}
		break;
	default:
		mlt_log_error( MLT_FILTER_SERVICE( filter ), "Invalid image format: %s\n", mlt_image_format_name( format ) );
		break;
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	int error = 0;

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Regenerate the LUT if necessary
	refresh_lut( filter, frame );

	// Make sure the format is acceptable
	if( *format != mlt_image_rgb24 && *format != mlt_image_rgb24a )
	{
		*format = mlt_image_rgb24;
	}

	// Get the image
	writable = 1;
	error = mlt_frame_get_image( frame, image, format, width, height, writable );

	// Apply the LUT
	if( !error )
	{
		apply_lut( filter, *image, *format, *width, *height );
	}

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	private_data* private = (private_data*)filter->child;

	if ( private )
	{
		free( private );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

mlt_filter filter_lift_gamma_gain_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* private = (private_data*)calloc( 1, sizeof(private_data) );
	int i = 0;

	if ( filter && private )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

		// Initialize private data
		for( i = 0; i < 256; i++ )
		{
			private->rlut[i] = i;
			private->glut[i] = i;
			private->blut[i] = i;
		}
		private->rlift = private->glift = private->blift = 0.0;
		private->rgamma = private->ggamma = private->bgamma = 1.0;
		private->rgain = private->ggain = private->bgain = 1.0;

		// Initialize filter properties
		mlt_properties_set_double( properties, "lift_r", private->rlift );
		mlt_properties_set_double( properties, "lift_g", private->glift );
		mlt_properties_set_double( properties, "lift_b", private->blift );
		mlt_properties_set_double( properties, "gamma_r", private->rgamma );
		mlt_properties_set_double( properties, "gamma_g", private->ggamma );
		mlt_properties_set_double( properties, "gamma_b", private->bgamma );
		mlt_properties_set_double( properties, "gain_r", private->rgain );
		mlt_properties_set_double( properties, "gain_g", private->ggain );
		mlt_properties_set_double( properties, "gain_b", private->bgain );

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = private;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter lift_gamma_gain init failed\n" );

		if( filter )
		{
			mlt_filter_close( filter );
			filter = NULL;
		}

		if( private )
		{
			free( private );
		}
	}

	return filter;
}
