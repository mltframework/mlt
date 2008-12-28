/*
 * filter_rescale.c -- scale the producer video frame size to match the consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef int ( *image_scaler )( mlt_frame this, uint8_t **image, mlt_image_format iformat, mlt_image_format oformat, int iwidth, int iheight, int owidth, int oheight );

static void scale_alpha( mlt_frame this, int iwidth, int iheight, int owidth, int oheight );

static int filter_scale( mlt_frame this, uint8_t **image, mlt_image_format iformat, mlt_image_format oformat, int iwidth, int iheight, int owidth, int oheight )
{
	// Get the properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get the rescaling interpolsation
	char *interps = mlt_properties_get( properties, "rescale.interp" );

	// Carry out the rescaling
	if ( iformat == mlt_image_yuv422 && oformat == mlt_image_yuv422 )
	{
		// Scale the frame
		mlt_frame_rescale_yuv422( this, owidth, oheight );
		
		// Return the output
		*image = mlt_properties_get_data( properties, "image", NULL );

		// Scale the alpha channel only if exists and not correct size
		int alpha_size = 0;
		mlt_properties_get_data( properties, "alpha", &alpha_size );
		if ( alpha_size > 0 && alpha_size != ( owidth * oheight ) )
			scale_alpha( this, iwidth, iheight, owidth, oheight );
	}
	else if ( iformat == mlt_image_rgb24 || iformat == mlt_image_rgb24a )
	{
		int bpp = (iformat == mlt_image_rgb24a ? 4 : 3 );
			
		// Create the yuv image
		uint8_t *output = mlt_pool_alloc( iwidth * ( iheight + 1 ) * 2 );

		if ( strcmp( interps, "none" ) && ( iwidth != owidth || iheight != oheight ) )
		{
			// Extract YUV422 and alpha
			if ( bpp == 4 )
			{
				// Allocate the alpha mask
				uint8_t *alpha = mlt_pool_alloc( iwidth * ( iheight + 1 ) );

				// Convert the image and extract alpha
				mlt_convert_rgb24a_to_yuv422( *image, iwidth, iheight, iwidth * 4, output, alpha );

				mlt_properties_set_data( properties, "alpha", alpha, iwidth * ( iheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );

				scale_alpha( this, iwidth, iheight, owidth, oheight );
			}
			else
			{
				// No alpha to extract
				mlt_convert_rgb24_to_yuv422( *image, iwidth, iheight, iwidth * 3, output );
			}

			mlt_properties_set_data( properties, "image", output, iwidth * ( iheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );

			// Scale the frame
			output = mlt_frame_rescale_yuv422( this, owidth, oheight );

		}
		else
		{
			// Extract YUV422 and alpha
			if ( bpp == 4 )
			{
				// Allocate the alpha mask
				uint8_t *alpha = mlt_pool_alloc( owidth * ( oheight + 1 ) );

				// Convert the image and extract alpha
				mlt_convert_rgb24a_to_yuv422( *image, owidth, oheight, owidth * 4, output, alpha );

				mlt_properties_set_data( properties, "alpha", alpha, owidth * ( oheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );

				scale_alpha( this, iwidth, iheight, owidth, oheight );
			}
			else
			{
				// No alpha to extract
				mlt_convert_rgb24_to_yuv422( *image, owidth, oheight, owidth * 3, output );
			}
		}

		// Now update the frame
		mlt_properties_set_data( properties, "image", output, owidth * ( oheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "width", owidth );
		mlt_properties_set_int( properties, "height", oheight );

		*image = output;
	}

	return 0;
}

static void scale_alpha( mlt_frame this, int iwidth, int iheight, int owidth, int oheight )
{
	// Scale the alpha
	uint8_t *output = NULL;
	uint8_t *input = mlt_frame_get_alpha_mask( this );

	if ( input != NULL )
	{
		uint8_t *out_line;
		int x, y;
		int ox = ( iwidth << 10 ) / owidth;
		int oy = ( iheight << 10 ) / oheight;

		output = mlt_pool_alloc( owidth * oheight );
		out_line = output;

   		// Loop for the entirety of our output height.
		for ( y = 0; y < oheight; y ++ )
			for ( x = 0; x < owidth; x ++ )
				*out_line ++ = *( input + ( ( 512 + ( y * oy * iwidth ) + x * ox ) >> 10 ) );

		// Set it back on the frame
		mlt_properties_set_data( MLT_FRAME_PROPERTIES( this ), "alpha", output, owidth * oheight, mlt_pool_release, NULL );
	}
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the frame properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get the filter from the stack
	mlt_filter filter = mlt_frame_pop_service( this );

	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the image scaler method
	image_scaler scaler_method = mlt_properties_get_data( filter_properties, "method", NULL );

	// Correct Width/height if necessary
	if ( *width == 0 || *height == 0 )
	{
		*width = mlt_properties_get_int( properties, "normalised_width" );
		*height = mlt_properties_get_int( properties, "normalised_height" );
	}

	// There can be problems with small images - avoid them (by hacking - gah)
	if ( *width >= 6 && *height >= 6 )
	{
		int iwidth = *width;
		int iheight = *height;
		int owidth = *width;
		int oheight = *height;
		char *interps = mlt_properties_get( properties, "rescale.interp" );
		int wanted_format = *format;

		// Default from the scaler if not specifed on the frame
		if ( interps == NULL )
		{
			interps = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "interpolation" );
			mlt_properties_set( properties, "rescale.interp", interps );
		}
	
		// If real_width/height exist, we want that as minimum information
		if ( mlt_properties_get_int( properties, "real_width" ) )
		{
			iwidth = mlt_properties_get_int( properties, "real_width" );
			iheight = mlt_properties_get_int( properties, "real_height" );
		}
	
		// Let the producer know what we are actually requested to obtain
		if ( *format == mlt_image_yuv422 && strcmp( interps, "none" ) )
		{
			mlt_properties_set_int( properties, "rescale_width", *width );
			mlt_properties_set_int( properties, "rescale_height", *height );
		}
		else
		{
			// When no scaling is requested, revert the requested dimensions if possible
			mlt_properties_set_int( properties, "rescale_width", iwidth );
			mlt_properties_set_int( properties, "rescale_height", iheight );
		}

		// Deinterlace if height is changing to prevent fields mixing on interpolation
		// One exception: non-interpolated, integral scaling
		if ( iheight != oheight && ( strcmp( interps, "nearest" ) || ( iheight % oheight != 0 ) ) )
			mlt_properties_set_int( properties, "consumer_deinterlace", 1 );

		// Get the image as requested
		mlt_frame_get_image( this, image, format, &iwidth, &iheight, writable );

		// Get rescale interpretation again, in case the producer wishes to override scaling
		interps = mlt_properties_get( properties, "rescale.interp" );
	
		if ( *image != NULL && ( *format != mlt_image_yuv422 || ( iwidth != owidth || iheight != oheight ) ) )
		{
			// If the colour space is correct and scaling is off, do nothing
			if ( *format == mlt_image_yuv422 && !strcmp( interps, "none" ) )
			{
				*width = iwidth;
				*height = iheight;
			}
			else if ( *format == mlt_image_yuv422 )
			{
				// Call the local scaler
				scaler_method( this, image, *format, mlt_image_yuv422, iwidth, iheight, owidth, oheight );
				*width = owidth;
				*height = oheight;
			}
			else if ( *format == mlt_image_rgb24 && wanted_format == mlt_image_rgb24 )
			{
				// Call the local scaler
				scaler_method( this, image, *format, mlt_image_rgb24, iwidth, iheight, owidth, oheight );

				// Return the output
				*width = owidth;
				*height = oheight;
			}
			else if ( *format == mlt_image_rgb24 || *format == mlt_image_rgb24a )
			{
				// Call the local scaler
				scaler_method( this, image, *format, mlt_image_yuv422, iwidth, iheight, owidth, oheight );

				// Return the output
				*format = mlt_image_yuv422;
				*width = owidth;
				*height = oheight;
			}
			else
			{
				*width = iwidth;
				*height = iheight;
			}
		}
		else
		{
			*width = iwidth;
			*height = iheight;
		}
	}
	else
	{
		// Store the requested width/height
		int iwidth = *width;
		int iheight = *height;

		// Get the image as requested
		mlt_frame_get_image( this, image, format, &iwidth, &iheight, writable );

		// Too small - for now just assign as though we got what we wanted
		*width = iwidth;
		*height = iheight;
	}


	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Push the filter
	mlt_frame_push_service( frame, this );

	// Push the get image method
	mlt_frame_push_service( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_rescale_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new scaler
	mlt_filter this = mlt_filter_new( );

	// If successful, then initialise it
	if ( this != NULL )
	{
		// Get the properties
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );

		// Set the process method
		this->process = filter_process;

		// Set the inerpolation
		mlt_properties_set( properties, "interpolation", arg == NULL ? "bilinear" : arg );

		// Set the method
		mlt_properties_set_data( properties, "method", filter_scale, 0, NULL, NULL );
	}

	return this;
}

