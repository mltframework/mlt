/*
 * consumer_sdl.c -- A Simple DirectMedia Layer consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

/** This classes definition.
*/

typedef struct consumer_sdl_s *consumer_sdl;

struct consumer_sdl_s
{
	struct mlt_consumer_s parent;
	mlt_properties properties;
	int format;
	int video;
	pthread_t thread;
	int running;
	uint8_t audio_buffer[ 4096 * 6 ];
	int audio_avail;
	pthread_mutex_t audio_mutex;
	pthread_cond_t audio_cond;
	int window_width;
	int window_height;
	int width;
	int height;
};

/** Forward references to static functions.
*/

static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static int consumer_get_dimensions( int *width, int *height );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_sdl_init( char *arg )
{
	// Create the consumer object
	consumer_sdl this = calloc( sizeof( struct consumer_sdl_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( &this->parent, this ) == 0 )
	{
		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// get a handle on properties
		mlt_service service = mlt_consumer_service( parent );
		this->properties = mlt_service_properties( service );

		// Set the default volume
		mlt_properties_set_double( this->properties, "volume", 1.0 );

		// This is the initialisation of the consumer
		this->running = 1;
		pthread_mutex_init( &this->audio_mutex, NULL );
		pthread_cond_init( &this->audio_cond, NULL);
		
		// process actual param
		if ( arg == NULL || !strcmp( arg, "PAL" ) )
		{
			this->width = 720;
			this->height = 576;
		}
		else if ( !strcmp( arg, "NTSC" ) )
		{
			this->width = 720;
			this->height = 480;
		}
		else if ( sscanf( arg, "%dx%d", &this->width, &this->height ) != 2 )
		{
			this->width = 720;
			this->height = 576;
		}


		// Create the the thread
		pthread_create( &this->thread, NULL, consumer_thread, this );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
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

void sdl_fill_audio( void *udata, uint8_t *stream, int len )
{
	consumer_sdl this = udata;

	// Get the volume
	float volume = mlt_properties_get_double( this->properties, "volume" );

	pthread_mutex_lock( &this->audio_mutex );

	// Experimental - block until audio received
	while ( this->running && len > this->audio_avail )
		pthread_cond_wait( &this->audio_cond, &this->audio_mutex );

	if ( this->audio_avail >= len )
	{
		// Place in the audio buffer
		SDL_MixAudio( stream, this->audio_buffer, len, ( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );

		// Remove len from the audio available
		this->audio_avail -= len;

		// Remove the samples
		memmove( this->audio_buffer, this->audio_buffer + len, this->audio_avail );
	}
	else
	{
		// Just to be safe, wipe the stream first
		memset( stream, 0, len );

		// Copy what we have into the stream
		memcpy( stream, this->audio_buffer, this->audio_avail );

		// Mix the audio 
		SDL_MixAudio( stream, stream, len, ( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );

		// No audio left
		this->audio_avail = 0;
	}
	pthread_cond_broadcast( &this->audio_cond );
	pthread_mutex_unlock( &this->audio_mutex );
}

/** Threaded wrapper for pipe.
*/

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_sdl this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;

	// Get the service assoicated to the consumer
	mlt_service service = mlt_consumer_service( consumer );

	// Define a frame pointer
	mlt_frame frame;

	// internal intialization
	int sdl_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWACCEL | SDL_RESIZABLE;
	SDL_Surface *sdl_screen = NULL;
	SDL_Overlay *sdl_overlay = NULL;
	uint8_t *buffer = NULL;
	int init_audio = 1;
	int bytes;

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE ) < 0 )
	{
		fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
		return NULL;
	}
	
	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the service (should never return anything other than 0)
		if ( mlt_service_get_frame( service, &frame, 0 ) == 0 )
		{
			mlt_image_format vfmt = mlt_image_yuv422;
			int width = this->width, height = this->height;
			uint8_t *image;

			mlt_audio_format afmt = mlt_audio_pcm;
			int channels;
			int samples;
			int frequency;
			int16_t *pcm;
			int changed = 0;

			mlt_frame_get_audio( frame, &pcm, &afmt, &frequency, &channels, &samples );
			if ( init_audio == 1 )
			{
				SDL_AudioSpec request;

				// specify audio format
				request.freq = frequency;
				request.format = AUDIO_S16;
				request.channels = channels;
				request.samples = 1024;
				request.callback = sdl_fill_audio;
				request.userdata = (void *)this;
				if ( SDL_OpenAudio( &request, NULL ) < 0 )
				{
					fprintf( stderr, "SDL failed to open audio: %s\n", SDL_GetError() );
					break;
				}
				SDL_PauseAudio( 0 );
				init_audio = 0;
			}
			bytes = ( samples * channels * 2 );
			pthread_mutex_lock( &this->audio_mutex );
			while ( bytes > ( sizeof( this->audio_buffer) - this->audio_avail ) )
				pthread_cond_wait( &this->audio_cond, &this->audio_mutex );
			memcpy( &this->audio_buffer[ this->audio_avail ], pcm, bytes );
			this->audio_avail += bytes;
			pthread_cond_broadcast( &this->audio_cond );
			pthread_mutex_unlock( &this->audio_mutex );

			// Get the image, width and height
			mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );

			if ( sdl_screen != NULL )
			{
				SDL_Event event;

				changed = consumer_get_dimensions( &this->window_width, &this->window_height );

				while ( SDL_PollEvent( &event ) )
				{
					switch( event.type )
					{
						case SDL_VIDEORESIZE:
							this->window_width = event.resize.w;
							this->window_height = event.resize.h;
							changed = 1;
							break;
					}
				}
			}

			if ( sdl_screen == NULL || changed )
			{
				double aspect_ratio = mlt_frame_get_aspect_ratio( frame );

				if ( this->window_width == 0 || this->window_height == 0 )
				{
					this->window_width = width;
					this->window_height = height;
				}

				// open SDL window with video overlay, if possible
				sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, 0, sdl_flags );

				if ( sdl_screen != NULL )
				{
					SDL_Rect rect;
					if ( this->window_width < this->window_height * aspect_ratio )
					{
						rect.w = this->window_width;
						rect.h = this->window_width / aspect_ratio;
					}
					else
					{
						rect.w = this->window_height * aspect_ratio;
						rect.h = this->window_height;
					}

					rect.x = ( this->window_width - rect.w ) / 2;
					rect.y = ( this->window_height - rect.h ) / 2;

					SDL_SetClipRect( sdl_screen, &rect );
					
					// Force an overlay recreation
					if ( sdl_overlay != NULL )
						SDL_FreeYUVOverlay( sdl_overlay );
					sdl_lock_display();
					sdl_overlay = SDL_CreateYUVOverlay( this->width, this->height, SDL_YUY2_OVERLAY, sdl_screen );
					sdl_unlock_display();
				}
			}
			
			if ( sdl_screen != NULL && sdl_overlay != NULL )
			{
				buffer = sdl_overlay->pixels[ 0 ];
				if ( sdl_lock_display() )
				{
					if ( SDL_LockYUVOverlay( sdl_overlay ) >= 0 )
					{
						mlt_resize_yuv422( buffer, this->width, this->height, image, width, height );
						SDL_UnlockYUVOverlay( sdl_overlay );
						SDL_DisplayYUVOverlay( sdl_overlay, &sdl_screen->clip_rect );
					}
					sdl_unlock_display();
				}
			}
			else
			{
				// TODO: allocate buffer?
			}

			// Close the frame
			mlt_frame_close( frame );
		}
		else
		{
			break;
		}
	} // while

	// internal cleanup
	if ( init_audio == 0 )
		SDL_AudioQuit( );
	if ( sdl_overlay != NULL )
		SDL_FreeYUVOverlay( sdl_overlay );
	SDL_Quit( );

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

	// Kill the thread and clean up
	this->running = 0;

	pthread_mutex_lock( &this->audio_mutex );
	pthread_cond_broadcast( &this->audio_cond );
	pthread_mutex_unlock( &this->audio_mutex );

	pthread_join( this->thread, NULL );
	pthread_mutex_destroy( &this->audio_mutex );
	pthread_cond_destroy( &this->audio_cond );
		
	// Now clean up the rest (the close = NULL is a bit nasty but needed for now)
	parent->close = NULL;
	mlt_consumer_close( parent );

	// Finally clean up this
	free( this );
}

