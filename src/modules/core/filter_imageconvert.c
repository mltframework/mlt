/*
 * filter_imageconvert.c -- colorspace and pixel format converter
 * Copyright (C) 2009 Ushodaya Enterprises Limited
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
#include <framework/mlt_log.h>
#include <framework/mlt_pool.h>

#include <stdlib.h>


static int convert_yuv422_to_rgb24a( uint8_t *yuv, uint8_t *rgba, unsigned int total )
{
	int ret = 0;
	int yy, uu, vv;
	int r,g,b;
	total /= 2;
	while (total--)
	{
		yy = yuv[0];
		uu = yuv[1];
		vv = yuv[3];
		YUV2RGB(yy, uu, vv, r, g, b);
		rgba[0] = r;
		rgba[1] = g;
		rgba[2] = b;
		rgba[3] = 255;
		yy = yuv[2];
		YUV2RGB(yy, uu, vv, r, g, b);
		rgba[4] = r;
		rgba[5] = g;
		rgba[6] = b;
		rgba[7] = 255;
		yuv += 4;
		rgba += 8;
	}
	return ret;
}

static int convert_yuv422_to_rgb24( uint8_t *yuv, uint8_t *rgb, unsigned int total )
{
	int ret = 0;
	int yy, uu, vv;
	int r,g,b;
	total /= 2;
	while (total--) 
	{
		yy = yuv[0];
		uu = yuv[1];
		vv = yuv[3];
		YUV2RGB(yy, uu, vv, r, g, b);
		rgb[0] = r;
		rgb[1] = g;
		rgb[2] = b;
		yy = yuv[2];
		YUV2RGB(yy, uu, vv, r, g, b);
		rgb[3] = r;
		rgb[4] = g;
		rgb[5] = b;
		yuv += 4;
		rgb += 6;
	}
	return ret;
}

static int convert_rgb24a_to_yuv422( uint8_t *rgba, int width, int height, int stride, uint8_t *yuv, uint8_t *alpha )
{
	int ret = 0;
	register int y0, y1, u0, u1, v0, v1;
	register int r, g, b;
	register uint8_t *d = yuv;
	register int i, j;

	if ( alpha )
	for ( i = 0; i < height; i++ )
	{
		register uint8_t *s = rgba + ( stride * i );
		for ( j = 0; j < ( width / 2 ); j++ )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV (r, g, b, y1, u1 , v1);
			*d++ = y0;
			*d++ = (u0+u1) >> 1;
			*d++ = y1;
			*d++ = (v0+v1) >> 1;
		}
		if ( width % 2 )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			*d++ = y0;
			*d++ = u0;
		}
	}
	else
	for ( i = 0; i < height; i++ )
	{
		register uint8_t *s = rgba + ( stride * i );
		for ( j = 0; j < ( width / 2 ); j++ )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			r = *s++;
			g = *s++;
			b = *s++;
			s++;
			RGB2YUV (r, g, b, y1, u1 , v1);
			*d++ = y0;
			*d++ = (u0+u1) >> 1;
			*d++ = y1;
			*d++ = (v0+v1) >> 1;
		}
		if ( width % 2 )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			*d++ = y0;
			*d++ = u0;
		}
	}

	return ret;
}

static int convert_rgb24_to_yuv422( uint8_t *rgb, int width, int height, int stride, uint8_t *yuv )
{
	int ret = 0;
	register int y0, y1, u0, u1, v0, v1;
	register int r, g, b;
	register uint8_t *d = yuv;
	register int i, j;

	for ( i = 0; i < height; i++ )
	{
		register uint8_t *s = rgb + ( stride * i );
		for ( j = 0; j < ( width / 2 ); j++ )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV (r, g, b, y1, u1 , v1);
			*d++ = y0;
			*d++ = (u0+u1) >> 1;
			*d++ = y1;
			*d++ = (v0+v1) >> 1;
		}
		if ( width % 2 )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			*d++ = y0;
			*d++ = u0;
		}
	}
	return ret;
}

static int convert_yuv420p_to_yuv422( uint8_t *yuv420p, int width, int height, uint8_t *yuv )
{
	int ret = 0;
	register int i, j;

	int half = width >> 1;

	uint8_t *Y = yuv420p;
	uint8_t *U = Y + width * height;
	uint8_t *V = U + width * height / 4;

	register uint8_t *d = yuv;

	for ( i = 0; i < height; i++ )
	{
		register uint8_t *u = U + ( i / 2 ) * ( half );
		register uint8_t *v = V + ( i / 2 ) * ( half );

		for ( j = 0; j < half; j++ )
		{
			*d ++ = *Y ++;
			*d ++ = *u ++;
			*d ++ = *Y ++;
			*d ++ = *v ++;
		}
	}
	return ret;
}

static int convert_rgb24_to_rgb24a( uint8_t *rgb, uint8_t *rgba, int total )
{
	uint8_t *s = rgb;
	uint8_t *d = rgba;
	while ( total-- )
	{
		*d++ = s[0];
		*d++ = s[1];
		*d++ = s[2];
		*d++ = 0xff;
		s += 3;
	}
	return 0;
}

static int convert_rgb24a_to_rgb24( uint8_t *rgba, uint8_t *rgb, int total, uint8_t *alpha )
{
	uint8_t *s = rgba;
	uint8_t *d = rgb;
	while ( total-- )
	{
		*d++ = s[0];
		*d++ = s[1];
		*d++ = s[2];
		*alpha++ = s[3];
		s += 4;
	}
	return 0;
}

static int convert_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, mlt_image_format requested_format )
{
	int error = 0;
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );

	if ( *format != requested_format )
	{
		mlt_log_debug( NULL, "[filter imageconvert] %s -> %s\n",
			mlt_image_format_name( *format ), mlt_image_format_name( requested_format ) );
		switch ( *format )
		{
		case mlt_image_yuv422:
			switch ( requested_format )
			{
			case mlt_image_rgb24:
			{
				uint8_t *rgb = mlt_pool_alloc( width * height * 3 );
				error = convert_yuv422_to_rgb24( *buffer, rgb, width * height );
				if ( error )
				{
					mlt_pool_release( rgb );
				}
				else
				{
					mlt_properties_set_data( properties, "image", rgb, width * height * 3, mlt_pool_release, NULL );
					*buffer = rgb;
					*format = mlt_image_rgb24;
				}
				break;
			}
			case mlt_image_rgb24a:
			case mlt_image_opengl:
			{
				uint8_t *rgba = mlt_pool_alloc( width * height * 4 );
				error = convert_yuv422_to_rgb24a( *buffer, rgba, width * height );
				if ( error )
				{
					mlt_pool_release( rgba );
				}
				else
				{
					mlt_properties_set_data( properties, "image", rgba, width * height * 4, mlt_pool_release, NULL );
					*buffer = rgba;
					*format = requested_format;
				}
				break;
			}
			default:
				error = 1;
			}
			break;
		case mlt_image_rgb24:
			switch ( requested_format )
			{
			case mlt_image_yuv422:
			{
				uint8_t *yuv = mlt_pool_alloc( width * height * 2 );
				error = convert_rgb24_to_yuv422( *buffer, width, height, width * 3, yuv );
				if ( error )
				{
					mlt_pool_release( yuv );
				}
				else
				{
					mlt_properties_set_data( properties, "image", yuv, width * height * 2, mlt_pool_release, NULL );
					*buffer = yuv;
					*format = mlt_image_yuv422;
				}
				break;
			}
			case mlt_image_rgb24a:
			case mlt_image_opengl:
			{
				uint8_t *rgba = mlt_pool_alloc( width * height * 4 );
				error = convert_rgb24_to_rgb24a( *buffer, rgba, width * height );
				if ( error )
				{
					mlt_pool_release( rgba );
				}
				else
				{
					mlt_properties_set_data( properties, "image", rgba, width * height * 4, mlt_pool_release, NULL );
					*buffer = rgba;
					*format = requested_format;
				}
				break;
			}
			default:
				error = 1;
			}
			break;
		case mlt_image_rgb24a:
		case mlt_image_opengl:
			switch ( requested_format )
			{
			case mlt_image_yuv422:
			{
				uint8_t *yuv = mlt_pool_alloc( width * height * 2 );
				uint8_t *alpha = mlt_pool_alloc( width * height );
				error = convert_rgb24a_to_yuv422( *buffer, width, height, width * 4, yuv, alpha );
				if ( error )
				{
					mlt_pool_release( yuv );
					mlt_pool_release( alpha );
				}
				else
				{
					mlt_properties_set_data( properties, "image", yuv, width * height * 2, mlt_pool_release, NULL );
					mlt_properties_set_data( properties, "alpha", alpha, width * height, mlt_pool_release, NULL );
					*buffer = yuv;
					*format = mlt_image_yuv422;
				}
				break;
			}
			case mlt_image_rgb24:
			{
				uint8_t *rgb = mlt_pool_alloc( width * height * 3 );
				uint8_t *alpha = mlt_pool_alloc( width * height );
				error = convert_rgb24a_to_rgb24( *buffer, rgb, width * height, alpha );
				if ( error )
				{
					mlt_pool_release( rgb );
					mlt_pool_release( alpha );
				}
				else
				{
					mlt_properties_set_data( properties, "image", rgb, width * height * 3, mlt_pool_release, NULL );
					mlt_properties_set_data( properties, "alpha", alpha, width * height, mlt_pool_release, NULL );
					*buffer = rgb;
					*format = mlt_image_rgb24;
				}
				break;
			}
			case mlt_image_rgb24a:
			case mlt_image_opengl:
				*format = requested_format;
				break;
			default:
				error = 1;
			}
			break;
		case mlt_image_yuv420p:
			switch ( requested_format )
			{
			case mlt_image_rgb24:
			case mlt_image_rgb24a:
			case mlt_image_opengl:
			case mlt_image_yuv422:
			{
				uint8_t *yuv = mlt_pool_alloc( width * height * 2 );
				int error = convert_yuv420p_to_yuv422( *buffer, width, height, yuv );
				if ( error )
				{
					mlt_pool_release( yuv );
				}
				else
				{
					mlt_properties_set_data( properties, "image", yuv, width * height * 2, mlt_pool_release, NULL );
					*buffer = yuv;
					*format = mlt_image_yuv422;
				}
				switch ( requested_format )
				{
				case mlt_image_yuv422:
					break;
				case mlt_image_rgb24:
				case mlt_image_rgb24a:
				case mlt_image_opengl:
					error = convert_image( frame, buffer, format, requested_format );
					break;
				default:
					error = 1;
					break;
				}
				break;
			}
			default:
				error = 1;
			}
			break;
		default:
			error = 1;
		}
	}
	if ( !error )
		mlt_properties_set_int( properties, "format", *format );

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	frame->convert_image = convert_image;
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_imageconvert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
	{
		this->process = filter_process;
	}
	return this;
}
