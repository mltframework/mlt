/*
 * filter_freeze.c -- simple frame freezing filter
 * Copyright (C) 2022 Eoin O'Neill <eoinoneill1991@gmail.com>
 * Copyright (C) 2022 Emmet O'Neill <emmetoneill.pdx@gmail.com>
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_service.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_property.h>



typedef struct
{
	int frame_start;
	int frame_end;
	int limit_enabled;
	double speed;
	mlt_producer producer_internal;
} private_data;


static int restrict_range( int input, int min, int max ) {
	const int span = max - min;
	return (MAX(input - min, 0) % (span + 1)) + min;
}


int is_valid_range( const private_data* pdata ) {
	return pdata->frame_start > -1 && pdata->frame_end > -1 && pdata->frame_end > pdata->frame_start;
}

int is_limit_enabled( const private_data* pdata) {
	return pdata->limit_enabled != 0;
}

static int producer_get_audio( mlt_frame frame, void** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_producer producer = mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)producer->child;
	struct mlt_audio_s audio;
	mlt_audio_set_values( &audio, *buffer, *frequency, *format, *samples, *channels );

	int error = mlt_frame_get_audio( frame, &audio.data, &audio.format, &audio.frequency, &audio.channels, &audio.samples );

	// Scale the frequency to account for the speed change.
	audio.frequency = (double)audio.frequency * fabs(pdata->speed);

	if( pdata->speed < 0.0 )
	{
		mlt_audio_reverse( &audio );
	}

	mlt_audio_get_values( &audio, buffer, frequency, format, samples, channels );

	return error;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	private_data* pdata = (private_data*)producer->child;

	const int position = mlt_producer_position(pdata->producer_internal);
	if (is_valid_range(pdata) && is_limit_enabled(pdata)) {
		mlt_properties_set_position(MLT_PRODUCER_PROPERTIES(pdata->producer_internal), "_position", restrict_range(position, pdata->frame_start, pdata->frame_end));
	}

	int r = mlt_service_get_frame((mlt_service)pdata->producer_internal, frame, index);

	if ( !mlt_frame_is_test_audio( *frame ) )
	{
		mlt_frame_push_audio( *frame, producer );
		mlt_frame_push_audio( *frame, producer_get_audio );
	}

	return r;
}

static int producer_seek( mlt_producer producer, mlt_position position)
{
	private_data* pdata = (private_data*)producer->child;

	int r = mlt_producer_seek(pdata->producer_internal, position);

	return r;
}

static void producer_property_changed( mlt_service owner, mlt_producer self, mlt_event_data event_data)
{
	const char *name = mlt_event_data_to_string(event_data);
	if (!name) return;

	if (strcmp(name, "start_frame") || strcmp( name, "end_frame" )){
		private_data* pdata = (private_data*)self->child;
		mlt_properties props = MLT_PRODUCER_PROPERTIES(self);
		pdata->frame_start = mlt_properties_get_int(props, "start_frame");
		pdata->frame_end = mlt_properties_get_int(props, "end_frame");
	}

	// TODO: Find out why this strcmp doesn't work...
	//if (strcmp(name, "limit_enabled")) {
		private_data* pdata = (private_data*)self->child;
		mlt_properties props = MLT_PRODUCER_PROPERTIES(self);
		pdata->limit_enabled = mlt_properties_get_int(props, "limit_enabled");
	//}

	if (strcmp(name, "speed")) {
		mlt_properties props = MLT_PRODUCER_PROPERTIES(self);
		pdata->speed = mlt_properties_get_double(props, "speed");
	}

	if (strcmp(name, "_profile")) {
		private_data* pdata = (private_data*)self->child;
		mlt_service_set_profile((mlt_service)pdata->producer_internal, mlt_service_profile((mlt_service)self));
	}
}


static void producer_close( mlt_producer producer )
{
	private_data* pdata = (private_data*)producer->child;

	if ( pdata )
	{
		mlt_producer_close( pdata->producer_internal );
		free( pdata );
	}
}


/** Constructor for the producer.
*/
mlt_producer producer_krita_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( arg && producer && pdata )
	{
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

		// Initialize the producer
		mlt_properties_set( producer_properties, "resource", arg );
		producer->child = pdata;
		producer->get_frame = producer_get_frame;
		producer->seek = producer_seek;
		producer->close = (mlt_destructor)producer_close;

		// Get the resource to be passed to the clip producer
		char* resource = arg;

		// Default frame start / end props.
		pdata->frame_start = -1;
		pdata->frame_end = -1;
		pdata->limit_enabled = 1;
		pdata->speed = 1.0;

		// Initialize property values for start / end frames.
		mlt_properties_set_int(producer_properties, "start_frame", pdata->frame_start);
		mlt_properties_set_int(producer_properties, "end_frame", pdata->frame_end);
		mlt_properties_set_int(producer_properties, "limit_enabled", pdata->limit_enabled);
		mlt_properties_set_double(producer_properties, "speed", pdata->speed);

		// Create a producer for the clip using the false profile.
		pdata->producer_internal = mlt_factory_producer( profile, "abnormal", resource);

		if (pdata->producer_internal) {
			mlt_producer_set_speed(pdata->producer_internal, 1.0);
		}

		mlt_events_listen( producer_properties, producer, "property-changed", ( mlt_listener )producer_property_changed );
	}

	if ( !producer || !pdata || !pdata->producer_internal )
	{
		if ( pdata )
		{
			mlt_producer_close( pdata->producer_internal );
			free( pdata );
		}

		if ( producer )
		{
			producer->child = NULL;
			producer->close = NULL;
			mlt_producer_close( producer );
			free( producer );
			producer = NULL;
		}
	}

	return producer;
}


