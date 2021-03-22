/*
 * filter_imageconvert.c -- colorspace and pixel format converter
 * Copyright (C) 2009-2021 Meltytech, LLC
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
#include <framework/mlt_image.h>
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

static void convert_yuv422_to_rgba( mlt_image src, mlt_image dst )
{
	int yy, uu, vv;
	int r,g,b;

	mlt_image_set_values( dst, NULL, mlt_image_rgba, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pAlpha = src->planes[3] + src->strides[3] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int total = src->width / 2 + 1;

		if ( pAlpha )
			while ( --total )
			{
				yy = pSrc[0];
				uu = pSrc[1];
				vv = pSrc[3];
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[0] = r;
				pDst[1] = g;
				pDst[2] = b;
				pDst[3] = *pAlpha++;
				yy = pSrc[2];
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[4] = r;
				pDst[5] = g;
				pDst[6] = b;
				pDst[7] = *pAlpha++;
				pSrc += 4;
				pDst += 8;
			}
		else
			while ( --total )
			{
				yy = pSrc[0];
				uu = pSrc[1];
				vv = pSrc[3];
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[0] = r;
				pDst[1] = g;
				pDst[2] = b;
				pDst[3] = 0xff;
				yy = pSrc[2];
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[4] = r;
				pDst[5] = g;
				pDst[6] = b;
				pDst[7] = 0xff;
				pSrc += 4;
				pDst += 8;
			}
	}
}

static void convert_yuv422_to_rgb( mlt_image src, mlt_image dst )
{
	int yy, uu, vv;
	int r,g,b;

	mlt_image_set_values( dst, NULL, mlt_image_rgb, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int total = src->width / 2 + 1;
		while ( --total )
		{
			yy = pSrc[0];
			uu = pSrc[1];
			vv = pSrc[3];
			YUV2RGB_601( yy, uu, vv, r, g, b );
			pDst[0] = r;
			pDst[1] = g;
			pDst[2] = b;
			yy = pSrc[2];
			YUV2RGB_601( yy, uu, vv, r, g, b );
			pDst[3] = r;
			pDst[4] = g;
			pDst[5] = b;
			pSrc += 4;
			pDst += 6;
		}
	}
}

static void convert_rgba_to_yuv422( mlt_image src, mlt_image dst )
{
	int y0, y1, u0, u1, v0, v1;
	int r, g, b;

	mlt_image_set_values( dst, NULL, mlt_image_yuv422, src->width, src->height );
	mlt_image_alloc_data( dst );
	mlt_image_alloc_alpha( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		uint8_t* pAlpha = dst->planes[3] + dst->strides[3] * line;
		int j = src->width / 2 + 1;
		while ( --j )
		{
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			*pAlpha++ = *pSrc++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			*pAlpha++ = *pSrc++;
			RGB2YUV_601( r, g, b, y1, u1 , v1 );
			*pDst++ = y0;
			*pDst++ = (u0+u1) >> 1;
			*pDst++ = y1;
			*pDst++ = (v0+v1) >> 1;
		}
		if ( src->width % 2 )
		{
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			*pAlpha++ = *pSrc++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			*pDst++ = y0;
			*pDst++ = u0;
		}
	}
}

static void convert_rgb_to_yuv422( mlt_image src, mlt_image dst )
{
	int y0, y1, u0, u1, v0, v1;
	int r, g, b;

	mlt_image_set_values( dst, NULL, mlt_image_yuv422, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int j = src->width / 2 + 1;
		while ( --j )
		{
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			RGB2YUV_601( r, g, b, y1, u1 , v1 );
			*pDst++ = y0;
			*pDst++ = (u0+u1) >> 1;
			*pDst++ = y1;
			*pDst++ = (v0+v1) >> 1;
		}
		if ( src->width % 2 )
		{
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			RGB2YUV_601( r, g, b, y0, u0 , v0 );
			*pDst++ = y0;
			*pDst++ = u0;
		}
	}
}

static void convert_yuv420p_to_yuv422( mlt_image src, mlt_image dst )
{
	mlt_image_set_values( dst, NULL, mlt_image_yuv422, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrcY = src->planes[0] + src->strides[0] * line;
		uint8_t* pSrcU = src->planes[1] + src->strides[1] * line / 2;
		uint8_t* pSrcV = src->planes[2] + src->strides[2] * line / 2;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int j = src->width / 2 + 1;
		while ( --j )
		{
			*pDst++ = *pSrcY++;
			*pDst++ = *pSrcU++;
			*pDst++ = *pSrcY++;
			*pDst++ = *pSrcV++;
		}
	}
}

static void convert_yuv420p_to_rgb( mlt_image src, mlt_image dst )
{
	int yy, uu, vv;
	int r,g,b;

	mlt_image_set_values( dst, NULL, mlt_image_rgb, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrcY = src->planes[0] + src->strides[0] * line;
		uint8_t* pSrcU = src->planes[1] + src->strides[1] * line / 2;
		uint8_t* pSrcV = src->planes[2] + src->strides[2] * line / 2;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int total = src->width / 2 + 1;
		while ( --total )
		{
			yy = *pSrcY++;
			uu = *pSrcU++;
			vv = *pSrcV++;
			YUV2RGB_601( yy, uu, vv, r, g, b );
			pDst[0] = r;
			pDst[1] = g;
			pDst[2] = b;
			yy = *pSrcY++;
			YUV2RGB_601( yy, uu, vv, r, g, b );
			pDst[3] = r;
			pDst[4] = g;
			pDst[5] = b;
			pDst += 6;
		}
	}
}

static void convert_yuv420p_to_rgba( mlt_image src, mlt_image dst )
{
	int yy, uu, vv;
	int r,g,b;

	mlt_image_set_values( dst, NULL, mlt_image_rgba, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrcY = src->planes[0] + src->strides[0] * line;
		uint8_t* pSrcU = src->planes[1] + src->strides[1] * line / 2;
		uint8_t* pSrcV = src->planes[2] + src->strides[2] * line / 2;
		uint8_t* pSrcA = src->planes[3] + src->strides[3] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int total = src->width / 2 + 1;
		if ( pSrcA )
			while ( --total )
			{
				yy = *pSrcY++;
				uu = *pSrcU++;
				vv = *pSrcV++;
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[0] = r;
				pDst[1] = g;
				pDst[2] = b;
				pDst[3] = *pSrcA++;
				yy = *pSrcY++;
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[4] = r;
				pDst[5] = g;
				pDst[6] = b;
				pDst[7] = *pSrcA++;
				pDst += 8;
			}
		else
			while ( --total )
			{
				yy = *pSrcY++;
				uu = *pSrcU++;
				vv = *pSrcV++;
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[0] = r;
				pDst[1] = g;
				pDst[2] = b;
				pDst[3] = 0xff;
				yy = *pSrcY++;
				YUV2RGB_601( yy, uu, vv, r, g, b );
				pDst[4] = r;
				pDst[5] = g;
				pDst[6] = b;
				pDst[7] = 0xff;
				pDst += 8;
			}
	}
}

static void convert_yuv422_to_yuv420p( mlt_image src, mlt_image dst )
{
	int lines = src->height;
	int pixels = src->width;

	mlt_image_set_values( dst, NULL, mlt_image_yuv420p, src->width, src->height );
	mlt_image_alloc_data( dst );

	// Y
	for ( int line = 0; line < lines; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		for ( int pixel = 0; pixel < pixels; pixel++ )
		{
			*pDst++ = *pSrc;
			pSrc += 2;
		}
	}

	lines = src->height / 2;
	pixels = src->width / 2;

	// U
	for ( int line = 0; line < lines; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line * 2 + 1;
		uint8_t* pDst = dst->planes[1] + dst->strides[1] * line;
		for ( int pixel = 0; pixel < pixels; pixel++ )
		{
			*pDst++ = *pSrc;
			pSrc += 4;
		}
	}

	// V
	for ( int line = 0; line < lines; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line * 2 + 3;
		uint8_t* pDst = dst->planes[2] + dst->strides[2] * line;
		for ( int pixel = 0; pixel < pixels; pixel++ )
		{
			*pDst++ = *pSrc;
			pSrc += 4;
		}
	}
}

static void convert_rgb_to_rgba( mlt_image src, mlt_image dst )
{
	mlt_image_set_values( dst, NULL, mlt_image_rgba, src->width, src->height );
	mlt_image_alloc_data( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pAlpha = src->planes[3] + src->strides[3] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		int total = src->width + 1;
		if ( pAlpha )
			while ( --total )
			{
				*pDst++ = pSrc[0];
				*pDst++ = pSrc[1];
				*pDst++ = pSrc[2];
				*pDst++ = *pAlpha++;
				pSrc += 3;
			}
		else
			while ( --total )
			{
				*pDst++ = pSrc[0];
				*pDst++ = pSrc[1];
				*pDst++ = pSrc[2];
				*pDst++ = 0xff;
				pSrc += 3;
			}
	}
}

static void convert_rgba_to_rgb( mlt_image src, mlt_image dst )
{
	mlt_image_set_values( dst, NULL, mlt_image_rgb, src->width, src->height );
	mlt_image_alloc_data( dst );
	mlt_image_alloc_alpha( dst );

	for ( int line = 0; line < src->height; line++ )
	{
		uint8_t* pSrc = src->planes[0] + src->strides[0] * line;
		uint8_t* pDst = dst->planes[0] + dst->strides[0] * line;
		uint8_t* pAlpha = dst->planes[3] + dst->strides[3] * line;
		int total = src->width + 1;
		while ( --total )
		{
			*pDst++ = pSrc[0];
			*pDst++ = pSrc[1];
			*pDst++ = pSrc[2];
			*pAlpha++ = pSrc[3];
			pSrc += 4;
		}
	}
}

typedef void ( *conversion_function )( mlt_image src, mlt_image dst );

static conversion_function conversion_matrix[ mlt_image_invalid - 1 ][ mlt_image_invalid - 1 ] = {
	{ NULL, convert_rgb_to_rgba, convert_rgb_to_yuv422, NULL, NULL, NULL, NULL },
	{ convert_rgba_to_rgb, NULL, convert_rgba_to_yuv422, NULL, NULL, NULL, NULL },
	{ convert_yuv422_to_rgb, convert_yuv422_to_rgba, NULL, convert_yuv422_to_yuv420p, NULL, NULL, NULL },
	{ convert_yuv420p_to_rgb, convert_yuv420p_to_rgba, convert_yuv420p_to_yuv422, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

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
			struct mlt_image_s src;
			struct mlt_image_s dst;
			mlt_image_set_values( &src, *buffer, *format, width, height );
			converter( &src, &dst );
			mlt_frame_set_image( frame, dst.data, 0, dst.release_data );
			if ( requested_format == mlt_image_rgba )
			{
				// Clear the alpha buffer on the frame
				mlt_frame_set_alpha( frame, NULL, 0, NULL );
			}
			else if ( dst.alpha )
			{
				mlt_frame_set_alpha( frame, dst.alpha, 0, dst.release_alpha );
			}
			*buffer = dst.data;
			*format = dst.format;
		}
		else
		{
			mlt_log_error( NULL, "imageconvert: no conversion from %s to %s\n", mlt_image_format_name(*format), mlt_image_format_name(requested_format));
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

