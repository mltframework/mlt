/*
 * filter_swscale.c -- image scaling filter
 * Copyright (C) 2008-2017 Meltytech, LLC
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
#include <framework/mlt_factory.h>


// ffmpeg Header files
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_ALIGN (1)

static inline int convert_mlt_to_av_cs( mlt_image_format format )
{
	int value = 0;

	switch( format )
	{
		case mlt_image_rgb24:
			value = AV_PIX_FMT_RGB24;
			break;
		case mlt_image_rgb24a:
		case mlt_image_opengl:
			value = AV_PIX_FMT_RGBA;
			break;
		case mlt_image_yuv422:
			value = AV_PIX_FMT_YUYV422;
			break;
		case mlt_image_yuv420p:
			value = AV_PIX_FMT_YUV420P;
			break;
		default:
			fprintf( stderr, "Invalid format...\n" );
			break;
	}

	return value;
}

static int filter_scale( mlt_frame frame, uint8_t **image, mlt_image_format *format, int iwidth, int iheight, int owidth, int oheight )
{
	// Get the properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

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
	interp |= SWS_ACCURATE_RND;

	// Determine the output image size.
	int out_size = mlt_image_format_size( *format, owidth, oheight, NULL );

	// Set swscale flags to get good quality
	switch ( *format )
	{
		case mlt_image_yuv422:
			interp |= SWS_FULL_CHR_H_INP;
			break;
		case mlt_image_rgb24:
			interp |= SWS_FULL_CHR_H_INT;
			break;
		case mlt_image_rgb24a:
		case mlt_image_opengl:
			interp |= SWS_FULL_CHR_H_INT;
			break;
		default:
			// XXX: we only know how to rescale packed formats
			return 1;
	}

	// Convert the pixel formats
	int avformat = convert_mlt_to_av_cs( *format );

	// Fill out the AVPictures
	uint8_t *in_data[4];
	int in_stride[4];
	uint8_t *out_data[4];
	int out_stride[4];
	uint8_t *outbuf = mlt_pool_alloc( out_size );

	av_image_fill_arrays(in_data, in_stride, *image, avformat, iwidth, iheight, IMAGE_ALIGN);
	av_image_fill_arrays(out_data, out_stride, outbuf, avformat, owidth, oheight, IMAGE_ALIGN);

	// Create the context and output image
	struct SwsContext *context = sws_getContext( iwidth, iheight, avformat, owidth, oheight, avformat, interp, NULL, NULL, NULL);
	if ( context )
	{
		// Perform the scaling
		sws_scale( context, (const uint8_t **) in_data, in_stride, 0, iheight, out_data, out_stride);
		sws_freeContext( context );
	
		// Now update the frame
		mlt_frame_set_image( frame, outbuf, out_size, mlt_pool_release );
	
		// Return the output
		*image = outbuf;
	
		// Scale the alpha channel only if exists and not correct size
		int alpha_size = 0;
		mlt_properties_get_data( properties, "alpha", &alpha_size );
		if ( alpha_size > 0 && alpha_size != ( owidth * oheight ) )
		{
			// Create the context and output image
			uint8_t *alpha = mlt_frame_get_alpha( frame );
			if ( alpha )
			{
				avformat = AV_PIX_FMT_GRAY8;
				struct SwsContext *context = sws_getContext( iwidth, iheight, avformat, owidth, oheight, avformat, interp, NULL, NULL, NULL);
				outbuf = mlt_pool_alloc( owidth * oheight );
				av_image_fill_arrays(in_data, in_stride, alpha, avformat, iwidth, iheight, IMAGE_ALIGN);
				av_image_fill_arrays(out_data, out_stride, outbuf, avformat, owidth, oheight, IMAGE_ALIGN);
	
				// Perform the scaling
				sws_scale( context, (const uint8_t **) in_data, in_stride, 0, iheight, out_data, out_stride);
				sws_freeContext( context );
	
				// Set it back on the frame
				mlt_frame_set_alpha( frame, outbuf, owidth * oheight, mlt_pool_release );
			}
		}
	
		return 0;
	}
	else
	{
		return 1;
	}
}

/** Constructor for the filter.
*/

mlt_filter filter_swscale_init( mlt_profile profile, void *arg )
{
	// Test to see if swscale accepts the arg as resolution
	if ( arg )
	{
		int *width = (int*) arg;
		if ( *width > 0 )
		{
			struct SwsContext *context = sws_getContext( *width, *width, AV_PIX_FMT_RGB32, 64, 64, AV_PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
			if ( context )
				sws_freeContext( context );
			else
				return NULL;
	}	}

	// Create a new scaler
	mlt_filter filter = mlt_factory_filter( profile, "rescale", NULL );
	
	// If successful, then initialise it
	if ( filter != NULL )
	{
		// Get the properties
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

		// Set the inerpolation
		mlt_properties_set( properties, "interpolation", "bilinear" );

		// Set the method
		mlt_properties_set_data( properties, "method", filter_scale, 0, NULL, NULL );
	}

	return filter;
}
