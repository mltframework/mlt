/**
 * \file mlt_events.c
 * \brief event handling
 * \see mlt_events_struct
 *
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "mlt_properties.h"
#include "mlt_events.h"

/* Memory leak checks. */

#undef _MLT_EVENT_CHECKS_

#ifdef _MLT_EVENT_CHECKS_
static int events_created = 0;
static int events_destroyed = 0;
#endif

/** \brief Events class
 *
 * Events provide messages and notifications between services and the application.
 * A service can register an event and fire/send it upon certain conditions or times.
 * Likewise, a service or an application can listen/receive specific events on specific
 * services.
 */

struct mlt_events_struct
{
	mlt_properties owner;
	mlt_properties list;
};

typedef struct mlt_events_struct *mlt_events;

/** \brief Event class
 *
 */

struct mlt_event_struct
{
	mlt_events owner;
	int ref_count;
	int block_count;
	mlt_listener listener;
	void *service;
};

/** Increment the reference count on self event.
 *
 * \public \memberof mlt_event_struct
 * \param self an event
 */

void mlt_event_inc_ref( mlt_event self )
{
	if ( self != NULL )
		self->ref_count ++;
}

/** Increment the block count on self event.
 *
 * \public \memberof mlt_event_struct
 * \param self an event
 */

void mlt_event_block( mlt_event self )
{
	if ( self != NULL && self->owner != NULL )
		self->block_count ++;
}

/** Decrement the block count on self event.
 *
 * \public \memberof mlt_event_struct
 * \param self an event
 */

void mlt_event_unblock( mlt_event self )
{
	if ( self != NULL && self->owner != NULL )
		self->block_count --;
}

/** Close self event.
 *
 * \public \memberof mlt_event_struct
 * \param self an event
 */

void mlt_event_close( mlt_event self )
{
	if ( self != NULL )
	{
		if ( -- self->ref_count == 1 )
			self->owner = NULL;
		if ( self->ref_count <= 0 )
		{
#ifdef _MLT_EVENT_CHECKS_
			mlt_log( NULL, MLT_LOG_DEBUG, "Events created %d, destroyed %d\n", events_created, ++events_destroyed );
#endif
			free( self );
		}
	}
}

/* Forward declaration to private functions.
*/

static mlt_events mlt_events_fetch( mlt_properties );
static void mlt_events_store( mlt_properties, mlt_events );
static void mlt_events_close( mlt_events );

/** Initialise the events structure.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 */

void mlt_events_init( mlt_properties self )
{
	mlt_events events = mlt_events_fetch( self );
	if ( events == NULL )
	{
		events = calloc( 1, sizeof( struct mlt_events_struct ) );
		events->list = mlt_properties_new( );
		mlt_events_store( self, events );
	}
}

/** Register an event and transmitter.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param id the name of an event
 * \param transmitter the callback function to send an event message
 * \return true if there was an error
 */

int mlt_events_register( mlt_properties self, const char *id, mlt_transmitter transmitter )
{
	int error = 1;
	mlt_events events = mlt_events_fetch( self );
	if ( events != NULL )
	{
		mlt_properties list = events->list;
		char temp[ 128 ];
		error = mlt_properties_set_data( list, id, transmitter, 0, NULL, NULL );
		sprintf( temp, "list:%s", id );
		if ( mlt_properties_get_data( list, temp, NULL ) == NULL )
			mlt_properties_set_data( list, temp, mlt_properties_new( ), 0, ( mlt_destructor )mlt_properties_close, NULL );
	}
	return error;
}

/** Fire an event.
 *
 * This takes a variable number of arguments to supply to the listener.

 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param id the name of an event
 * \return the number of listeners
 */

int mlt_events_fire( mlt_properties self, const char *id, ... )
{
	int result = 0;
	mlt_events events = mlt_events_fetch( self );
	if ( events != NULL )
	{
		int i = 0;
		va_list alist;
		void *args[ 10 ];
		mlt_properties list = events->list;
		mlt_properties listeners = NULL;
		char temp[ 128 ];
		mlt_transmitter transmitter = mlt_properties_get_data( list, id, NULL );
		sprintf( temp, "list:%s", id );
		listeners = mlt_properties_get_data( list, temp, NULL );

		va_start( alist, id );
		do
			args[ i ] = va_arg( alist, void * );
		while( args[ i ++ ] != NULL );
		va_end( alist );

		if ( listeners != NULL )
		{
			for ( i = 0; i < mlt_properties_count( listeners ); i ++ )
			{
				mlt_event event = mlt_properties_get_data_at( listeners, i, NULL );
				if ( event != NULL && event->owner != NULL && event->block_count == 0 )
				{
					if ( transmitter != NULL )
						transmitter( event->listener, event->owner, event->service, args );
					else
						event->listener( event->owner, event->service );
					++result;
				}
			}
		}
	}
	return result;
}

/** Register a listener.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param service an opaque pointer
 * \param id the name of the event to listen for
 * \param listener the callback to receive an event message
 * \return
 */

mlt_event mlt_events_listen( mlt_properties self, void *service, const char *id, mlt_listener listener )
{
	mlt_event event = NULL;
	mlt_events events = mlt_events_fetch( self );
	if ( events != NULL )
	{
		mlt_properties list = events->list;
		mlt_properties listeners = NULL;
		char temp[ 128 ];
		sprintf( temp, "list:%s", id );
		listeners = mlt_properties_get_data( list, temp, NULL );
		if ( listeners != NULL )
		{
			int first_null = -1;
			int i = 0;
			for ( i = 0; event == NULL && i < mlt_properties_count( listeners ); i ++ )
			{
				mlt_event entry = mlt_properties_get_data_at( listeners, i, NULL );
				if ( entry != NULL && entry->owner != NULL )
				{
					if ( entry->service == service && entry->listener == listener )
						event = entry;
				}
				else if ( ( entry == NULL || entry->owner == NULL ) && first_null == -1 )
				{
					first_null = i;
				}
			}

			if ( event == NULL )
			{
				event = malloc( sizeof( struct mlt_event_struct ) );
				if ( event != NULL )
				{
#ifdef _MLT_EVENT_CHECKS_
					events_created ++;
#endif
					sprintf( temp, "%d", first_null == -1 ? mlt_properties_count( listeners ) : first_null );
					event->owner = events;
					event->ref_count = 0;
					event->block_count = 0;
					event->listener = listener;
					event->service = service;
					mlt_properties_set_data( listeners, temp, event, 0, ( mlt_destructor )mlt_event_close, NULL );
					mlt_event_inc_ref( event );
				}
			}

		}
	}
	return event;
}

/** Block all events for a given service.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param service an opaque pointer
 */

void mlt_events_block( mlt_properties self, void *service )
{
	mlt_events events = mlt_events_fetch( self );
	if ( events != NULL )
	{
		int i = 0, j = 0;
		mlt_properties list = events->list;
		for ( j = 0; j < mlt_properties_count( list ); j ++ )
		{
			char *temp = mlt_properties_get_name( list, j );
			if ( !strncmp( temp, "list:", 5 ) )
			{
				mlt_properties listeners = mlt_properties_get_data( list, temp, NULL );
				for ( i = 0; i < mlt_properties_count( listeners ); i ++ )
				{
					mlt_event entry = mlt_properties_get_data_at( listeners, i, NULL );
					if ( entry != NULL && entry->service == service )
						mlt_event_block( entry );
				}
			}
		}
	}
}

/** Unblock all events for a given service.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param service an opaque pointer
 */

void mlt_events_unblock( mlt_properties self, void *service )
{
	mlt_events events = mlt_events_fetch( self );
	if ( events != NULL )
	{
		int i = 0, j = 0;
		mlt_properties list = events->list;
		for ( j = 0; j < mlt_properties_count( list ); j ++ )
		{
			char *temp = mlt_properties_get_name( list, j );
			if ( !strncmp( temp, "list:", 5 ) )
			{
				mlt_properties listeners = mlt_properties_get_data( list, temp, NULL );
				for ( i = 0; i < mlt_properties_count( listeners ); i ++ )
				{
					mlt_event entry = mlt_properties_get_data_at( listeners, i, NULL );
					if ( entry != NULL && entry->service == service )
						mlt_event_unblock( entry );
				}
			}
		}
	}
}

/** Disconnect all events for a given service.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param service an opaque pointer
 */

void mlt_events_disconnect( mlt_properties self, void *service )
{
	mlt_events events = mlt_events_fetch( self );
	if ( events != NULL )
	{
		int i = 0, j = 0;
		mlt_properties list = events->list;
		for ( j = 0; j < mlt_properties_count( list ); j ++ )
		{
			char *temp = mlt_properties_get_name( list, j );
			if ( !strncmp( temp, "list:", 5 ) )
			{
				mlt_properties listeners = mlt_properties_get_data( list, temp, NULL );
				for ( i = 0; i < mlt_properties_count( listeners ); i ++ )
				{
					mlt_event entry = mlt_properties_get_data_at( listeners, i, NULL );
					char *name = mlt_properties_get_name( listeners, i );
					if ( entry != NULL && entry->service == service )
						mlt_properties_set_data( listeners, name, NULL, 0, NULL, NULL );
				}
			}
		}
	}
}

/** \brief private to mlt_events_struct, used by mlt_events_wait_for() */

typedef struct
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
}
condition_pair;

/** The event listener callback for the wait functions.
 *
 * \private \memberof mlt_events_struct
 * \param self a properties list
 * \param pair a condition pair
 */

static void mlt_events_listen_for( mlt_properties self, condition_pair *pair )
{
	pthread_mutex_lock( &pair->mutex );
	pthread_cond_signal( &pair->cond );
	pthread_mutex_unlock( &pair->mutex );
}

/** Prepare to wait for an event.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param id the name of the event to wait for
 * \return an event
 */

mlt_event mlt_events_setup_wait_for( mlt_properties self, const char *id )
{
	condition_pair *pair = malloc( sizeof( condition_pair ) );
	pthread_cond_init( &pair->cond, NULL );
	pthread_mutex_init( &pair->mutex, NULL );
	pthread_mutex_lock( &pair->mutex );
	return mlt_events_listen( self, pair, id, ( mlt_listener )mlt_events_listen_for );
}

/** Wait for an event.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param event an event
 */

void mlt_events_wait_for( mlt_properties self, mlt_event event )
{
	if ( event != NULL )
	{
		condition_pair *pair = event->service;
		pthread_cond_wait( &pair->cond, &pair->mutex );
	}
}

/** Cleanup after waiting for an event.
 *
 * \public \memberof mlt_events_struct
 * \param self a properties list
 * \param event an event
 */

void mlt_events_close_wait_for( mlt_properties self, mlt_event event )
{
	if ( event != NULL )
	{
		condition_pair *pair = event->service;
		event->owner = NULL;
		pthread_mutex_unlock( &pair->mutex );
		pthread_mutex_destroy( &pair->mutex );
		pthread_cond_destroy( &pair->cond );
		free( pair );
	}
}

/** Fetch the events object.
 *
 * \private \memberof mlt_events_struct
 * \param self a properties list
 * \return an events object
 */

static mlt_events mlt_events_fetch( mlt_properties self )
{
	mlt_events events = NULL;
	if ( self != NULL )
		events = mlt_properties_get_data( self, "_events", NULL );
	return events;
}

/** Store the events object.
 *
 * \private \memberof mlt_events_struct
 * \param self a properties list
 * \param events an events object
 */

static void mlt_events_store( mlt_properties self, mlt_events events )
{
	if ( self != NULL && events != NULL )
		mlt_properties_set_data( self, "_events", events, 0, ( mlt_destructor )mlt_events_close, NULL );
}

/** Close the events object.
 *
 * \private \memberof mlt_events_struct
 * \param events an events object
 */

static void mlt_events_close( mlt_events events )
{
	if ( events != NULL )
	{
		mlt_properties_close( events->list );
		free( events );
	}
}
