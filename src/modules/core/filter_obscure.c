/*
 * filter_obscure.c -- obscure filter
 * Copyright (C) 2003-2014 Meltytech, LLC
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
#include <framework/mlt_profile.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/** Scale a rectangle by the specified factors. */
static mlt_rect scale_rect( mlt_rect rect, double x_scale, double y_scale )
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
static mlt_rect constrain_rect( mlt_rect rect, int max_x, int max_y )
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

/** The averaging function...
*/

static inline void obscure_average( uint8_t *start, int width, int height, int stride )
{
	register int y;
	register int x;
	register int Y = ( *start + *( start + 2 ) ) / 2;
	register int U = *( start + 1 );
	register int V = *( start + 3 );
	register uint8_t *p;
	register int components = width >> 1;

	y = height;
	while( y -- )
	{
		p = start;
		x = components;
		while( x -- )
		{
			Y = ( Y + *p ++ ) >> 1;
			U = ( U + *p ++ ) >> 1;
			Y = ( Y + *p ++ ) >> 1;
			V = ( V + *p ++ ) >> 1;
		}
		start += stride;
	}

	start -= height * stride;
	y = height;
	while( y -- )
	{
		p = start;
		x = components;
		while( x -- )
		{
			*p ++ = Y;
			*p ++ = U;
			*p ++ = Y;
			*p ++ = V;
		}
		start += stride;
	}
}


/** The obscurer rendering function...
*/

static void obscure_render( uint8_t *image, int width, int height, mlt_rect rect, int mw, int mh)
 {
	int w;
	int h;
	int aw;
	int ah;
	if ( mw < 1) mw = 1;
	if ( mw > rect.w) mw = rect.w;
	if ( mw > 2000) mw = 2000;
	if ( mh < 1) mh = 1;
	if ( mh > rect.h) mh = rect.h;
	if ( mh > 2000) mh = 2000;

	uint8_t *p = image + (int)rect.y * width * 2 + (int)rect.x * 2;

	for ( w = 0; w < rect.w; w += mw )
	{
		for ( h = 0; h < rect.h; h += mh )
		{
			aw = w + mw > rect.w ? mw - ( w + mw - rect.w ) : mw;
			ah = h + mh > rect.h ? mh - ( h + mh - rect.h ) : mh;
			if ( aw > 1 && ah > 1 )
				obscure_average( p + h * ( width << 1 ) + ( w << 1 ), aw, ah, width << 1 );
		}
	}
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Pop the top of stack now
	mlt_filter filter = mlt_frame_pop_service( frame );

	if ( filter == NULL ) {
		return 1;
	}

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
	double scale = mlt_profile_scale_width(profile, *width);
	rect.x *= scale;
	rect.w *= scale;
	scale = mlt_profile_scale_height(profile, *height);
	rect.y *= scale;
	rect.h *= scale;
	rect = constrain_rect( rect, profile->width * scale, profile->height * scale);
	if ( rect.w < 1 || rect.h < 1 )
	{
		mlt_log_info( MLT_FILTER_SERVICE(filter), "rect invalid\n" );
		return mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	int maskWidth = 20;
	if (mlt_properties_get( filter_properties, "maskWidth" ) )
	{
		maskWidth = mlt_properties_anim_get_int( filter_properties, "maskWidth", position, length );
	}

	int maskHeight = 20;
	if (mlt_properties_get( filter_properties, "maskHeight" ) )
	{
		maskHeight = mlt_properties_anim_get_int( filter_properties, "maskHeight", position, length );
	}

	// Get the image from the frame
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Get the image from the frame
	if ( error == 0 )
	{
		// Now actually render it
		obscure_render( *image, *width, *height, rect, maskWidth, maskHeight );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Push this on to the service stack
	mlt_frame_push_service( frame, filter );
	
	// Push the get image call
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_obscure_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set( properties, "rect", "0% 0% 10% 10%" );
		mlt_properties_set( properties, "maskWidth", NULL );
		mlt_properties_set( properties, "maskHeight", NULL );
		filter->process = filter_process;
	}
	return filter;
}

