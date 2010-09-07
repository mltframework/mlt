/*
 * filter_avcolour_space.c -- Colour space filter
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
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
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>

// ffmpeg Header files
#include <avformat.h>
#ifdef SWSCALE
#include <swscale.h>
#endif

#if LIBAVUTIL_VERSION_INT < (50<<16)
#define PIX_FMT_YUYV422 PIX_FMT_YUV422
#endif

#include <stdio.h>
#include <stdlib.h>

#if 0 // This test might come in handy elsewhere someday.
static int is_big_endian( )
{
	union { int i; char c[ 4 ]; } big_endian_test;
	big_endian_test.i = 1;

	return big_endian_test.c[ 0 ] != 1;
}
#endif

static int convert_mlt_to_av_cs( mlt_image_format format )
{
	int value = 0;

	switch( format )
	{
		case mlt_image_rgb24:
			value = PIX_FMT_RGB24;
			break;
		case mlt_image_rgb24a:
		case mlt_image_opengl:
			value = PIX_FMT_RGBA;
			break;
		case mlt_image_yuv422:
			value = PIX_FMT_YUYV422;
			break;
		case mlt_image_yuv420p:
			value = PIX_FMT_YUV420P;
			break;
		case mlt_image_none:
			mlt_log_error( NULL, "[filter avcolor_space] Invalid format\n" );
			break;
	}

	return value;
}

static void av_convert_image( mlt_properties properties, uint8_t *out, uint8_t *in, int out_fmt, int in_fmt, int width, int height )
{
	AVPicture input;
	AVPicture output;
	avpicture_fill( &input, in, in_fmt, width, height );
	avpicture_fill( &output, out, out_fmt, width, height );

#ifdef SWSCALE
	int flags = SWS_BILINEAR | SWS_ACCURATE_RND;
	struct SwsContext *context = mlt_properties_get_data( properties, "avcolorspace.swscale", NULL );
	struct SwsContext *new_context;

	if ( out_fmt == PIX_FMT_YUYV422 )
		flags |= SWS_FULL_CHR_H_INP;
	else
		flags |= SWS_FULL_CHR_H_INT;
#ifdef USE_MMX
	flags |= SWS_CPU_CAPS_MMX;
#endif
#ifdef USE_SSE
	flags |= SWS_CPU_CAPS_MMX2;
#endif
	new_context = sws_getCachedContext( context, width, height, in_fmt,
		width, height, out_fmt, flags, NULL, NULL, NULL);
//	Disable using cached context here due to crashing in libswscale.
//	if ( new_context != context )
//		mlt_properties_set_data( properties, "avcolorspace.swscale", new_context, 0, NULL, NULL );
	sws_scale( new_context, input.data, input.linesize, 0, height,
		output.data, output.linesize);
	sws_freeContext( new_context );

#else
	img_convert( &output, out_fmt, &input, in_fmt, width, height );
#endif
}

/** Do it :-).
*/

static int convert_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, mlt_image_format output_format )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );
	int error = 0;

	if ( *format != output_format )
	{
		mlt_log_debug( NULL, "[filter avcolor_space] %s -> %s\n",
			mlt_image_format_name( *format ), mlt_image_format_name( output_format ) );

		int in_fmt = convert_mlt_to_av_cs( *format );
		int out_fmt = convert_mlt_to_av_cs( output_format );
		int size = avpicture_get_size( out_fmt, width, height );
		uint8_t *output = mlt_pool_alloc( size );

		if ( *format == mlt_image_rgb24a || *format == mlt_image_opengl )
		{
			register int len = width * height;
			uint8_t *alpha = mlt_pool_alloc( len );

			if ( alpha )
			{
				// Extract the alpha mask from the RGBA image using Duff's Device
				register uint8_t *s = *image + 3; // start on the alpha component
				register uint8_t *d = alpha;
				register int n = ( len + 7 ) / 8;

				switch ( len % 8 )
				{
					case 0:	do { *d++ = *s; s += 4;
					case 7:		 *d++ = *s; s += 4;
					case 6:		 *d++ = *s; s += 4;
					case 5:		 *d++ = *s; s += 4;
					case 4:		 *d++ = *s; s += 4;
					case 3:		 *d++ = *s; s += 4;
					case 2:		 *d++ = *s; s += 4;
					case 1:		 *d++ = *s; s += 4;
							}
							while ( --n > 0 );
				}
				mlt_properties_set_data( properties, "alpha", alpha, len, mlt_pool_release, NULL );
				frame->get_alpha_mask = NULL;
			}
		}

		// Update the output
		mlt_producer producer = mlt_frame_get_original_producer( frame );
		mlt_properties prod_props = MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( producer ) );
		av_convert_image( prod_props, output, *image, out_fmt, in_fmt, width, height );
		*image = output;
		*format = output_format;
		mlt_properties_set_data( properties, "image", output, size, mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "format", output_format );

		if ( output_format == mlt_image_rgb24a || output_format == mlt_image_opengl )
		{
			register int len = width * height;
			int alpha_size = 0;
			uint8_t *alpha = mlt_frame_get_alpha_mask( frame );
			mlt_properties_get_data( properties, "alpha", &alpha_size );

			if ( alpha && alpha_size >= len )
			{
				// Merge the alpha mask from into the RGBA image using Duff's Device
				register uint8_t *s = alpha;
				register uint8_t *d = *image + 3; // start on the alpha component
				register int n = ( len + 7 ) / 8;

				switch ( len % 8 )
				{
					case 0:	do { *d = *s++; d += 4;
					case 7:		 *d = *s++; d += 4;
					case 6:		 *d = *s++; d += 4;
					case 5:		 *d = *s++; d += 4;
					case 4:		 *d = *s++; d += 4;
					case 3:		 *d = *s++; d += 4;
					case 2:		 *d = *s++; d += 4;
					case 1:		 *d = *s++; d += 4;
							}
							while ( --n > 0 );
				}
			}
		}
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	frame->convert_image = convert_image;
	return frame;
}

static void filter_close( mlt_filter filter )
{
#ifdef SWSCALE
	struct SwsContext *context = mlt_properties_get_data( MLT_FILTER_PROPERTIES(filter), "avcolorspace.swscale", NULL );
	sws_freeContext( context );
#endif
}

/** Constructor for the filter.
*/

mlt_filter filter_avcolour_space_init( void *arg )
{
#ifdef SWSCALE
#if (LIBSWSCALE_VERSION_INT >= ((0<<16)+(7<<8)+2))
	// Test to see if swscale accepts the arg as resolution
	if ( arg )
	{
		int width = (int) arg;
		struct SwsContext *context = sws_getContext( width, width, PIX_FMT_RGB32, 64, 64, PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
		if ( context )
			sws_freeContext( context );
		else
			return NULL;
	}		
#else
	return NULL;
#endif
#endif
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		this->close = filter_close;
	}
	return this;
}

