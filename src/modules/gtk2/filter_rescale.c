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
#include <framework/mlt_factory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

static int filter_scale( mlt_frame this, uint8_t **image, mlt_image_format iformat, mlt_image_format oformat, int iwidth, int iheight, int owidth, int oheight )
{
	// Get the properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get the requested interpolation method
	char *interps = mlt_properties_get( properties, "rescale.interp" );

	// Convert to the GTK flag
	int interp = PIXOPS_INTERP_BILINEAR;

	if ( strcmp( interps, "nearest" ) == 0 )
		interp = PIXOPS_INTERP_NEAREST;
	else if ( strcmp( interps, "tiles" ) == 0 )
		interp = PIXOPS_INTERP_TILES;
	else if ( strcmp( interps, "hyper" ) == 0 )
		interp = PIXOPS_INTERP_HYPER;

	// Carry out the rescaling
	if ( iformat == mlt_image_yuv422 && oformat == mlt_image_yuv422 )
	{
		// Create the output image
		uint8_t *output = mlt_pool_alloc( owidth * ( oheight + 1 ) * 2 );

		// Calculate strides
		int istride = iwidth * 2;
		int ostride = owidth * 2;

		yuv422_scale_simple( output, owidth, oheight, ostride, *image, iwidth, iheight, istride, interp );
		
		// Now update the frame
		mlt_properties_set_data( properties, "image", output, owidth * ( oheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "width", owidth );
		mlt_properties_set_int( properties, "height", oheight );

		// Return the output
		*image = output;
	}
	else if ( iformat == mlt_image_rgb24 || iformat == mlt_image_rgb24a )
	{
		int bpp = (iformat == mlt_image_rgb24a ? 4 : 3 );
			
		// Create the yuv image
		uint8_t *output = mlt_pool_alloc( owidth * ( oheight + 1 ) * 2 );

		if ( strcmp( interps, "none" ) && ( iwidth != owidth || iheight != oheight ) )
		{
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data( *image, GDK_COLORSPACE_RGB,
				( iformat == mlt_image_rgb24a ), 8, iwidth, iheight,
				iwidth * bpp, NULL, NULL );

			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf, owidth, oheight, interp );
			g_object_unref( pixbuf );
			
			// Extract YUV422 and alpha
			if ( bpp == 4 )
			{
				// Allocate the alpha mask
				uint8_t *alpha = mlt_pool_alloc( owidth * ( oheight + 1 ) );

				// Convert the image and extract alpha
				mlt_convert_rgb24a_to_yuv422( gdk_pixbuf_get_pixels( scaled ), owidth, oheight, gdk_pixbuf_get_rowstride( scaled ), output, alpha );

				mlt_properties_set_data( properties, "alpha", alpha, owidth * ( oheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );
			}
			else
			{
				// No alpha to extract
				mlt_convert_rgb24_to_yuv422( gdk_pixbuf_get_pixels( scaled ), owidth, oheight, gdk_pixbuf_get_rowstride( scaled ), output );
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
				mlt_convert_rgb24a_to_yuv422( *image, owidth, oheight, owidth * 4, output, alpha );

				mlt_properties_set_data( properties, "alpha", alpha, owidth * ( oheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );
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

/** Constructor for the filter.
*/

mlt_filter filter_rescale_init( char *arg )
{
	// Create a new scaler
	mlt_filter this = mlt_factory_filter( "rescale", arg );

	// If successful, then initialise it
	if ( this != NULL )
	{
		// Get the properties
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );

		// Set the inerpolation
		mlt_properties_set( properties, "interpolation", arg == NULL ? "bilinear" : arg );

		// Set the method
		mlt_properties_set_data( properties, "method", filter_scale, 0, NULL, NULL );
	}

	return this;
}

