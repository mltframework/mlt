/*
 * filter_box_blur.c
 * Copyright (C) 2011-2021 Meltytech, LLC
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
#include <framework/mlt_profile.h>

#include <math.h>

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter =  (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	double hradius = mlt_properties_anim_get_double( properties, "hradius", position, length );
	double vradius = mlt_properties_anim_get_double( properties, "vradius", position, length );
	// Convert from percent to pixels
	hradius = hradius * (double)profile->width * mlt_profile_scale_width( profile, *width ) / 1000.0;
	hradius = MAX(lrint(hradius), 0);
	vradius = vradius * (double)profile->width * mlt_profile_scale_width( profile, *width ) / 1000.0;
	vradius = MAX(lrint(vradius), 0);

	if ( hradius == 0 && vradius == 0 )
	{
		// Nothing to blur
		error = mlt_frame_get_image( frame, image, format, width, height, writable );
	}
	else
	{
		// Get the image
		*format = mlt_image_rgba;
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );
		if ( error == 0 )
		{
			struct mlt_image_s img;
			mlt_image_set_values( &img, *image, *format, *width, *height );
			mlt_image_box_blur( &img, hradius, vradius );
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

mlt_filter filter_box_blur_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "hradius", "1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "vradius", "1" );
	}
	return filter;
}



