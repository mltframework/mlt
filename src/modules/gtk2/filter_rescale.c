/*
 * filter_rescale.c -- scale the producer video frame size to match the consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "filter_rescale.h"
#include "pixops.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	if ( *width == 0 )
		*width = 720;
	if ( *height == 0 )
		*height = 576;

	mlt_properties properties = mlt_frame_properties( this );
	int iwidth = *width;
	int iheight = *height;
	int owidth = *width;
	int oheight = *height;
	uint8_t *input = NULL;
	char *interps = mlt_properties_get( properties, "rescale.interp" );
	int interp = PIXOPS_INTERP_BILINEAR;
	
	if ( strcmp( interps, "nearest" ) == 0 )
		interp = PIXOPS_INTERP_NEAREST;
	else if ( strcmp( interps, "tiles" ) == 0 )
		interp = PIXOPS_INTERP_TILES;
	else if ( strcmp( interps, "hyper" ) == 0 )
		interp = PIXOPS_INTERP_HYPER;

	// If real_width/height exist, we want that as minimum information
	if ( mlt_properties_get_int( properties, "real_width" ) )
	{
		iwidth = mlt_properties_get_int( properties, "real_width" );
		iheight = mlt_properties_get_int( properties, "real_height" );
	}

	// Let the producer know what we are actually requested to obtain
	mlt_properties_set_int( properties, "rescale_width", *width );
	mlt_properties_set_int( properties, "rescale_height", *height );

	// Get the image as requested
	mlt_frame_get_image( this, &input, format, &iwidth, &iheight, writable );

	if ( input != NULL )
	{
		// If width and height are correct, don't do anything
		if ( *format == mlt_image_yuv422 && strcmp( interps, "none" ) && ( iwidth != owidth || iheight != oheight ) )
		{
			// Create the output image
			uint8_t *output = mlt_pool_alloc( owidth * ( oheight + 1 ) * 2 );

			// Calculate strides
			int istride = iwidth * 2;
			int ostride = owidth * 2;

			yuv422_scale_simple( output, owidth, oheight, ostride, input, iwidth, iheight, istride, interp );
		
			// Now update the frame
			mlt_properties_set_data( properties, "image", output, owidth * ( oheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );
			mlt_properties_set_int( properties, "width", owidth );
			mlt_properties_set_int( properties, "height", oheight );

			// Return the output
			*image = output;
		}
		else if ( *format == mlt_image_rgb24 || *format == mlt_image_rgb24a )
		{
			int bpp = (*format == mlt_image_rgb24a ? 4 : 3 );
			
			// Create the yuv image
			uint8_t *output = mlt_pool_alloc( owidth * ( oheight + 1 ) * 2 );

			if ( strcmp( interps, "none" ) && ( iwidth != owidth || iheight != oheight ) )
			{
				GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data( input, GDK_COLORSPACE_RGB,
					( *format == mlt_image_rgb24a ), 8, iwidth, iheight,
					iwidth * bpp, NULL, NULL );

				GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf, owidth, oheight, interp );
				g_object_unref( pixbuf );
				
				// Extract YUV422 and alpha
				if ( bpp == 4 )
				{
					// Allocate the alpha mask
					uint8_t *alpha = mlt_pool_alloc( owidth * ( oheight + 1 ) );

					// Convert the image and extract alpha
					mlt_convert_rgb24a_to_yuv422( gdk_pixbuf_get_pixels( scaled ),
										  owidth, oheight,
										  gdk_pixbuf_get_rowstride( scaled ),
										  output, alpha );

					mlt_properties_set_data( properties, "alpha", alpha, owidth * ( oheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );
				}
				else
				{
					// No alpha to extract
					mlt_convert_rgb24_to_yuv422( gdk_pixbuf_get_pixels( scaled ),
										 owidth, oheight,
										 gdk_pixbuf_get_rowstride( scaled ),
										 output );
				}
				g_object_unref( scaled );
			}
			else
			{
				// Extract YUV422 and alpha
				if ( bpp == 4 )
				{
					// Allocate the alpha mask
					uint8_t *alpha = mlt_pool_alloc( owidth * ( oheight + 1 ) );

					// Convert the image and extract alpha
					mlt_convert_rgb24a_to_yuv422( input,
										  owidth, oheight,
										  owidth * 4,
										  output, alpha );

					mlt_properties_set_data( properties, "alpha", alpha, owidth * ( oheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );
				}
				else
				{
					// No alpha to extract
					mlt_convert_rgb24_to_yuv422( input,
										 owidth, oheight,
										 owidth * 3,
										 output );
				}
			}

			// Now update the frame
			mlt_properties_set_data( properties, "image", output, owidth * ( oheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );
			mlt_properties_set_int( properties, "width", owidth );
			mlt_properties_set_int( properties, "height", oheight );

			// Return the output
			*format = mlt_image_yuv422;
			*width = owidth;
			*height = oheight;
			*image = output;
		}
		else
			*image = input;
	}
	else
		*image = input;
		
	return 0;
}

static uint8_t *producer_get_alpha_mask( mlt_frame this )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// Return the alpha mask
	return mlt_properties_get_data( properties, "alpha", NULL );
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, filter_get_image );
	mlt_properties_set( mlt_frame_properties( frame ), "rescale.interp",
		mlt_properties_get( mlt_filter_properties( this ), "interpolation" ) );
		
	// Set alpha call back
	frame->get_alpha_mask = producer_get_alpha_mask;
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_rescale_init( char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
	{
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set( mlt_filter_properties( this ), "interpolation", arg );
		else
			mlt_properties_set( mlt_filter_properties( this ), "interpolation", "bilinear" );
	}
	return this;
}

