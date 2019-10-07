/*
 * filter_freeze.c -- simple frame freezing filter
 * Copyright (C) 2007 Jean-Baptiste Mardelle <jb@kdenlive.org>
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
#include <framework/mlt_producer.h>
#include <framework/mlt_service.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_property.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <string.h>

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties props = MLT_FRAME_PROPERTIES( frame );

	int error = 0;
	int freeze_before = mlt_properties_get_int( properties, "freeze_before" );
	int freeze_after = mlt_properties_get_int( properties, "freeze_after" );
	mlt_position pos = mlt_properties_get_position( properties, "frame" ) + mlt_producer_get_in( mlt_frame_get_original_producer( frame ) );
	mlt_position currentpos = mlt_filter_get_position( filter, frame );

	int do_freeze = 0;
	if (freeze_before == 0 && freeze_after == 0 && mlt_properties_get_position( properties, "frame" ) >= 0) {
		do_freeze = 1;
	} else if (freeze_before != 0 && pos > currentpos) {
		do_freeze = 1;
	} else if (freeze_after != 0 && pos < currentpos) {
		do_freeze = 1;
	}

	if (do_freeze) {
		int size = 0;
		uint8_t *buffer = mlt_properties_get_data( properties, "image_buffer", &size );
		uint8_t *alpha_buffer = mlt_properties_get_data( properties, "alpha_buffer", NULL );

		if ( !buffer || mlt_properties_get_position( properties, "_frame" ) != pos )
		{
			mlt_frame freeze_frame = NULL;
			// freeze_frame has not been fetched yet or is not useful, so fetch it and cache it.
			// get parent producer
			mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
			mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
			mlt_producer_seek( producer, pos );

			// Get the frame
			mlt_service_get_frame( mlt_producer_service(producer), &freeze_frame, 0 );

			mlt_properties freeze_properties = MLT_FRAME_PROPERTIES( freeze_frame );
			mlt_properties_set( freeze_properties, "rescale.interp", mlt_properties_get( props, "rescale.interp" ) );
			mlt_properties_set_double( freeze_properties, "aspect_ratio", mlt_frame_get_aspect_ratio( frame ) );
			mlt_properties_set_int( freeze_properties, "progressive", mlt_properties_get_int( props, "progressive" ) );
			mlt_properties_set_int( freeze_properties, "consumer_deinterlace", mlt_properties_get_int( props, "consumer_deinterlace" ) || mlt_properties_get_int( properties, "deinterlace" ) );

			// Get frozen image
			mlt_properties_set_int( properties, "disable", 1);
			mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
			error = mlt_frame_get_image( freeze_frame, &buffer, format, width, height, 0 );
			mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
			mlt_properties_set_int( properties, "disable", 0);

			if (!error && buffer) {
				// Copy and buffer the frozen image
				size = mlt_image_format_size(*format, *width, *height, NULL);
				uint8_t *image_copy = mlt_pool_alloc( size );
				if (image_copy) {
					memcpy( image_copy, buffer, size );
					mlt_properties_set_data( properties, "image_buffer", image_copy, size, mlt_pool_release, NULL );
					mlt_properties_set_position( properties, "_frame", pos );

					// Copy and buffer the alpha channel
					if ((alpha_buffer = mlt_frame_get_alpha( freeze_frame ))) {
						int alpha_size = *width * *height;
						uint8_t *alpha_copy = mlt_pool_alloc( alpha_size );
						if (alpha_copy) {
							memcpy( alpha_copy, alpha_buffer, alpha_size );
							mlt_properties_set_data( properties, "alpha_buffer", alpha_copy, alpha_size, mlt_pool_release, NULL );
						}
						alpha_buffer = alpha_copy;
					}
				}
				buffer = image_copy;
			}
			mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
			mlt_frame_close(freeze_frame);
		}

		error = mlt_frame_get_image( frame, image, format, width, height, 1 );

		// Copy the buffered image and alpha channel to current frame
		if (*image && buffer && size == mlt_image_format_size(*format, *width, *height, NULL)) {
			memcpy(*image, buffer, size);

			if (alpha_buffer) {
				int alphasize = *width * *height;
				uint8_t *alpha_copy = mlt_pool_alloc( alphasize );
				if (alpha_copy) {
					memcpy( alpha_copy, alpha_buffer, alphasize );
					mlt_frame_set_alpha( frame, alpha_copy, alphasize, mlt_pool_release );
				}
			}
		}
	}
	else
	{
		error = mlt_frame_get_image( frame, image, format, width, height, writable );
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{

	// Push the filter on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the frame filter
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_freeze_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		// Set the frame which will be chosen for freeze
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "frame", "0" );

		// If freeze_after = 1, only frames after the "frame" value will be frozen
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "freeze_after", "0" );

		// If freeze_before = 1, only frames before the "frame" value will be frozen
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "freeze_before", "0" );
	}
	return filter;
}



