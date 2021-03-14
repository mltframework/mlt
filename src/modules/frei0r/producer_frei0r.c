/*
 * producer_frei0r.c -- frei0r producer
 * Copyright (c) 2009 Jean-Baptiste Mardelle <jb@kdenlive.org>
 * Copyright (C) 2009-2019 Meltytech, LLC
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

#include "frei0r_helper.h"

#include <stdlib.h>
#include <string.h>

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_producer producer = mlt_frame_pop_service( frame );

	// Choose suitable out values if nothing specific requested
	if ( *width <= 0 )
		*width = mlt_service_profile( MLT_PRODUCER_SERVICE(producer) )->width;
	if ( *height <= 0 )
		*height = mlt_service_profile( MLT_PRODUCER_SERVICE(producer) )->height;

	// Allocate the image
	*format = mlt_image_rgba;
	int size = mlt_image_format_size( *format, *width, *height, NULL );

	// Allocate the image
	*buffer = mlt_pool_alloc( size );

	// Update the frame
	mlt_frame_set_image( frame, *buffer, size, mlt_pool_release );

	if ( *buffer != NULL )
	{
		mlt_position position = mlt_frame_get_position( frame );
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		double time = (double) position / mlt_profile_fps( profile );
		int length = mlt_producer_get_playtime( producer );
		process_frei0r_item( MLT_PRODUCER_SERVICE(producer), position, time, length, frame, buffer, width, height );
	}

	return 0;
}

int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Obtain properties of frame and producer
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", 1 );
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
		mlt_properties_set_int( properties, "meta.media.width", profile->width );
		mlt_properties_set_int( properties, "meta.media.height", profile->height );

		// Push the get_image method
		mlt_frame_push_service( *frame, producer );
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

void producer_close( mlt_producer producer )
{
	producer->close = NULL;
	mlt_producer_close( producer );
	free( producer );
}
