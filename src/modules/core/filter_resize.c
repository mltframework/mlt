/*
 * filter_resize.c -- resizing filter
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
#include <framework/mlt_profile.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/** Swapbytes inline.
*/

static inline void swap_bytes( uint8_t *upper, uint8_t *lower )
{
	uint8_t t = *lower;
	*lower = *upper;
	*upper = t;
}

static uint8_t *resize_alpha( uint8_t *input, int owidth, int oheight, int iwidth, int iheight, uint8_t alpha_value )
{
	uint8_t *output = NULL;

	if ( input != NULL && ( iwidth != owidth || iheight != oheight ) && ( owidth > 6 && oheight > 6 ) )
	{
		uint8_t *out_line;
		int offset_x = ( owidth - iwidth ) / 2;
		int offset_y = ( oheight - iheight ) / 2;
		int iused = iwidth;

		output = mlt_pool_alloc( owidth * oheight );
		memset( output, alpha_value, owidth * oheight );

		offset_x -= offset_x % 2;

		out_line = output + offset_y * owidth;
		out_line += offset_x;

		// Loop for the entirety of our output height.
		while ( iheight -- )
		{
			// We're in the input range for this row.
			memcpy( out_line, input, iused );

			// Move to next input line
			input += iwidth;

			// Move to next output line
			out_line += owidth;
		}
	}

	return output;
}

static void resize_image( uint8_t *output, int owidth, int oheight, uint8_t *input, int iwidth, int iheight, int bpp )
{
	// Calculate strides
	int istride = iwidth * bpp;
	int ostride = owidth * bpp;
	int offset_x = ( owidth - iwidth ) / 2 * bpp;
	int offset_y = ( oheight - iheight ) / 2;
	uint8_t *in_line = input;
	uint8_t *out_line;
	int size = owidth * oheight;
	uint8_t *p = output;

	// Optimisation point
	if ( output == NULL || input == NULL || ( owidth <= 6 || oheight <= 6 || iwidth <= 6 || oheight <= 6 ) )
	{
		return;
	}
	else if ( iwidth == owidth && iheight == oheight )
	{
		memcpy( output, input, iheight * istride );
		return;
	}

	if ( bpp == 2 )
	{
		while( size -- )
		{
			*p ++ = 16;
			*p ++ = 128;
		}
		offset_x -= offset_x % 4;
	}
	else
	{
		size *= bpp;
		while ( size-- )
			*p ++ = 0;
	}

	out_line = output + offset_y * ostride;
	out_line += offset_x;

	// Loop for the entirety of our output height.
	while ( iheight -- )
	{
		// We're in the input range for this row.
		memcpy( out_line, in_line, istride );

		// Move to next input line
		in_line += istride;

		// Move to next output line
		out_line += ostride;
	}
}

/** A padding function for frames - this does not rescale, but simply
	resizes.
*/

static uint8_t *frame_resize_image( mlt_frame this, int owidth, int oheight, int bpp )
{
	// Get properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get the input image, width and height
	uint8_t *input = mlt_properties_get_data( properties, "image", NULL );
	uint8_t *alpha = mlt_frame_get_alpha_mask( this );
	int alpha_size = 0;
	mlt_properties_get_data( properties, "alpha", &alpha_size );

	int iwidth = mlt_properties_get_int( properties, "width" );
	int iheight = mlt_properties_get_int( properties, "height" );

	// If width and height are correct, don't do anything
	if ( iwidth < owidth || iheight < oheight )
	{
		uint8_t alpha_value = mlt_properties_get_int( properties, "resize_alpha" );

		// Create the output image
		uint8_t *output = mlt_pool_alloc( owidth * ( oheight + 1 ) * bpp );

		// Call the generic resize
		resize_image( output, owidth, oheight, input, iwidth, iheight, bpp );

		// Now update the frame
		mlt_frame_set_image( this, output, owidth * ( oheight + 1 ) * bpp, mlt_pool_release );

		// We should resize the alpha too
		if ( alpha && alpha_size >= iwidth * iheight )
		{
			alpha = resize_alpha( alpha, owidth, oheight, iwidth, iheight, alpha_value );
			if ( alpha )
			{
				mlt_frame_set_alpha( this, alpha, owidth * oheight, mlt_pool_release );
				this->get_alpha_mask = NULL;
			}
		}

		// Return the output
		return output;
	}
	// No change, return input
	return input;
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;

	// Get the properties from the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Pop the top of stack now
	mlt_filter filter = mlt_frame_pop_service( this );

	// Retrieve the aspect ratio
	double aspect_ratio = mlt_deque_pop_back_double( MLT_FRAME_IMAGE_STACK( this ) );
	double consumer_aspect = mlt_profile_sar( mlt_service_profile( MLT_FILTER_SERVICE( filter ) ) );

	// Correct Width/height if necessary
	if ( *width == 0 || *height == 0 )
	{
		*width = mlt_properties_get_int( properties, "normalised_width" );
		*height = mlt_properties_get_int( properties, "normalised_height" );
	}

	// Assign requested width/height from our subordinate
	int owidth = *width;
	int oheight = *height;

	// Check for the special case - no aspect ratio means no problem :-)
	if ( aspect_ratio == 0.0 )
		aspect_ratio = consumer_aspect;

	// Reset the aspect ratio
	mlt_properties_set_double( properties, "aspect_ratio", aspect_ratio );

	// Hmmm...
	char *rescale = mlt_properties_get( properties, "rescale.interp" );
	if ( rescale != NULL && !strcmp( rescale, "none" ) )
		return mlt_frame_get_image( this, image, format, width, height, writable );

	if ( mlt_properties_get_int( properties, "distort" ) == 0 )
	{
		// Normalise the input and out display aspect
		int normalised_width = mlt_properties_get_int( properties, "normalised_width" );
		int normalised_height = mlt_properties_get_int( properties, "normalised_height" );
		int real_width = mlt_properties_get_int( properties, "real_width" );
		int real_height = mlt_properties_get_int( properties, "real_height" );
		if ( real_width == 0 )
			real_width = mlt_properties_get_int( properties, "width" );
		if ( real_height == 0 )
			real_height = mlt_properties_get_int( properties, "height" );
		double input_ar = aspect_ratio * real_width / real_height;
		double output_ar = consumer_aspect * owidth / oheight;
		
// 		fprintf( stderr, "real %dx%d normalised %dx%d output %dx%d sar %f in-dar %f out-dar %f\n",
// 		real_width, real_height, normalised_width, normalised_height, owidth, oheight, aspect_ratio, input_ar, output_ar);

		// Optimised for the input_ar > output_ar case (e.g. widescreen on standard)
		int scaled_width = rint( ( input_ar * normalised_width ) / output_ar );
		int scaled_height = normalised_height;

		// Now ensure that our images fit in the output frame
		if ( scaled_width > normalised_width )
		{
			scaled_width = normalised_width;
			scaled_height = rint( ( output_ar * normalised_height ) / input_ar );
		}

		// Now calculate the actual image size that we want
		owidth = rint( scaled_width * owidth / normalised_width );
		oheight = rint( scaled_height * oheight / normalised_height );

		// Tell frame we have conformed the aspect to the consumer
		mlt_frame_set_aspect_ratio( this, consumer_aspect );
	}

	mlt_properties_set_int( properties, "distort", 0 );

	// Now pass on the calculations down the line
	mlt_properties_set_int( properties, "resize_width", *width );
	mlt_properties_set_int( properties, "resize_height", *height );

	// Now get the image
	if ( *format == mlt_image_yuv422 )
		owidth -= owidth % 2;
	error = mlt_frame_get_image( this, image, format, &owidth, &oheight, writable );

	if ( error == 0 && *image )
	{
		// Get the requested scale operation
		char *op = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "scale" );
		int bpp;
		int size = mlt_image_format_size( *format, owidth, oheight, &bpp );

		// Provides a manual override for misreported field order
		if ( mlt_properties_get( properties, "meta.top_field_first" ) )
			mlt_properties_set_int( properties, "top_field_first", mlt_properties_get_int( properties, "meta.top_field_first" ) );

		// Correct field order if needed
		if ( mlt_properties_get_int( properties, "top_field_first" ) == 1 &&
		     mlt_properties_get( properties, "progressive" ) &&
		     mlt_properties_get_int( properties, "progressive" ) == 0 )
		{
			// Get the input image, width and height
			uint8_t *new_image = mlt_pool_alloc( size );
			mlt_frame_set_image( this, new_image, size, mlt_pool_release );
			uint8_t *ptr = new_image + owidth * bpp;
			memcpy( new_image, *image, owidth * bpp );
			memcpy( ptr, *image, owidth * ( oheight - 1 ) * bpp );
			*image = new_image;
			
			// Set the normalised field order
			mlt_properties_set_int( properties, "top_field_first", 0 );
			mlt_properties_set_int( properties, "meta.top_field_first", 0 );
		}

		if ( !strcmp( op, "affine" ) )
		{
			// TODO: Determine where this is needed and find a different way
			// *image = mlt_frame_rescale_image( this, *width, *height, bpp );
		}
		else if ( strcmp( op, "none" ) != 0 )
		{
			*image = frame_resize_image( this, *width, *height, bpp );
		}
		else
		{
			*width = owidth;
			*height = oheight;
		}
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Store the aspect ratio reported by the source
	mlt_deque_push_back_double( MLT_FRAME_IMAGE_STACK( frame ), mlt_frame_get_aspect_ratio( frame ) );

	// Push this on to the service stack
	mlt_frame_push_service( frame, this );

	// Push the get_image method on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_resize_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "scale", arg == NULL ? "off" : arg );
	}
	return this;
}
