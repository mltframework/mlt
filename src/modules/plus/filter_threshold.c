/*
 * filter_threshold.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2008-2022 Meltytech, LLC
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

/** Get the images and apply the luminance of the mask to the alpha of the frame.
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service(frame);

	// Render the frame
	*format = mlt_image_yuv422;
	if ( mlt_frame_get_image( frame, image, format, width, height, writable ) == 0 )
	{
		mlt_properties properties = mlt_filter_properties(filter);
		mlt_position position = mlt_filter_get_position(filter, frame);
		mlt_position length = mlt_filter_get_length2(filter, frame);
		int midpoint = mlt_properties_anim_get_int(properties, "midpoint", position, length);
		int use_alpha = mlt_properties_get_int(properties, "use_alpha");
		int invert = mlt_properties_get_int(properties, "invert");
		int full_luma = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_luma");
		uint8_t white = full_luma? 255 : 235;
		uint8_t black = full_luma? 0 : 16;
		uint8_t *p = *image;
		uint8_t A = invert? white : black;
		uint8_t B = invert? black : white;
		int size = *width * *height + 1;

		if ( !use_alpha )
		{
			while (--size)
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
			uint8_t *alpha = mlt_frame_get_alpha( frame );
			if ( alpha )
			{
				while (--size)
				{
					if ( *alpha ++ < midpoint )
						*p ++ = A;
					else
						*p ++ = B;
					*p ++ = 128;
				}
			}
			else
			{
				while (--size)
				{
					*p ++ = B;
					*p ++ = 128;
				}
			}
		}
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service(frame, filter);
	mlt_frame_push_get_image(frame, filter_get_image);
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_threshold_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "midpoint", 128 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "use_alpha", 0 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "invert", 0 );
		filter->process = filter_process;
	}
	return filter;
}
