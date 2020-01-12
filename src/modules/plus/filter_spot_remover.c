/*
 * filter_remover.c -- filter to interpolate pixels to cover an area
 * Copyright (c) 2018-2020 Meltytech, LLC
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

/** Scale a rectangle by the specified factors. */
mlt_rect scale_rect( mlt_rect rect, double x_scale, double y_scale )
{
	rect.x = rect.x / x_scale;
	rect.y = rect.y / y_scale;
	rect.w = rect.w / x_scale;
	rect.h = rect.h / y_scale;
	return rect;
}

/** Constrain a rect to be within the max dimensions with an additional 1 pixel
  * padding.
  */
mlt_rect constrain_rect( mlt_rect rect, int max_x, int max_y )
{
	rect.x = round( rect.x );
	rect.y = round( rect.y );
	rect.w = round( rect.w );
	rect.h = round( rect.h );
	if ( rect.x < 0 )
	{
		rect.w = rect.w + rect.x - 1;
		rect.x = 1;
	}
	if ( rect.y < 0 )
	{
		rect.h = rect.h + rect.y - 1;
		rect.y = 1;
	}
	if ( rect.x + rect.w < 0 )
	{
		rect.w = 0;
	}
	if ( rect.y + rect.h < 0 )
	{
		rect.h = 0;
	}
	if ( rect.x < 1 )
	{
		rect.x = 1;
	}
	if ( rect.y < 1 )
	{
		rect.y = 1;
	}
	if ( rect.x + rect.w > max_x - 1 )
	{
		rect.w = max_x - rect.x - 1;
	}
	if ( rect.y + rect.h > max_y - 1 )
	{
		rect.h = max_y - rect.y - 1;
	}
	return rect;
}

/** Perform spot removal on a channel.
  *
  * Values within the rectangle are replaced with interpolated values.
  * Each value is an interpolation of the first values outside of the rect on
  * the top, bottom, left and right of the value being interpolated.
  *
  * \param chan a pointer to the first value in the channel
  * \param rowCount the number of values in each line (row)
  * \param step the space between values in each line
  * \param rect the area to be removed
  */
static void remove_spot_channel( uint8_t *chan, int rowCount, int step, mlt_rect rect )
{
	int yStop = rect.y + rect.h;
	int xStop = rect.x + rect.w;
	int rowSize = rowCount * step;
	int y;
	for ( y = rect.y; y < yStop; y++ )
	{
		uint8_t* xValueL = chan + ( y * rowSize ) + ( ( (int)rect.x - 1 ) * step );
		uint8_t* xValueR = xValueL + ( (int)rect.w * step );
		uint8_t* p = chan + ( y * rowSize ) + ( (int)rect.x * step );
		double yRatio = 1.0 - ( ( y - rect.y ) / rect.h );
		int x;
		for ( x = rect.x; x < xStop; x++ )
		{
			uint8_t* yValueT = chan  + ( ( (int)rect.y - 1 ) * rowSize ) + ( x * step );
			uint8_t* yValueB = yValueT + (int)rect.h * rowSize;
			double xRatio = 1.0 - ( ( x - rect.x ) / rect.w );
			unsigned int xValueInterp = ( *xValueL * xRatio ) + ( *xValueR * ( 1.0 - xRatio ) );
			unsigned int yValueInterp = ( *yValueT * yRatio ) + ( *yValueB * ( 1.0 - yRatio ) );
			unsigned int value = ( xValueInterp + yValueInterp ) / 2;
			if ( value > 255 ) value = 255;
			*p = value;
			p += step;
		}
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
	double scale = mlt_frame_resolution_scale(frame);
	rect.x *= scale;
	rect.y *= scale;
	rect.w *= scale;
	rect.h *= scale;
	rect = constrain_rect( rect, profile->width * scale, profile->height * scale);
	if ( rect.w < 1 || rect.h < 1 )
	{
		mlt_log_info( MLT_FILTER_SERVICE(filter), "rect invalid\n" );
		return mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	switch( *format )
	{
		case mlt_image_rgb24a:
		case mlt_image_rgb24:
		case mlt_image_yuv422:
		case mlt_image_yuv420p:
			// These formats are all supported
			break;
		default:
			*format = mlt_image_rgb24a;
			break;
	}
	error = mlt_frame_get_image( frame, image, format, width, height, 1 );
	if (error) return error;

	int i;
	switch( *format )
	{
		case mlt_image_rgb24a:
			for ( i = 0; i < 4; i++ )
			{
				remove_spot_channel( *image + i, *width, 4, rect );
			}
			break;
		case mlt_image_rgb24:
			for ( i = 0; i < 3; i++ )
			{
				remove_spot_channel( *image + i, *width, 3, rect );
			}
			break;
		case mlt_image_yuv422:
			// Y
			remove_spot_channel( *image, *width, 2, rect );
			// U
			remove_spot_channel( *image + 1, *width / 2, 4,
								 constrain_rect( scale_rect( rect, 2, 1 ), *width / 2, *height ) );
			// V
			remove_spot_channel( *image + 3, *width / 2, 4,
								 constrain_rect( scale_rect( rect, 2, 1 ), *width / 2, *height ) );
			break;
		case mlt_image_yuv420p:
			// Y
			remove_spot_channel( *image, *width, 1, rect );
			// U
			remove_spot_channel( *image + (*width * *height), *width / 2, 1,
								 constrain_rect( scale_rect( rect, 2, 2 ), *width / 2, *height / 2 ) );
			// V
			remove_spot_channel( *image + (*width * *height * 5 / 4), *width / 2, 1,
								 constrain_rect( scale_rect( rect, 2, 2 ), *width / 2, *height / 2 ) );
			break;
		default:
			return 1;
	}

	uint8_t *alpha = mlt_frame_get_alpha( frame );
	if ( alpha && *format != mlt_image_rgb24a )
	{
		remove_spot_channel( alpha, *width, 1, rect );
	}

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_spot_remover_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();

	if ( filter )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set( properties, "rect", "0% 0% 10% 10%" );
		filter->process = filter_process;
	}
	else
	{
		mlt_log_error( NULL, "Filter spot_remover initialization failed\n" );
	}
	return filter;
}
