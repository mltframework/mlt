/*
 * consumer_sdl_preview.c -- A Simple DirectMedia Layer consumer
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
 * Author: Charles Yates
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

#include "consumer_sdl.h"
#include <framework/mlt_frame.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_producer.h>
#include <stdlib.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

typedef struct consumer_sdl_s *consumer_sdl;

struct consumer_sdl_s
{
	struct mlt_consumer_s parent;
	mlt_consumer active;
	int ignore_change;
	mlt_consumer play;
	mlt_consumer still;
	pthread_t thread;
	int joined;
	int running;
	int sdl_flags;
	double last_speed;
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static void consumer_frame_show_cb( mlt_consumer sdl, mlt_consumer this, mlt_frame frame );
static void consumer_sdl_event_cb( mlt_consumer sdl, mlt_consumer this, SDL_Event *event );

mlt_consumer consumer_sdl_preview_init( char *arg )
{
	consumer_sdl this = calloc( sizeof( struct consumer_sdl_s ), 1 );
	if ( this != NULL && mlt_consumer_init( &this->parent, this ) == 0 )
	{
		// Get the parent consumer object
		mlt_consumer parent = &this->parent;
		this->play = mlt_factory_consumer( "sdl", arg );
		this->still = mlt_factory_consumer( "sdl_still", arg );
		mlt_properties_set( mlt_consumer_properties( parent ), "real_time", "0" );
		parent->close = consumer_close;
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;
		this->joined = 1;
		mlt_events_listen( mlt_consumer_properties( this->play ), this, "consumer-frame-show", ( mlt_listener )consumer_frame_show_cb );
		mlt_events_listen( mlt_consumer_properties( this->still ), this, "consumer-frame-show", ( mlt_listener )consumer_frame_show_cb );
		return parent;
	}
	free( this );
	return NULL;
}

void consumer_frame_show_cb( mlt_consumer sdl, mlt_consumer parent, mlt_frame frame )
{
	consumer_sdl this = parent->child;
	this->last_speed = mlt_properties_get_double( mlt_frame_properties( frame ), "_speed" );
	mlt_events_fire( mlt_consumer_properties( parent ), "consumer-frame-show", frame, NULL );
}

static void consumer_sdl_event_cb( mlt_consumer sdl, mlt_consumer parent, SDL_Event *event )
{
	mlt_events_fire( mlt_consumer_properties( parent ), "consumer-sdl-event", event, NULL );
}

static int consumer_start( mlt_consumer parent )
{
	consumer_sdl this = parent->child;

	if ( !this->running )
	{
		pthread_attr_t thread_attributes;
		
		consumer_stop( parent );

		this->running = 1;
		this->joined = 0;
		this->last_speed = 1;

		// Inherit the scheduling priority
		pthread_attr_init( &thread_attributes );
		pthread_attr_setinheritsched( &thread_attributes, PTHREAD_INHERIT_SCHED );

		pthread_create( &this->thread, &thread_attributes, consumer_thread, this );
	}

	return 0;
}

static int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl this = parent->child;

	if ( this->joined == 0 )
	{
		// Kill the thread and clean up
		this->running = 0;

		pthread_join( this->thread, NULL );
		this->joined = 1;
	}

	return 0;
}

static int consumer_is_stopped( mlt_consumer parent )
{
	consumer_sdl this = parent->child;
	return !this->running;
}

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_sdl this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;

	// internal intialization
	int first = 1;
	mlt_frame frame = NULL;

	// properties
	mlt_properties properties = mlt_consumer_properties( consumer );
	mlt_properties play = mlt_consumer_properties( this->play );
	mlt_properties still = mlt_consumer_properties( this->still );

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) < 0 )
	{
		fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
		return NULL;
	}

	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
	SDL_EnableUNICODE( 1 );

	// Inform child consumers that we control the sdl
	mlt_properties_set_int( play, "sdl_started", 1 );
	mlt_properties_set_int( still, "sdl_started", 1 );

	// Pass properties down
	mlt_properties_set_data( play, "transport_producer", mlt_properties_get_data( properties, "transport_producer", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "transport_producer", mlt_properties_get_data( properties, "transport_producer", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( play, "transport_callback", mlt_properties_get_data( properties, "transport_callback", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "transport_callback", mlt_properties_get_data( properties, "transport_callback", NULL ), 0, NULL, NULL );
	mlt_properties_set( play, "rescale", mlt_properties_get( properties, "rescale" ) );
	mlt_properties_set( still, "rescale", mlt_properties_get( properties, "rescale" ) );
	mlt_properties_set( play, "width", mlt_properties_get( properties, "width" ) );
	mlt_properties_set( still, "width", mlt_properties_get( properties, "width" ) );
	mlt_properties_set( play, "height", mlt_properties_get( properties, "height" ) );
	mlt_properties_set( still, "height", mlt_properties_get( properties, "height" ) );

	mlt_properties_pass( play, mlt_consumer_properties( consumer ), "play." );
	mlt_properties_pass( still, mlt_consumer_properties( consumer ), "still." );

	mlt_properties_set_data( play, "app_lock", mlt_properties_get_data( properties, "app_lock", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "app_lock", mlt_properties_get_data( properties, "app_lock", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( play, "app_unlock", mlt_properties_get_data( properties, "app_unlock", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "app_unlock", mlt_properties_get_data( properties, "app_unlock", NULL ), 0, NULL, NULL );

	mlt_properties_set_int( play, "put_mode", 1 );
	mlt_properties_set_int( still, "put_mode", 1 );

	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_get_frame( consumer );

		// Ensure that we have a frame
		if ( frame != NULL )
		{
			// Get the speed of the frame
			double speed = mlt_properties_get_double( mlt_frame_properties( frame ), "_speed" );

			// Determine which speed to use
			double use_speed = first ? speed : this->last_speed;

			// Get changed requests to the preview
			int changed = mlt_properties_get_int( properties, "changed" );
			mlt_properties_set_int( properties, "changed", 0 );

			// Make sure the recipient knows that this frame isn't really rendered
			mlt_properties_set_int( mlt_frame_properties( frame ), "rendered", 0 );

			if ( !first && mlt_consumer_is_stopped( this->play ) && mlt_consumer_is_stopped( this->still ) )
			{
				this->running = 0;
				mlt_frame_close( frame );
			}
			else if ( this->ignore_change -- > 0 && this->active != NULL && !mlt_consumer_is_stopped( this->active ) )
			{
				mlt_consumer_put_frame( this->active, frame );
				if ( this->active == this->still )
					mlt_properties_set_int( still, "changed", changed );
			}
			else if ( use_speed != 1 )
			{
				if ( !mlt_consumer_is_stopped( this->play ) )
				{
					mlt_consumer_stop( this->play );
				}
				if ( mlt_consumer_is_stopped( this->still ) )
				{
					this->last_speed = use_speed;
					this->active = this->still;
					this->ignore_change = 5;
					mlt_consumer_start( this->still );
				}
				mlt_properties_set_int( still, "changed", changed );
				mlt_consumer_put_frame( this->still, frame );
			}
			else
			{
				if ( !mlt_consumer_is_stopped( this->still ) )
				{
					mlt_consumer_stop( this->still );
				}
				if ( mlt_consumer_is_stopped( this->play ) )
				{
					this->last_speed = use_speed;
					this->active = this->play;
					this->ignore_change = 25;
					mlt_consumer_start( this->play );
				}
				mlt_consumer_put_frame( this->play, frame );
			}
			first = 0;
		}
	}

	mlt_consumer_stop( this->play );
	mlt_consumer_stop( this->still );

	SDL_Quit( );

	return NULL;
}

/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl this = parent->child;

	// Stop the consumer
	mlt_consumer_stop( parent );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the child consumers
	mlt_consumer_close( this->play );
	mlt_consumer_close( this->still );

	// Finally clean up this
	free( this );
}
