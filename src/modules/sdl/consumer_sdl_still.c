/*
 * consumer_sdl_still.c -- A Simple DirectMedia Layer consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
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
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <sys/time.h>

/** This classes definition.
*/

typedef struct consumer_sdl_s *consumer_sdl;

struct consumer_sdl_s
{
	struct mlt_consumer_s parent;
	mlt_properties properties;
	pthread_t thread;
	int joined;
	int running;
	int window_width;
	int window_height;
	float aspect_ratio;
	float display_aspect;
	double last_frame_aspect;
	int width;
	int height;
	int playing;
	int sdl_flags;
	SDL_Surface *sdl_screen;
	SDL_Rect rect;
	uint8_t *buffer;
	int last_position;
	mlt_producer last_producer;
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static int consumer_get_dimensions( int *width, int *height );
static void consumer_sdl_event( mlt_listener listener, mlt_properties owner, mlt_service this, void **args );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_sdl_still_init( char *arg )
{
	// Create the consumer object
	consumer_sdl this = calloc( sizeof( struct consumer_sdl_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( &this->parent, this ) == 0 )
	{
		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// Attach a colour space converter
		mlt_filter filter = mlt_factory_filter( "avcolour_space", NULL );
		mlt_properties_set_int( mlt_filter_properties( filter ), "forced", mlt_image_yuv422 );
		mlt_service_attach( mlt_consumer_service( &this->parent ), filter );
		mlt_filter_close( filter );

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// get a handle on properties
		mlt_service service = mlt_consumer_service( parent );
		this->properties = mlt_service_properties( service );

		// Default scaler (for now we'll use nearest)
		mlt_properties_set( this->properties, "rescale", "nearest" );

		// We're always going to run this in non-realtime mode
		mlt_properties_set( this->properties, "real_time", "0" );

		// Default progressive true
		mlt_properties_set_int( this->properties, "progressive", 1 );

		// Get sample aspect ratio
		this->aspect_ratio = mlt_properties_get_double( this->properties, "aspect_ratio" );

		// Ensure we don't join on a non-running object
		this->joined = 1;
		
		// Default display aspect ratio
		this->display_aspect = 4.0 / 3.0;
		
		// process actual param
		if ( arg == NULL || sscanf( arg, "%dx%d", &this->width, &this->height ) != 2 )
		{
			this->width = mlt_properties_get_int( this->properties, "width" );
			this->height = mlt_properties_get_int( this->properties, "height" );
		}

		// Default window size
		this->window_width = ( float )this->height * this->display_aspect;
		this->window_height = this->height;

		// Set the sdl flags
		this->sdl_flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE | SDL_DOUBLEBUF;

		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;

		// Register specific events
		mlt_events_register( this->properties, "consumer-sdl-event", ( mlt_transmitter )consumer_sdl_event );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

static void consumer_sdl_event( mlt_listener listener, mlt_properties owner, mlt_service this, void **args )
{
	if ( listener != NULL )
		listener( owner, this, ( SDL_Event * )args[ 0 ] );
}

static int consumer_start( mlt_consumer parent )
{
	consumer_sdl this = parent->child;

	if ( !this->running )
	{
		pthread_attr_t thread_attributes;
		
		consumer_stop( parent );

		this->last_position = -1;
		this->running = 1;
		this->joined = 0;

		// Allow the user to force resizing to window size
		if ( mlt_properties_get_int( this->properties, "resize" ) )
		{
			mlt_properties_set_int( this->properties, "width", this->width );
			mlt_properties_set_int( this->properties, "height", this->height );
		}

		//this->width = this->height * this->display_aspect;

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

static int sdl_lock_display( )
{
	SDL_Surface *screen = SDL_GetVideoSurface( );
	return screen != NULL && ( !SDL_MUSTLOCK( screen ) || SDL_LockSurface( screen ) >= 0 );
}

static void sdl_unlock_display( )
{
	SDL_Surface *screen = SDL_GetVideoSurface( );
	if ( screen != NULL && SDL_MUSTLOCK( screen ) )
		SDL_UnlockSurface( screen );
}

static int consumer_play_video( consumer_sdl this, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;

	mlt_image_format vfmt = mlt_image_rgb24;
	int height = this->height;
	int width = this->width;
	uint8_t *image = NULL;
	int changed = 0;

	void ( *lock )( void ) = mlt_properties_get_data( properties, "app_lock", NULL );
	void ( *unlock )( void ) = mlt_properties_get_data( properties, "app_unlock", NULL );

	if ( lock != NULL ) lock( );

	sdl_lock_display();
	
	// Handle events
	if ( this->sdl_screen != NULL )
	{
		SDL_Event event;

		changed = consumer_get_dimensions( &this->window_width, &this->window_height );

		while ( SDL_PollEvent( &event ) )
		{
			mlt_events_fire( this->properties, "consumer-sdl-event", &event, NULL );

			switch( event.type )
			{
				case SDL_VIDEORESIZE:
					this->window_width = event.resize.w;
					this->window_height = event.resize.h;
					changed = 1;
					break;
				case SDL_QUIT:
					this->running = 0;
					break;
				case SDL_KEYDOWN:
					{
						mlt_producer producer = mlt_properties_get_data( properties, "transport_producer", NULL );
						char keyboard[ 2 ] = " ";
						void (*callback)( mlt_producer, char * ) = mlt_properties_get_data( properties, "transport_callback", NULL );
						if ( callback != NULL && producer != NULL && event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0 )
						{
							keyboard[ 0 ] = ( char )event.key.keysym.unicode;
							callback( producer, keyboard );
						}
					}
					break;
			}
		}
	}

	if ( ( ( int )( this->last_frame_aspect * 1000 ) != ( int )( mlt_frame_get_aspect_ratio( frame ) * 1000 ) &&
		 ( mlt_frame_get_aspect_ratio( frame ) != 1.0 || this->last_frame_aspect == 0.0 ) ) )
	{
		this->last_frame_aspect = mlt_frame_get_aspect_ratio( frame );
		changed = 1;
	}

	if ( this->sdl_screen == NULL || changed || mlt_properties_get_int( properties, "changed" ) == 2 )
	{
		// open SDL window 
		this->sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, 16, this->sdl_flags );
		if ( consumer_get_dimensions( &this->window_width, &this->window_height ) )
			this->sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, 16, this->sdl_flags );

		changed = 1;
		mlt_properties_set_int( properties, "changed", 0 );

		// May as well use the mlt rescaler...
		//if ( mlt_properties_get( properties, "rescale" ) != NULL && !strcmp( mlt_properties_get( properties, "rescale" ), "none" ) )
			//mlt_properties_set( properties, "rescale", "nearest" );

		// Determine window's new display aspect ratio
		float this_aspect = this->display_aspect / ( ( float )this->window_width / ( float )this->window_height );

		this->rect.w = this_aspect * this->window_width;
		this->rect.h = this->window_height;
		if ( this->rect.w > this->window_width )
		{
			this->rect.w = this->window_width;
			this->rect.h = ( 1.0 / this_aspect ) * this->window_height;
		}

		this->rect.x = ( this->window_width - this->rect.w ) / 2;
		this->rect.y = ( this->window_height - this->rect.h ) / 2;

		mlt_properties_set_int( this->properties, "rect_x", this->rect.x );
		mlt_properties_set_int( this->properties, "rect_y", this->rect.y );
		mlt_properties_set_int( this->properties, "rect_w", this->rect.w );
		mlt_properties_set_int( this->properties, "rect_h", this->rect.h );
	}
	else
	{
		changed = mlt_properties_get_int( properties, "changed" ) | mlt_properties_get_int( mlt_frame_properties( frame ), "refresh" );
		mlt_properties_set_int( properties, "changed", 0 );
	}
		
	if ( changed == 0 &&
		 this->last_position == mlt_frame_get_position( frame ) &&
		 this->last_producer == mlt_properties_get_data( mlt_frame_properties( frame ), "_producer", NULL ) )
	{
		sdl_unlock_display( );
		if ( unlock != NULL )
			unlock( );
		return 0;
	}

	// Update last frame shown info
	this->last_position = mlt_frame_get_position( frame );
	this->last_producer = mlt_properties_get_data( mlt_frame_properties( frame ), "_producer", NULL );

	// Get the image, width and height
	if ( image == NULL )
	{
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

		// I would like to provide upstream scaling here, but this is incorrect
		// Something? (or everything?) is too sensitive to aspect ratio
		//width = this->rect.w;
		//height = this->rect.h;
		//mlt_properties_set( mlt_frame_properties( frame ), "distort", "true" );
		//mlt_properties_set_int( mlt_frame_properties( frame ), "normalised_width", width );
		//mlt_properties_set_int( mlt_frame_properties( frame ), "normalised_height", height );

		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
	}

	if ( this->sdl_screen != NULL )
	{
		// Calculate the scan length
		int scanlength = this->sdl_screen->pitch / 2;

		// Obtain the clip rect from the screen
		SDL_Rect rect = this->rect;

		// Generate the affine transform scaling values
		int scale_width = ( width << 16 ) / rect.w;
		int scale_height = ( height << 16 ) / rect.h;

		// Constants defined for clarity and optimisation
		int stride = width * 3;
		uint16_t *start = ( uint16_t * )this->sdl_screen->pixels + rect.y * scanlength + rect.x;

		int x, y, row_index;
		uint16_t *p;
		uint8_t *q, *row;

		// Iterate through the screen using a very basic scaling algorithm
		for ( y = 0; y < rect.h && this->last_position != -1; y ++ )
		{
			// Obtain the pointer to the current screen row
			p = start;

			// Calculate the row_index
			row_index = ( scale_height * y ) >> 16;

			// Calculate the pointer for the y offset (positioned on B instead of R)
			row = image + stride * row_index;

			// Iterate through the screen width
			for ( x = 0; x < rect.w; x ++ )
			{
				// Obtain the pixel pointer
				q = row + ( ( ( scale_width * x ) >> 16 ) * 3 );

				// Map the BGR colour from the frame to the SDL format
				*p ++ = SDL_MapRGB( this->sdl_screen->format, *q, *( q + 1 ), *( q + 2 ) );
			}

			// Move to the next row
			start += scanlength;
		}

		// Flip it into sight
		SDL_Flip( this->sdl_screen );
	}

	sdl_unlock_display();

	if ( unlock != NULL ) unlock( );

	return 0;
}

/** Threaded wrapper for pipe.
*/

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_sdl this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;

	// internal intialization
	mlt_frame frame = NULL;
	struct timespec tm = { 0, 1000 };

	if ( mlt_properties_get_int( mlt_consumer_properties( consumer ), "sdl_started" ) == 0 )
	{
		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) < 0 )
		{
			fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
			return NULL;
		}

		SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
		SDL_EnableUNICODE( 1 );
	}
	else
	{
		mlt_properties_set_int( mlt_consumer_properties( consumer ), "changed", 2 );
		if ( SDL_GetVideoSurface( ) != NULL )
			consumer_get_dimensions( &this->window_width, &this->window_height );
	}

	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_rt_frame( consumer );

		// Ensure that we have a frame
		if ( frame != NULL )
		{
			consumer_play_video( this, frame );
			mlt_frame_close( frame );
			nanosleep( &tm, NULL );
		}
	}

	if ( mlt_properties_get_int( mlt_consumer_properties( consumer ), "sdl_started" ) == 0 )
		SDL_Quit( );

	this->sdl_screen = NULL;

	return NULL;
}

static int consumer_get_dimensions( int *width, int *height )
{
	int changed = 0;

	// SDL windows manager structure
	SDL_SysWMinfo wm;

	// Specify the SDL Version
	SDL_VERSION( &wm.version );

	// Get the wm structure
	if ( SDL_GetWMInfo( &wm ) == 1 )
	{
		// Check that we have the X11 wm
		if ( wm.subsystem == SDL_SYSWM_X11 ) 
		{
			// Get the SDL window
			Window window = wm.info.x11.window;

			// Get the display session
			Display *display = wm.info.x11.display;

			// Get the window attributes
			XWindowAttributes attr;
			XGetWindowAttributes( display, window, &attr );

			// Determine whether window has changed
			changed = *width != attr.width || *height != attr.height;

			// Return width and height
			*width = attr.width;
			*height = attr.height;
		}
	}

	return changed;
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

	// Finally clean up this
	free( this );
}
