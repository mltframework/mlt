/*
 * consumer_sdl_still.c -- A Simple DirectMedia Layer consumer
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <framework/mlt_consumer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <sys/time.h>
#include "consumer_sdl_osx.h"

extern pthread_mutex_t mlt_sdl_mutex;

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
	int width;
	int height;
	int playing;
	int sdl_flags;
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

mlt_consumer consumer_sdl_still_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	consumer_sdl this = calloc( 1, sizeof( struct consumer_sdl_s ) );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( &this->parent, this, profile ) == 0 )
	{
		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// get a handle on properties
		mlt_service service = MLT_CONSUMER_SERVICE( parent );
		this->properties = MLT_SERVICE_PROPERTIES( service );

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// Default scaler (for now we'll use nearest)
		mlt_properties_set( this->properties, "rescale", "nearest" );

		// We're always going to run this in non-realtime mode
		mlt_properties_set( this->properties, "real_time", "0" );

		// Ensure we don't join on a non-running object
		this->joined = 1;
		
		// process actual param
		if ( arg == NULL || sscanf( arg, "%dx%d", &this->width, &this->height ) != 2 )
		{
			this->width = mlt_properties_get_int( this->properties, "width" );
			this->height = mlt_properties_get_int( this->properties, "height" );
		}
		else
		{
			mlt_properties_set_int( this->properties, "width", this->width );
			mlt_properties_set_int( this->properties, "height", this->height );
		}

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
		int preview_off = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "preview_off" );
		int sdl_started = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "sdl_started" );

		consumer_stop( parent );

		this->last_position = -1;
		this->running = 1;
		this->joined = 0;

		// Allow the user to force resizing to window size
		this->width = mlt_properties_get_int( this->properties, "width" );
		this->height = mlt_properties_get_int( this->properties, "height" );

		// Default window size
		double display_ratio = mlt_properties_get_double( this->properties, "display_ratio" );
		this->window_width = ( double )this->height * display_ratio + 0.5;
		this->window_height = this->height;

		if ( sdl_started == 0 && preview_off == 0 )
		{
			pthread_mutex_lock( &mlt_sdl_mutex );
			int ret = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE );
			pthread_mutex_unlock( &mlt_sdl_mutex );
			if ( ret < 0 )
			{
				fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
				return -1;
			}

			SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
			SDL_EnableUNICODE( 1 );
		}

		pthread_mutex_lock( &mlt_sdl_mutex );
		if ( !SDL_GetVideoSurface() && preview_off == 0 )
			SDL_SetVideoMode( this->window_width, this->window_height, 0, this->sdl_flags );
		pthread_mutex_unlock( &mlt_sdl_mutex );

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
		int preview_off = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "preview_off" );
		int sdl_started = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "sdl_started" );
	
		// Kill the thread and clean up
		this->running = 0;

		pthread_join( this->thread, NULL );
		this->joined = 1;

		if ( sdl_started == 0 && preview_off == 0 )
		{
			pthread_mutex_lock( &mlt_sdl_mutex );
			SDL_Quit( );
			pthread_mutex_unlock( &mlt_sdl_mutex );
		}
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
	pthread_mutex_lock( &mlt_sdl_mutex );
	SDL_Surface *screen = SDL_GetVideoSurface( );
	int result = screen != NULL && ( !SDL_MUSTLOCK( screen ) || SDL_LockSurface( screen ) >= 0 );
	pthread_mutex_unlock( &mlt_sdl_mutex );
	return result;
}

static void sdl_unlock_display( )
{
	pthread_mutex_lock( &mlt_sdl_mutex );
	SDL_Surface *screen = SDL_GetVideoSurface( );
	if ( screen != NULL && SDL_MUSTLOCK( screen ) )
		SDL_UnlockSurface( screen );
	pthread_mutex_unlock( &mlt_sdl_mutex );
}

static inline void display_1( SDL_Surface *screen, SDL_Rect rect, uint8_t *image, int width, int height )
{
	// Generate the affine transform scaling values
	if ( rect.w == 0 || rect.h == 0 ) return;
	int scale_width = ( width << 16 ) / rect.w;
	int scale_height = ( height << 16 ) / rect.h;
	int stride = width * 4;
	int x, y, row_index;
	uint8_t *q, *row;

	// Constants defined for clarity and optimisation
	int scanlength = screen->pitch;
	uint8_t *start = ( uint8_t * )screen->pixels + rect.y * scanlength + rect.x;
	uint8_t *p;

	// Iterate through the screen using a very basic scaling algorithm
	for ( y = 0; y < rect.h; y ++ )
	{
		p = start;
		row_index = ( 32768 + scale_height * y ) >> 16;
		row = image + stride * row_index;
		for ( x = 0; x < rect.w; x ++ )
		{
			q = row + ( ( ( 32768 + scale_width * x ) >> 16 ) * 4 );
			*p ++ = SDL_MapRGB( screen->format, *q, *( q + 1 ), *( q + 2 ) );
		}
		start += scanlength;
	}
}

static inline void display_2( SDL_Surface *screen, SDL_Rect rect, uint8_t *image, int width, int height )
{
	// Generate the affine transform scaling values
	if ( rect.w == 0 || rect.h == 0 ) return;
	int scale_width = ( width << 16 ) / rect.w;
	int scale_height = ( height << 16 ) / rect.h;
	int stride = width * 4;
	int x, y, row_index;
	uint8_t *q, *row;

	// Constants defined for clarity and optimisation
	int scanlength = screen->pitch / 2;
	uint16_t *start = ( uint16_t * )screen->pixels + rect.y * scanlength + rect.x;
	uint16_t *p;

	// Iterate through the screen using a very basic scaling algorithm
	for ( y = 0; y < rect.h; y ++ )
	{
		p = start;
		row_index = ( 32768 + scale_height * y ) >> 16;
		row = image + stride * row_index;
		for ( x = 0; x < rect.w; x ++ )
		{
			q = row + ( ( ( 32768 + scale_width * x ) >> 16 ) * 4 );
			*p ++ = SDL_MapRGB( screen->format, *q, *( q + 1 ), *( q + 2 ) );
		}
		start += scanlength;
	}
}

static inline void display_3( SDL_Surface *screen, SDL_Rect rect, uint8_t *image, int width, int height )
{
	// Generate the affine transform scaling values
	if ( rect.w == 0 || rect.h == 0 ) return;
	int scale_width = ( width << 16 ) / rect.w;
	int scale_height = ( height << 16 ) / rect.h;
	int stride = width * 4;
	int x, y, row_index;
	uint8_t *q, *row;

	// Constants defined for clarity and optimisation
	int scanlength = screen->pitch;
	uint8_t *start = ( uint8_t * )screen->pixels + rect.y * scanlength + rect.x;
	uint8_t *p;
	uint32_t pixel;

	// Iterate through the screen using a very basic scaling algorithm
	for ( y = 0; y < rect.h; y ++ )
	{
		p = start;
		row_index = ( 32768 + scale_height * y ) >> 16;
		row = image + stride * row_index;
		for ( x = 0; x < rect.w; x ++ )
		{
			q = row + ( ( ( 32768 + scale_width * x ) >> 16 ) * 4 );
			pixel = SDL_MapRGB( screen->format, *q, *( q + 1 ), *( q + 2 ) );
			*p ++ = (pixel & 0xFF0000) >> 16;
			*p ++ = (pixel & 0x00FF00) >> 8;
			*p ++ = (pixel & 0x0000FF);
		}
		start += scanlength;
	}
}

static inline void display_4( SDL_Surface *screen, SDL_Rect rect, uint8_t *image, int width, int height )
{
	// Generate the affine transform scaling values
	if ( rect.w == 0 || rect.h == 0 ) return;
	int scale_width = ( width << 16 ) / rect.w;
	int scale_height = ( height << 16 ) / rect.h;
	int stride = width * 4;
	int x, y, row_index;
	uint8_t *q, *row;

	// Constants defined for clarity and optimisation
	int scanlength = screen->pitch / 4;
	uint32_t *start = ( uint32_t * )screen->pixels + rect.y * scanlength + rect.x;
	uint32_t *p;

	// Iterate through the screen using a very basic scaling algorithm
	for ( y = 0; y < rect.h; y ++ )
	{
		p = start;
		row_index = ( 32768 + scale_height * y ) >> 16;
		row = image + stride * row_index;
		for ( x = 0; x < rect.w; x ++ )
		{
			q = row + ( ( ( 32768 + scale_width * x ) >> 16 ) * 4 );
			*p ++ = SDL_MapRGB( screen->format, *q, *( q + 1 ), *( q + 2 ) );
		}
		start += scanlength;
	}
}

static int consumer_play_video( consumer_sdl this, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;

	mlt_image_format vfmt = mlt_image_rgb24a;
	int height = this->height;
	int width = this->width;
	uint8_t *image = NULL;
	int changed = 0;
	double display_ratio = mlt_properties_get_double( this->properties, "display_ratio" );

	void ( *lock )( void ) = mlt_properties_get_data( properties, "app_lock", NULL );
	void ( *unlock )( void ) = mlt_properties_get_data( properties, "app_unlock", NULL );

	if ( lock != NULL ) lock( );
	void *pool = mlt_cocoa_autorelease_init();
	sdl_lock_display();
	
	// Handle events
	if ( SDL_GetVideoSurface() )
	{
		SDL_Event event;

		pthread_mutex_lock( &mlt_sdl_mutex );
		changed = consumer_get_dimensions( &this->window_width, &this->window_height );
		pthread_mutex_unlock( &mlt_sdl_mutex );

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

	if ( !SDL_GetVideoSurface() || changed )
	{
		// open SDL window
		pthread_mutex_lock( &mlt_sdl_mutex );
		SDL_Surface *screen = SDL_SetVideoMode( this->window_width, this->window_height, 0, this->sdl_flags );
		if ( consumer_get_dimensions( &this->window_width, &this->window_height ) )
			screen = SDL_SetVideoMode( this->window_width, this->window_height, 0, this->sdl_flags );

		if ( screen )
		{
			uint32_t color = mlt_properties_get_int( this->properties, "window_background" );
			SDL_FillRect( screen, NULL, color >> 8 );
			changed = 1;
		}
		pthread_mutex_unlock( &mlt_sdl_mutex );
	}
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "test_audio", 1 );
	if ( changed == 0 &&
		 this->last_position == mlt_frame_get_position( frame ) &&
		 this->last_producer == mlt_frame_get_original_producer( frame ) &&
		 !mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "refresh" ) )
	{
		sdl_unlock_display( );
		mlt_cocoa_autorelease_close( pool );
		if ( unlock != NULL ) unlock( );
		struct timespec tm = { 0, 100000 };
		nanosleep( &tm, NULL );
		return 0;
	}

	// Update last frame shown info
	this->last_position = mlt_frame_get_position( frame );
	this->last_producer = mlt_frame_get_original_producer( frame );

	// Get the image, width and height
	mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );

	if ( image != NULL )
	{
		char *rescale = mlt_properties_get( properties, "rescale" );
		if ( rescale != NULL && strcmp( rescale, "none" ) )
		{
			double this_aspect = display_ratio / ( ( double )this->window_width / ( double )this->window_height );
			this->rect.w = this_aspect * this->window_width;
			this->rect.h = this->window_height;
			if ( this->rect.w > this->window_width )
			{
				this->rect.w = this->window_width;
				this->rect.h = ( 1.0 / this_aspect ) * this->window_height;
			}
		}
		else
		{
			double frame_aspect = mlt_frame_get_aspect_ratio( frame ) * width / height;
			this->rect.w = frame_aspect * this->window_height;
			this->rect.h = this->window_height;
			if ( this->rect.w > this->window_width )
			{
				this->rect.w = this->window_width;
				this->rect.h = ( 1.0 / frame_aspect ) * this->window_width;
			}
		}

		this->rect.x = ( this->window_width - this->rect.w ) / 2;
		this->rect.y = ( this->window_height - this->rect.h ) / 2;

		mlt_properties_set_int( this->properties, "rect_x", this->rect.x );
		mlt_properties_set_int( this->properties, "rect_y", this->rect.y );
		mlt_properties_set_int( this->properties, "rect_w", this->rect.w );
		mlt_properties_set_int( this->properties, "rect_h", this->rect.h );
	}
	
	pthread_mutex_lock( &mlt_sdl_mutex );
	SDL_Surface *screen = SDL_GetVideoSurface( );
	if ( !mlt_consumer_is_stopped( &this->parent ) && screen && screen->pixels )
	{
		switch( screen->format->BytesPerPixel )
		{
			case 1:
				display_1( screen, this->rect, image, width, height );
				break;
			case 2:
				display_2( screen, this->rect, image, width, height );
				break;
			case 3:
				display_3( screen, this->rect, image, width, height );
				break;
			case 4:
				display_4( screen, this->rect, image, width, height );
				break;
			default:
				fprintf( stderr, "Unsupported video depth %d\n", screen->format->BytesPerPixel );
				break;
		}

		// Flip it into sight
		SDL_Flip( screen );
	}
	pthread_mutex_unlock( &mlt_sdl_mutex );

	sdl_unlock_display();
	mlt_cocoa_autorelease_close( pool );
	if ( unlock != NULL ) unlock( );
	mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

	return 1;
}

/** Threaded wrapper for pipe.
*/

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_sdl this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	mlt_frame frame = NULL;

	// Allow the hosting app to provide the preview
	int preview_off = mlt_properties_get_int( properties, "preview_off" );

	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_rt_frame( consumer );

		// Ensure that we have a frame
		if ( this->running && frame != NULL )
		{
			if ( preview_off == 0 )
			{
				consumer_play_video( this, frame );
			}
			else
			{
				mlt_image_format vfmt = mlt_image_rgb24a;
				int height = this->height;
				int width = this->width;
				uint8_t *image = NULL;
				mlt_image_format preview_format = mlt_properties_get_int( properties, "preview_format" );

				// Check if a specific colour space has been requested
				if ( preview_off && preview_format != mlt_image_none )
					vfmt = preview_format;
			
				mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "format", vfmt );
				mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			}
			mlt_frame_close( frame );
		}
		else
		{
			if ( frame ) mlt_frame_close( frame );
			this->running = 0;
		}
	}

	return NULL;
}

static int consumer_get_dimensions( int *width, int *height )
{
	int changed = 0;

	// SDL windows manager structure
	SDL_SysWMinfo wm;

	// Specify the SDL Version
	SDL_VERSION( &wm.version );

#ifndef __APPLE__
	// Get the wm structure
	if ( SDL_GetWMInfo( &wm ) == 1 )
	{
#ifndef _WIN32
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
#endif
	}
#endif

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
