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

static int get_value( mlt_properties properties, char *preferred, char *fallback )
{
	int value = mlt_properties_get_int( properties, preferred );
	if ( value == 0 )
		value = mlt_properties_get_int( properties, fallback );
	return value;
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties from the frame
	mlt_properties properties = mlt_frame_properties( this );

	// Assign requested width/height from our subordinate
	int owidth = *width;
	int oheight = *height;

	if ( mlt_properties_get( properties, "distort" ) == NULL )
	{
		// Now do additional calcs based on real_width/height etc
		int normalised_width = mlt_properties_get_int( properties, "normalised_width" );
		int normalised_height = mlt_properties_get_int( properties, "normalised_height" );
		int real_width = get_value( properties, "real_width", "width" );
		int real_height = get_value( properties, "real_height", "height" );
		double input_ar = mlt_frame_get_aspect_ratio( this );
		double output_ar = mlt_properties_get_double( properties, "consumer_aspect_ratio" );
		int scaled_width = ( input_ar > output_ar ? input_ar / output_ar : output_ar / input_ar ) * real_width;
		int scaled_height = ( input_ar > output_ar ? input_ar / output_ar : output_ar / input_ar ) * real_height;

		// Now ensure that our images fit in the normalised frame
		if ( scaled_width > normalised_width )
		{
			scaled_height = scaled_height * normalised_width / scaled_width;
			scaled_width = normalised_width;
		}
		if ( scaled_height > normalised_height )
		{
			scaled_width = scaled_width * normalised_height / scaled_height;
			scaled_height = normalised_height;
		}

		if ( input_ar == output_ar && scaled_height == normalised_height )
			scaled_width = normalised_width;
	
		// Now calculate the actual image size that we want
		owidth = scaled_width * owidth / normalised_width;
		oheight = scaled_height * oheight / normalised_height;
	}

	// Now pass on the calculations down the line
	mlt_properties_set_int( properties, "resize_width", *width );
	mlt_properties_set_int( properties, "resize_height", *height );

	// Now get the image
	mlt_frame_get_image( this, image, format, &owidth, &oheight, writable );
	
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

	if ( !strcmp( mlt_properties_get( properties, "resize.scale" ), "affine" ) )
		*image = mlt_frame_rescale_yuv422( this, *width, *height );
	else if ( strcmp( mlt_properties_get( properties, "resize.scale" ), "none" ) != 0 )
		*image = mlt_frame_resize_yuv422( this, *width, *height );
	else
	{
		*width = owidth;
		*height = oheight;
	}
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, filter_get_image );
	mlt_properties_set( mlt_frame_properties( frame ), "resize.scale", mlt_properties_get( mlt_filter_properties( this ), "scale" ) );
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
		if ( arg != NULL )
			mlt_properties_set( mlt_filter_properties( this ), "scale", arg );
		else
			mlt_properties_set( mlt_filter_properties( this ), "scale", "off" );
	}
	return this;
}

