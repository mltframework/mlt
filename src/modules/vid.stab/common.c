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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

/** Convert an MLT image to one that can be used by VS.
 * Use free_vsimage() when done with the resulting image.
 */

VSPixelFormat mltimage_to_vsimage( mlt_image_format mlt_format, int width, int height, uint8_t* mlt_img, uint8_t** vs_img )
{
	switch( mlt_format )
	{
	case mlt_image_rgb24:
		// Convert packed RGB24 to planar YUV444
		// Note: vid.stab 0.98 does not seem to support PF_RGB24
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
		// Convert packed YUV422 to planar YUV444
		// Note: vid.stab 0.98 seems to suffer chroma bleeding
		// when using PF_YUV422P - which is why PF_YUV444P is used.
		{
			*vs_img = mlt_pool_alloc( width * height * 3 );
			uint8_t* yp = *vs_img;
			uint8_t* up = yp + ( width * height );
			uint8_t* vp = up + ( width * height );
			int i, j, n = width / 2 + 1;

			for ( i = 0; i < height; i++ )
			{
				j = n;
				while ( --j )
				{
					*yp++ = mlt_img[0];
					*up++ = mlt_img[1];
					*vp++ = mlt_img[3];
					*yp++ = mlt_img[2];
					*up++ = mlt_img[1];
					*vp++ = mlt_img[3];
					mlt_img += 4;
				}
				if ( width % 2 )
				{
					*yp++ = mlt_img[0];
					*up++ = mlt_img[1];
					*vp++ = (mlt_img - 4)[3];
					mlt_img += 2;
				}
			}

			return PF_YUV444P;
		}
	default:
		return PF_NONE;
	}
}

/** Convert a VS image back to the MLT image it originally came from in mltimage_to_vsimage().
 */

void vsimage_to_mltimage( uint8_t* vs_img, uint8_t* mlt_img, mlt_image_format mlt_format, int width, int height )
{
	switch( mlt_format )
	{
	case mlt_image_rgb24:
		// Convert packet YUV444 to packed RGB24.
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
		// Convert planar YUV444 to packed YUV422
		{
			uint8_t* yp = vs_img;
			uint8_t* up = yp + ( width * height );
			uint8_t* vp = up + ( width * height );
			int i, j, n = width / 2 + 1;

			for ( i = 0; i < height; i++ )
			{
				j = n;
				while ( --j )
				{
					*mlt_img++ = yp[0];
					*mlt_img++ = ( up[0] + up[1] ) >> 1;
					*mlt_img++ = yp[1];
					*mlt_img++ = ( vp[0] + vp[1] ) >> 1;
					yp += 2;
					up += 2;
					vp += 2;
				}
				if ( width % 2 )
				{
					*mlt_img++ = yp[0];
					*mlt_img++ = up[0];
					yp += 1;
					up += 1;
					vp += 1;
				}
			}
		}
		break;
	default:
		break;
	}
}

/** Free an image allocated by mltimage_to_vsimage().
 */

void free_vsimage( uint8_t* vs_img, VSPixelFormat format )
{
	if( format != PF_YUV420P )
	{
		mlt_pool_release( vs_img );
	}
}

/** Compare two VSMotionDetectConfig structures.
 * Return 1 if they are different. 0 if they are the same.
 */

int compare_motion_config( VSMotionDetectConfig* a, VSMotionDetectConfig* b )
{
	if( a->shakiness != b->shakiness ||
		a->accuracy != b->accuracy ||
		a->stepSize != b->stepSize ||
		// Skip: Deprecated
		// a->algo != b->algo ||
		a->virtualTripod != b->virtualTripod ||
		a->show != b->show ||
		// Skip: inconsequential?
		// a->modName != b->modName ||
		a->contrastThreshold != b->contrastThreshold )
	{
		return 1;
	}
	return 0;
}

/** Compare two VSTransformConfig structures.
 * Return 1 if they are different. 0 if they are the same.
 */

int compare_transform_config( VSTransformConfig* a, VSTransformConfig* b )
{
	if( a->relative != b->relative ||
		a->smoothing != b->smoothing ||
		a->crop != b->crop ||
		a->invert != b->invert ||
		a->zoom != b->zoom ||
		a->optZoom != b->optZoom ||
		a->zoomSpeed != b->zoomSpeed ||
		a->interpolType != b->interpolType ||
		a->maxShift != b->maxShift ||
		a->maxAngle != b->maxAngle ||
		// Skip: inconsequential?
		// a->modName != b->modName ||
		// Skip: unused?
		// a->verbose != b->verbose ||
		a->simpleMotionCalculation != b->simpleMotionCalculation ||
		// Skip: unused?
		// a->storeTransforms != b->storeTransforms ||
		a->smoothZoom != b->smoothZoom ||
		a->camPathAlgo != b->camPathAlgo )
	{
		return 1;
	}
	return 0;
}

static int vs_log_wrapper( int type, const char *tag, const char *format, ... )
{
	va_list vl;

	if ( type > mlt_log_get_level() )
		return VS_OK;

	va_start( vl, format );
	fprintf( stderr, "[%s] ", tag );
	vfprintf( stderr, format, vl );
	va_end( vl );

	return VS_OK;
}

void init_vslog()
{
	VS_ERROR_TYPE = MLT_LOG_ERROR;
	VS_WARN_TYPE  = MLT_LOG_WARNING;
	VS_INFO_TYPE  = MLT_LOG_INFO;
	VS_MSG_TYPE   = MLT_LOG_VERBOSE;
	vs_log = vs_log_wrapper;
}
