/*
 * filter_crop.c -- cropping filter
 * Copyright (C) 2009 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
#include <framework/mlt_log.h>
#include <framework/mlt_profile.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static void crop( uint8_t *src, uint8_t *dest, int bpp, int width, int height, int left, int right, int top, int bottom )
{
	int stride = ( width - left - right ) * bpp;
	int y      = height - top - bottom + 1;
	uint8_t *s = &src[ ( ( top * width ) + left ) * bpp ];

	while ( --y )
	{
		memcpy( dest, s, stride );
		dest += stride;
		s += stride + ( right + left ) * bpp;
	}
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;

	// Get the properties from the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Correct Width/height if necessary
	if ( *width == 0 || *height == 0 )
	{
		*width  = mlt_properties_get_int( properties, "normalised_width" );
		*height = mlt_properties_get_int( properties, "normalised_height" );
	}

	int left    = mlt_properties_get_int( properties, "crop.left" );
	int right   = mlt_properties_get_int( properties, "crop.right" );
	int top     = mlt_properties_get_int( properties, "crop.top" );
	int bottom  = mlt_properties_get_int( properties, "crop.bottom" );

	// Request the image at its original resolution
	if ( left || right || top || bottom )
	{
		mlt_properties_set_int( properties, "rescale_width", mlt_properties_get_int( properties, "crop.original_width" ) );
		mlt_properties_set_int( properties, "rescale_height", mlt_properties_get_int( properties, "crop.original_height" ) );
	}
	
	// Now get the image
	error = mlt_frame_get_image( this, image, format, width, height, writable );

	int owidth  = *width - left - right;
	int oheight = *height - top - bottom;
	owidth = owidth < 0 ? 0 : owidth;
	oheight = oheight < 0 ? 0 : oheight;

	if ( ( owidth != *width || oheight != *height ) &&
		error == 0 && *image != NULL && owidth > 0 && oheight > 0 )
	{
		int bpp;

		switch ( *format )
		{
			case mlt_image_yuv422:
				bpp = 2;
				break;
			case mlt_image_rgb24:
				bpp = 3;
				break;
			case mlt_image_rgb24a:
			case mlt_image_opengl:
				bpp = 4;
				break;
			default:
				// XXX: we only know how to crop packed formats
				return 1;
		}

		// Provides a manual override for misreported field order
		if ( mlt_properties_get( properties, "meta.top_field_first" ) )
		{
			mlt_properties_set_int( properties, "top_field_first", mlt_properties_get_int( properties, "meta.top_field_first" ) );
			mlt_properties_set_int( properties, "meta.top_field_first", 0 );
		}

		if ( top % 2 )
			mlt_properties_set_int( properties, "top_field_first", !mlt_properties_get_int( properties, "top_field_first" ) );
		
		// Create the output image
		uint8_t *output = mlt_pool_alloc( owidth * ( oheight + 1 ) * bpp );
		if ( output )
		{
			// Call the generic resize
			crop( *image, output, bpp, *width, *height, left, right, top, bottom );

			// Now update the frame
			*image = output;
			mlt_properties_set_data( properties, "image", output, owidth * ( oheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );
			mlt_properties_set_int( properties, "width", owidth );
			mlt_properties_set_int( properties, "height", oheight );
		}

		// We should resize the alpha too
		uint8_t *alpha = mlt_frame_get_alpha_mask( this );
		if ( alpha != NULL )
		{
			uint8_t *newalpha = mlt_pool_alloc( owidth * oheight );
			if ( newalpha )
			{
				crop( alpha, newalpha, 1, *width, *height, left, right, top, bottom );
				mlt_properties_set_data( properties, "alpha", newalpha, owidth * oheight, ( mlt_destructor )mlt_pool_release, NULL );
				this->get_alpha_mask = NULL;
			}
		}
		*width = owidth;
		*height = oheight;
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	if ( mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "active" ) )
	{
		// Push the get_image method on to the stack
		mlt_frame_push_get_image( frame, filter_get_image );
	}
	else
	{
		mlt_properties filter_props = MLT_FILTER_PROPERTIES( this );
		mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );
		int left   = mlt_properties_get_int( filter_props, "left" );
		int right  = mlt_properties_get_int( filter_props, "right" );
		int top    = mlt_properties_get_int( filter_props, "top" );
		int bottom = mlt_properties_get_int( filter_props, "bottom" );
		int width  = mlt_properties_get_int( frame_props, "real_width" );
		int height = mlt_properties_get_int( frame_props, "real_height" );

		if ( mlt_properties_get_int( filter_props, "center" ) )
		{
			double aspect_ratio = mlt_frame_get_aspect_ratio( frame );
			if ( aspect_ratio == 0.0 )
				aspect_ratio = mlt_properties_get_double( frame_props, "consumer_aspect_ratio" );
			double input_ar = aspect_ratio * width / height;
			double output_ar = mlt_profile_dar( mlt_service_profile( MLT_FILTER_SERVICE(this) ) );
			int bias = mlt_properties_get_int( filter_props, "center_bias" );
			
			if ( input_ar > output_ar )
			{
				left = right = ( width - rint( output_ar * height / aspect_ratio ) ) / 2;
				if ( abs(bias) > left )
					bias = bias < 0 ? -left : left;
				left -= bias;
				right += bias;
			}
			else
			{
				top = bottom = ( height - rint( aspect_ratio * width / output_ar ) ) / 2;
				if ( abs(bias) > top )
					bias = bias < 0 ? -top : top;
				top -= bias;
				bottom += bias;
			}
		}		

		left  -= left % 2;
		right -= right % 2;
		if ( width - left - right < 8 )
			left = right = 0;
		if ( height - top - bottom < 8 )
			top = bottom = 0;
		mlt_properties_set_int( frame_props, "crop.left", left );
		mlt_properties_set_int( frame_props, "crop.right", right );
		mlt_properties_set_int( frame_props, "crop.top", top );
		mlt_properties_set_int( frame_props, "crop.bottom", bottom );
		mlt_properties_set_int( frame_props, "crop.original_width", width );
		mlt_properties_set_int( frame_props, "crop.original_height", height );
		mlt_properties_set_int( frame_props, "real_width", width - left - right );
		mlt_properties_set_int( frame_props, "real_height", height - top - bottom );
	}
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_crop_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
	{
		this->process = filter_process;
		if ( arg )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "active", atoi( arg ) );
	}
	return this;
}
