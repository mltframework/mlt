/*
 * filter_avcolour_space.c -- Colour space filter
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
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

#include "filter_avcolour_space.h"

#include <framework/mlt_frame.h>

// ffmpeg Header files
#include <avformat.h>

#include <stdio.h>
#include <stdlib.h>

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
		case mlt_image_none:
			fprintf( stderr, "Invalid format...\n" );
			break;
	}

	return value;
}

static inline void convert_image( uint8_t *out, uint8_t *in, int out_fmt, int in_fmt, int width, int height )
{
	if ( in_fmt == PIX_FMT_YUV420P && out_fmt == PIX_FMT_YUV422 )
	{
		register int i, j;
		register int half = width >> 1;
		register uint8_t *Y = in;
		register uint8_t *U = Y + width * height;
		register uint8_t *V = U + width * height / 2;
		register uint8_t *d = out;
		register uint8_t *y, *u, *v;

		i = height >> 1;
		while ( i -- )
		{
			y = Y;
			u = U;
			v = V;
			j = half;
			while ( j -- )
			{
				*d ++ = *y ++;
				*d ++ = *u ++;
				*d ++ = *y ++;
				*d ++ = *v ++;
			}

			Y += width;
			y = Y;
			u = U;
			v = V;
			j = half;
			while ( j -- )
			{
				*d ++ = *y ++;
				*d ++ = *u ++;
				*d ++ = *y ++;
				*d ++ = *v ++;
			}

			Y += width;
			U += width / 2;
			V += width / 2;
		}
	}
	else 
	{
		AVPicture input;
		AVPicture output;
		avpicture_fill( &output, out, out_fmt, width, height );
		avpicture_fill( &input, in, in_fmt, width, height );
		img_convert( &output, out_fmt, &input, in_fmt, width, height );
	}
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service( this );
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );
	int output_format = *format;
	mlt_image_format forced = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "forced" );
	int error = 0;

	// Allow this filter to force processing in a colour space other than requested
	*format = forced != 0 ? forced : *format;
   
	error = mlt_frame_get_image( this, image, format, width, height, 0 );

	if ( error == 0 && *format != output_format )
	{
		int in_fmt = convert_mlt_to_av_cs( *format );
		int out_fmt = convert_mlt_to_av_cs( output_format );
		int size = avpicture_get_size( out_fmt, *width, *height );
		uint8_t *output = mlt_pool_alloc( size );
		convert_image( output, *image, out_fmt, in_fmt, *width, *height );
		*image = output;
		*format = output_format;
		mlt_properties_set_data( properties, "image", output, size, mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "format", output_format );
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_frame_push_service( frame, this );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_avcolour_space_init( void *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
		this->process = filter_process;
	return this;
}

