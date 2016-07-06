/*
 * filter_mono.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2005 Visual Media Fx Inc.
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <string.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_geometry.h>

/** Get the images and apply the luminance of the mask to the alpha of the frame.
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int use_alpha = mlt_deque_pop_back_int( MLT_FRAME_IMAGE_STACK( this ) );
	int midpoint = mlt_deque_pop_back_int( MLT_FRAME_IMAGE_STACK( this ) );
	int invert = mlt_deque_pop_back_int( MLT_FRAME_IMAGE_STACK( this ) );

	// Render the frame
	*format = mlt_image_yuv422;
	if ( mlt_frame_get_image( this, image, format, width, height, writable ) == 0 )
	{
		uint8_t *p = *image;
		uint8_t A = invert? 235 : 16;
		uint8_t B = invert? 16 : 235;
		int size = *width * *height;

		if ( !use_alpha )
		{
			while( size -- )
			{
				if ( *p < midpoint )
					*p ++ = A;
				else
					*p ++ = B;
				*p ++ = 128;
			}
		}
		else
		{
			uint8_t *alpha = mlt_frame_get_alpha_mask( this );
			while( size -- )
			{
				if ( *alpha ++ < midpoint )
					*p ++ = A;
				else
					*p ++ = B;
				*p ++ = 128;
			}
		}
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	int midpoint = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "midpoint" );
	int use_alpha = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "use_alpha" );
	int invert = mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "invert" );
	mlt_deque_push_back_int( MLT_FRAME_IMAGE_STACK( frame ), invert );
	mlt_deque_push_back_int( MLT_FRAME_IMAGE_STACK( frame ), midpoint );
	mlt_deque_push_back_int( MLT_FRAME_IMAGE_STACK( frame ), use_alpha );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mono_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "midpoint", 128 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "use_alpha", 0 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "invert", 0 );
		this->process = filter_process;
	}
	return this;
}

