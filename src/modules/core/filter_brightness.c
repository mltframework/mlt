/*
 * filter_brightness.c -- brightness, fade, and opacity filter
 * Copyright (C) 2003-2016 Meltytech, LLC
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter =  (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	double level = 1.0;

	// Use animated "level" property only if it has been set since init
	char* level_property = mlt_properties_get( properties, "level" );
	if ( level_property != NULL )
	{
		level = mlt_properties_anim_get_double( properties, "level", position, length );
	}
	else
	{
		// Get level using old "start,"end" mechanics
		// Get the starting brightness level
		level = fabs( mlt_properties_get_double( properties, "start" ) );

		// If there is an end adjust gain to the range
		if ( mlt_properties_get( properties, "end" ) != NULL )
		{
			// Determine the time position of this frame in the transition duration
			double end = fabs( mlt_properties_get_double( properties, "end" ) );
			level += ( end - level ) * mlt_filter_get_progress( filter, frame );
		}
	}

	// Do not cause an image conversion unless there is real work to do.
	if ( level != 1.0 )
		*format = mlt_image_yuv422;

	// Get the image
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only process if we have no error.
	if ( error == 0 )
	{
		// Only process if level is something other than 1
		if ( level != 1.0 && *format == mlt_image_yuv422 )
		{
			int i = *width * *height + 1;
			uint8_t *p = *image;
			int32_t m = level * ( 1 << 16 );
			int32_t n = 128 * ( ( 1 << 16 ) - m );

			while ( --i )
			{
				p[0] = CLAMP( (p[0] * m) >> 16, 16, 235 );
				p[1] = CLAMP( (p[1] * m + n) >> 16, 16, 240 );
				p += 2;
			}
		}

		// Process the alpha channel if requested.
		if ( mlt_properties_get( properties, "alpha" ) )
		{
			double alpha = mlt_properties_anim_get_double( properties, "alpha", position, length );
			alpha = alpha >= 0.0 ? alpha : level;
			if ( alpha != 1.0 )
			{
				int32_t m = alpha * ( 1 << 16 );
				int i = *width * *height + 1;

				if ( *format == mlt_image_rgb24a ) {
					uint8_t *p = *image + 3;
					for ( ; --i; p += 4 )
						p[0] = ( p[0] * m ) >> 16;
				} else {
					uint8_t *p = mlt_frame_get_alpha_mask( frame );
					for ( ; --i; ++p )
						p[0] = ( p[0] * m ) >> 16;
				}
			}
		}
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_brightness_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "start", arg == NULL ? "1" : arg );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "level", NULL );
	}
	return filter;
}

