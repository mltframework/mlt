/*
 * mlt_events.h -- event handling 
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "mlt_properties.h"
#include "mlt_events.h"

/** Memory leak checks.
*/

//#define _MLT_EVENT_CHECKS_

#ifdef _MLT_EVENT_CHECKS_
static int events_created = 0;
static int events_destroyed = 0;
#endif

struct mlt_events_struct
{
	mlt_properties owner;
	mlt_properties list;
};

typedef struct mlt_events_struct *mlt_events;

struct mlt_event_struct
{
	mlt_events owner;
	int ref_count;
	int block_count;
	mlt_listener listener;
	void *service;
};

/** Increment the reference count on this event.
*/

void mlt_event_inc_ref( mlt_event this )
{
	if ( this != NULL )
		this->ref_count ++;
}

/** Increment the block count on this event.
*/

void mlt_event_block( mlt_event this )
{
	if ( this != NULL && this->owner != NULL )
		this->block_count ++;
}

/** Decrement the block count on this event.
*/

void mlt_event_unblock( mlt_event this )
{
	if ( this != NULL && this->owner != NULL )
		this->block_count --;
}

/** Close this event.
*/

void mlt_event_close( mlt_event this )
{
	if ( this != NULL )
	{
		if ( -- this->ref_count == 1 )
			this->owner = NULL;
		if ( this->ref_count <= 0 )
		{
#ifdef _MLT_EVENT_CHECKS_
			events_destroyed ++;
			fprintf( stderr, "Events created %d, destroyed %d\n", events_created, events_destroyed );
#endif
			free( this );
		}
	}
}

/** Forward declaration to private functions.
*/

static mlt_events mlt_events_fetch( mlt_properties );
static void mlt_events_store( mlt_properties, mlt_events );
static void mlt_events_close( mlt_events );

/** Initialise the events structure.
*/

void mlt_events_init( mlt_properties this )
{
	mlt_events events = mlt_events_fetch( this );
	if ( events == NULL )
	{
		events = malloc( sizeof( struct mlt_events_struct ) );
		events->list = mlt_properties_new( );
		mlt_events_store( this, events );
	}
}

/** Register an event and transmitter.
*/

int mlt_events_register( mlt_properties this, char *id, mlt_transmitter transmitter )
{
	int error = 1;
	mlt_events events = mlt_events_fetch( this );
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
*/

void mlt_events_fire( mlt_properties this, char *id, ... )
{
	mlt_events events = mlt_events_fetch( this );
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
				}
			}
		}
	}
}

/** Register a listener.
*/

mlt_event mlt_events_listen( mlt_properties this, void *service, char *id, mlt_listener listener )
{
	mlt_event event = NULL;
	mlt_events events = mlt_events_fetch( this );
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
*/

void mlt_events_block( mlt_properties this, void *service )
{
	mlt_events events = mlt_events_fetch( this );
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
*/

void mlt_events_unblock( mlt_properties this, void *service )
{
	mlt_events events = mlt_events_fetch( this );
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
*/

void mlt_events_disconnect( mlt_properties this, void *service )
{
	mlt_events events = mlt_events_fetch( this );
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

typedef struct
{
	int done;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
}
condition_pair;

static void mlt_events_listen_for( mlt_properties this, condition_pair *pair )
{
	pthread_mutex_lock( &pair->mutex );
	if ( pair->done == 0 )
	{
		pthread_cond_signal( &pair->cond );
		pthread_mutex_unlock( &pair->mutex );
	}
}

mlt_event mlt_events_setup_wait_for( mlt_properties this, char *id )
{
	condition_pair *pair = malloc( sizeof( condition_pair ) );
	pair->done = 0;
	pthread_cond_init( &pair->cond, NULL );
	pthread_mutex_init( &pair->mutex, NULL );
	pthread_mutex_lock( &pair->mutex );
	return mlt_events_listen( this, pair, id, ( mlt_listener )mlt_events_listen_for );
}

void mlt_events_wait_for( mlt_properties this, mlt_event event )
{
	if ( event != NULL )
	{
		condition_pair *pair = event->service;
		pthread_cond_wait( &pair->cond, &pair->mutex );
	}
}

void mlt_events_close_wait_for( mlt_properties this, mlt_event event )
{
	if ( event != NULL )
	{
		condition_pair *pair = event->service;
		event->owner = NULL;
		pair->done = 0;
		pthread_mutex_unlock( &pair->mutex );
		pthread_mutex_destroy( &pair->mutex );
		pthread_cond_destroy( &pair->cond );
	}
}

/** Fetch the events object.
*/

static mlt_events mlt_events_fetch( mlt_properties this )
{
	mlt_events events = NULL;
	if ( this != NULL )
		events = mlt_properties_get_data( this, "_events", NULL );
	return events;
}

/** Store the events object.
*/

static void mlt_events_store( mlt_properties this, mlt_events events )
{
	if ( this != NULL && events != NULL )
		mlt_properties_set_data( this, "_events", events, 0, ( mlt_destructor )mlt_events_close, NULL );
}

/** Close the events object.
*/

static void mlt_events_close( mlt_events events )
{
	if ( events != NULL )
	{
		mlt_properties_close( events->list );
		free( events );
	}
}

