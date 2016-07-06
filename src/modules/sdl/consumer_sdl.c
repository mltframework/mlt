/*
 * consumer_sdl.c -- A Simple DirectMedia Layer consumer
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
	mlt_deque queue;
	pthread_t thread;
	int joined;
	int running;
	uint8_t audio_buffer[ 4096 * 10 ];
	int audio_avail;
	pthread_mutex_t audio_mutex;
	pthread_cond_t audio_cond;
	pthread_mutex_t video_mutex;
	pthread_cond_t video_cond;
	int window_width;
	int window_height;
	int previous_width;
	int previous_height;
	int width;
	int height;
	int playing;
	int sdl_flags;
	SDL_Overlay *sdl_overlay;
	SDL_Rect rect;
	uint8_t *buffer;
	int bpp;
	int is_purge;
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_purge( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static int consumer_get_dimensions( int *width, int *height );
static void consumer_sdl_event( mlt_listener listener, mlt_properties owner, mlt_service self, void **args );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_sdl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	consumer_sdl self = calloc( 1, sizeof( struct consumer_sdl_s ) );

	// If no malloc'd and consumer init ok
	if ( self != NULL && mlt_consumer_init( &self->parent, self, profile ) == 0 )
	{
		// Create the queue
		self->queue = mlt_deque_init( );

		// Get the parent consumer object
		mlt_consumer parent = &self->parent;

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// get a handle on properties
		mlt_service service = MLT_CONSUMER_SERVICE( parent );
		self->properties = MLT_SERVICE_PROPERTIES( service );

		// Set the default volume
		mlt_properties_set_double( self->properties, "volume", 1.0 );

		// This is the initialisation of the consumer
		pthread_mutex_init( &self->audio_mutex, NULL );
		pthread_cond_init( &self->audio_cond, NULL);
		pthread_mutex_init( &self->video_mutex, NULL );
		pthread_cond_init( &self->video_cond, NULL);
		
		// Default scaler (for now we'll use nearest)
		mlt_properties_set( self->properties, "rescale", "nearest" );
		mlt_properties_set( self->properties, "deinterlace_method", "onefield" );
		mlt_properties_set_int( self->properties, "top_field_first", -1 );

		// Default buffer for low latency
		mlt_properties_set_int( self->properties, "buffer", 1 );

		// Default audio buffer
		mlt_properties_set_int( self->properties, "audio_buffer", 2048 );

		// Ensure we don't join on a non-running object
		self->joined = 1;
		
		// process actual param
		if ( arg && sscanf( arg, "%dx%d", &self->width, &self->height ) )
		{
			mlt_properties_set_int( self->properties, "_arg_size", 1 );
		}
		else
		{
			self->width = mlt_properties_get_int( self->properties, "width" );
			self->height = mlt_properties_get_int( self->properties, "height" );
		}
	
		// Set the sdl flags
		self->sdl_flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_DOUBLEBUF;
#if !defined(__APPLE__)
		self->sdl_flags |= SDL_RESIZABLE;
#endif		
		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;
		parent->purge = consumer_purge;

		// Register specific events
		mlt_events_register( self->properties, "consumer-sdl-event", ( mlt_transmitter )consumer_sdl_event );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( self );

	// Indicate failure
	return NULL;
}

static void consumer_sdl_event( mlt_listener listener, mlt_properties owner, mlt_service self, void **args )
{
	if ( listener != NULL )
		listener( owner, self, ( SDL_Event * )args[ 0 ] );
}

int consumer_start( mlt_consumer parent )
{
	consumer_sdl self = parent->child;

	if ( !self->running )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		int video_off = mlt_properties_get_int( properties, "video_off" );
		int preview_off = mlt_properties_get_int( properties, "preview_off" );
		int display_off = video_off | preview_off;
		int audio_off = mlt_properties_get_int( properties, "audio_off" );
		int sdl_started = mlt_properties_get_int( properties, "sdl_started" );
		char *output_display = mlt_properties_get( properties, "output_display" );
		char *window_id = mlt_properties_get( properties, "window_id" );
		char *audio_driver = mlt_properties_get( properties, "audio_driver" );
		char *video_driver = mlt_properties_get( properties, "video_driver" );
		char *audio_device = mlt_properties_get( properties, "audio_device" );

		consumer_stop( parent );

		self->running = 1;
		self->joined = 0;

		if ( output_display != NULL )
			setenv( "DISPLAY", output_display, 1 );

		if ( window_id != NULL )
			setenv( "SDL_WINDOWID", window_id, 1 );

		if ( video_driver != NULL )
			setenv( "SDL_VIDEODRIVER", video_driver, 1 );

		if ( audio_driver != NULL )
			setenv( "SDL_AUDIODRIVER", audio_driver, 1 );

		if ( audio_device != NULL )
			setenv( "AUDIODEV", audio_device, 1 );

		if ( ! mlt_properties_get_int( self->properties, "_arg_size" ) )
		{
			if ( mlt_properties_get_int( self->properties, "width" ) > 0 )
				self->width = mlt_properties_get_int( self->properties, "width" );
			if ( mlt_properties_get_int( self->properties, "height" ) > 0 )
				self->height = mlt_properties_get_int( self->properties, "height" );
		}

		self->bpp = mlt_properties_get_int( self->properties, "bpp" );

		if ( sdl_started == 0 && display_off == 0 )
		{
			pthread_mutex_lock( &mlt_sdl_mutex );
			int ret = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE );
			pthread_mutex_unlock( &mlt_sdl_mutex );
			if ( ret < 0 )
			{
				mlt_log_error( MLT_CONSUMER_SERVICE(parent), "Failed to initialize SDL: %s\n", SDL_GetError() );
				return -1;
			}

			SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
			SDL_EnableUNICODE( 1 );
		}

		if ( audio_off == 0 )
			SDL_InitSubSystem( SDL_INIT_AUDIO );

		// Default window size
		if ( mlt_properties_get_int( self->properties, "_arg_size" ) )
		{
			self->window_width = self->width;
			self->window_height = self->height;
		}
		else
		{
			double display_ratio = mlt_properties_get_double( self->properties, "display_ratio" );
			self->window_width = ( double )self->height * display_ratio + 0.5;
			self->window_height = self->height;
		}

		pthread_mutex_lock( &mlt_sdl_mutex );
		if ( !SDL_GetVideoSurface() && display_off == 0 )
		{
			if ( mlt_properties_get_int( self->properties, "fullscreen" ) )
			{
				const SDL_VideoInfo *vi;
				vi = SDL_GetVideoInfo();
				self->window_width = vi->current_w;
				self->window_height = vi->current_h;
				self->sdl_flags |= SDL_FULLSCREEN;
				SDL_ShowCursor( SDL_DISABLE );
			}
			SDL_SetVideoMode( self->window_width, self->window_height, 0, self->sdl_flags );
		}
		pthread_mutex_unlock( &mlt_sdl_mutex );

		pthread_create( &self->thread, NULL, consumer_thread, self );
	}

	return 0;
}

int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl self = parent->child;

	if ( self->joined == 0 )
	{
		// Kill the thread and clean up
		self->joined = 1;
		self->running = 0;
#ifndef _WIN32
		if ( self->thread )
#endif
			pthread_join( self->thread, NULL );

		// internal cleanup
		if ( self->sdl_overlay != NULL )
			SDL_FreeYUVOverlay( self->sdl_overlay );
		self->sdl_overlay = NULL;

		if ( !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "audio_off" ) )
		{
			pthread_mutex_lock( &self->audio_mutex );
			pthread_cond_broadcast( &self->audio_cond );
			pthread_mutex_unlock( &self->audio_mutex );
			SDL_QuitSubSystem( SDL_INIT_AUDIO );
		}

		if ( mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "sdl_started" ) == 0 )
		{
			pthread_mutex_lock( &mlt_sdl_mutex );
			SDL_Quit( );
			pthread_mutex_unlock( &mlt_sdl_mutex );
		}
	}

	return 0;
}

int consumer_is_stopped( mlt_consumer parent )
{
	consumer_sdl self = parent->child;
	return !self->running;
}

void consumer_purge( mlt_consumer parent )
{
	consumer_sdl self = parent->child;
	if ( self->running )
	{
		pthread_mutex_lock( &self->video_mutex );
		while ( mlt_deque_count( self->queue ) )
			mlt_frame_close( mlt_deque_pop_back( self->queue ) );
		self->is_purge = 1;
		pthread_cond_broadcast( &self->video_cond );
		pthread_mutex_unlock( &self->video_mutex );
	}
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

static void sdl_fill_audio( void *udata, uint8_t *stream, int len )
{
	consumer_sdl self = udata;

	// Get the volume
	double volume = mlt_properties_get_double( self->properties, "volume" );

	pthread_mutex_lock( &self->audio_mutex );

	// Block until audio received
	while ( self->running && len > self->audio_avail )
		pthread_cond_wait( &self->audio_cond, &self->audio_mutex );

	if ( self->audio_avail >= len )
	{
		// Place in the audio buffer
		if ( volume != 1.0 )
			SDL_MixAudio( stream, self->audio_buffer, len, ( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );
		else
			memcpy( stream, self->audio_buffer, len );

		// Remove len from the audio available
		self->audio_avail -= len;

		// Remove the samples
		memmove( self->audio_buffer, self->audio_buffer + len, self->audio_avail );
	}
	else
	{
		// Just to be safe, wipe the stream first
		memset( stream, 0, len );

		// Mix the audio 
		SDL_MixAudio( stream, self->audio_buffer, len, ( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );

		// No audio left
		self->audio_avail = 0;
	}

	// We're definitely playing now
	self->playing = 1;

	pthread_cond_broadcast( &self->audio_cond );
	pthread_mutex_unlock( &self->audio_mutex );
}

static int consumer_play_audio( consumer_sdl self, mlt_frame frame, int init_audio, int *duration )
{
	// Get the properties of self consumer
	mlt_properties properties = self->properties;
	mlt_audio_format afmt = mlt_audio_s16;

	// Set the preferred params of the test card signal
	int channels = mlt_properties_get_int( properties, "channels" );
	int dest_channels = channels;
	int frequency = mlt_properties_get_int( properties, "frequency" );
	static int counter = 0;

	int samples = mlt_sample_calculator( mlt_properties_get_double( self->properties, "fps" ), frequency, counter++ );
	
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, (void**) &pcm, &afmt, &frequency, &channels, &samples );
	*duration = ( ( samples * 1000 ) / frequency );
	pcm += mlt_properties_get_int( properties, "audio_offset" );

	if ( mlt_properties_get_int( properties, "audio_off" ) )
	{
		self->playing = 1;
		init_audio = 1;
		return init_audio;
	}

	if ( init_audio == 1 )
	{
		SDL_AudioSpec request;
		SDL_AudioSpec got;

		int audio_buffer = mlt_properties_get_int( properties, "audio_buffer" );

		// specify audio format
		memset( &request, 0, sizeof( SDL_AudioSpec ) );
		self->playing = 0;
		request.freq = frequency;
		request.format = AUDIO_S16SYS;
		request.channels = dest_channels;
		request.samples = audio_buffer;
		request.callback = sdl_fill_audio;
		request.userdata = (void *)self;
		if ( SDL_OpenAudio( &request, &got ) != 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE( self ), "SDL failed to open audio: %s\n", SDL_GetError() );
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
		mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
		
		bytes = samples * dest_channels * sizeof(*pcm);
		pthread_mutex_lock( &self->audio_mutex );
		while ( self->running && bytes > ( sizeof( self->audio_buffer) - self->audio_avail ) )
			pthread_cond_wait( &self->audio_cond, &self->audio_mutex );
		if ( self->running )
		{
			if ( mlt_properties_get_double( properties, "_speed" ) == 1 )
			{
				if ( channels == dest_channels )
				{
					memcpy( &self->audio_buffer[ self->audio_avail ], pcm, bytes );
				}
				else
				{
					int16_t *dest = (int16_t*) &self->audio_buffer[ self->audio_avail ];
					int i = samples + 1;
					
					while ( --i )
					{
						memcpy( dest, pcm, dest_channels * sizeof(*pcm) );
						pcm += channels;
						dest += dest_channels;
					}
				}
			}
			else
			{
				memset( &self->audio_buffer[ self->audio_avail ], 0, bytes );
			}
			self->audio_avail += bytes;
		}
		pthread_cond_broadcast( &self->audio_cond );
		pthread_mutex_unlock( &self->audio_mutex );
	}
	else
	{
		self->playing = 1;
	}

	return init_audio;
}

static int consumer_play_video( consumer_sdl self, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = self->properties;

	mlt_image_format vfmt = mlt_properties_get_int( properties, "mlt_image_format" );
	int width = self->width, height = self->height;
	uint8_t *image;
	int changed = 0;

	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	mlt_image_format preview_format = mlt_properties_get_int( properties, "preview_format" );
	int display_off = video_off | preview_off;

	if ( self->running && display_off == 0 )
	{
		// Get the image, width and height
		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		
		void *pool = mlt_cocoa_autorelease_init();

		// Handle events
		if ( SDL_GetVideoSurface() )
		{
			SDL_Event event;
	
			sdl_lock_display( );
			pthread_mutex_lock( &mlt_sdl_mutex );
			changed = consumer_get_dimensions( &self->window_width, &self->window_height );
			pthread_mutex_unlock( &mlt_sdl_mutex );
			sdl_unlock_display( );

			while ( SDL_PollEvent( &event ) )
			{
				mlt_events_fire( self->properties, "consumer-sdl-event", &event, NULL );

				switch( event.type )
				{
					case SDL_VIDEORESIZE:
						self->window_width = event.resize.w;
						self->window_height = event.resize.h;
						changed = 1;
						break;
					case SDL_QUIT:
						self->running = 0;
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
	
		sdl_lock_display();

		if ( width != self->width || height != self->height )
		{
			if ( self->sdl_overlay != NULL )
				SDL_FreeYUVOverlay( self->sdl_overlay );
			self->sdl_overlay = NULL;
		}

		if ( self->running && ( !SDL_GetVideoSurface() || changed ) )
		{
			// Force an overlay recreation
			if ( self->sdl_overlay != NULL )
				SDL_FreeYUVOverlay( self->sdl_overlay );
			self->sdl_overlay = NULL;

			// open SDL window with video overlay, if possible
			pthread_mutex_lock( &mlt_sdl_mutex );
			SDL_Surface *screen = SDL_SetVideoMode( self->window_width, self->window_height, self->bpp, self->sdl_flags );
			if ( consumer_get_dimensions( &self->window_width, &self->window_height ) )
				screen = SDL_SetVideoMode( self->window_width, self->window_height, self->bpp, self->sdl_flags );
			pthread_mutex_unlock( &mlt_sdl_mutex );

			if ( screen )
			{
				uint32_t color = mlt_properties_get_int( self->properties, "window_background" );
				SDL_FillRect( screen, NULL, color >> 8 );
				SDL_Flip( screen );
			}
		}

		if ( self->running )
		{
			// Determine window's new display aspect ratio
			double this_aspect = ( double )self->window_width / self->window_height;

			// Get the display aspect ratio
			double display_ratio = mlt_properties_get_double( properties, "display_ratio" );

			// Determine frame's display aspect ratio
			double frame_aspect = mlt_frame_get_aspect_ratio( frame ) * width / height;

			// Store the width and height received
			self->width = width;
			self->height = height;

			// If using hardware scaler
			if ( mlt_properties_get( properties, "rescale" ) != NULL &&
				!strcmp( mlt_properties_get( properties, "rescale" ), "none" ) )
			{
				// Use hardware scaler to normalise display aspect ratio
				self->rect.w = frame_aspect / this_aspect * self->window_width;
				self->rect.h = self->window_height;
				if ( self->rect.w > self->window_width )
				{
					self->rect.w = self->window_width;
					self->rect.h = this_aspect / frame_aspect * self->window_height;
				}
			}
			// Special case optimisation to negate odd effect of sample aspect ratio
			// not corresponding exactly with image resolution.
			else if ( (int)( this_aspect * 1000 ) == (int)( display_ratio * 1000 ) ) 
			{
				self->rect.w = self->window_width;
				self->rect.h = self->window_height;
			}
			// Use hardware scaler to normalise sample aspect ratio
			else if ( self->window_height * display_ratio > self->window_width )
			{
				self->rect.w = self->window_width;
				self->rect.h = self->window_width / display_ratio;
			}
			else
			{
				self->rect.w = self->window_height * display_ratio;
				self->rect.h = self->window_height;
			}
			
			self->rect.x = ( self->window_width - self->rect.w ) / 2;
			self->rect.y = ( self->window_height - self->rect.h ) / 2;
			self->rect.x -= self->rect.x % 2;

			mlt_properties_set_int( self->properties, "rect_x", self->rect.x );
			mlt_properties_set_int( self->properties, "rect_y", self->rect.y );
			mlt_properties_set_int( self->properties, "rect_w", self->rect.w );
			mlt_properties_set_int( self->properties, "rect_h", self->rect.h );

			SDL_SetClipRect( SDL_GetVideoSurface(), &self->rect );
		}

		if ( self->running && SDL_GetVideoSurface() && self->sdl_overlay == NULL )
		{
			SDL_SetClipRect( SDL_GetVideoSurface(), &self->rect );
			self->sdl_overlay = SDL_CreateYUVOverlay( width, height,
				( vfmt == mlt_image_yuv422 ? SDL_YUY2_OVERLAY : SDL_IYUV_OVERLAY ),
				SDL_GetVideoSurface() );
		}

		if ( self->running && SDL_GetVideoSurface() && self->sdl_overlay != NULL )
		{
			self->buffer = self->sdl_overlay->pixels[ 0 ];
			if ( SDL_LockYUVOverlay( self->sdl_overlay ) >= 0 )
			{
				// We use height-1 because mlt_image_format_size() uses height + 1.
				// XXX Remove -1 when mlt_image_format_size() is changed.
				int size = mlt_image_format_size( vfmt, width, height - 1, NULL );
				if ( image != NULL )
					memcpy( self->buffer, image, size );
				SDL_UnlockYUVOverlay( self->sdl_overlay );
				SDL_DisplayYUVOverlay( self->sdl_overlay, &SDL_GetVideoSurface()->clip_rect );
			}
		}

		sdl_unlock_display();
		mlt_cocoa_autorelease_close( pool );
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
	}
	else if ( self->running )
	{
		vfmt = preview_format == mlt_image_none ? mlt_image_rgb24a : preview_format;
		if ( !video_off )
			mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
	}

	return 0;
}

static void *video_thread( void *arg )
{
	// Identify the arg
	consumer_sdl self = arg;

	// Obtain time of thread start
	struct timeval now;
	int64_t start = 0;
	int64_t elapsed = 0;
	struct timespec tm;
	mlt_frame next = NULL;
	mlt_properties properties = NULL;
	double speed = 0;

	// Get real time flag
	int real_time = mlt_properties_get_int( self->properties, "real_time" );

	// Get the current time
	gettimeofday( &now, NULL );

	// Determine start time
	start = ( int64_t )now.tv_sec * 1000000 + now.tv_usec;

	while ( self->running )
	{
		// Pop the next frame
		pthread_mutex_lock( &self->video_mutex );
		next = mlt_deque_pop_front( self->queue );
		while ( next == NULL && self->running )
		{
			pthread_cond_wait( &self->video_cond, &self->video_mutex );
			next = mlt_deque_pop_front( self->queue );
		}
		pthread_mutex_unlock( &self->video_mutex );

		if ( !self->running || next == NULL ) break;

		// Get the properties
		properties =  MLT_FRAME_PROPERTIES( next );

		// Get the speed of the frame
		speed = mlt_properties_get_double( properties, "_speed" );

		// Get the current time
		gettimeofday( &now, NULL );

		// Get the elapsed time
		elapsed = ( ( int64_t )now.tv_sec * 1000000 + now.tv_usec ) - start;

		// See if we have to delay the display of the current frame
		if ( mlt_properties_get_int( properties, "rendered" ) == 1 && self->running )
		{
			// Obtain the scheduled playout time
			int64_t scheduled = mlt_properties_get_int( properties, "playtime" );

			// Determine the difference between the elapsed time and the scheduled playout time
			int64_t difference = scheduled - elapsed;

			// Smooth playback a bit
			if ( real_time && ( difference > 20000 && speed == 1.0 ) )
			{
				tm.tv_sec = difference / 1000000;
				tm.tv_nsec = ( difference % 1000000 ) * 500;
				nanosleep( &tm, NULL );
			}

			// Show current frame if not too old
			if ( !real_time || ( difference > -10000 || speed != 1.0 || mlt_deque_count( self->queue ) < 2 ) )
				consumer_play_video( self, next );

			// If the queue is empty, recalculate start to allow build up again
			if ( real_time && ( mlt_deque_count( self->queue ) == 0 && speed == 1.0 ) )
			{
				gettimeofday( &now, NULL );
				start = ( ( int64_t )now.tv_sec * 1000000 + now.tv_usec ) - scheduled + 20000;
			}
		}
		else
		{
			static int dropped = 0;
			mlt_log_info( MLT_CONSUMER_SERVICE(&self->parent), "dropped video frame %d\n", ++dropped );
		}

		// This frame can now be closed
		mlt_frame_close( next );
		next = NULL;
	}

	if ( next != NULL )
		mlt_frame_close( next );

	mlt_consumer_stopped( &self->parent );

	return NULL;
}

/** Threaded wrapper for pipe.
*/

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_sdl self = arg;

	// Get the consumer
	mlt_consumer consumer = &self->parent;

	// Convenience functionality
	int terminate_on_pause = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( consumer ), "terminate_on_pause" );
	int terminated = 0;

	// Video thread
	pthread_t thread;

	// internal intialization
	int init_audio = 1;
	int init_video = 1;
	mlt_frame frame = NULL;
	int duration = 0;
	int64_t playtime = 0;
	struct timespec tm = { 0, 100000 };

	// Loop until told not to
	while( self->running )
	{
		// Get a frame from the attached producer
		frame = !terminated? mlt_consumer_rt_frame( consumer ) : NULL;

		// Check for termination
		if ( terminate_on_pause && frame )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Ensure that we have a frame
		if ( frame )
		{
			// Play audio
			init_audio = consumer_play_audio( self, frame, init_audio, &duration );

			// Determine the start time now
			if ( self->playing && init_video )
			{
				// Create the video thread
				pthread_create( &thread, NULL, video_thread, self );

				// Video doesn't need to be initialised any more
				init_video = 0;
			}

			// Set playtime for this frame
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "playtime", playtime );

			while ( self->running && mlt_deque_count( self->queue ) > 15 )
				nanosleep( &tm, NULL );

			// Push this frame to the back of the queue
			pthread_mutex_lock( &self->video_mutex );
			if ( self->is_purge )
			{
				mlt_frame_close( frame );
				frame = NULL;
				self->is_purge = 0;
			}
			else
			{
				mlt_deque_push_back( self->queue, frame );
				pthread_cond_broadcast( &self->video_cond );
			}
			pthread_mutex_unlock( &self->video_mutex );

			// Calculate the next playtime
			playtime += ( duration * 1000 );
		}
		else if ( terminated )
		{
			if ( init_video || mlt_deque_count( self->queue ) == 0 )
				break;
			else
				nanosleep( &tm, NULL );
		}
	}

	self->running = 0;
	
	// Unblock sdl_preview
	if ( mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( consumer ), "put_mode" ) &&
	     mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( consumer ), "put_pending" ) )
	{
		frame = mlt_consumer_get_frame( consumer );
		if ( frame )
			mlt_frame_close( frame );
		frame = NULL;
	}

	// Kill the video thread
	if ( init_video == 0 )
	{
		pthread_mutex_lock( &self->video_mutex );
		pthread_cond_broadcast( &self->video_cond );
		pthread_mutex_unlock( &self->video_mutex );
		pthread_join( thread, NULL );
	}

	while( mlt_deque_count( self->queue ) )
		mlt_frame_close( mlt_deque_pop_back( self->queue ) );

	self->audio_avail = 0;

	return NULL;
}

static int consumer_get_dimensions( int *width, int *height )
{
	int changed = 0;

	// SDL windows manager structure
	SDL_SysWMinfo wm;

	// Specify the SDL Version
	SDL_VERSION( &wm.version );

	// Lock the display
	//sdl_lock_display();

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

	// Unlock the display
	//sdl_unlock_display();

	return changed;
}

/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl self = parent->child;

	// Stop the consumer
	///mlt_consumer_stop( parent );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the queue
	mlt_deque_close( self->queue );

	// Destroy mutexes
	pthread_mutex_destroy( &self->audio_mutex );
	pthread_cond_destroy( &self->audio_cond );
		
	// Finally clean up this
	free( self );
}
