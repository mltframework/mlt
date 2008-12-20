/*
 * filter_swscale.c -- image scaling filter
 * Copyright (C) 2008-2009 Ushodaya Enterprises Limited
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
#include <framework/mlt_factory.h>
#include <framework/mlt_factory.h>


// ffmpeg Header files
#include <avformat.h>
#include <swscale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int is_big_endian( )
{
	union { int i; char c[ 4 ]; } big_endian_test;
	big_endian_test.i = 1;

	return big_endian_test.c[ 0 ] != 1;
}

static inline int convert_mlt_to_av_cs( mlt_image_format format )
{
	int value = 0;

	switch( format )
	{
		case mlt_image_rgb24:
			value = PIX_FMT_RGB24;
			break;
		case mlt_image_rgb24a:
			value = PIX_FMT_RGBA32;
			break;
		case mlt_image_yuv422:
			value = PIX_FMT_YUV422;
			break;
		case mlt_image_yuv420p:
			value = PIX_FMT_YUV420P;
			break;
		case mlt_image_opengl:
		case mlt_image_none:
			fprintf( stderr, "Invalid format...\n" );
			break;
	}

	return value;
}

static int filter_scale( mlt_frame this, uint8_t **image, mlt_image_format iformat, mlt_image_format oformat, int iwidth, int iheight, int owidth, int oheight )
{
	// Get the properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get the requested interpolation method
	char *interps = mlt_properties_get( properties, "rescale.interp" );

	// Convert to the SwScale flag
	int interp = SWS_BILINEAR;
	if ( strcmp( interps, "nearest" ) == 0 || strcmp( interps, "neighbor" ) == 0 )
		interp = SWS_POINT;
	else if ( strcmp( interps, "tiles" ) == 0 || strcmp( interps, "fast_bilinear" ) == 0 )
		interp = SWS_FAST_BILINEAR;
	else if ( strcmp( interps, "bilinear" ) == 0 )
		interp = SWS_BILINEAR;
	else if ( strcmp( interps, "bicubic" ) == 0 )
		interp = SWS_BICUBIC;
	else if ( strcmp( interps, "bicublin" ) == 0 )
		interp = SWS_BICUBLIN;
	else if ( strcmp( interps, "gauss" ) == 0 )
		interp = SWS_GAUSS;
	else if ( strcmp( interps, "sinc" ) == 0 )
		interp = SWS_SINC;
	else if ( strcmp( interps, "hyper" ) == 0 || strcmp( interps, "lanczos" ) == 0 )
		interp = SWS_LANCZOS;
	else if ( strcmp( interps, "spline" ) == 0 )
		interp = SWS_SPLINE;

	AVPicture input;
	AVPicture output;
	uint8_t *outbuf = mlt_pool_alloc( owidth * ( oheight + 1 ) * 2 );

	// Convert the pixel formats
	iformat = convert_mlt_to_av_cs( iformat );
	oformat = convert_mlt_to_av_cs( oformat );

	avpicture_fill( &input, *image, iformat, iwidth, iheight );
	avpicture_fill( &output, outbuf, oformat, owidth, oheight );

	// Extract the alpha channel
	if ( iformat == PIX_FMT_RGBA32 && oformat == PIX_FMT_YUV422 )
	{
		// Allocate the alpha mask
		uint8_t *alpha = mlt_pool_alloc( iwidth * ( iheight + 1 ) );
		if ( alpha )
		{
			// Convert the image and extract alpha
			mlt_convert_rgb24a_to_yuv422( *image, iwidth, iheight, iwidth * 4, outbuf, alpha );
			mlt_properties_set_data( properties, "alpha", alpha, iwidth * ( iheight + 1 ), ( mlt_destructor )mlt_pool_release, NULL );
			iformat = PIX_FMT_YUV422;
			avpicture_fill( &input, outbuf, iformat, iwidth, iheight );
			avpicture_fill( &output, *image, oformat, owidth, oheight );
		}
	}

	// Create the context and output image
	struct SwsContext *context = sws_getContext( iwidth, iheight, iformat, owidth, oheight, oformat, interp, NULL, NULL, NULL);

	// Perform the scaling
	sws_scale( context, input.data, input.linesize, 0, iheight, output.data, output.linesize);
	sws_freeContext( context );

	// Now update the frame
	mlt_properties_set_data( properties, "image", output.data[0], owidth * ( oheight + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );
	mlt_properties_set_int( properties, "width", owidth );
	mlt_properties_set_int( properties, "height", oheight );

	// Return the output
	*image = output.data[0];

	// Scale the alpha channel only if exists and not correct size
	int alpha_size = 0;
	mlt_properties_get_data( properties, "alpha", &alpha_size );
	if ( alpha_size > 0 && alpha_size != ( owidth * oheight ) )
	{
		// Create the context and output image
		uint8_t *alpha = mlt_frame_get_alpha_mask( this );
		if ( alpha )
		{
			iformat = oformat = PIX_FMT_GRAY8;
			struct SwsContext *context = sws_getContext( iwidth, iheight, iformat, owidth, oheight, oformat, interp, NULL, NULL, NULL);
			avpicture_fill( &input, alpha, iformat, iwidth, iheight );
			outbuf = mlt_pool_alloc( owidth * oheight );
			avpicture_fill( &output, outbuf, oformat, owidth, oheight );

			// Perform the scaling
			sws_scale( context, input.data, input.linesize, 0, iheight, output.data, output.linesize);
			sws_freeContext( context );

			// Set it back on the frame
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( this ), "alpha", output.data[0], owidth * oheight, mlt_pool_release, NULL );
		}
	}

	return 0;
}

/** Constructor for the filter.
*/

mlt_filter filter_swscale_init( mlt_profile profile, void *arg )
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
