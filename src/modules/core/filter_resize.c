/*
 * filter_resize.c -- resizing filter
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

#include "filter_resize.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;

	// Get the properties from the frame
	mlt_properties properties = mlt_frame_properties( this );

	// Pop the top of stack now
	mlt_filter filter = mlt_frame_pop_service( this );

	// Assign requested width/height from our subordinate
	int owidth = *width;
	int oheight = *height;

	// Hmmm...
	char *rescale = mlt_properties_get( properties, "rescale.interp" );
	if ( rescale != NULL && !strcmp( rescale, "none" ) )
		return mlt_frame_get_image( this, image, format, width, height, writable );

	if ( mlt_properties_get( properties, "distort" ) == NULL )
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
		double input_ar = mlt_frame_get_aspect_ratio( this ) * real_width / real_height;
		double output_ar = mlt_properties_get_double( properties, "consumer_aspect_ratio" ) * owidth / oheight;
		
		// Optimised for the input_ar > output_ar case (e.g. widescreen on standard)
		int scaled_width = input_ar / output_ar * normalised_width + 0.5;
		int scaled_height = normalised_height;

		// Now ensure that our images fit in the output frame
		if ( scaled_width > normalised_width )
		{
			scaled_width = normalised_width;
			scaled_height = output_ar / input_ar * normalised_height + 0.5;
		}
	
		// Now calculate the actual image size that we want
		owidth = scaled_width * owidth / normalised_width;
		oheight = scaled_height * oheight / normalised_height;

		// Tell frame we have conformed the aspect to the consumer
		mlt_frame_set_aspect_ratio( this, output_ar );
	}

	// Now pass on the calculations down the line
	mlt_properties_set_int( properties, "resize_width", *width );
	mlt_properties_set_int( properties, "resize_height", *height );

	// Now get the image
	error = mlt_frame_get_image( this, image, format, &owidth, &oheight, writable );

	// We only know how to process yuv422 at the moment
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		// Get the requested scale operation
		char *op = mlt_properties_get( mlt_filter_properties( filter ), "scale" );

		// Correct field order if needed
		if ( mlt_properties_get_int( properties, "top_field_first" ) == 1 )
		{
			// Get the input image, width and height
			int size;
			uint8_t *image = mlt_properties_get_data( properties, "image", &size );

			// Keep the original image around to be destroyed on frame close
			mlt_properties_rename( properties, "image", "original_image" );

			// Offset the image pointer by one line
			image += owidth * 2;
			size -= owidth * 2;

			// Set the new image pointer with no destructor
			mlt_properties_set_data( properties, "image", image, size, NULL, NULL );

			// Set the normalised field order
			mlt_properties_set_int( properties, "top_field_first", 0 );
		}

		if ( !strcmp( op, "affine" ) )
		{
			*image = mlt_frame_rescale_yuv422( this, *width, *height );
		}
		else if ( strcmp( op, "none" ) != 0 )
		{
			*image = mlt_frame_resize_yuv422( this, *width, *height );
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
	// Push this on to the service stack
	mlt_frame_push_service( frame, this );

	// Push the get_image method on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_resize_init( char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
	{
		this->process = filter_process;
		mlt_properties_set( mlt_filter_properties( this ), "scale", arg == NULL ? "off" : arg );
	}
	return this;
}
