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
#include <sys/timeb.h>

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
	int width;
	int height;
	int playing;
	int sdl_flags;
	SDL_Surface *sdl_screen;
	SDL_Overlay *sdl_overlay;
	uint8_t *buffer;
	int time_taken;
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

		// Default progressive true
		mlt_properties_set_int( this->properties, "progressive", 1 );

		// Get aspect ratio
		this->aspect_ratio = mlt_properties_get_double( this->properties, "aspect_ratio" );
		
		// process actual param
		if ( arg == NULL || sscanf( arg, "%dx%d", &this->width, &this->height ) != 2 )
		{
			this->width = mlt_properties_get_int( this->properties, "width" );
			this->height = mlt_properties_get_int( this->properties, "height" );
		}

		// Default window size
		this->window_width = (int)( (float)this->height * this->aspect_ratio ) + 1;
		this->window_height = this->height;
		
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
		this->running = 1;
		pthread_create( &this->thread, NULL, consumer_thread, this );
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
	int channels = 2;
	int frequency = 48000;
	static int counter = 0;
	if ( mlt_properties_get_int( properties, "frequency" ) != 0 )
		frequency =  mlt_properties_get_int( properties, "frequency" );

	int samples = mlt_sample_calculator( mlt_properties_get_double( this->properties, "fps" ), frequency, counter++ );
	
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, &pcm, &afmt, &frequency, &channels, &samples );
	*duration = ( ( samples * 1000 ) / frequency );

	if ( mlt_properties_get_int( properties, "audio_off" ) )
	{
		this->playing = 1;
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
		request.samples = 2048;
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
		bytes = ( samples * channels * 2 );
		pthread_mutex_lock( &this->audio_mutex );
		while ( bytes > ( sizeof( this->audio_buffer) - this->audio_avail ) )
			pthread_cond_wait( &this->audio_cond, &this->audio_mutex );
		mlt_properties properties = mlt_frame_properties( frame );
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

static int consumer_play_video( consumer_sdl this, mlt_frame frame, int64_t elapsed, int64_t playtime )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;

	mlt_image_format vfmt = mlt_image_yuv422;
	int width = this->width, height = this->height;
	uint8_t *image;
	int changed = 0;

	struct timeb before;
	ftime( &before );

	if ( mlt_properties_get_int( properties, "video_off" ) )
	{
		mlt_frame_close( frame );
		return 0;
	}

	// Set skip
	mlt_properties_set_position( mlt_frame_properties( frame ), "playtime", playtime );
	mlt_properties_set_double( mlt_frame_properties( frame ), "consumer_scale", ( double )height / mlt_properties_get_double( properties, "height" ) );

	if ( !this->playing )
	{
		struct timeb render;
		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		ftime( &render );
		this->time_taken = ( ( int64_t )render.time * 1000 + render.millitm ) - ( ( int64_t )before.time * 1000 + before.millitm );
		mlt_properties_set( mlt_frame_properties( frame ), "rendered", "true" );
	}

	// Push this frame to the back of the queue
	mlt_deque_push_back( this->queue, frame );
	frame = NULL;

	if ( this->playing )
	{
		// We might want to use an old frame if the current frame is skipped
		mlt_frame candidate = NULL;

		while ( frame == NULL && mlt_deque_count( this->queue ) )
		{
			frame = mlt_deque_peek_front( this->queue );
			playtime = mlt_properties_get_position( mlt_frame_properties( frame ), "playtime" );

			// Check if frame is in the future or past
			if ( mlt_deque_count( this->queue ) > 25 )
			{
				frame = mlt_deque_pop_front( this->queue );
			}
			else if ( playtime > elapsed + 100 )
			{
				// no frame to show or remove
				frame = NULL;
				break;
			}
			else if ( playtime < elapsed - 20 )
			{
				mlt_frame_close( candidate );
				candidate = mlt_deque_pop_front( this->queue );
				if ( mlt_properties_get( mlt_frame_properties( frame ), "rendered" ) == NULL )
				{
					mlt_frame_close( candidate );
					candidate = NULL;
				}
				frame = NULL;
			}
			else
			{
				// Get the frame at the front of the queue
				frame = mlt_deque_pop_front( this->queue );
			}
		}

		if ( frame == NULL )
			frame = candidate;
		else if ( candidate != NULL )
			mlt_frame_close( candidate );
	}

	struct timeb render;
	ftime( &render );

	if ( this->playing && frame != NULL )
	{

		// Get the image, width and height
		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		ftime( &render );
		if ( mlt_properties_get( mlt_frame_properties( frame ), "rendered" ) == NULL )
			this->time_taken = ( ( int64_t )render.time * 1000 + render.millitm ) - ( ( int64_t )before.time * 1000 + before.millitm );

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
			double aspect_ratio = mlt_frame_get_aspect_ratio( frame );
			float display_aspect_ratio = (float)width / (float)height;
			SDL_Rect rect;

			if ( mlt_properties_get_double( properties, "aspect_ratio" ) )
				aspect_ratio = mlt_properties_get_double( properties, "aspect_ratio" );

			if ( aspect_ratio == 1 )
			{
				rect.w = this->window_width;
				rect.h = this->window_height;
			}
			else if ( this->window_width < this->window_height * aspect_ratio )
			{
				rect.w = this->window_width;
				rect.h = this->window_width / aspect_ratio;
			}
			else
			{
				rect.w = this->window_height * aspect_ratio;
				rect.h = this->window_height;
			}

			if ( mlt_properties_get_int( properties, "scale_overlay" ) )
			{
				if ( ( float )rect.w * display_aspect_ratio < this->window_width )
					rect.w = ( int )( ( float )rect.w * display_aspect_ratio );
				else if ( ( float )rect.h * display_aspect_ratio < this->window_height )
					rect.h = ( int )( ( float )rect.h * display_aspect_ratio );
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
				this->sdl_overlay = SDL_CreateYUVOverlay( this->width - (this->width % 4), this->height- (this->height % 2 ), SDL_YUY2_OVERLAY, this->sdl_screen );
				sdl_unlock_display();
			}
		}
			
		if ( this->sdl_screen != NULL && this->sdl_overlay != NULL )
		{
			this->buffer = this->sdl_overlay->pixels[ 0 ];
			if ( SDL_LockYUVOverlay( this->sdl_overlay ) >= 0 )
			{
				memcpy( this->buffer, image, width * height * 2 );
				SDL_UnlockYUVOverlay( this->sdl_overlay );
				SDL_DisplayYUVOverlay( this->sdl_overlay, &this->sdl_screen->clip_rect );
			}
		}
	}

	// Close the frame
	if ( frame != NULL )
		mlt_frame_close( frame );

	struct timeb after;
	ftime( &after );
	int processing = ( ( int64_t )after.time * 1000 + after.millitm ) - ( ( int64_t )render.time * 1000 + render.millitm );
	int delay = playtime - elapsed - processing;

	if ( delay > this->time_taken && mlt_deque_count( this->queue ) )
	{
		mlt_frame next = mlt_deque_peek_front( this->queue );
		playtime = mlt_properties_get_position( mlt_frame_properties( next ), "playtime" );
		if ( playtime > elapsed + delay )
		{
			struct timeb render;
			mlt_frame_get_image( next, &image, &vfmt, &width, &height, 0 );
			ftime( &render );
			this->time_taken = ( ( int64_t )render.time * 1000 + render.millitm ) - ( ( int64_t )after.time * 1000 + after.millitm );
			mlt_properties_set( mlt_frame_properties( next ), "rendered", "true" );
		}
		else
		{
			this->time_taken /= 2;
		}
	}
	else
	{
		this->time_taken /= 2;
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
	struct timeb now;
	int64_t start = 0;
	int64_t elapsed = 0;
	int duration = 0;
	int64_t playtime = 0;

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
		mlt_frame frame = mlt_consumer_get_frame( consumer );

		// Ensure that we have a frame
		if ( frame != NULL )
		{
			// Play audio
			init_audio = consumer_play_audio( this, frame, init_audio, &duration );

			if ( this->playing )
			{
				// Get the current time
				ftime( &now );

				// Determine elapsed time
				if ( start == 0 )
					start = ( int64_t )now.time * 1000 + now.millitm;
				else
					elapsed = ( ( int64_t )now.time * 1000 + now.millitm ) - start;
			}

			consumer_play_video( this, frame, elapsed, playtime );

			playtime += duration;
		}
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

	// Destroy mutexes
	pthread_mutex_destroy( &this->audio_mutex );
	pthread_cond_destroy( &this->audio_cond );
		
	// Now clean up the rest
	mlt_consumer_close( parent );

	// Finally clean up this
	free( this );
}

