/*
 * mlt_service.c -- interface for all service classes
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "mlt_service.h"
#include "mlt_filter.h"
#include "mlt_frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/** IMPORTANT NOTES

  	The base service implements a null frame producing service - as such,
	it is functional without extension and will produce test cards frames 
	and PAL sized audio frames.

	PLEASE DO NOT CHANGE THIS BEHAVIOUR!!! OVERRIDE THE METHODS THAT 
	CONTROL THIS IN EXTENDING CLASSES.
*/

/** Private service definition.
*/

typedef struct
{
	int size;
	int count;
	mlt_service *in;
	mlt_service out;
	int filter_count;
	int filter_size;
	mlt_filter *filters;
	pthread_mutex_t mutex;
}
mlt_service_base;

/** Private methods
*/

static void mlt_service_disconnect( mlt_service this );
static void mlt_service_connect( mlt_service this, mlt_service that );
static int service_get_frame( mlt_service this, mlt_frame_ptr frame, int index );
static void mlt_service_property_changed( mlt_listener, mlt_properties owner, mlt_service this, void **args );

/** Constructor
*/

int mlt_service_init( mlt_service this, void *child )
{
	int error = 0;

	// Initialise everything to NULL
	memset( this, 0, sizeof( struct mlt_service_s ) );

	// Assign the child
	this->child = child;

	// Generate local space
	this->local = calloc( sizeof( mlt_service_base ), 1 );

	// Associate the methods
	this->get_frame = service_get_frame;
	
	// Initialise the properties
	error = mlt_properties_init( &this->parent, this );
	if ( error == 0 )
	{
		this->parent.close = ( mlt_destructor )mlt_service_close;
		this->parent.close_object = this;

		mlt_events_init( &this->parent );
		mlt_events_register( &this->parent, "service-changed", NULL );
		mlt_events_register( &this->parent, "property-changed", ( mlt_transmitter )mlt_service_property_changed );
		pthread_mutex_init( &( ( mlt_service_base * )this->local )->mutex, NULL );
	}

	return error;
}

static void mlt_service_property_changed( mlt_listener listener, mlt_properties owner, mlt_service this, void **args )
{
	if ( listener != NULL )
		listener( owner, this, ( char * )args[ 0 ] );
}

void mlt_service_lock( mlt_service this )
{
	if ( this != NULL )
		pthread_mutex_lock( &( ( mlt_service_base * )this->local )->mutex );
}

void mlt_service_unlock( mlt_service this )
{
	if ( this != NULL )
		pthread_mutex_unlock( &( ( mlt_service_base * )this->local )->mutex );
}

mlt_service_type mlt_service_identify( mlt_service this )
{
	mlt_service_type type = invalid_type;
	if ( this != NULL )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( this );
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );
		char *resource = mlt_properties_get( properties, "resource" );
		if ( mlt_type == NULL )
			type = unknown_type;
		else if ( resource == NULL || !strcmp( resource, "<producer>" ) )
			type = producer_type;
		else if ( !strcmp( resource, "<playlist>" ) )
			type = playlist_type;
		else if ( !strcmp( resource, "<tractor>" ) )
			type = tractor_type;
		else if ( !strcmp( resource, "<multitrack>" ) )
			type = multitrack_type;
		else if ( !strcmp( mlt_type, "producer" ) )
			type = producer_type;
		else if ( !strcmp( mlt_type, "filter" ) )
			type = filter_type;
		else if ( !strcmp( mlt_type, "transition" ) )
			type = transition_type;
		else if ( !strcmp( mlt_type, "consumer" ) )
			type = consumer_type;
		else
			type = unknown_type;
	}
	return type;
}

/** Connect a producer service.
	Returns: > 0 warning, == 0 success, < 0 serious error
			 1 = this service does not accept input
			 2 = the producer is invalid
			 3 = the producer is already registered with this consumer
*/

int mlt_service_connect_producer( mlt_service this, mlt_service producer, int index )
{
	int i = 0;

	// Get the service base
	mlt_service_base *base = this->local;

	// Check if the producer is already registered with this service
	for ( i = 0; i < base->count; i ++ )
		if ( base->in[ i ] == producer )
			return 3;

	// Allocate space
	if ( index >= base->size )
	{
		int new_size = base->size + index + 10;
		base->in = realloc( base->in, new_size * sizeof( mlt_service ) );
		if ( base->in != NULL )
		{
			for ( i = base->size; i < new_size; i ++ )
				base->in[ i ] = NULL;
			base->size = new_size;
		}
	}

	// If we have space, assign the input
	if ( base->in != NULL && index >= 0 && index < base->size )
	{
		// Get the current service
		mlt_service current = base->in[ index ];

		// Increment the reference count on this producer
		if ( producer != NULL )
		{
			mlt_service_lock( producer );
			mlt_properties_inc_ref( MLT_SERVICE_PROPERTIES( producer ) );
			mlt_service_unlock( producer );
		}

		// Now we disconnect the producer service from its consumer
		mlt_service_disconnect( producer );
		
		// Add the service to index specified
		base->in[ index ] = producer;
		
		// Determine the number of active tracks
		if ( index >= base->count )
			base->count = index + 1;

		// Now we connect the producer to its connected consumer
		mlt_service_connect( producer, this );

		// Close the current service
		mlt_service_close( current );

		// Inform caller that all went well
		return 0;
	}
	else
	{
		return -1;
	}
}

/** Disconnect this service from its consumer.
*/

static void mlt_service_disconnect( mlt_service this )
{
	if ( this != NULL )
	{
		// Get the service base
		mlt_service_base *base = this->local;

		// Disconnect
		base->out = NULL;
	}
}

/** Obtain the consumer this service is connected to.
*/

mlt_service mlt_service_consumer( mlt_service this )
{
	// Get the service base
	mlt_service_base *base = this->local;

	// Return the connected consumer
	return base->out;
}

/** Obtain the producer this service is connected to.
*/

mlt_service mlt_service_producer( mlt_service this )
{
	// Get the service base
	mlt_service_base *base = this->local;

	// Return the connected producer
	return base->count > 0 ? base->in[ base->count - 1 ] : NULL;
}

/** Associate this service to the consumer.
*/

static void mlt_service_connect( mlt_service this, mlt_service that )
{
	if ( this != NULL )
	{
		// Get the service base
		mlt_service_base *base = this->local;

		// There's a bit more required here...
		base->out = that;
	}
}

/** Get the first connected producer service.
*/

mlt_service mlt_service_get_producer( mlt_service this )
{
	mlt_service producer = NULL;

	// Get the service base
	mlt_service_base *base = this->local;

	if ( base->in != NULL )
		producer = base->in[ 0 ];
	
	return producer;
}

/** Default implementation of get_frame.
*/

static int service_get_frame( mlt_service this, mlt_frame_ptr frame, int index )
{
	mlt_service_base *base = this->local;
	if ( index < base->count )
	{
		mlt_service producer = base->in[ index ];
		if ( producer != NULL )
			return mlt_service_get_frame( producer, frame, index );
	}
	*frame = mlt_frame_init( );
	return 0;
}

/** Return the properties object.
*/

mlt_properties mlt_service_properties( mlt_service self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Recursively apply attached filters
*/

void mlt_service_apply_filters( mlt_service this, mlt_frame frame, int index )
{
	int i;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_properties service_properties = MLT_SERVICE_PROPERTIES( this );
	mlt_service_base *base = this->local;
	mlt_position position = mlt_frame_get_position( frame );
	mlt_position this_in = mlt_properties_get_position( service_properties, "in" );
	mlt_position this_out = mlt_properties_get_position( service_properties, "out" );

	if ( index == 0 || mlt_properties_get_int( service_properties, "_filter_private" ) == 0 )
	{
		// Process the frame with the attached filters
		for ( i = 0; i < base->filter_count; i ++ )
		{
			if ( base->filters[ i ] != NULL )
			{
				mlt_position in = mlt_filter_get_in( base->filters[ i ] );
				mlt_position out = mlt_filter_get_out( base->filters[ i ] );
				if ( ( in == 0 && out == 0 ) || ( position >= in && ( position <= out || out == 0 ) ) )
				{
					mlt_properties_set_position( frame_properties, "in", in == 0 ? this_in : in );
					mlt_properties_set_position( frame_properties, "out", out == 0 ? this_out : out );
					mlt_filter_process( base->filters[ i ], frame );
					mlt_service_apply_filters( MLT_FILTER_SERVICE( base->filters[ i ] ), frame, index + 1 );
				}
			}
		}
	}
}

/** Obtain a frame.
*/

int mlt_service_get_frame( mlt_service this, mlt_frame_ptr frame, int index )
{
	mlt_service_lock( this );
	if ( this != NULL && this->get_frame != NULL )
	{
		int result = 0;
		mlt_properties properties = MLT_SERVICE_PROPERTIES( this );
		mlt_position in = mlt_properties_get_position( properties, "in" );
		mlt_position out = mlt_properties_get_position( properties, "out" );
		mlt_properties_inc_ref( properties );
		mlt_service_unlock( this );
		result = this->get_frame( this, frame, index );
		if ( result == 0 )
		{
			properties = MLT_FRAME_PROPERTIES( *frame );
			if ( in >=0 && out > 0 )
			{
				mlt_properties_set_position( properties, "in", in );
				mlt_properties_set_position( properties, "out", out );
			}
			mlt_service_apply_filters( this, *frame, 1 );
			mlt_deque_push_back( MLT_FRAME_SERVICE_STACK( *frame ), this );
		}
		else
		{
			mlt_service_close( this );
		}
		return result;
	}
	mlt_service_unlock( this );
	*frame = mlt_frame_init( );
	return 0;
}

static void mlt_service_filter_changed( mlt_service owner, mlt_service this )
{
	mlt_events_fire( MLT_SERVICE_PROPERTIES( this ), "service-changed", NULL );
}

/** Attach a filter.
*/

int mlt_service_attach( mlt_service this, mlt_filter filter )
{
	int error = this == NULL || filter == NULL;
	if ( error == 0 )
	{
		int i = 0;
		mlt_properties properties = MLT_SERVICE_PROPERTIES( this );
		mlt_service_base *base = this->local;

		for ( i = 0; error == 0 && i < base->filter_count; i ++ )
			if ( base->filters[ i ] == filter )
				error = 1;

		if ( error == 0 )
		{
			if ( base->filter_count == base->filter_size )
			{
				base->filter_size += 10;
				base->filters = realloc( base->filters, base->filter_size * sizeof( mlt_filter ) );
			}

			if ( base->filters != NULL )
			{
				mlt_properties props = MLT_FILTER_PROPERTIES( filter );
				mlt_properties_inc_ref( MLT_FILTER_PROPERTIES( filter ) );
				base->filters[ base->filter_count ++ ] = filter;
				mlt_events_fire( properties, "service-changed", NULL );
				mlt_events_listen( props, this, "service-changed", ( mlt_listener )mlt_service_filter_changed );
				mlt_events_listen( props, this, "property-changed", ( mlt_listener )mlt_service_filter_changed );
			}
			else
			{
				error = 2;
			}
		}
	}
	return error;
}

/** Detach a filter.
*/

int mlt_service_detach( mlt_service this, mlt_filter filter )
{
	int error = this == NULL || filter == NULL;
	if ( error == 0 )
	{
		int i = 0;
		mlt_service_base *base = this->local;
		mlt_properties properties = MLT_SERVICE_PROPERTIES( this );

		for ( i = 0; i < base->filter_count; i ++ )
			if ( base->filters[ i ] == filter )
				break;

		if ( i < base->filter_count )
		{
			base->filters[ i ] = NULL;
			for ( i ++ ; i < base->filter_count; i ++ )
				base->filters[ i - 1 ] = base->filters[ i ];
			base->filter_count --;
			mlt_events_disconnect( MLT_FILTER_PROPERTIES( filter ), this );
			mlt_filter_close( filter );
			mlt_events_fire( properties, "service-changed", NULL );
		}
	}
	return error;
}

/** Retrieve a filter.
*/

mlt_filter mlt_service_filter( mlt_service this, int index )
{
	mlt_filter filter = NULL;
	if ( this != NULL )
	{
		mlt_service_base *base = this->local;
		if ( index >= 0 && index < base->filter_count )
			filter = base->filters[ index ];
	}
	return filter;
}

/** Close the service.
*/

void mlt_service_close( mlt_service this )
{
	mlt_service_lock( this );
	if ( this != NULL && mlt_properties_dec_ref( MLT_SERVICE_PROPERTIES( this ) ) <= 0 )
	{
		mlt_service_unlock( this );
		if ( this->close != NULL )
		{
			this->close( this->close_object );
		}
		else
		{
			mlt_service_base *base = this->local;
			int i = 0;
			int count = base->filter_count;
			mlt_events_block( MLT_SERVICE_PROPERTIES( this ), this );
			while( count -- )
				mlt_service_detach( this, base->filters[ 0 ] );
			free( base->filters );
			for ( i = 0; i < base->count; i ++ )
				if ( base->in[ i ] != NULL )
					mlt_service_close( base->in[ i ] );
			this->parent.close = NULL;
			free( base->in );
			pthread_mutex_destroy( &base->mutex );
			free( base );
			mlt_properties_close( &this->parent );
		}
	}
	else
	{
		mlt_service_unlock( this );
	}
}

