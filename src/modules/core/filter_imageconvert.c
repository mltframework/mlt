/*
 * filter_imageconvert.c -- colorspace and pixel format converter
 * Copyright (C) 2009-2014 Meltytech, LLC
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
#include <framework/mlt_log.h>
#include <framework/mlt_pool.h>

#include <stdlib.h>

/** This macro converts a YUV value to the RGB color space. */
#define RGB2YUV_601_UNSCALED(r, g, b, y, u, v)\
  y = (299*r + 587*g + 114*b) >> 10;\
  u = ((-169*r - 331*g + 500*b) >> 10) + 128;\
  v = ((500*r - 419*g - 81*b) >> 10) + 128;\
  y = y < 16 ? 16 : y;\
  u = u < 16 ? 16 : u;\
  v = v < 16 ? 16 : v;\
  y = y > 235 ? 235 : y;\
  u = u > 240 ? 240 : u;\
  v = v > 240 ? 240 : v

/** This macro converts a YUV value to the RGB color space. */
#define YUV2RGB_601_UNSCALED( y, u, v, r, g, b ) \
  r = ((1024 * y + 1404 * ( v - 128 ) ) >> 10 ); \
  g = ((1024 * y - 715 * ( v - 128 ) - 345 * ( u - 128 ) ) >> 10 ); \
  b = ((1024 * y + 1774 * ( u - 128 ) ) >> 10 ); \
  r = r < 0 ? 0 : r > 255 ? 255 : r; \
  g = g < 0 ? 0 : g > 255 ? 255 : g; \
  b = b < 0 ? 0 : b > 255 ? 255 : b;

#define SCALED 1
#if SCALED
#define RGB2YUV_601 RGB2YUV_601_SCALED
#define YUV2RGB_601 YUV2RGB_601_SCALED
#else
#define RGB2YUV_601 RGB2YUV_601_UNSCALED
#define YUV2RGB_601 YUV2RGB_601_UNSCALED
#endif

static int convert_yuv422_to_rgb24a( uint8_t *yuv, uint8_t *rgba, uint8_t *alpha, int width, int height )
{
	int ret = 0;
	int yy, uu, vv;
	int r,g,b;
	int total = width * height / 2 + 1;

	while ( --total )
	{
		yy = yuv[0];
		uu = yuv[1];
		vv = yuv[3];
		YUV2RGB_601( yy, uu, vv, r, g, b );
		rgba[0] = r;
		rgba[1] = g;
		rgba[2] = b;
		rgba[3] = *alpha++;
		yy = yuv[2];
		YUV2RGB_601( yy, uu, vv, r, g, b );
		rgba[4] = r;
		rgba[5] = g;
		rgba[6] = b;
		rgba[7] = *alpha++;
		yuv += 4;
		rgba += 8;
	}
	return ret;
}

static int convert_yuv422_to_rgb24( uint8_t *yuv, uint8_t *rgb, uint8_t *alpha, int width, int height )
{
	int ret = 0;
	int yy, uu, vv;
	int r,g,b;
	int total = width * height / 2 + 1;

	while ( --total )
	{
		yy = yuv[0];
		uu = yuv[1];
		vv = yuv[3];
		YUV2RGB_601( yy, uu, vv, r, g, b );
		rgb[0] = r;
		rgb[1] = g;
		rgb[2] = b;
		yy = yuv[2];
		YUV2RGB_601( yy, uu, vv, r, g, b );
		rgb[3] = r;
		rgb[4] = g;
		rgb[5] = b;
		yuv += 4;
		rgb += 6;
	}
	return ret;
}

static int convert_rgb24a_to_yuv422( uint8_t *rgba, uint8_t *yuv, uint8_t *alpha, int width, int height )
{
	int ret = 0;
	int stride = width * 4;
	int y0, y1, u0, u1, v0, v1;
	int r, g, b;
	uint8_t *s, *d = yuv;
	int i, j, n = width / 2 + 1;

	if ( alpha )
	for ( i = 0; i < height; i++ )
	{
		s = rgba + ( stride * i );
		j = n;
		while ( --j )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV_601( r, g, b, y1, u1 , v1 );
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
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			*d++ = y0;
			*d++ = u0;
		}
	}
	else
	for ( i = 0; i < height; i++ )
	{
		s = rgba + ( stride * i );
		j = n;
		while ( --j )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			s++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			r = *s++;
			g = *s++;
			b = *s++;
			s++;
			RGB2YUV_601( r, g, b, y1, u1 , v1 );
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
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			*d++ = y0;
			*d++ = u0;
		}
	}

	return ret;
}

static int convert_rgb24_to_yuv422( uint8_t *rgb, uint8_t *yuv, uint8_t *alpha, int width, int height )
{
	int ret = 0;
	int stride = width * 3;
	int y0, y1, u0, u1, v0, v1;
	int r, g, b;
	uint8_t *s, *d = yuv;
	int i, j, n = width / 2 + 1;

	for ( i = 0; i < height; i++ )
	{
		s = rgb + ( stride * i );
		j = n;
		while ( --j )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV_601( r, g, b, y1, u1 , v1 );
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
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			*d++ = y0;
			*d++ = u0;
		}
	}
	return ret;
}

static int convert_yuv420p_to_yuv422( uint8_t *yuv420p, uint8_t *yuv, uint8_t *alpha, int width, int height )
{
	int ret = 0;
	int i, j;
	int half = width >> 1;
	uint8_t *Y = yuv420p;
	uint8_t *U = Y + width * height;
	uint8_t *V = U + width * height / 4;
	uint8_t *d = yuv;

	for ( i = 0; i < height; i++ )
	{
		uint8_t *u = U + ( i / 2 ) * ( half );
		uint8_t *v = V + ( i / 2 ) * ( half );

		j = half + 1;
		while ( --j )
		{
			*d ++ = *Y ++;
			*d ++ = *u ++;
			*d ++ = *Y ++;
			*d ++ = *v ++;
		}
	}
	return ret;
}

static int convert_rgb24_to_rgb24a( uint8_t *rgb, uint8_t *rgba, uint8_t *alpha, int width, int height )
{
	uint8_t *s = rgb;
	uint8_t *d = rgba;
	int total = width * height + 1;

	while ( --total )
	{
		*d++ = s[0];
		*d++ = s[1];
		*d++ = s[2];
		*d++ = 0xff;
		s += 3;
	}
	return 0;
}

static int convert_rgb24a_to_rgb24( uint8_t *rgba, uint8_t *rgb, uint8_t *alpha, int width, int height )
{
	uint8_t *s = rgba;
	uint8_t *d = rgb;
	int total = width * height + 1;

	while ( --total )
	{
		*d++ = s[0];
		*d++ = s[1];
		*d++ = s[2];
		*alpha++ = s[3];
		s += 4;
	}
	return 0;
}

typedef int ( *conversion_function )( uint8_t *yuv, uint8_t *rgba, uint8_t *alpha, int width, int height );

static conversion_function conversion_matrix[ mlt_image_invalid - 1 ][ mlt_image_invalid - 1 ] = {
	{ NULL, convert_rgb24_to_rgb24a, convert_rgb24_to_yuv422, NULL, convert_rgb24_to_rgb24a, NULL },
	{ convert_rgb24a_to_rgb24, NULL, convert_rgb24a_to_yuv422, NULL, NULL, NULL },
	{ convert_yuv422_to_rgb24, convert_yuv422_to_rgb24a, NULL, NULL, convert_yuv422_to_rgb24a, NULL },
	{ NULL, NULL, convert_yuv420p_to_yuv422, NULL, NULL, NULL },
	{ convert_rgb24a_to_rgb24, NULL, convert_rgb24a_to_yuv422, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL },
};

static uint8_t bpp_table[4] = { 3, 4, 2, 0 };

static int convert_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, mlt_image_format requested_format )
{
	int error = 0;
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );

	if ( *format != requested_format )
	{
		conversion_function converter = conversion_matrix[ *format - 1 ][ requested_format - 1 ];

		mlt_log_debug( NULL, "[filter imageconvert] %s -> %s @ %dx%d\n",
			mlt_image_format_name( *format ), mlt_image_format_name( requested_format ),
			width, height );
		if ( converter )
		{
			int size = width * height * bpp_table[ requested_format - 1 ];
			int alpha_size = width * height;
			uint8_t *image = mlt_pool_alloc( size );
			uint8_t *alpha = ( *format == mlt_image_rgb24a ||
			                   *format == mlt_image_opengl )
			                 ? mlt_pool_alloc( width * height ) : NULL;
			if ( requested_format == mlt_image_rgb24a || requested_format == mlt_image_opengl )
			{
				if ( alpha )
					mlt_pool_release( alpha );
				alpha = mlt_frame_get_alpha_mask( frame );
				mlt_properties_get_data( properties, "alpha", &alpha_size );
			}

			if ( !( error = converter( *buffer, image, alpha, width, height ) ) )
			{
				mlt_frame_set_image( frame, image, size, mlt_pool_release );
				if ( alpha && ( *format == mlt_image_rgb24a || *format == mlt_image_opengl ) )
					mlt_frame_set_alpha( frame, alpha, alpha_size, mlt_pool_release );
				*buffer = image;
				*format = requested_format;
			}
			else
			{
				mlt_pool_release( image );
				if ( alpha )
					mlt_pool_release( alpha );
			}
		}
		else
		{
			error = 1;
		}
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	if ( !frame->convert_image )
		frame->convert_image = convert_image;
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_imageconvert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( mlt_filter_init( filter, filter ) == 0 )
	{
		filter->process = filter_process;
	}
	return filter;
}

