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
		case mlt_image_opengl:
		case mlt_image_none:
			fprintf( stderr, "Invalid format...\n" );
			break;
	}

	return value;
}

static inline void convert_image( uint8_t *out, uint8_t *in, int out_fmt, int in_fmt, int width, int height )
{
	AVPicture input;
	AVPicture output;
	avpicture_fill( &output, out, out_fmt, width, height );
	avpicture_fill( &input, in, in_fmt, width, height );
	img_convert( &output, out_fmt, &input, in_fmt, width, height );
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

	if ( error == 0 && *format != output_format && *image != NULL && output_format != mlt_image_opengl )
	{
		int in_fmt = convert_mlt_to_av_cs( *format );
		int out_fmt = convert_mlt_to_av_cs( output_format );
		int size = avpicture_get_size( out_fmt, *width, *height );
		uint8_t *output = mlt_pool_alloc( size );
		convert_image( output, *image, out_fmt, in_fmt, *width, *height );

		// Special case for alpha rgb input
		if ( *format == mlt_image_rgb24a )
		{
			register uint8_t *alpha = mlt_frame_get_alpha_mask( this );
			register int len = *width * *height;
			register uint8_t *bits = *image;
			register int n = ( len + 7 ) / 8;

			// TODO: Proper check for big endian systems
			#ifndef __DARWIN__
			bits += 3;
			#endif

			// Extract alpha mask from the image using Duff's Device
			switch( len % 8 )
			{
				case 0:	do { *alpha ++ = *bits; bits += 4;
				case 7:		 *alpha ++ = *bits; bits += 4;
				case 6:		 *alpha ++ = *bits; bits += 4;
				case 5:		 *alpha ++ = *bits; bits += 4;
				case 4:		 *alpha ++ = *bits; bits += 4;
				case 3:		 *alpha ++ = *bits; bits += 4;
				case 2:		 *alpha ++ = *bits; bits += 4;
				case 1:		 *alpha ++ = *bits; bits += 4;
						}
						while( --n );
			}
		}

		// Update the output
		*image = output;
		*format = output_format;
		mlt_properties_set_data( properties, "image", output, size, mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "format", output_format );

		// Special case for alpha rgb output
		if ( *format == mlt_image_rgb24a )
		{
			// Fetch the alpha
			register uint8_t *alpha = mlt_frame_get_alpha_mask( this );

			if ( alpha != NULL )
			{
				register uint8_t *bits = *image;
				register int len = *width * *height;
				register int n = ( len + 7 ) / 8;

				// TODO: Proper check for big endian systems
				#ifndef __DARWIN__
				bits += 3;
				#endif

				// Merge the alpha mask into the RGB image using Duff's Device
				switch( len % 8 )
				{
					case 0:	do { *bits = *alpha++; bits += 4;
					case 7:		 *bits = *alpha++; bits += 4;
					case 6:		 *bits = *alpha++; bits += 4;
					case 5:		 *bits = *alpha++; bits += 4;
					case 4:		 *bits = *alpha++; bits += 4;
					case 3:		 *bits = *alpha++; bits += 4;
					case 2:		 *bits = *alpha++; bits += 4;
					case 1:		 *bits = *alpha++; bits += 4;
							}
							while( --n );
				}
			}
		}
	}
	else if ( error == 0 && *format != output_format && *image != NULL && output_format == mlt_image_opengl )
	{
		if ( *format == mlt_image_yuv422 )
		{
			int size = *width * *height * 4;
			uint8_t *output = mlt_pool_alloc( size );
			int h = *height;
			int w = *width;
			uint8_t *o = output + size;
			int ostride = w * 4;
			uint8_t *p = *image;
			uint8_t *alpha = mlt_frame_get_alpha_mask( this ) + *width * *height;
			int r, g, b;

			while( h -- )
			{
				w = *width;
				o -= ostride;
				alpha -= *width;
				while( w >= 2 )
				{
					YUV2RGB( *p, *( p + 1 ), *( p + 3 ), r, g, b );
					*o ++ = r;
					*o ++ = g;
					*o ++ = b;
					*o ++ = *alpha ++;
					YUV2RGB( *( p + 2 ), *( p + 1 ), *( p + 3 ), r, g, b );
					*o ++ = r;
					*o ++ = g;
					*o ++ = b;
					*o ++ = *alpha ++;
					w -= 2;
					p += 4;
				}
				o -= ostride;
				alpha -= *width;
			}

			mlt_properties_set_data( properties, "image", output, size, mlt_pool_release, NULL );
			mlt_properties_set_int( properties, "format", output_format );
			*image = output;
			*format = output_format;
		}
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

