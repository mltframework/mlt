/*
 * filter_pillar_echo.c -- filter to interpolate pixels outside an area of interest
 * Copyright (c) 2020-2021 Meltytech, LLC
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

#include <framework/mlt.h>
#include <framework/mlt_log.h>

#include <math.h>
#include <string.h>

/** Constrain a rect to be within the max dimensions
*/
static mlt_rect constrain_rect( mlt_rect rect, int max_x, int max_y )
{
	if ( rect.x < 0 )
	{
		rect.w = rect.w + rect.x ;
		rect.x = 0;
	}
	if ( rect.y < 0 )
	{
		rect.h = rect.h + rect.y;
		rect.y = 0;
	}
	if ( rect.x + rect.w < 0 )
	{
		rect.w = 0;
	}
	if ( rect.y + rect.h < 0 )
	{
		rect.h = 0;
	}
	if ( rect.x + rect.w > max_x )
	{
		rect.w = max_x - rect.x;
	}
	if ( rect.y + rect.h > max_y )
	{
		rect.h = max_y - rect.y;
	}
	return rect;
}

/** Perform a bilinear scale from the rect inside the source to fill the destination
  *
  * \param src a pointer to the source image
  * \param dst a pointer to the destination image
  * \param width the number of values in each row
  * \param height the number of values in each column
  * \param rect the area of interest in the src to be scaled to fit the dst
  */

static void bilinear_scale_rgba( uint8_t* src, uint8_t* dst, int width, int height, mlt_rect rect )
{
	mlt_rect srcRect = rect;

	// Crop out a section of the rect that has the same aspect ratio as the image
	double destAr = (double)width / (double)height;
	double sourceAr = rect.w / rect.h;
	if ( sourceAr > destAr )
	{
		// Crop sides to fit height
		srcRect.w = rect.w * destAr / sourceAr;
		srcRect.x = rect.x + (rect.w - srcRect.w) / 2.0;
	}
	else if ( destAr > sourceAr )
	{
		// Crop top and bottom to fit width.
		srcRect.h = rect.h * sourceAr / destAr;
		srcRect.y = rect.y + (rect.h - srcRect.h) / 2.0;
	}
	double srcScale = srcRect.h / (double)height;
	int linesize = width * 4;

	int y = 0;
	for ( y = 0; y < height; y++ )
	{
		double srcY = srcRect.y + (double)y * srcScale;
		int srcYindex = floor(srcY);
		double fbottom = srcY - srcYindex;
		double ftop = 1.0 - fbottom;

		int x = 0;
		for ( x = 0; x < width; x++ )
		{
			double srcX = srcRect.x + (double)x * srcScale;
			int srcXindex = floor(srcX);
			double fright = srcX - srcXindex;
			double fleft = 1.0 - fright;

			double valueSum[] = {0.0, 0.0, 0.0, 0.0};
			double factorSum[] = {0.0, 0.0, 0.0, 0.0};

			uint8_t* s = src + (srcYindex * linesize) + (srcXindex * 4);

			// Top Left
			double ftl = ftop * fleft;
			valueSum[0] += s[0] * ftl;
			factorSum[0] += ftl;
			valueSum[1] += s[1] * ftl;
			factorSum[1] += ftl;
			valueSum[2] += s[2] * ftl;
			factorSum[2] += ftl;
			valueSum[3] += s[3] * ftl;
			factorSum[3] += ftl;

			// Top Right
			if ( x < width - 1 )
			{
				double ftr = ftop * fright;
				valueSum[0] += s[4] * ftr;
				factorSum[0] += ftr;
				valueSum[1] += s[5] * ftr;
				factorSum[1] += ftr;
				valueSum[2] += s[6] * ftr;
				factorSum[2] += ftr;
				valueSum[3] += s[7] * ftr;
				factorSum[3] += ftr;
			}

			if( y < height - 1 )
			{
				uint8_t* sb = s + linesize;

				// Bottom Left
				double fbl = fbottom * fleft;
				valueSum[0] += sb[0] * fbl;
				factorSum[0] += fbl;
				valueSum[1] += sb[1] * fbl;
				factorSum[1] += fbl;
				valueSum[2] += sb[2] * fbl;
				factorSum[2] += fbl;
				valueSum[3] += sb[3] * fbl;
				factorSum[3] += fbl;

				// Bottom Right
				if ( x < width - 1 )
				{
					double fbr = fbottom * fright;
					valueSum[0] += sb[4] * fbr;
					factorSum[0] += fbr;
					valueSum[1] += sb[5] * fbr;
					factorSum[1] += fbr;
					valueSum[2] += sb[6] * fbr;
					factorSum[2] += fbr;
					valueSum[3] += sb[7] * fbr;
					factorSum[3] += fbr;
				}
			}

			dst[0] = (uint8_t)round( valueSum[0] / factorSum[0] );
			dst[1] = (uint8_t)round( valueSum[1] / factorSum[1] );
			dst[2] = (uint8_t)round( valueSum[2] / factorSum[2] );
			dst[3] = (uint8_t)round( valueSum[3] / factorSum[3] );
			dst += 4;
		}
	}
}

/** Perform a box blur from the source to the destination
  *
  * src and dst can be the same location
  *
  * This function uses a sliding window accumulator method - applied
  * horizontally first and then vertically.
  *
  * \param src a pointer to the source image
  * \param dst a pointer to the destination image
  * \param width the number of values in each row
  * \param height the number of values in each column
  * \param radius the radius of the box blur operation
  */

static void box_blur( uint8_t* src, uint8_t* dst, int width, int height, int radius )
{
	int accumulator[] = {0, 0, 0, 0};
	int x = 0;
	int y = 0;
	int step = 4;
	int linesize = step * width;
	uint8_t* d = NULL;
	uint8_t* tmpbuff = mlt_pool_alloc( width * height * step );

	if ( radius > (width / 2) )
	{
		radius = width / 2;
	}
	if ( radius > (height / 2) )
	{
		radius = height / 2;
	}

	double diameter = (radius * 2) + 1;

	// Horizontal Blur
	d = tmpbuff;
	for ( y = 0; y < height; y++ )
	{
		uint8_t* first = src + (y * linesize);
		uint8_t* last = first + linesize - step;
		uint8_t* s1 = first;
		uint8_t* s2 = first;
		accumulator[0] = first[0] * (radius + 1);
		accumulator[1] = first[1] * (radius + 1);
		accumulator[2] = first[2] * (radius + 1);
		accumulator[3] = first[3] * (radius + 1);

		for ( x = 0; x < radius; x++ )
		{
			accumulator[0] += s1[0];
			accumulator[1] += s1[1];
			accumulator[2] += s1[2];
			accumulator[3] += s1[3];
			s1 += step;
		}
		for ( x = 0; x <= radius; x++ )
		{
			accumulator[0] += s1[0] - first[0];
			accumulator[1] += s1[1] - first[1];
			accumulator[2] += s1[2] - first[2];
			accumulator[3] += s1[3] - first[3];
			d[0] = lrint((double)accumulator[0] / diameter);
			d[1] = lrint((double)accumulator[1] / diameter);
			d[2] = lrint((double)accumulator[2] / diameter);
			d[3] = lrint((double)accumulator[3] / diameter);
			s1 += step;
			d += step;
		}
		for ( x= radius + 1; x < width - radius; x++)
		{
			accumulator[0] += s1[0] - s2[0];
			accumulator[1] += s1[1] - s2[1];
			accumulator[2] += s1[2] - s2[2];
			accumulator[3] += s1[3] - s2[3];
			d[0] = lrint((double)accumulator[0] / diameter);
			d[1] = lrint((double)accumulator[1] / diameter);
			d[2] = lrint((double)accumulator[2] / diameter);
			d[3] = lrint((double)accumulator[3] / diameter);
			s1 += step;
			s2 += step;
			d += step;
		}
		for ( x = width - radius; x < width; x++ )
		{
			accumulator[0] += last[0] - s2[0];
			accumulator[1] += last[1] - s2[1];
			accumulator[2] += last[2] - s2[2];
			accumulator[3] += last[3] - s2[3];
			d[0] = lrint((double)accumulator[0] / diameter);
			d[1] = lrint((double)accumulator[1] / diameter);
			d[2] = lrint((double)accumulator[2] / diameter);
			d[3] = lrint((double)accumulator[3] / diameter);
			s2 += step;
			d += step;
		}
	}

	// Vertical Blur
	for ( x = 0; x < width; x++ )
	{
		uint8_t* first = tmpbuff + (x * step);
		uint8_t* last = first + (linesize * (height - 1));
		uint8_t* s1 = first;
		uint8_t* s2 = first;
		d = dst + (x * step);
		accumulator[0] = first[0] * (radius + 1);
		accumulator[1] = first[1] * (radius + 1);
		accumulator[2] = first[2] * (radius + 1);
		accumulator[3] = first[3] * (radius + 1);

		for ( y = 0; y < radius; y++ )
		{
			accumulator[0] += s1[0];
			accumulator[1] += s1[1];
			accumulator[2] += s1[2];
			accumulator[3] += s1[3];
			s1 += linesize;
		}
		for ( y = 0; y <= radius; y++ )
		{
			accumulator[0] += s1[0] - first[0];
			accumulator[1] += s1[1] - first[1];
			accumulator[2] += s1[2] - first[2];
			accumulator[3] += s1[3] - first[3];
			d[0] = lrint((double)accumulator[0] / diameter);
			d[1] = lrint((double)accumulator[1] / diameter);
			d[2] = lrint((double)accumulator[2] / diameter);
			d[3] = lrint((double)accumulator[3] / diameter);
			s1 += linesize;
			d += linesize;
		}
		for ( y = radius + 1; y < height - radius; y++)
		{
			accumulator[0] += s1[0] - s2[0];
			accumulator[1] += s1[1] - s2[1];
			accumulator[2] += s1[2] - s2[2];
			accumulator[3] += s1[3] - s2[3];
			d[0] = lrint((double)accumulator[0] / diameter);
			d[1] = lrint((double)accumulator[1] / diameter);
			d[2] = lrint((double)accumulator[2] / diameter);
			d[3] = lrint((double)accumulator[3] / diameter);
			s1 += linesize;
			s2 += linesize;
			d += linesize;
		}
		for ( y = height - radius; y < height; y++ )
		{
			accumulator[0] += last[0] - s2[0];
			accumulator[1] += last[1] - s2[1];
			accumulator[2] += last[2] - s2[2];
			accumulator[3] += last[3] - s2[3];
			d[0] = lrint((double)accumulator[0] / diameter);
			d[1] = lrint((double)accumulator[1] / diameter);
			d[2] = lrint((double)accumulator[2] / diameter);
			d[3] = lrint((double)accumulator[3] / diameter);
			s2 += linesize;
			d += linesize;
		}
	}

	mlt_pool_release( tmpbuff );
}

/** Copy pixels from source to destination
  *
  * \param src a pointer to the source image
  * \param dst a pointer to the destination image
  * \param width the number of values in each row
  * \param rect the area of interest in the src to be copied to the dst
  */

static void blit_rect( uint8_t* src, uint8_t* dst, int width, mlt_rect rect )
{
	int blitHeight = rect.h;
	int blitWidth = rect.w * 4;
	int linesize = width * 4;
	src += (int)rect.y * linesize + (int)rect.x * 4;
	dst += (int)rect.y * linesize + (int)rect.x * 4;
	while ( blitHeight-- )
	{
		memcpy( dst, src, blitWidth );
		src += linesize;
		dst += linesize;
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	char* rect_str = mlt_properties_get( filter_properties, "rect" );
	if ( !rect_str )
	{
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "rect property not set\n" );
		return mlt_frame_get_image( frame, image, format, width, height, writable );
	}
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "rect", position, length );
	if ( strchr( rect_str, '%' ) )
	{
		rect.x *= profile->width;
		rect.w *= profile->width;
		rect.y *= profile->height;
		rect.h *= profile->height;
	}
	double scale = mlt_profile_scale_width( profile, *width );
	rect.x *= scale;
	rect.w *= scale;
	scale = mlt_profile_scale_height( profile, *height );
	rect.y *= scale;
	rect.h *= scale;
	rect = constrain_rect( rect, profile->width * scale, profile->height * scale );

	if ( rect.w < 1 || rect.h < 1 )
	{
		mlt_log_info( MLT_FILTER_SERVICE(filter), "rect invalid\n" );
		return mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	*format = mlt_image_rgba;
	error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if (error) return error;
	if (rect.x <= 0 && rect.y <= 0 && rect.w >= *width && rect.h >= *height)
	{
		// Rect fills the image. Nothing to blur
		return error;
	}

	double blur = mlt_properties_anim_get_int( filter_properties, "blur", position, length );
	// Convert from percent to pixels.
	blur = blur * (double)profile->width * mlt_profile_scale_width( profile, *width ) / 100.0;
	blur = lrint(blur);

	int size = mlt_image_format_size( *format, *width, *height, NULL );
	uint8_t* dst = mlt_pool_alloc( size );

	bilinear_scale_rgba( *image, dst, *width, *height, rect );
	box_blur( dst, dst, *width, *height, blur );
	blit_rect( *image, dst, *width, rect );

	*image = dst;
	mlt_frame_set_image( frame, dst, size, mlt_pool_release );

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_pillar_echo_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();

	if ( filter )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set( properties, "rect", "0% 0% 10% 10%" );
		mlt_properties_set_double( properties, "blur", 4.0 );
		filter->process = filter_process;
	}
	else
	{
		mlt_log_error( NULL, "Filter pillar_echo initialization failed\n" );
	}
	return filter;
}
