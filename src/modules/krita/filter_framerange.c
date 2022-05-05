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

#include <stdio.h>
#include <string.h>

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter self = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( self );

	int frame_start = mlt_properties_get_int( properties, "frame_start" );
	int frame_end = mlt_properties_get_int( properties, "frame_end" );
	mlt_position pos = mlt_producer_get_in( mlt_frame_get_original_producer( frame ) );
	mlt_position currentpos = mlt_filter_get_position( self, frame );
	int real_frame_index = (currentpos % (frame_end - frame_start) + frame_start);

	mlt_service_lock( MLT_FILTER_SERVICE( self ) );

	//Get producer
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_producer_seek( producer, pos );

	//Get and set frame data
	mlt_frame real_frame = NULL;;
	mlt_service_get_frame( mlt_producer_service(producer), &real_frame, 0 );
	mlt_properties_set_position( properties, "_frame", real_frame_index );

	// Get real image from producer
	uint8_t *buffer = NULL;
	int error = mlt_frame_get_image( real_frame, &buffer, format, width, height, 1 );
	mlt_service_unlock( MLT_FILTER_SERVICE( self ) );

	// Copy its data to current frame
	int size = mlt_image_format_size( *format, *width, *height, NULL );
	uint8_t *image_copy = mlt_pool_alloc( size );
	memcpy( image_copy, buffer, size );
	*image = image_copy;
	mlt_frame_set_image( frame, *image, size, mlt_pool_release );

	uint8_t *alpha_buffer = mlt_frame_get_alpha( real_frame );
	if ( alpha_buffer )
	{
		int alphasize = *width * *height;
		uint8_t *alpha_copy = mlt_pool_alloc( alphasize );
		memcpy( alpha_copy, alpha_buffer, alphasize );
		mlt_frame_set_alpha( frame, alpha_copy, alphasize, mlt_pool_release );
	}

	return error;
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter self = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( self );

	int frame_start = mlt_properties_get_int( properties, "frame_start" );
	int frame_end = mlt_properties_get_int( properties, "frame_end" );
	mlt_position pos = mlt_producer_get_in( mlt_frame_get_original_producer( frame ) );
	mlt_position currentpos = mlt_filter_get_position( self, frame );
	int real_frame_index = (currentpos % (frame_end - frame_start) + frame_start);

	mlt_service_lock( MLT_FILTER_SERVICE( self ) );

	//Get producer
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_producer_seek( producer, pos );

	//Get and set frame data
	mlt_frame real_frame = NULL;;
	mlt_service_get_frame( mlt_producer_service(producer), &real_frame, 0 );
	mlt_properties_set_position( properties, "_frame", real_frame_index );

	// Get real audio from producer
	return mlt_frame_get_audio( real_frame, buffer, format, frequency, channels, samples );
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter self, mlt_frame frame )
{

	// Push the filter on to the stack
	mlt_frame_push_service( frame, self);

	// Push the frame filter
	mlt_frame_push_get_image( frame, filter_get_image );

	mlt_frame_push_audio( frame, filter_get_audio );


	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_framerange_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		// Set the frame which will be chosen for freeze
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "frame_start", "0" );

		// If freeze_after = 1, only frames after the "frame" value will be frozen
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "frame_end", "100" );

	}
	return filter;
}



