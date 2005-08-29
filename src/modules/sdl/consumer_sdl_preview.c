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
#include <string.h>
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

	pthread_cond_t refresh_cond;
	pthread_mutex_t refresh_mutex;
	int refresh_count;
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
static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer this, char *name );

mlt_consumer consumer_sdl_preview_init( char *arg )
{
	consumer_sdl this = calloc( sizeof( struct consumer_sdl_s ), 1 );
	if ( this != NULL && mlt_consumer_init( &this->parent, this ) == 0 )
	{
		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// Get the properties
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );

		// Get the width and height
		int width = mlt_properties_get_int( properties, "width" );
		int height = mlt_properties_get_int( properties, "height" );

		// Process actual param
		if ( arg == NULL || sscanf( arg, "%dx%d", &width, &height ) == 2 )
		{
			mlt_properties_set_int( properties, "width", width );
			mlt_properties_set_int( properties, "height", height );
		}

		// Create child consumers
		this->play = mlt_factory_consumer( "sdl", arg );
		this->still = mlt_factory_consumer( "sdl_still", arg );
		mlt_properties_set( MLT_CONSUMER_PROPERTIES( parent ), "real_time", "0" );
		mlt_properties_set( MLT_CONSUMER_PROPERTIES( parent ), "rescale", "nearest" );
		parent->close = consumer_close;
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;
		this->joined = 1;
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( this->play ), this, "consumer-frame-show", ( mlt_listener )consumer_frame_show_cb );
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( this->still ), this, "consumer-frame-show", ( mlt_listener )consumer_frame_show_cb );
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( this->play ), this, "consumer-sdl-event", ( mlt_listener )consumer_sdl_event_cb );
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( this->still ), this, "consumer-sdl-event", ( mlt_listener )consumer_sdl_event_cb );
		pthread_cond_init( &this->refresh_cond, NULL );
		pthread_mutex_init( &this->refresh_mutex, NULL );
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( parent ), this, "property-changed", ( mlt_listener )consumer_refresh_cb );
		return parent;
	}
	free( this );
	return NULL;
}

void consumer_frame_show_cb( mlt_consumer sdl, mlt_consumer parent, mlt_frame frame )
{
	consumer_sdl this = parent->child;
	this->last_speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" );
	mlt_events_fire( MLT_CONSUMER_PROPERTIES( parent ), "consumer-frame-show", frame, NULL );
}

static void consumer_sdl_event_cb( mlt_consumer sdl, mlt_consumer parent, SDL_Event *event )
{
	mlt_events_fire( MLT_CONSUMER_PROPERTIES( parent ), "consumer-sdl-event", event, NULL );
}

static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer parent, char *name )
{
	if ( !strcmp( name, "refresh" ) )
	{
		consumer_sdl this = parent->child;
		pthread_mutex_lock( &this->refresh_mutex );
		this->refresh_count = this->refresh_count <= 0 ? 1 : this->refresh_count ++;
		pthread_cond_broadcast( &this->refresh_cond );
		pthread_mutex_unlock( &this->refresh_mutex );
	}
}

static int consumer_start( mlt_consumer parent )
{
	consumer_sdl this = parent->child;

	if ( !this->running )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		char *window_id = mlt_properties_get( properties, "window_id" );
		char *audio_driver = mlt_properties_get( properties, "audio_driver" );

		consumer_stop( parent );

		this->running = 1;
		this->joined = 0;
		this->last_speed = 1;

		if ( window_id != NULL )
			setenv( "SDL_WINDOWID", window_id, 1 );

		if ( audio_driver != NULL )
			setenv( "SDL_AUDIODRIVER", audio_driver, 1 );

		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) < 0 )
		{
			fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
			return -1;
		}

		SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
		SDL_EnableUNICODE( 1 );

		pthread_create( &this->thread, NULL, consumer_thread, this );
	}

	return 0;
}

static int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl this = parent->child;

	if ( this->joined == 0 )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		int app_locked = mlt_properties_get_int( properties, "app_locked" );
		void ( *lock )( void ) = mlt_properties_get_data( properties, "app_lock", NULL );
		void ( *unlock )( void ) = mlt_properties_get_data( properties, "app_unlock", NULL );

		if ( app_locked && unlock ) unlock( );

		// Kill the thread and clean up
		this->running = 0;

		pthread_mutex_lock( &this->refresh_mutex );
		pthread_cond_broadcast( &this->refresh_cond );
		pthread_mutex_unlock( &this->refresh_mutex );

		pthread_join( this->thread, NULL );
		this->joined = 1;

		if ( app_locked && lock ) lock( );

		SDL_Quit( );
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
	int last_position = -1;

	// properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_properties play = MLT_CONSUMER_PROPERTIES( this->play );
	mlt_properties still = MLT_CONSUMER_PROPERTIES( this->still );

	int progressive = mlt_properties_get_int( properties, "progressive" ) | mlt_properties_get_int( properties, "deinterlace" );

	// Inform child consumers that we control the sdl
	mlt_properties_set_int( play, "sdl_started", 1 );
	mlt_properties_set_int( still, "sdl_started", 1 );

	// Pass properties down
	mlt_properties_set_data( play, "transport_producer", mlt_properties_get_data( properties, "transport_producer", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "transport_producer", mlt_properties_get_data( properties, "transport_producer", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( play, "transport_callback", mlt_properties_get_data( properties, "transport_callback", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "transport_callback", mlt_properties_get_data( properties, "transport_callback", NULL ), 0, NULL, NULL );
	mlt_properties_set_int( play, "resize", mlt_properties_get_int( properties, "resize" ) );
	mlt_properties_set_int( still, "resize", mlt_properties_get_int( properties, "resize" ) );
	mlt_properties_set( play, "rescale", mlt_properties_get( properties, "rescale" ) );
	mlt_properties_set( still, "rescale", mlt_properties_get( properties, "rescale" ) );
	mlt_properties_set_int( play, "width", mlt_properties_get_int( properties, "width" ) );
	mlt_properties_set_int( still, "width", mlt_properties_get_int( properties, "width" ) );
	mlt_properties_set_int( play, "height", mlt_properties_get_int( properties, "height" ) );
	mlt_properties_set_int( still, "height", mlt_properties_get_int( properties, "height" ) );
	mlt_properties_set_double( play, "aspect_ratio", mlt_properties_get_double( properties, "aspect_ratio" ) );
	mlt_properties_set_double( still, "aspect_ratio", mlt_properties_get_double( properties, "aspect_ratio" ) );
	mlt_properties_set_double( play, "display_ratio", mlt_properties_get_double( properties, "display_ratio" ) );
	mlt_properties_set_double( still, "display_ratio", mlt_properties_get_double( properties, "display_ratio" ) );

	mlt_properties_set_int( play, "progressive", progressive );
	mlt_properties_set_int( still, "progressive", progressive );
	mlt_properties_set( play, "deinterlace_method", mlt_properties_get( properties, "deinterlace_method" ) );
	mlt_properties_set( still, "deinterlace_method", mlt_properties_get( properties, "deinterlace_method" ) );

	mlt_properties_pass( play, MLT_CONSUMER_PROPERTIES( consumer ), "play." );
	mlt_properties_pass( still, MLT_CONSUMER_PROPERTIES( consumer ), "still." );

	mlt_properties_set_data( play, "app_lock", mlt_properties_get_data( properties, "app_lock", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "app_lock", mlt_properties_get_data( properties, "app_lock", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( play, "app_unlock", mlt_properties_get_data( properties, "app_unlock", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( still, "app_unlock", mlt_properties_get_data( properties, "app_unlock", NULL ), 0, NULL, NULL );

	mlt_properties_set_int( play, "put_mode", 1 );
	mlt_properties_set_int( still, "put_mode", 1 );

	this->refresh_count = 0;

	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_get_frame( consumer );

		// Ensure that we have a frame
		if ( this->running && frame != NULL )
		{
			// Get the speed of the frame
			double speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" );

			// Lock during the operation
			mlt_service_lock( MLT_CONSUMER_SERVICE( consumer ) );

			// Get refresh request for the current frame
			int refresh = mlt_properties_get_int( properties, "refresh" );

			// Decrement refresh and clear changed
			mlt_events_block( properties, properties );
			mlt_properties_set_int( properties, "refresh", 0 );
			mlt_events_unblock( properties, properties );

			// Unlock after the operation
			mlt_service_unlock( MLT_CONSUMER_SERVICE( consumer ) );

			// Set the changed property on this frame for the benefit of still
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "refresh", refresh );

			// Make sure the recipient knows that this frame isn't really rendered
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 0 );

			// Optimisation to reduce latency
			if ( speed == 1.0 )
			{
				if ( last_position != -1 && last_position + 1 != mlt_frame_get_position( frame ) )
					mlt_consumer_purge( this->play );
				last_position = mlt_frame_get_position( frame );
			}
			else
			{
				//mlt_consumer_purge( this->play );
				last_position = -1;
			}

			// If we're not the first frame and both consumers are stopped, then stop ourselves
			if ( !first && mlt_consumer_is_stopped( this->play ) && mlt_consumer_is_stopped( this->still ) )
			{
				this->running = 0;
				mlt_frame_close( frame );
			}
			// Allow a little grace time before switching consumers on speed changes
			else if ( this->ignore_change -- > 0 && this->active != NULL && !mlt_consumer_is_stopped( this->active ) )
			{
				mlt_consumer_put_frame( this->active, frame );
			}
			// If we aren't playing normally, then use the still
			else if ( speed != 1 )
			{
				if ( !mlt_consumer_is_stopped( this->play ) )
					mlt_consumer_stop( this->play );
				if ( mlt_consumer_is_stopped( this->still ) )
				{
					this->last_speed = speed;
					this->active = this->still;
					this->ignore_change = 0;
					mlt_consumer_start( this->still );
				}
				mlt_consumer_put_frame( this->still, frame );
			}
			// Otherwise use the normal player
			else
			{
				if ( !mlt_consumer_is_stopped( this->still ) )
					mlt_consumer_stop( this->still );
				if ( mlt_consumer_is_stopped( this->play ) )
				{
					this->last_speed = speed;
					this->active = this->play;
					this->ignore_change = 25;
					mlt_consumer_start( this->play );
				}
				mlt_consumer_put_frame( this->play, frame );
			}

			// Copy the rectangle info from the active consumer
			if ( this->running )
			{
				mlt_properties active = MLT_CONSUMER_PROPERTIES( this->active );
				mlt_service_lock( MLT_CONSUMER_SERVICE( consumer ) );
				mlt_properties_set_int( properties, "rect_x", mlt_properties_get_int( active, "rect_x" ) );
				mlt_properties_set_int( properties, "rect_y", mlt_properties_get_int( active, "rect_y" ) );
				mlt_properties_set_int( properties, "rect_w", mlt_properties_get_int( active, "rect_w" ) );
				mlt_properties_set_int( properties, "rect_h", mlt_properties_get_int( active, "rect_h" ) );
				mlt_service_unlock( MLT_CONSUMER_SERVICE( consumer ) );
			}

			if ( this->active == this->still )
			{
				pthread_mutex_lock( &this->refresh_mutex );
				if ( this->running && speed == 0 && this->refresh_count <= 0 )
					pthread_cond_wait( &this->refresh_cond, &this->refresh_mutex );
				this->refresh_count --;
				pthread_mutex_unlock( &this->refresh_mutex );
			}

			// We are definitely not waiting on the first frame any more
			first = 0;
		}
		else
		{
			if ( frame ) mlt_frame_close( frame );
			mlt_consumer_put_frame( this->active, NULL );
			this->running = 0;
		}
	}

	if ( this->play ) mlt_consumer_stop( this->play );
	if ( this->still ) mlt_consumer_stop( this->still );

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

	// Close the child consumers
	mlt_consumer_close( this->play );
	mlt_consumer_close( this->still );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Finally clean up this
	free( this );
}
