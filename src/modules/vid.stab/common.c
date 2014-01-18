/*
 * common.c
 * Copyright (C) 2014 Brian Matherly <pez4brian@yahoo.com>
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
#include <framework/mlt.h>
#include <vid.stab/libvidstab.h>
#include "common.h"

mlt_image_format validate_format( mlt_image_format format )
{
	switch( format )
	{
	case mlt_image_rgb24a:
	case mlt_image_rgb24:
		return mlt_image_rgb24;
	case mlt_image_yuv420p:
		return mlt_image_yuv420p;
	default:
	case mlt_image_none:
	case mlt_image_yuv422:
	case mlt_image_opengl:
	case mlt_image_glsl:
	case mlt_image_glsl_texture:
		return mlt_image_yuv422;
	}
}

VSPixelFormat mltimage_to_vsimage( mlt_image_format mlt_format, int width, int height, uint8_t* mlt_img, uint8_t** vs_img )
{
	switch( mlt_format )
	{
	case mlt_image_rgb24:
		// Convert RGB24 to YUV444 because it is the only planar
		// format with comparable bit depth.
		{
			*vs_img = mlt_pool_alloc( width * height * 3 );
			int y, u, v, r, g, b;
			int total = width * height + 1;
			uint8_t* yp = *vs_img;
			uint8_t* up = yp + ( width * height );
			uint8_t* vp = up + ( width * height );

			while( --total )
			{
				r = *mlt_img++;
				g = *mlt_img++;
				b = *mlt_img++;
				RGB2YUV_601_SCALED(r, g, b, y, u, v);
				*yp++ = y;
				*up++ = u;
				*vp++ = v;
			}

			return PF_YUV444P;
		}
	case mlt_image_yuv420p:
		// This format maps with no conversion
		{
			*vs_img = mlt_img;
			return PF_YUV420P;
		}
	case mlt_image_yuv422:
		// Convert packed to planar
		{
			*vs_img = mlt_pool_alloc( width * height * 2 );
			uint8_t* yp = *vs_img;
			uint8_t* up = yp + ( width * height );
			uint8_t* vp = up + ( width * height / 2 );
			int total = ( width * height / 2 ) + 1;

			while( --total )
			{
				*yp++ = *mlt_img++;
				*up++ = *mlt_img++;
				*yp++ = *mlt_img++;
				*vp++ = *mlt_img++;
			}

			return PF_YUV422P;
		}
	default:
		return PF_NONE;
	}
}

void vsimage_to_mltimage( uint8_t* vs_img, uint8_t* mlt_img, mlt_image_format mlt_format, int width, int height )
{
	switch( mlt_format )
	{
	case mlt_image_rgb24:
		// Convert YUV444 to RGB24.
		{
			int y, u, v, r, g, b;
			int total = width * height + 1;
			uint8_t* yp = vs_img;
			uint8_t* up = yp + ( width * height );
			uint8_t* vp = up + ( width * height );

			while( --total )
			{
				y = *yp++;
				u = *up++;
				v = *vp++;
				YUV2RGB_601_SCALED( y, u, v, r, g, b );
				*mlt_img++ = r;
				*mlt_img++ = g;
				*mlt_img++ = b;
			}
		}
		break;
	case mlt_image_yuv420p:
		// This format was never converted
		break;
	case mlt_image_yuv422:
		// Convert planar to packed
		{
			uint8_t* yp = vs_img;
			uint8_t* up = yp + ( width * height );
			uint8_t* vp = up + ( width * height / 2 );
			int total = ( width * height / 2 ) + 1;

			while( --total )
			{
				*mlt_img++ = *yp++;
				*mlt_img++ = *up++;
				*mlt_img++ = *yp++;
				*mlt_img++ = *vp++;
			}
		}
		break;
	default:
		return PF_NONE;
	}
}

void free_vsimage( uint8_t* vs_img, VSPixelFormat format )
{
	if( format != PF_YUV420P )
	{
		mlt_pool_release( vs_img );
	}
}
