/*
 * filter_rescale.c -- scale the producer video frame size to match the consumer
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include "pixops.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_factory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

static int filter_scale( mlt_frame this, uint8_t **image, mlt_image_format *format, int iwidth, int iheight, int owidth, int oheight )
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
	else if ( strcmp( interps, "hyper" ) == 0 || strcmp( interps, "bicubic" ) == 0 )
		interp = PIXOPS_INTERP_HYPER;

	int bpp;
	int size = mlt_image_format_size( *format, owidth, oheight, &bpp );

	// Carry out the rescaling
	switch ( *format )
	{
	case mlt_image_yuv422:
	{
		// Create the output image
		uint8_t *output = mlt_pool_alloc( size );

		// Calculate strides
		int istride = iwidth * 2;
		int ostride = owidth * 2;

		yuv422_scale_simple( output, owidth, oheight, ostride, *image, iwidth, iheight, istride, interp );
		
		// Now update the frame
		mlt_frame_set_image( this, output, size, mlt_pool_release );

		// Return the output
		*image = output;
		break;
	}
	case mlt_image_rgb24:
	case mlt_image_rgb24a:
	case mlt_image_opengl:
	{
		if ( strcmp( interps, "none" ) && ( iwidth != owidth || iheight != oheight ) )
		{
			// Create the output image
			uint8_t *output = mlt_pool_alloc( size );
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data( *image, GDK_COLORSPACE_RGB,
				( *format == mlt_image_rgb24a || *format == mlt_image_opengl ), 8, iwidth, iheight,
				iwidth * bpp, NULL, NULL );
			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf, owidth, oheight, interp );
			g_object_unref( pixbuf );

			int src_stride = gdk_pixbuf_get_rowstride( scaled );
			int dst_stride = owidth * bpp;
			if ( src_stride != dst_stride )
			{
				int y = oheight;
				uint8_t *src = gdk_pixbuf_get_pixels( scaled );
				uint8_t *dst = output;
				while ( y-- )
				{
					memcpy( dst, src, dst_stride );
					dst += dst_stride;
					src += src_stride;
				}
			}
			else
			{
				memcpy( output, gdk_pixbuf_get_pixels( scaled ), owidth * oheight * bpp );
			}

			g_object_unref( scaled );

			// Now update the frame
			mlt_frame_set_image( this, output, size, mlt_pool_release );
	
			// Return the output
			*image = output;
		}
		break;
	}
	default:
		break;
	}

	return 0;
}

/** Constructor for the filter.
*/

mlt_filter filter_rescale_init( mlt_profile profile, char *arg )
{
	// Create a new scaler
	mlt_filter this = mlt_factory_filter( profile, "rescale", arg );

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

