/*
 * filter_crop.c -- cropping filter
 * Copyright (C) 2009-2014 Meltytech, LLC
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
#include <framework/mlt_log.h>
#include <framework/mlt_profile.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static void crop( uint8_t *src, uint8_t *dest, int bpp, int width, int height, int left, int right, int top, int bottom )
{
	int src_stride = ( width  ) * bpp;
	int dest_stride = ( width - left - right ) * bpp;
	int y      = height - top - bottom + 1;
	src += top * src_stride + left * bpp;

	while ( --y )
	{
		memcpy( dest, src, dest_stride );
		dest += dest_stride;
		src += src_stride;
	}
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_profile profile = mlt_frame_pop_service( frame );

	// Get the properties from the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Correct Width/height if necessary
	if ( *width == 0 || *height == 0 )
	{
		*width  = profile->width;
		*height = profile->height;
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
	error = mlt_frame_get_image( frame, image, format, width, height, writable );

	int owidth  = *width - left - right;
	int oheight = *height - top - bottom;
	owidth = owidth < 0 ? 0 : owidth;
	oheight = oheight < 0 ? 0 : oheight;

	if ( ( owidth != *width || oheight != *height ) &&
		error == 0 && *image != NULL && owidth > 0 && oheight > 0 )
	{
		int bpp;

		// Subsampled YUV is messy and less precise.
		if ( *format == mlt_image_yuv422 && frame->convert_image && ( left & 1 ) )
		{
			mlt_image_format requested_format = mlt_image_rgb24;
			frame->convert_image( frame, image, format, requested_format );
		}
	
		mlt_log_debug( NULL, "[filter crop] %s %dx%d -> %dx%d\n", mlt_image_format_name(*format),
				 *width, *height, owidth, oheight);

		if ( top % 2 )
			mlt_properties_set_int( properties, "top_field_first", !mlt_properties_get_int( properties, "top_field_first" ) );
		
		// Create the output image
		int size = mlt_image_format_size( *format, owidth, oheight, &bpp );
		uint8_t *output = mlt_pool_alloc( size );
		if ( output )
		{
			// Call the generic resize
			crop( *image, output, bpp, *width, *height, left, right, top, bottom );

			// Now update the frame
			mlt_frame_set_image( frame, output, size, mlt_pool_release );
			*image = output;
		}

		// We should resize the alpha too
		uint8_t *alpha = mlt_frame_get_alpha( frame );
		int alpha_size = 0;
		mlt_properties_get_data( properties, "alpha", &alpha_size );
		if ( alpha && alpha_size >= ( *width * *height ) )
		{
			uint8_t *newalpha = mlt_pool_alloc( owidth * oheight );
			if ( newalpha )
			{
				crop( alpha, newalpha, 1, *width, *height, left, right, top, bottom );
				mlt_frame_set_alpha( frame, newalpha, owidth * oheight, mlt_pool_release );
			}
		}
		*width = owidth;
		*height = oheight;
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	if ( mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "active" ) )
	{
		// Push the get_image method on to the stack
		mlt_frame_push_service( frame, mlt_service_profile( MLT_FILTER_SERVICE( filter ) ) );
		mlt_frame_push_get_image( frame, filter_get_image );
	}
	else
	{
		mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter );
		mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );
		int left   = mlt_properties_get_int( filter_props, "left" );
		int right  = mlt_properties_get_int( filter_props, "right" );
		int top    = mlt_properties_get_int( filter_props, "top" );
		int bottom = mlt_properties_get_int( filter_props, "bottom" );
		int width  = mlt_properties_get_int( frame_props, "meta.media.width" );
		int height = mlt_properties_get_int( frame_props, "meta.media.height" );
		int use_profile = mlt_properties_get_int( filter_props, "use_profile" );
		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );

		if ( use_profile )
		{
			top = top * height / profile->height;
			bottom = bottom * height / profile->height;
			left = left * width / profile->width;
			right = right * width / profile->width;
		}
		if ( mlt_properties_get_int( filter_props, "center" ) )
		{
			double aspect_ratio = mlt_frame_get_aspect_ratio( frame );
			if ( aspect_ratio == 0.0 )
				aspect_ratio = mlt_profile_sar( profile );
			double input_ar = aspect_ratio * width / height;
			double output_ar = mlt_profile_dar( mlt_service_profile( MLT_FILTER_SERVICE(filter) ) );
			int bias = mlt_properties_get_int( filter_props, "center_bias" );
			
			if ( input_ar > output_ar )
			{
				left = right = ( width - rint( output_ar * height / aspect_ratio ) ) / 2;
				if ( abs(bias) > left )
					bias = bias < 0 ? -left : left;
				else if ( use_profile )
					bias = bias * width / profile->width;
				left -= bias;
				right += bias;
			}
			else
			{
				top = bottom = ( height - rint( aspect_ratio * width / output_ar ) ) / 2;
				if ( abs(bias) > top )
					bias = bias < 0 ? -top : top;
				else if ( use_profile )
					bias = bias * height / profile->height;
				top -= bias;
				bottom += bias;
			}
		}		

		// Coerce the output to an even width because subsampled YUV with odd widths is too
		// risky for downstream processing to handle correctly.
		left += ( width - left - right ) & 1;
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
		mlt_properties_set_int( frame_props, "meta.media.width", width - left - right );
		mlt_properties_set_int( frame_props, "meta.media.height", height - top - bottom );
	}
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_crop_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( mlt_filter_init( filter, filter ) == 0 )
	{
		filter->process = filter_process;
		if ( arg )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "active", atoi( arg ) );
	}
	return filter;
}
