/*
 * producer_consumer.c -- produce as a consumer of an encapsulated producer
 * Copyright (C) 2008 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

struct context_s {
	mlt_producer this;
	mlt_producer producer;
	mlt_consumer consumer;
	mlt_profile profile;
};
typedef struct context_s *context; 


static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	context cx = mlt_frame_pop_service( frame );
	mlt_frame nested_frame = mlt_frame_pop_service( frame );

	*width = cx->profile->width;
	*height = cx->profile->height;

	int result = mlt_frame_get_image( nested_frame, image, format, width, height, writable );

	// Allocate the image
	int size = *width * *height * ( *format == mlt_image_yuv422 ? 2 : *format == mlt_image_rgb24 ? 3 : *format == mlt_image_rgb24a ? 4 : ( 3 / 2 ) );
	uint8_t *new_image = mlt_pool_alloc( size );

	// Update the frame
	mlt_properties properties = mlt_frame_properties( frame );
	mlt_properties_set_data( properties, "image", new_image, size, mlt_pool_release, NULL );
	memcpy( new_image, *image, size );
	mlt_frame_close( nested_frame );
	*image = new_image;

// 	mlt_properties_debug( properties, "frame", stderr );
// 	mlt_properties_debug( mlt_frame_properties( nested_frame ), "nested_frame", stderr );

	return result;
}

static int get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_frame nested_frame = mlt_frame_pop_audio( frame );
	int result = mlt_frame_get_audio( nested_frame, buffer, format, frequency, channels, samples );
	int size = *channels * *samples * sizeof( int16_t );
	int16_t *new_buffer = mlt_pool_alloc( size );
	mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "audio", new_buffer, size, mlt_pool_release, NULL );
	memcpy( new_buffer, *buffer, size );
	*buffer = new_buffer;
	mlt_frame_close( nested_frame );
	return result;
}

static int get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES(this);
	context cx = mlt_properties_get_data( properties, "context", NULL );

	if ( !cx )
	{
		// Allocate and initialize our context
		cx = mlt_pool_alloc( sizeof( struct context_s ) );
		mlt_properties_set_data( properties, "context", cx, 0, mlt_pool_release, NULL );
		cx->this = this;
		cx->profile = mlt_profile_init( mlt_properties_get( properties, "profile" ) );

		// For now, we must conform the nested network's frame rate to the parent network's
		// framerate.
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( this ) );
		cx->profile->frame_rate_num = profile->frame_rate_num;
		cx->profile->frame_rate_den = profile->frame_rate_den;

		// We will encapsulate a consumer
		cx->consumer = mlt_consumer_new( cx->profile );
		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( cx->consumer ), "real_time", 0 );
	
		// Encapsulate a real producer for the resource
		cx->producer = mlt_factory_producer( cx->profile, mlt_environment( "MLT_PRODUCER" ),
			mlt_properties_get( properties, "resource" ) );
		mlt_properties_pass_list( properties, MLT_PRODUCER_PROPERTIES( cx->producer ),
			"in, out, length, resource" );

		// Since we control the seeking, prevent it from seeking on its own
		mlt_producer_set_speed( cx->producer, 0 );

		// Connect it all together
		mlt_consumer_connect( cx->consumer, MLT_PRODUCER_SERVICE( cx->producer ) );
		mlt_consumer_start( cx->consumer );
	}

	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( this ) );
	if ( frame )
	{
		// Our "in" needs to be the same, keep it so
		mlt_properties_pass_list( MLT_PRODUCER_PROPERTIES( cx->producer ), properties, "in" );

		// Seek the producer to the correct place
		// Calculate our positions
		double actual_position = mlt_producer_get_speed( this ) * (double)mlt_producer_position( this );
		mlt_position need_first = floor( actual_position );
		mlt_producer_seek( cx->producer, need_first );
		
		// Get the nested frame
		mlt_frame nested_frame = mlt_consumer_rt_frame( cx->consumer );

		// Stack the producer and our methods on the nested frame
		mlt_frame_push_service( *frame, nested_frame );
		mlt_frame_push_service( *frame, cx );
		mlt_frame_push_get_image( *frame, get_image );
		mlt_frame_push_audio( *frame, nested_frame );
		mlt_frame_push_audio( *frame, get_audio );
		
		// Give the returned frame temporal identity
		mlt_frame_set_position( *frame, mlt_producer_position( this ) );
		
		// Put additional references on the frame so both get_image and get_audio
		// methods can close it.
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES( nested_frame ) );

		// Inform the normalizers about our video properties
		mlt_properties frame_props = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties_set_double( frame_props, "aspect_ratio", mlt_profile_sar( cx->profile ) );
		mlt_properties_set_double( frame_props, "width", cx->profile->width );
		mlt_properties_set_double( frame_props, "height", cx->profile->height );
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( this );

	return 0;
}

static void producer_close( mlt_producer this )
{
	context cx = mlt_properties_get_data( MLT_PRODUCER_PROPERTIES( this ), "context", NULL );
	
	// Shut down all the encapsulated services
	if ( cx )
	{
		mlt_consumer_stop( cx->consumer );
		mlt_consumer_close( cx->consumer );
		mlt_producer_close( cx->producer );
		free( cx->profile );
	}
	
	this->close = NULL;
	mlt_producer_close( this );
	free( this );
}

mlt_producer producer_consumer_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_producer this = mlt_producer_new( );

	// Encapsulate the real producer
	mlt_producer real_producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), arg );

	if ( this && real_producer )
	{
		// Override some producer methods
		this->close = ( mlt_destructor )producer_close;
		this->get_frame = get_frame;
		
		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );
		mlt_properties_set( properties, "resource", arg );
		mlt_properties_pass_list( properties, MLT_PRODUCER_PROPERTIES( real_producer ), "out, length" );

		// Done with the producer - will re-open later when we have the profile property
		mlt_producer_close( real_producer );
	}
	else
	{
		if ( this )
			mlt_producer_close( this );
		if ( real_producer )
			mlt_producer_close( real_producer );

		this = NULL;
	}
	return this;
}
