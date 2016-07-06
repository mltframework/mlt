/**
 * \file mlt_service.c
 * \brief interface definition for all service classes
 * \see mlt_service_s
 *
 * Copyright (C) 2003-2016 Meltytech, LLC
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

#include "mlt_service.h"
#include "mlt_filter.h"
#include "mlt_frame.h"
#include "mlt_cache.h"
#include "mlt_factory.h"
#include "mlt_log.h"
#include "mlt_producer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


/*  IMPORTANT NOTES

  	The base service implements a null frame producing service - as such,
	it is functional without extension and will produce test cards frames
	and PAL sized audio frames.

	PLEASE DO NOT CHANGE THIS BEHAVIOUR!!! OVERRIDE THE METHODS THAT
	CONTROL THIS IN EXTENDING CLASSES.
*/

/** \brief private service definition */

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

/* Private methods
 */

static void mlt_service_disconnect( mlt_service self );
static void mlt_service_connect( mlt_service self, mlt_service that );
static int service_get_frame( mlt_service self, mlt_frame_ptr frame, int index );
static void mlt_service_property_changed( mlt_listener, mlt_properties owner, mlt_service self, void **args );

/** Initialize a service.
 *
 * \public \memberof mlt_service_s
 * \param self the service structure to initialize
 * \param child pointer to the child object for the subclass
 * \return true if there was an error
 */

int mlt_service_init( mlt_service self, void *child )
{
	int error = 0;

	// Initialise everything to NULL
	memset( self, 0, sizeof( struct mlt_service_s ) );

	// Assign the child
	self->child = child;

	// Generate local space
	self->local = calloc( 1, sizeof( mlt_service_base ) );

	// Associate the methods
	self->get_frame = service_get_frame;

	// Initialise the properties
	error = mlt_properties_init( &self->parent, self );
	if ( error == 0 )
	{
		self->parent.close = ( mlt_destructor )mlt_service_close;
		self->parent.close_object = self;

		mlt_events_init( &self->parent );
		mlt_events_register( &self->parent, "service-changed", NULL );
		mlt_events_register( &self->parent, "property-changed", ( mlt_transmitter )mlt_service_property_changed );
		pthread_mutex_init( &( ( mlt_service_base * )self->local )->mutex, NULL );
	}

	return error;
}

/** The transmitter for property changes.
 *
 * Invokes the listener.
 *
 * \private \memberof mlt_service_s
 * \param listener a function pointer that will be invoked
 * \param owner a properties list that will be passed to \p listener
 * \param self a service that will be passed to \p listener
 * \param args an array of pointers - the first entry is passed as a string to \p listener
 */

static void mlt_service_property_changed( mlt_listener listener, mlt_properties owner, mlt_service self, void **args )
{
	if ( listener != NULL )
		listener( owner, self, ( char * )args[ 0 ] );
}

/** Acquire a mutual exclusion lock on this service.
 *
 * \public \memberof mlt_service_s
 * \param self the service to lock
 */

void mlt_service_lock( mlt_service self )
{
	if ( self != NULL )
		pthread_mutex_lock( &( ( mlt_service_base * )self->local )->mutex );
}

/** Release a mutual exclusion lock on this service.
 *
 * \public \memberof mlt_service_s
 * \param self the service to unlock
 */

void mlt_service_unlock( mlt_service self )
{
	if ( self != NULL )
		pthread_mutex_unlock( &( ( mlt_service_base * )self->local )->mutex );
}

/** Identify the subclass of the service.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the subclass
 */

mlt_service_type mlt_service_identify( mlt_service self )
{
	mlt_service_type type = invalid_type;
	if ( self != NULL )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( self );
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );
		char *resource = mlt_properties_get( properties, "resource" );
		if ( mlt_type == NULL )
			type = unknown_type;
		else if (resource != NULL && !strcmp( resource, "<playlist>" ) )
			type = playlist_type;
		else if (resource != NULL && !strcmp( resource, "<tractor>" ) )
			type = tractor_type;
		else if (resource != NULL && !strcmp( resource, "<multitrack>" ) )
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

/** Connect a producer to the service.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param producer a producer
 * \param index which of potentially multiple producers to this service (0 based)
 * \return 0 for success, -1 for error, or 3 if \p producer is already connected to \p self
 */

int mlt_service_connect_producer( mlt_service self, mlt_service producer, int index )
{
	int i = 0;

	// Get the service base
	mlt_service_base *base = self->local;

	// Special case 'track' index - only works for last filter(s) in a particular chain
	// but allows a filter to apply to the output frame regardless of which track it comes from
	if ( index == -1 )
		index = 0;

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
		mlt_service current = ( index < base->count )? base->in[ index ] : NULL;

		// Increment the reference count on this producer
		if ( producer != NULL )
			mlt_properties_inc_ref( MLT_SERVICE_PROPERTIES( producer ) );

		// Now we disconnect the producer service from its consumer
		mlt_service_disconnect( producer );

		// Add the service to index specified
		base->in[ index ] = producer;

		// Determine the number of active tracks
		if ( index >= base->count )
			base->count = index + 1;

		// Now we connect the producer to its connected consumer
		mlt_service_connect( producer, self );

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

/** Insert a producer connected to the service.
 *
 * mlt_service_connect_producer() appends or overwrites a producer at input
 * \p index whereas this function inserts pushing all of the inputs down by
 * 1 in the list of inputs.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param producer a producer
 * \param index which of potentially multiple producers to this service (0 based)
 * \return 0 for success, -1 for error, or 3 if \p producer is already connected to \p self
 */

int mlt_service_insert_producer( mlt_service self, mlt_service producer, int index )
{
	// Get the service base
	mlt_service_base *base = self->local;

	if ( index >= base->count )
		return mlt_service_connect_producer( self, producer, index );

	int i = 0;

	// Special case 'track' index - only works for last filter(s) in a particular chain
	// but allows a filter to apply to the output frame regardless of which track it comes from
	if ( index == -1 )
		index = 0;

	// Check if the producer is already registered with this service.
	for ( i = 0; i < base->count; i ++ )
		if ( base->in[ i ] == producer )
			return 3;

	// Allocate space if needed.
	if ( base->count + 1 > base->size )
	{
		int new_size = base->size + 10;
		base->in = realloc( base->in, new_size * sizeof( mlt_service ) );
		if ( base->in != NULL )
		{
			memset( &base->in[ base->size ], 0, new_size - base->size );
			base->size = new_size;
		}
	}

	// If we have space, assign the input
	if ( base->in && index >= 0 && index < base->size )
	{
		// Increment the reference count on this producer.
		if ( producer != NULL )
			mlt_properties_inc_ref( MLT_SERVICE_PROPERTIES( producer ) );

		// Disconnect the producer from its consumer.
		mlt_service_disconnect( producer );

		// Make room in the list for the producer.
		memmove( &base->in[ index + 1 ], &base->in[ index ],
				( base->count - index ) * sizeof( mlt_service ) );

		// Add the service to index specified.
		base->in[ index ] = producer;

		// Increase the number of active tracks.
		base->count ++;

		// Connect the producer to its connected consumer.
		mlt_service_connect( producer, self );

		// Inform caller that all went well
		return 0;
	}
	else
	{
		return -1;
	}
}

/** Remove the N-th producer.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param index which producer to remove (0-based)
 * \return true if there was an error
 */

int mlt_service_disconnect_producer( mlt_service self, int index )
{
	mlt_service_base *base = self->local;

	if ( base->in && index >= 0 && index < base->count )
	{
		mlt_service current = base->in[ index ];

		if ( current )
		{
			// Close the current producer.
			mlt_service_disconnect( current );
			mlt_service_close( current );
			base->in[ index ] = NULL;

			// Contract the list of producers.
			for ( ; index + 1 < base->count; index ++ )
				base->in[ index ] = base->in[ index + 1 ];
			base->count --;
			return 0;
		}
	}
	return -1;
}

/** Disconnect a service from its consumer.
 *
 * \private \memberof mlt_service_s
 * \param self a service
 */

static void mlt_service_disconnect( mlt_service self )
{
	if ( self != NULL )
	{
		// Get the service base
		mlt_service_base *base = self->local;

		// Disconnect
		base->out = NULL;
	}
}

/** Obtain the consumer a service is connected to.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the consumer
 */

mlt_service mlt_service_consumer( mlt_service self )
{
	// Get the service base
	mlt_service_base *base = self->local;

	// Return the connected consumer
	return base->out;
}

/** Obtain the producer a service is connected to.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the last-most producer
 */

mlt_service mlt_service_producer( mlt_service self )
{
	// Get the service base
	mlt_service_base *base = self->local;

	// Return the connected producer
	return base->count > 0 ? base->in[ base->count - 1 ] : NULL;
}

/** Associate a service to a consumer.
 *
 * Overwrites connection to any existing consumer.
 * \private \memberof mlt_service_s
 * \param self a service
 * \param that a consumer
 */

static void mlt_service_connect( mlt_service self, mlt_service that )
{
	if ( self != NULL )
	{
		// Get the service base
		mlt_service_base *base = self->local;

		// There's a bit more required here...
		base->out = that;
	}
}

/** Get the first connected producer.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the first producer
 */

mlt_service mlt_service_get_producer( mlt_service self )
{
	mlt_service producer = NULL;

	// Get the service base
	mlt_service_base *base = self->local;

	if ( base->in != NULL )
		producer = base->in[ 0 ];

	return producer;
}

/** Default implementation of the get_frame virtual function.
 *
 * \private \memberof mlt_service_s
 * \param self a service
 * \param[out] frame a frame by reference
 * \param index as determined by the producer
 * \return false
 */

static int service_get_frame( mlt_service self, mlt_frame_ptr frame, int index )
{
	mlt_service_base *base = self->local;
	if ( index < base->count )
	{
		mlt_service producer = base->in[ index ];
		if ( producer != NULL )
			return mlt_service_get_frame( producer, frame, index );
	}
	*frame = mlt_frame_init( self );
	return 0;
}

/** Return the properties object.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the properties
 */

mlt_properties mlt_service_properties( mlt_service self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Recursively apply attached filters.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param frame a frame
 * \param index used to track depth of recursion, top caller should supply 0
 */

void mlt_service_apply_filters( mlt_service self, mlt_frame frame, int index )
{
	int i;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_properties service_properties = MLT_SERVICE_PROPERTIES( self );
	mlt_service_base *base = self->local;
	mlt_position position = mlt_frame_get_position( frame );
	mlt_position self_in = mlt_properties_get_position( service_properties, "in" );
	mlt_position self_out = mlt_properties_get_position( service_properties, "out" );

	if ( index == 0 || mlt_properties_get_int( service_properties, "_filter_private" ) == 0 )
	{
		// Process the frame with the attached filters
		for ( i = 0; i < base->filter_count; i ++ )
		{
			if ( base->filters[ i ] != NULL )
			{
				mlt_position in = mlt_filter_get_in( base->filters[ i ] );
				mlt_position out = mlt_filter_get_out( base->filters[ i ] );
				int disable = mlt_properties_get_int( MLT_FILTER_PROPERTIES( base->filters[ i ] ), "disable" );
				if ( !disable && ( ( in == 0 && out == 0 ) || ( position >= in && ( position <= out || out == 0 ) ) ) )
				{
					mlt_properties_set_position( frame_properties, "in", in == 0 ? self_in : in );
					mlt_properties_set_position( frame_properties, "out", out == 0 ? self_out : out );
					mlt_filter_process( base->filters[ i ], frame );
					mlt_service_apply_filters( MLT_FILTER_SERVICE( base->filters[ i ] ), frame, index + 1 );
				}
			}
		}
	}
}

/** Obtain a frame.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param[out] frame a frame by reference
 * \param index as determined by the producer
 * \return true if there was an error
 */

int mlt_service_get_frame( mlt_service self, mlt_frame_ptr frame, int index )
{
	int result = 0;

	// Lock the service
	mlt_service_lock( self );

	// Ensure that the frame is NULL
	*frame = NULL;

	// Only process if we have a valid service
	if ( self != NULL && self->get_frame != NULL )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( self );
		mlt_position in = mlt_properties_get_position( properties, "in" );
		mlt_position out = mlt_properties_get_position( properties, "out" );
		mlt_position position = mlt_service_identify( self ) == producer_type ? mlt_producer_position( MLT_PRODUCER( self ) ) : -1;

		result = self->get_frame( self, frame, index );

		if ( result == 0 )
		{
			mlt_properties_inc_ref( properties );
			properties = MLT_FRAME_PROPERTIES( *frame );
			
			if ( in >=0 && out > 0 )
			{
				mlt_properties_set_position( properties, "in", in );
				mlt_properties_set_position( properties, "out", out );
			}
			mlt_service_apply_filters( self, *frame, 1 );
			mlt_deque_push_back( MLT_FRAME_SERVICE_STACK( *frame ), self );
			
			if ( mlt_service_identify( self ) == producer_type &&
			     mlt_properties_get_int( MLT_SERVICE_PROPERTIES( self ), "_need_previous_next" ) )
			{
				// Save the new position from self->get_frame
				mlt_position new_position = mlt_producer_position( MLT_PRODUCER( self ) );
				
				// Get the preceding frame, unfiltered
				mlt_frame previous_frame;
				mlt_producer_seek( MLT_PRODUCER(self), position - 1 );
				result = self->get_frame( self, &previous_frame, index );
				if ( !result )
					mlt_properties_set_data( properties, "previous frame",
						previous_frame, 0, ( mlt_destructor ) mlt_frame_close, NULL );

				// Get the following frame, unfiltered
				mlt_frame next_frame;
				mlt_producer_seek( MLT_PRODUCER(self), position + 1 );
				result = self->get_frame( self, &next_frame, index );
				if ( !result )
				{
					mlt_properties_set_data( properties, "next frame",
						next_frame, 0, ( mlt_destructor ) mlt_frame_close, NULL );
				}
				
				// Restore the new position
				mlt_producer_seek( MLT_PRODUCER(self), new_position );
			}
		}
	}

	// Make sure we return a frame
	if ( *frame == NULL )
		*frame = mlt_frame_init( self );

	// Unlock the service
	mlt_service_unlock( self );

	return result;
}

/** The service-changed event handler.
 *
 * \private \memberof mlt_service_s
 * \param owner ignored
 * \param self the service on which the "service-changed" event is fired
 */

static void mlt_service_filter_changed( mlt_service owner, mlt_service self )
{
	mlt_events_fire( MLT_SERVICE_PROPERTIES( self ), "service-changed", NULL );
}

/** The property-changed event handler.
 *
 * \private \memberof mlt_service_s
 * \param owner ignored
 * \param self the service on which the "property-changed" event is fired
 * \param name the name of the property that changed
 */

static void mlt_service_filter_property_changed( mlt_service owner, mlt_service self, char *name )
{
    mlt_events_fire( MLT_SERVICE_PROPERTIES( self ), "property-changed", name, NULL );
}

/** Attach a filter.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param filter the filter to attach
 * \return true if there was an error
 */

int mlt_service_attach( mlt_service self, mlt_filter filter )
{
	int error = self == NULL || filter == NULL;
	if ( error == 0 )
	{
		int i = 0;
		mlt_properties properties = MLT_SERVICE_PROPERTIES( self );
		mlt_service_base *base = self->local;

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
				mlt_properties_set_data( props, "service", self, 0, NULL, NULL );
				mlt_events_fire( properties, "service-changed", NULL );
				mlt_events_fire( props, "service-changed", NULL );
				mlt_service cp = mlt_properties_get_data( properties, "_cut_parent", NULL );
				if ( cp )
					mlt_events_fire( MLT_SERVICE_PROPERTIES(cp), "service-changed", NULL );
				mlt_events_listen( props, self, "service-changed", ( mlt_listener )mlt_service_filter_changed );
				mlt_events_listen( props, self, "property-changed", ( mlt_listener )mlt_service_filter_property_changed );
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
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param filter the filter to detach
 * \return true if there was an error
 */

int mlt_service_detach( mlt_service self, mlt_filter filter )
{
	int error = self == NULL || filter == NULL;
	if ( error == 0 )
	{
		int i = 0;
		mlt_service_base *base = self->local;
		mlt_properties properties = MLT_SERVICE_PROPERTIES( self );

		for ( i = 0; i < base->filter_count; i ++ )
			if ( base->filters[ i ] == filter )
				break;

		if ( i < base->filter_count )
		{
			base->filters[ i ] = NULL;
			for ( i ++ ; i < base->filter_count; i ++ )
				base->filters[ i - 1 ] = base->filters[ i ];
			base->filter_count --;
			mlt_events_disconnect( MLT_FILTER_PROPERTIES( filter ), self );
			mlt_filter_close( filter );
			mlt_events_fire( properties, "service-changed", NULL );
		}
	}
	return error;
}

/** Get the number of filters attached.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the number of attached filters or -1 if there was an error
 */

int mlt_service_filter_count( mlt_service self )
{
	int result = -1;
	if ( self )
	{
		mlt_service_base *base = self->local;
		result = base->filter_count;
	}
	return result;
}

/** Reorder the attached filters.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param from the current index value of the filter to move
 * \param to the new index value for the filter specified in \p from
 * \return true if there was an error
 */

int mlt_service_move_filter( mlt_service self, int from, int to )
{
	int error = -1;
	if ( self )
	{
		mlt_service_base *base = self->local;
		if ( from < 0 ) from = 0;
		if ( from >= base->filter_count ) from = base->filter_count - 1;
		if ( to < 0 ) to = 0;
		if ( to >= base->filter_count ) to = base->filter_count - 1;
		if ( from != to && base->filter_count > 1 )
		{
			mlt_filter filter = base->filters[from];
			int i;
			if ( from > to )
			{
				for ( i = from; i > to; i-- )
					base->filters[i] = base->filters[i - 1];
			}
			else
			{
				for ( i = from; i < to; i++ )
					base->filters[i] = base->filters[i + 1];
			}
			base->filters[to] = filter;
			mlt_events_fire( MLT_SERVICE_PROPERTIES(self), "service-changed", NULL );
			error = 0;
		}
	}
	return error;
}

/** Retrieve an attached filter.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param index which one of potentially multiple filters
 * \return the filter or null if there was an error
 */

mlt_filter mlt_service_filter( mlt_service self, int index )
{
	mlt_filter filter = NULL;
	if ( self != NULL )
	{
		mlt_service_base *base = self->local;
		if ( index >= 0 && index < base->filter_count )
			filter = base->filters[ index ];
	}
	return filter;
}

/** Retrieve the profile.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \return the profile
 */

mlt_profile mlt_service_profile( mlt_service self )
{
	return self? mlt_properties_get_data( MLT_SERVICE_PROPERTIES( self ), "_profile", NULL ) : NULL;
}

/** Set the profile for a service.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param profile the profile to set onto the service
 */

void mlt_service_set_profile( mlt_service self, mlt_profile profile )
{
    mlt_properties_set_data( MLT_SERVICE_PROPERTIES( self ), "_profile", profile, 0, NULL, NULL );
}

/** Destroy a service.
 *
 * \public \memberof mlt_service_s
 * \param self the service to destroy
 */

void mlt_service_close( mlt_service self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_SERVICE_PROPERTIES( self ) ) <= 0 )
	{
		if ( self->close != NULL )
		{
			self->close( self->close_object );
		}
		else
		{
			mlt_service_base *base = self->local;
			int i = 0;
			int count = base->filter_count;
			mlt_events_block( MLT_SERVICE_PROPERTIES( self ), self );
			while( count -- )
				mlt_service_detach( self, base->filters[ 0 ] );
			free( base->filters );
			for ( i = 0; i < base->count; i ++ )
				if ( base->in[ i ] != NULL )
					mlt_service_close( base->in[ i ] );
			self->parent.close = NULL;
			free( base->in );
			pthread_mutex_destroy( &base->mutex );
			free( base );
			mlt_properties_close( &self->parent );
		}
	}
}

/** Release a service's cache items.
 *
 * \private \memberof mlt_service_s
 * \param self a service
 */

void mlt_service_cache_purge( mlt_service self )
{
	mlt_properties caches = mlt_properties_get_data( mlt_global_properties(), "caches", NULL );

	if ( caches )
	{
		int i = mlt_properties_count( caches );
		while ( i-- )
		{
			mlt_cache_purge( mlt_properties_get_data_at( caches, i, NULL ), self );
			mlt_properties_set_data( mlt_global_properties(), mlt_properties_get_name( caches, i ), NULL, 0, NULL, NULL );
		}
	}
}

/** Lookup the cache object for a service.
 *
 * \private \memberof mlt_service_s
 * \param self a service
 * \param name a name for the object
 * \return a cache
 */

static mlt_cache get_cache( mlt_service self, const char *name )
{
	mlt_cache result = NULL;
	mlt_properties caches = mlt_properties_get_data( mlt_global_properties(), "caches", NULL );

	if ( !caches )
	{
		caches = mlt_properties_new();
		mlt_properties_set_data( mlt_global_properties(), "caches", caches, 0, ( mlt_destructor )mlt_properties_close, NULL );
	}
	if ( caches )
	{
		result = mlt_properties_get_data( caches, name, NULL );
		if ( !result )
		{
			result = mlt_cache_init();
			mlt_properties_set_data( caches, name, result, 0, ( mlt_destructor )mlt_cache_close, NULL );
		}
	}
	
	return result;
}

/** Put an object into a service's cache.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param name a name for the object that is unique to the service class, but not to the instance
 * \param data an opaque pointer to the object to put into the cache
 * \param size the number of bytes pointed to by data
 * \param destructor a function that releases the data
 */

void mlt_service_cache_put( mlt_service self, const char *name, void* data, int size, mlt_destructor destructor )
{
	mlt_log( self, MLT_LOG_DEBUG, "%s: name %s object %p data %p\n", __FUNCTION__, name, self, data );
	mlt_cache cache = get_cache( self, name );

	if ( cache )
		mlt_cache_put( cache, self, data, size, destructor );
}

/** Get an object from a service's cache.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param name a name for the object that is unique to the service class, but not to the instance
 * \return a cache item or NULL if an object is not found
 * \see mlt_cache_item_data
 */

mlt_cache_item mlt_service_cache_get( mlt_service self, const char *name )
{
	mlt_log( self, MLT_LOG_DEBUG, "%s: name %s object %p\n", __FUNCTION__, name, self );
	mlt_cache_item result = NULL;
	mlt_cache cache = get_cache( self, name );

	if ( cache )
		result = mlt_cache_get( cache, self );

	return result;
}

/** Set the number of items to cache for the named cache.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param name a name for the object that is unique to the service class, but not to the instance
 * \param size the number of items to cache
 */

void mlt_service_cache_set_size( mlt_service self, const char *name, int size )
{
	mlt_cache cache = get_cache( self, name );
	if ( cache )
		mlt_cache_set_size( cache, size );
}

/** Get the current maximum size of the named cache.
 *
 * \public \memberof mlt_service_s
 * \param self a service
 * \param name a name for the object that is unique to the service class, but not to the instance
 * \return the current maximum number of items to cache or zero if there is an error
 */

int mlt_service_cache_get_size( mlt_service self, const char *name )
{
	mlt_cache cache = get_cache( self, name );
	if ( cache )
		return mlt_cache_get_size( cache );
	else
		return 0;
}
