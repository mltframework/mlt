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
#include <framework/mlt_deque.h>
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
	mlt_deque queue;
	pthread_t thread;
	int running;
	uint8_t audio_buffer[ 4096 * 19 ];
	int audio_avail;
	pthread_mutex_t audio_mutex;
	pthread_cond_t audio_cond;
	int window_width;
	int window_height;
	float aspect_ratio;
	float display_aspect;
	int width;
	int height;
	int playing;
	int sdl_flags;
	SDL_Surface *sdl_screen;
	SDL_Overlay *sdl_overlay;
	uint8_t *buffer;
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
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
		// Create the queue
		this->queue = mlt_deque_init( );

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
		pthread_mutex_init( &this->audio_mutex, NULL );
		pthread_cond_init( &this->audio_cond, NULL);
		
		// Default scaler (for now we'll use nearest)
		mlt_properties_set( this->properties, "rescale", "nearest" );

		// Default buffer for low latency
		mlt_properties_set_int( this->properties, "buffer", 1 );

		// Default progressive true
		mlt_properties_set_int( this->properties, "progressive", 0 );

		// Get sample aspect ratio
		this->aspect_ratio = mlt_properties_get_double( this->properties, "aspect_ratio" );
		
		// Default display aspect ratio
		this->display_aspect = 4.0 / 3.0;
		
		// process actual param
		if ( arg == NULL || sscanf( arg, "%dx%d", &this->width, &this->height ) != 2 )
		{
			this->width = mlt_properties_get_int( this->properties, "width" );
			this->height = mlt_properties_get_int( this->properties, "height" );
			
			// Default window size
			this->window_width = ( float )this->height * this->display_aspect + 0.5;
			this->window_height = this->height;
		}
		else
		{
			if ( (int)( ( float )this->width / this->height * 1000 ) != 
				 (int)( this->display_aspect * 1000 ) ) 
			{
				// Override these
				this->display_aspect = ( float )this->width / this->height;
				this->aspect_ratio = 1.0;
				mlt_properties_set_double( this->properties, "aspect_ratio", this->aspect_ratio );
			}
			
			// Set window size
			this->window_width = this->width;
			this->window_height = this->height;
		}

		// Set the sdl flags
		this->sdl_flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE;

		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

int consumer_start( mlt_consumer parent )
{
	consumer_sdl this = parent->child;

	if ( !this->running )
	{
		pthread_attr_t thread_attributes;
		
		this->running = 1;

		// Allow the user to force resizing to window size
		if ( mlt_properties_get_int( this->properties, "resize" ) )
		{
			mlt_properties_set_int( this->properties, "width", this->width );
			mlt_properties_set_int( this->properties, "height", this->height );
		}

		// Inherit the scheduling priority
		pthread_attr_init( &thread_attributes );
		pthread_attr_setinheritsched( &thread_attributes, PTHREAD_INHERIT_SCHED );
	
		pthread_create( &this->thread, &thread_attributes, consumer_thread, this );
	}

	return 0;
}

int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl this = parent->child;

	if ( this->running )
	{
		// Kill the thread and clean up
		this->running = 0;

		pthread_mutex_lock( &this->audio_mutex );
		pthread_cond_broadcast( &this->audio_cond );
		pthread_mutex_unlock( &this->audio_mutex );

		pthread_join( this->thread, NULL );
	}

	return 0;
}

int consumer_is_stopped( mlt_consumer parent )
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

static void sdl_fill_audio( void *udata, uint8_t *stream, int len )
{
	consumer_sdl this = udata;

	// Get the volume
	float volume = mlt_properties_get_double( this->properties, "volume" );

	pthread_mutex_lock( &this->audio_mutex );

	// Block until audio received
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

	// We're definitely playing now
	this->playing = 1;

	pthread_cond_broadcast( &this->audio_cond );
	pthread_mutex_unlock( &this->audio_mutex );
}

static int consumer_play_audio( consumer_sdl this, mlt_frame frame, int init_audio, int *duration )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;
	mlt_audio_format afmt = mlt_audio_pcm;

	// Set the preferred params of the test card signal
	int channels = mlt_properties_get_int( properties, "channels" );
	int frequency = mlt_properties_get_int( properties, "frequency" );
	static int counter = 0;

	int samples = mlt_sample_calculator( mlt_properties_get_double( this->properties, "fps" ), frequency, counter++ );
	
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, &pcm, &afmt, &frequency, &channels, &samples );
	*duration = ( ( samples * 1000 ) / frequency );

	if ( mlt_properties_get_int( properties, "audio_off" ) )
	{
		this->playing = 1;
		init_audio = 1;
		return init_audio;
	}

	if ( init_audio == 1 )
	{
		SDL_AudioSpec request;
		SDL_AudioSpec got;

		// specify audio format
		memset( &request, 0, sizeof( SDL_AudioSpec ) );
		this->playing = 0;
		request.freq = frequency;
		request.format = AUDIO_S16;
		request.channels = channels;
		request.samples = 1024;
		request.callback = sdl_fill_audio;
		request.userdata = (void *)this;
		if ( SDL_OpenAudio( &request, &got ) != 0 )
		{
			fprintf( stderr, "SDL failed to open audio: %s\n", SDL_GetError() );
			init_audio = 2;
		}
		else if ( got.size != 0 )
		{
			SDL_PauseAudio( 0 );
			init_audio = 0;
		}
	}

	if ( init_audio == 0 )
	{
		mlt_properties properties = mlt_frame_properties( frame );
		bytes = ( samples * channels * 2 );
		pthread_mutex_lock( &this->audio_mutex );
		while ( bytes > ( sizeof( this->audio_buffer) - this->audio_avail ) )
			pthread_cond_wait( &this->audio_cond, &this->audio_mutex );
		if ( mlt_properties_get_double( properties, "_speed" ) == 1 )
			memcpy( &this->audio_buffer[ this->audio_avail ], pcm, bytes );
		else
			memset( &this->audio_buffer[ this->audio_avail ], 0, bytes );
		this->audio_avail += bytes;
		pthread_cond_broadcast( &this->audio_cond );
		pthread_mutex_unlock( &this->audio_mutex );
	}
	else
	{
		this->playing = 1;
	}

	return init_audio;
}

static int consumer_play_video( consumer_sdl this, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;

	mlt_image_format vfmt = mlt_image_yuv422;
	int width = this->width, height = this->height;
	uint8_t *image;
	int changed = 0;

	if ( mlt_properties_get_int( properties, "video_off" ) == 0 )
	{
		// Get the image, width and height
		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		
		// Handle events
		if ( this->sdl_screen != NULL )
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
	
		if ( width != this->width || height != this->height )
		{
			this->width = width;
			this->height = height;
			changed = 1;
		}

		if ( this->sdl_screen == NULL || changed )
		{
			SDL_Rect rect;
			
			// Determine frame's display aspect ratio
			float frame_aspect = mlt_frame_get_aspect_ratio( frame ) * this->width / this->height;
			
			// Determine window's new display aspect ratio
			float this_aspect = ( float )this->window_width / this->window_height;

			// If using hardware scaler
			if ( mlt_properties_get( properties, "rescale" ) != NULL &&
				!strcmp( mlt_properties_get( properties, "rescale" ), "none" ) )
			{
				// Special case optimisation to negate odd effect of sample aspect ratio
				// not corresponding exactly with image resolution.
				if ( ( (int)( this_aspect * 1000 ) == (int)( this->display_aspect * 1000 ) ) && 
					 ( (int)( mlt_frame_get_aspect_ratio( frame ) * 1000 ) == (int)( this->aspect_ratio * 1000 ) ) )
				{
					rect.w = this->window_width;
					rect.h = this->window_height;
				}
				else
				{
					// Use hardware scaler to normalise display aspect ratio
					rect.w = frame_aspect / this_aspect * this->window_width + 0.5;
					rect.h = this->window_height;
					if ( rect.w > this->window_width )
					{
						rect.w = this->window_width;
						rect.h = this_aspect / frame_aspect * this->window_height + 0.5;
					}
				}
			}
			// Special case optimisation to negate odd effect of sample aspect ratio
			// not corresponding exactly with image resolution.
			else if ( (int)( this_aspect * 1000 ) == (int)( this->display_aspect * 1000 ) ) 
			{
				rect.w = this->window_width;
				rect.h = this->window_height;
			}
			// Use hardware scaler to normalise sample aspect ratio
			else if ( this->window_height * frame_aspect > this->window_width )
			{
				rect.w = this->window_width;
				rect.h = this->window_width / frame_aspect + 0.5;
			}
			else
			{
				rect.w = this->window_height * frame_aspect + 0.5;
				rect.h = this->window_height;
			}
			
			rect.x = ( this->window_width - rect.w ) / 2;
			rect.y = ( this->window_height - rect.h ) / 2;
			
			// Force an overlay recreation
			if ( this->sdl_overlay != NULL )
				SDL_FreeYUVOverlay( this->sdl_overlay );

			// open SDL window with video overlay, if possible
			this->sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, 0, this->sdl_flags );

			if ( this->sdl_screen != NULL )
			{
				SDL_SetClipRect( this->sdl_screen, &rect );
				sdl_lock_display();
				this->sdl_overlay = SDL_CreateYUVOverlay( this->width, this->height, SDL_YUY2_OVERLAY, this->sdl_screen );
				sdl_unlock_display();
			}
		}
			
		if ( this->sdl_screen != NULL && this->sdl_overlay != NULL )
		{
			this->buffer = this->sdl_overlay->pixels[ 0 ];
			if ( SDL_LockYUVOverlay( this->sdl_overlay ) >= 0 )
			{
				if ( image != NULL )
					memcpy( this->buffer, image, width * height * 2 );
				SDL_UnlockYUVOverlay( this->sdl_overlay );
				SDL_DisplayYUVOverlay( this->sdl_overlay, &this->sdl_screen->clip_rect );
			}
		}
	}

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
	int init_audio = 1;

	// Obtain time of thread start
	struct timeval now;
	int64_t start = 0;
	int64_t elapsed = 0;
	int duration = 0;
	int64_t playtime = 0;
	struct timespec tm;
	mlt_frame next = NULL;
	mlt_frame frame = NULL;
	mlt_properties properties = NULL;

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE ) < 0 )
	{
		fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
		return NULL;
	}

	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
	SDL_EnableUNICODE( 1 );

	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_rt_frame( consumer );

		// Ensure that we have a frame
		if ( frame != NULL )
		{
			// Get the frame properties
			properties =  mlt_frame_properties( frame );

			// Play audio
			init_audio = consumer_play_audio( this, frame, init_audio, &duration );

			// Determine the start time now
			if ( this->playing && start == 0 )
			{
				// Get the current time
				gettimeofday( &now, NULL );

				// Determine start time
				start = ( int64_t )now.tv_sec * 1000000 + now.tv_usec;
			}

			// Set playtime for this frame
			mlt_properties_set_position( properties, "playtime", playtime );

			// Push this frame to the back of the queue
			mlt_deque_push_back( this->queue, frame );

			// Calculate the next playtime
			playtime += ( duration * 1000 );
		}

		// Pop the next frame
		next = mlt_deque_pop_front( this->queue );

		while ( next != NULL && this->playing )
		{
			// Get the properties
			properties =  mlt_frame_properties( next );

			// Get the current time
			gettimeofday( &now, NULL );

			// Get the elapsed time
			elapsed = ( ( int64_t )now.tv_sec * 1000000 + now.tv_usec ) - start;

			// See if we have to delay the display of the current frame
			if ( mlt_properties_get_int( properties, "rendered" ) == 1 )
			{
				// Obtain the scheduled playout time
				mlt_position scheduled = mlt_properties_get_position( properties, "playtime" );

				// Determine the difference between the elapsed time and the scheduled playout time
				mlt_position difference = scheduled - elapsed;

				// If the frame is quite some way in the future, go get another
				if ( difference > 80000 && mlt_deque_count( this->queue ) < 6 )
					break;

				// Smooth playback a bit
				if ( difference > 20000 && mlt_properties_get_double( properties, "_speed" ) == 1.0 )
				{
					tm.tv_sec = difference / 1000000;
					tm.tv_nsec = ( difference % 1000000 ) * 1000;
					nanosleep( &tm, NULL );
				}

				// Show current frame if not too old
				if ( difference > -10000 || mlt_properties_get_double( properties, "_speed" ) != 1.0 )
					consumer_play_video( this, next );
				else
					start = start - difference;
			}

			// This is an unrendered frame - just close it
			mlt_frame_close( next );

			// Pop the next frame
			next = mlt_deque_pop_front( this->queue );
		}

		if ( next != NULL )
			mlt_deque_push_front( this->queue, next );
	}

	// internal cleanup
	if ( init_audio == 0 )
		SDL_AudioQuit( );
	if ( this->sdl_overlay != NULL )
		SDL_FreeYUVOverlay( this->sdl_overlay );
	SDL_Quit( );

	while( mlt_deque_count( this->queue ) )
		mlt_frame_close( mlt_deque_pop_back( this->queue ) );

	this->sdl_screen = NULL;
	this->sdl_overlay = NULL;
	this->audio_avail = 0;

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

	// Close the queue
	mlt_deque_close( this->queue );

	// Destroy mutexes
	pthread_mutex_destroy( &this->audio_mutex );
	pthread_cond_destroy( &this->audio_cond );
		
	// Now clean up the rest
	mlt_consumer_close( parent );

	// Finally clean up this
	free( this );
}
