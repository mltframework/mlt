/*
 * filter_fieldorder.c -- change field dominance
 * Copyright (C) 2011-2014 Meltytech, LLC
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties from the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Get the input image, width and height
	int error = mlt_frame_get_image( frame, image, format, width, height, writable );

	if ( !error && *image )
	{
		int tff = mlt_properties_get_int( properties, "consumer_tff" );

		// Provides a manual override for misreported field order
		if ( mlt_properties_get( properties, "meta.top_field_first" ) )
			mlt_properties_set_int( properties, "top_field_first", mlt_properties_get_int( properties, "meta.top_field_first" ) );
		mlt_log_debug( NULL, "TFF in %d out %d\n", mlt_properties_get_int( properties, "top_field_first" ), tff );

		if ( mlt_properties_get_int( properties, "meta.swap_fields" ) &&
			mlt_properties_get( properties, "progressive" ) &&
			mlt_properties_get_int( properties, "progressive" ) == 0 )
		{
			// We only work with non-planar formats
			if ( *format == mlt_image_yuv420p && frame->convert_image )
				error = frame->convert_image( frame, image, format, mlt_image_yuv422 );

			// Make a new image
			int bpp;
			int size = mlt_image_format_size( *format, *width, *height, &bpp );
			uint8_t *new_image = mlt_pool_alloc( size );
			int stride = *width * bpp;
			int i = *height + 1;
			uint8_t *src = *image;

			// Set the new image
			mlt_frame_set_image( frame, new_image, size, mlt_pool_release );
			*image = new_image;

			while ( --i )
			{
				memcpy( new_image, src + stride * !(i % 2), stride );
				new_image += stride;
				src += stride * (i % 2) * 2;
			}
		}

		// Correct field order if needed
		if ( tff != -1 &&
		     mlt_properties_get_int( properties, "top_field_first" ) != tff &&
		     mlt_properties_get( properties, "progressive" ) &&
		     mlt_properties_get_int( properties, "progressive" ) == 0 )
		{
			mlt_log_timings_begin();

			// We only work with non-vertical luma/chroma scaled
			if ( *format == mlt_image_yuv420p )
			{
				*format = mlt_image_yuv422;
				mlt_frame_get_image( frame, image, format, width, height, writable );
			}

			// Shift the entire image down by one line
			int p, strides[4], size;
			uint8_t *new_planes[4], *old_planes[4], *new_image;

			size = mlt_image_format_size( *format, *width, *height, NULL );
			new_image = mlt_pool_alloc( size );
			mlt_image_format_planes( *format, *width, *height, new_image, new_planes, strides );
			mlt_image_format_planes( *format, *width, *height, *image, old_planes, strides );

			for( p = 0; p < 4; p++ )
			{
				if( !new_planes[p] )
					continue;
				memcpy( new_planes[p], old_planes[p], strides[p] );
				memcpy( new_planes[p] + strides[p], old_planes[p], strides[p] * ( *height - 1 ) );
			}

			// Set the new image
			mlt_frame_set_image( frame, new_image, size, mlt_pool_release );
			*image = new_image;

			// Set the normalised field order
			mlt_properties_set_int( properties, "top_field_first", tff );
			mlt_properties_set_int( properties, "meta.top_field_first", tff );

			mlt_log_timings_end( NULL, "shifting_fields" );
		}
	}

	return error;
}

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, get_image );
	return frame;
}

mlt_filter filter_fieldorder_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = calloc( 1, sizeof( *filter ) );
	if ( mlt_filter_init( filter, NULL ) == 0 )
	{
		filter->process = process;
	}
	return filter;
}
