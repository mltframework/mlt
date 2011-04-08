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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_service.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_property.h>

#include <stdio.h>
#include <string.h>

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	mlt_filter filter = mlt_frame_pop_service( this );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties props = MLT_FRAME_PROPERTIES( this );

	mlt_frame freeze_frame = NULL;;
	int freeze_before = mlt_properties_get_int( properties, "freeze_before" );
	int freeze_after = mlt_properties_get_int( properties, "freeze_after" );
	mlt_position pos = mlt_properties_get_position( properties, "frame" );
	mlt_position currentpos = mlt_filter_get_position( filter, this );

	int do_freeze = 0;
	if (freeze_before == 0 && freeze_after == 0) {
		do_freeze = 1;
	} else if (freeze_before != 0 && pos > currentpos) {
		do_freeze = 1;
	} else if (freeze_after != 0 && pos < currentpos) {
		do_freeze = 1;
	}

	if (do_freeze == 1) {
		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
		freeze_frame = mlt_properties_get_data( properties, "freeze_frame", NULL );
		if ( freeze_frame == NULL || mlt_properties_get_position( properties, "_frame" ) != pos )
		{
			// freeze_frame has not been fetched yet or is not useful, so fetch it and cache it.
			mlt_producer producer = mlt_frame_get_original_producer(this);
			mlt_producer_seek( producer, pos );

			// Get the frame
			mlt_service_get_frame( mlt_producer_service(producer), &freeze_frame, 0 );

			mlt_properties freeze_properties = MLT_FRAME_PROPERTIES( freeze_frame );
			mlt_properties_set_double( freeze_properties, "consumer_aspect_ratio", mlt_properties_get_double( props, "consumer_aspect_ratio" ) );
			mlt_properties_set( freeze_properties, "rescale.interp", mlt_properties_get( props, "rescale.interp" ) );
			mlt_properties_set_double( freeze_properties, "aspect_ratio", mlt_frame_get_aspect_ratio( this ) );
			mlt_properties_set_int( freeze_properties, "progressive", mlt_properties_get_int( props, "progressive" ) );
			mlt_properties_set_int( freeze_properties, "consumer_deinterlace", mlt_properties_get_int( props, "consumer_deinterlace" ) || mlt_properties_get_int( properties, "deinterlace" ) );
			mlt_properties_set_double( freeze_properties, "output_ratio", mlt_properties_get_double( props, "output_ratio" ) );
			mlt_properties_set_data( properties, "freeze_frame", freeze_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
			mlt_properties_set_position( properties, "_frame", pos );
		}
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

		// Get frozen image
		uint8_t *buffer = NULL;
		int error = mlt_frame_get_image( freeze_frame, &buffer, format, width, height, 1 );

		// Copy it to current frame
		int size = mlt_image_format_size( *format, *width, *height, NULL );
		uint8_t *image_copy = mlt_pool_alloc( size );
		memcpy( image_copy, buffer, size );
		*image = image_copy;
		mlt_frame_set_image( this, *image, size, mlt_pool_release );

		return error;
	}

	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{

	// Push the filter on to the stack
	mlt_frame_push_service( frame, this );

	// Push the frame filter
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_freeze_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		// Set the frame which will be chosen for freeze
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "frame", "0" );

		// If freeze_after = 1, only frames after the "frame" value will be frozen
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "freeze_after", "0" );

		// If freeze_before = 1, only frames after the "frame" value will be frozen
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "freeze_before", "0" );
	}
	return this;
}



