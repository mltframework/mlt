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

	mlt_frame_get_image( this, &input, format, &iwidth, &iheight, 0 );

#if 0
	// Determine maximum size within the aspect ratio:
	double i_aspect_ratio = mlt_frame_get_aspect_ratio( this );
	// TODO: this needs to be provided      q
	#define o_aspect_ratio ( double )( 4.0 / 3.0 )

	if ( ( owidth * i_aspect_ratio * o_aspect_ratio ) > owidth )
		oheight *= o_aspect_ratio / i_aspect_ratio;
	else
		owidth *= i_aspect_ratio * o_aspect_ratio;

	fprintf( stderr, "rescale: from %dx%d (%f) to %dx%d\n", iwidth, iheight, i_aspect_ratio, owidth, oheight );
#endif
	
	// If width and height are correct, don't do anything
	if ( strcmp( interps, "none" ) && input != NULL && ( iwidth != owidth || iheight != oheight ) )
	{
		if ( *format == mlt_image_yuv422 )
		{
			// Create the output image
			uint8_t *output = malloc( owidth * oheight * 2 );

			// Calculate strides
			int istride = iwidth * 2;
			int ostride = owidth * 2;

			yuv422_scale_simple( output, owidth, oheight, ostride, input, iwidth, iheight, istride, interp );
		
			// Now update the frame
			mlt_properties_set_data( properties, "image", output, owidth * oheight * 2, free, NULL );
			mlt_properties_set_int( properties, "width", owidth );
			mlt_properties_set_int( properties, "height", oheight );

			// Return the output
			*image = output;
		}
		else if ( *format == mlt_image_rgb24 || *format == mlt_image_rgb24a )
		{
			int bpp = (*format == mlt_image_rgb24a ? 4 : 3 );
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data( input, GDK_COLORSPACE_RGB,
				(*format == mlt_image_rgb24a), 24, iwidth, iheight,
				iwidth * bpp, NULL, NULL );
			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf, owidth, oheight, interp );

			// Create the output image
			uint8_t *output = malloc( owidth * oheight * bpp );

			int i;
			for ( i = 0; i < oheight; i++ )
				memcpy( output + i * owidth * bpp,
						gdk_pixbuf_get_pixels( scaled ) + i * gdk_pixbuf_get_rowstride( scaled ),
						gdk_pixbuf_get_width( scaled ) * bpp );

			g_object_unref( pixbuf );
			g_object_unref( scaled );
			
			// Now update the frame
			mlt_properties_set_data( properties, "image", output, owidth * oheight * bpp, free, NULL );
			mlt_properties_set_int( properties, "width", owidth );
			mlt_properties_set_int( properties, "height", oheight );

			// Return the output
			*image = output;
		}
	}
	else
		*image = input;
		
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, filter_get_image );
	mlt_properties_set( mlt_frame_properties( frame ), "rescale.interp",
		mlt_properties_get( mlt_filter_properties( this ), "interpolation" ) );
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

