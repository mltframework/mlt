/*
 * consumer_sdl.c -- A Simple DirectMedia Layer consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <sys/time.h>

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
	SDL_Surface *sdl_screen;
	SDL_Overlay *sdl_overlay;
	SDL_Rect rect;
	uint8_t *buffer;
	int bpp;
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

mlt_consumer consumer_sdl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	consumer_sdl this = calloc( sizeof( struct consumer_sdl_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( &this->parent, this, profile ) == 0 )
	{
		// Create the queue
		this->queue = mlt_deque_init( );

		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// get a handle on properties
		mlt_service service = MLT_CONSUMER_SERVICE( parent );
		this->properties = MLT_SERVICE_PROPERTIES( service );

		// Set the default volume
		mlt_properties_set_double( this->properties, "volume", 1.0 );

		// This is the initialisation of the consumer
		pthread_mutex_init( &this->audio_mutex, NULL );
		pthread_cond_init( &this->audio_cond, NULL);
		pthread_mutex_init( &this->video_mutex, NULL );
		pthread_cond_init( &this->video_cond, NULL);
		
		// Default scaler (for now we'll use nearest)
		mlt_properties_set( this->properties, "rescale", "nearest" );

		// Default buffer for low latency
		mlt_properties_set_int( this->properties, "buffer", 1 );

		// Default audio buffer
		mlt_properties_set_int( this->properties, "audio_buffer", 512 );

		// Ensure we don't join on a non-running object
		this->joined = 1;
		
		// process actual param
		if ( arg == NULL || sscanf( arg, "%dx%d", &this->width, &this->height ) != 2 )
		{
			this->width = mlt_properties_get_int( this->properties, "width" );
			this->height = mlt_properties_get_int( this->properties, "height" );
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

int consumer_start( mlt_consumer parent )
{
	consumer_sdl this = parent->child;

	if ( !this->running )
	{
		int video_off = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "video_off" );
		int preview_off = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "preview_off" );
		int display_off = video_off | preview_off;
		int audio_off = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "audio_off" );
		int sdl_started = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "sdl_started" );

		consumer_stop( parent );

		this->running = 1;
		this->joined = 0;

		if ( mlt_properties_get_int( this->properties, "width" ) > 0 )
			this->width = mlt_properties_get_int( this->properties, "width" );
		if ( mlt_properties_get_int( this->properties, "height" ) > 0 )
			this->height = mlt_properties_get_int( this->properties, "height" );

		this->bpp = mlt_properties_get_int( this->properties, "bpp" );

		if ( sdl_started == 0 && display_off == 0 )
		{
			if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) < 0 )
			{
				mlt_log_error( MLT_CONSUMER_SERVICE(parent), "Failed to initialize SDL: %s\n", SDL_GetError() );
				return -1;
			}

			SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
			SDL_EnableUNICODE( 1 );
			
#ifndef __DARWIN__
			// Get the wm structure
			SDL_SysWMinfo wm;
			SDL_VERSION( &wm.version );
			// Only on X11
			if ( SDL_GetWMInfo( &wm ) == 1 && wm.subsystem == SDL_SYSWM_X11 )
			{
				// Set the x11_display property for use by VDPAU
				mlt_properties_set_int( this->properties, "x11_display", (int) wm.info.x11.display );
				mlt_log_verbose( MLT_CONSUMER_SERVICE(parent), "X11 Display = %p\n", wm.info.x11.display );
				char tmp[20];
				sprintf( tmp, "%p", wm.info.x11.display );
				mlt_environment_set( "x11_display", tmp );
			}
#endif
		}
		else if ( display_off == 0 )
		{
			this->sdl_screen = SDL_GetVideoSurface( );
		}

		if ( audio_off == 0 )
			SDL_InitSubSystem( SDL_INIT_AUDIO );

		// Default window size
		double display_ratio = mlt_properties_get_double( this->properties, "display_ratio" );
		this->window_width = ( double )this->height * display_ratio;
		this->window_height = this->height;

		if ( this->sdl_screen == NULL && display_off == 0 )
		{
			if ( mlt_properties_get_int( this->properties, "fullscreen" ) )
			{
				const SDL_VideoInfo *vi = SDL_GetVideoInfo();
				this->window_width = vi->current_w;
				this->window_height = vi->current_h;
				this->sdl_flags |= SDL_FULLSCREEN;
				SDL_ShowCursor( SDL_DISABLE );
			}
			pthread_mutex_lock( &mlt_sdl_mutex );
			this->sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, 0, this->sdl_flags );
			pthread_mutex_unlock( &mlt_sdl_mutex );
		}

		pthread_create( &this->thread, NULL, consumer_thread, this );
	}

	return 0;
}

int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl this = parent->child;

	if ( this->joined == 0 )
	{
		// Kill the thread and clean up
		this->joined = 1;
		this->running = 0;
		if ( this->thread )
			pthread_join( this->thread, NULL );

		// internal cleanup
		if ( this->sdl_overlay != NULL )
			SDL_FreeYUVOverlay( this->sdl_overlay );
		this->sdl_overlay = NULL;

		if ( !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "audio_off" ) )
		{
			pthread_mutex_lock( &this->audio_mutex );
			pthread_cond_broadcast( &this->audio_cond );
			pthread_mutex_unlock( &this->audio_mutex );
			SDL_QuitSubSystem( SDL_INIT_AUDIO );
		}

		this->sdl_screen = NULL;
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
	consumer_sdl this = udata;

	// Get the volume
	double volume = mlt_properties_get_double( this->properties, "volume" );

	pthread_mutex_lock( &this->audio_mutex );

	// Block until audio received
	while ( this->running && len > this->audio_avail )
		pthread_cond_wait( &this->audio_cond, &this->audio_mutex );

	if ( this->audio_avail >= len )
	{
		// Place in the audio buffer
		if ( volume != 1.0 )
			SDL_MixAudio( stream, this->audio_buffer, len, ( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );
		else
			memcpy( stream, this->audio_buffer, len );

		// Remove len from the audio available
		this->audio_avail -= len;

		// Remove the samples
		memmove( this->audio_buffer, this->audio_buffer + len, this->audio_avail );
	}
	else
	{
		// Just to be safe, wipe the stream first
		memset( stream, 0, len );

		// Mix the audio 
		SDL_MixAudio( stream, this->audio_buffer, len, ( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );

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
	mlt_audio_format afmt = mlt_audio_s16;

	// Set the preferred params of the test card signal
	int channels = mlt_properties_get_int( properties, "channels" );
	int dest_channels = channels;
	int frequency = mlt_properties_get_int( properties, "frequency" );
	static int counter = 0;

	int samples = mlt_sample_calculator( mlt_properties_get_double( this->properties, "fps" ), frequency, counter++ );
	
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, (void**) &pcm, &afmt, &frequency, &channels, &samples );
	*duration = ( ( samples * 1000 ) / frequency );
	pcm += mlt_properties_get_int( properties, "audio_offset" );

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

		int audio_buffer = mlt_properties_get_int( properties, "audio_buffer" );

		// specify audio format
		memset( &request, 0, sizeof( SDL_AudioSpec ) );
		this->playing = 0;
		request.freq = frequency;
		request.format = AUDIO_S16SYS;
		request.channels = dest_channels;
		request.samples = audio_buffer;
		request.callback = sdl_fill_audio;
		request.userdata = (void *)this;
		if ( SDL_OpenAudio( &request, &got ) != 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE( this ), "SDL failed to open audio: %s\n", SDL_GetError() );
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
		pthread_mutex_lock( &this->audio_mutex );
		while ( this->running && bytes > ( sizeof( this->audio_buffer) - this->audio_avail ) )
			pthread_cond_wait( &this->audio_cond, &this->audio_mutex );
		if ( this->running )
		{
			if ( mlt_properties_get_double( properties, "_speed" ) == 1 )
			{
				if ( channels == dest_channels )
				{
					memcpy( &this->audio_buffer[ this->audio_avail ], pcm, bytes );
				}
				else
				{
					int16_t *dest = (int16_t*) &this->audio_buffer[ this->audio_avail ];
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
				memset( &this->audio_buffer[ this->audio_avail ], 0, bytes );
			}
			this->audio_avail += bytes;
		}
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

	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	mlt_image_format preview_format = mlt_properties_get_int( properties, "preview_format" );
	int display_off = video_off | preview_off;

	if ( this->running && display_off == 0 )
	{
		// Get the image, width and height
		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "format", vfmt );
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
		
		// Handle events
		if ( this->sdl_screen != NULL )
		{
			SDL_Event event;
	
			sdl_lock_display( );
			pthread_mutex_lock( &mlt_sdl_mutex );
			changed = consumer_get_dimensions( &this->window_width, &this->window_height );
			pthread_mutex_unlock( &mlt_sdl_mutex );
			sdl_unlock_display( );

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
	
		sdl_lock_display();

		if ( width != this->width || height != this->height )
		{
			if ( this->sdl_overlay != NULL )
				SDL_FreeYUVOverlay( this->sdl_overlay );
			this->sdl_overlay = NULL;
		}

		if ( this->running && ( this->sdl_screen == NULL || changed ) )
		{
			// Force an overlay recreation
			if ( this->sdl_overlay != NULL )
				SDL_FreeYUVOverlay( this->sdl_overlay );
			this->sdl_overlay = NULL;

			// open SDL window with video overlay, if possible
			pthread_mutex_lock( &mlt_sdl_mutex );
			this->sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, this->bpp, this->sdl_flags );
			if ( consumer_get_dimensions( &this->window_width, &this->window_height ) )
				this->sdl_screen = SDL_SetVideoMode( this->window_width, this->window_height, this->bpp, this->sdl_flags );
			pthread_mutex_unlock( &mlt_sdl_mutex );

			uint32_t color = mlt_properties_get_int( this->properties, "window_background" );
			SDL_FillRect( this->sdl_screen, NULL, color >> 8 );
			SDL_Flip( this->sdl_screen );
		}

		if ( this->running )
		{
			// Determine window's new display aspect ratio
			double this_aspect = ( double )this->window_width / this->window_height;

			// Get the display aspect ratio
			double display_ratio = mlt_properties_get_double( properties, "display_ratio" );

			// Determine frame's display aspect ratio
			double frame_aspect = mlt_frame_get_aspect_ratio( frame ) * width / height;

			// Store the width and height received
			this->width = width;
			this->height = height;

			// If using hardware scaler
			if ( mlt_properties_get( properties, "rescale" ) != NULL &&
				!strcmp( mlt_properties_get( properties, "rescale" ), "none" ) )
			{
				// Use hardware scaler to normalise display aspect ratio
				this->rect.w = frame_aspect / this_aspect * this->window_width;
				this->rect.h = this->window_height;
				if ( this->rect.w > this->window_width )
				{
					this->rect.w = this->window_width;
					this->rect.h = this_aspect / frame_aspect * this->window_height;
				}
			}
			// Special case optimisation to negate odd effect of sample aspect ratio
			// not corresponding exactly with image resolution.
			else if ( (int)( this_aspect * 1000 ) == (int)( display_ratio * 1000 ) ) 
			{
				this->rect.w = this->window_width;
				this->rect.h = this->window_height;
			}
			// Use hardware scaler to normalise sample aspect ratio
			else if ( this->window_height * display_ratio > this->window_width )
			{
				this->rect.w = this->window_width;
				this->rect.h = this->window_width / display_ratio;
			}
			else
			{
				this->rect.w = this->window_height * display_ratio;
				this->rect.h = this->window_height;
			}
			
			this->rect.x = ( this->window_width - this->rect.w ) / 2;
			this->rect.y = ( this->window_height - this->rect.h ) / 2;
			this->rect.x -= this->rect.x % 2;

			mlt_properties_set_int( this->properties, "rect_x", this->rect.x );
			mlt_properties_set_int( this->properties, "rect_y", this->rect.y );
			mlt_properties_set_int( this->properties, "rect_w", this->rect.w );
			mlt_properties_set_int( this->properties, "rect_h", this->rect.h );

			SDL_SetClipRect( this->sdl_screen, &this->rect );
		}

		if ( this->running && this->sdl_screen != NULL && this->sdl_overlay == NULL )
		{
			SDL_SetClipRect( this->sdl_screen, &this->rect );
			this->sdl_overlay = SDL_CreateYUVOverlay( width, height, SDL_YUY2_OVERLAY, this->sdl_screen );
		}

		if ( this->running && this->sdl_screen != NULL && this->sdl_overlay != NULL )
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

		sdl_unlock_display();
	}
	else if ( this->running )
	{
		vfmt = preview_format == mlt_image_none ? mlt_image_rgb24a : preview_format;
		if ( !video_off )
			mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "format", vfmt );
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
	}

	return 0;
}

static void *video_thread( void *arg )
{
	// Identify the arg
	consumer_sdl this = arg;

	// Obtain time of thread start
	struct timeval now;
	int64_t start = 0;
	int64_t elapsed = 0;
	struct timespec tm;
	mlt_frame next = NULL;
	mlt_properties properties = NULL;
	double speed = 0;

	// Get real time flag
	int real_time = mlt_properties_get_int( this->properties, "real_time" );

	// Get the current time
	gettimeofday( &now, NULL );

	// Determine start time
	start = ( int64_t )now.tv_sec * 1000000 + now.tv_usec;

	while ( this->running )
	{
		// Pop the next frame
		pthread_mutex_lock( &this->video_mutex );
		next = mlt_deque_pop_front( this->queue );
		while ( next == NULL && this->running )
		{
			pthread_cond_wait( &this->video_cond, &this->video_mutex );
			next = mlt_deque_pop_front( this->queue );
		}
		pthread_mutex_unlock( &this->video_mutex );

		if ( !this->running || next == NULL ) break;

		// Get the properties
		properties =  MLT_FRAME_PROPERTIES( next );

		// Get the speed of the frame
		speed = mlt_properties_get_double( properties, "_speed" );

		// Get the current time
		gettimeofday( &now, NULL );

		// Get the elapsed time
		elapsed = ( ( int64_t )now.tv_sec * 1000000 + now.tv_usec ) - start;

		// See if we have to delay the display of the current frame
		if ( mlt_properties_get_int( properties, "rendered" ) == 1 && this->running )
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
			if ( !real_time || ( difference > -10000 || speed != 1.0 || mlt_deque_count( this->queue ) < 2 ) )
				consumer_play_video( this, next );

			// If the queue is empty, recalculate start to allow build up again
			if ( real_time && ( mlt_deque_count( this->queue ) == 0 && speed == 1.0 ) )
			{
				gettimeofday( &now, NULL );
				start = ( ( int64_t )now.tv_sec * 1000000 + now.tv_usec ) - scheduled + 20000;
			}
		}
		else
		{
			mlt_log_debug( MLT_CONSUMER_SERVICE(&this->parent), "dropped video frame\n" );
		}

		// This frame can now be closed
		mlt_frame_close( next );
		next = NULL;
	}

	if ( next != NULL )
		mlt_frame_close( next );

	mlt_consumer_stopped( &this->parent );

	return NULL;
}

/** Threaded wrapper for pipe.
*/

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_sdl this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;

	// Convenience functionality
	int terminate_on_pause = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( consumer ), "terminate_on_pause" );
	int terminated = 0;

	// Video thread
	pthread_t thread;

	// internal intialization
	int init_audio = 1;
	int init_video = 1;
	mlt_frame frame = NULL;
	mlt_properties properties = NULL;
	int duration = 0;
	int64_t playtime = 0;
	struct timespec tm = { 0, 100000 };

	// Loop until told not to
	while( !terminated && this->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_rt_frame( consumer );

		// Check for termination
		if ( terminate_on_pause && frame != NULL )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Ensure that we have a frame
		if ( frame != NULL )
		{
			// Get the frame properties
			properties =  MLT_FRAME_PROPERTIES( frame );

			// Play audio
			init_audio = consumer_play_audio( this, frame, init_audio, &duration );

			// Determine the start time now
			if ( this->playing && init_video )
			{
				// Create the video thread
				pthread_create( &thread, NULL, video_thread, this );

				// Video doesn't need to be initialised any more
				init_video = 0;
			}

			// Set playtime for this frame
			mlt_properties_set_int( properties, "playtime", playtime );

			while ( this->running && mlt_deque_count( this->queue ) > 15 )
				nanosleep( &tm, NULL );

			// Push this frame to the back of the queue
			pthread_mutex_lock( &this->video_mutex );
			mlt_deque_push_back( this->queue, frame );
			pthread_cond_broadcast( &this->video_cond );
			pthread_mutex_unlock( &this->video_mutex );

			// Calculate the next playtime
			playtime += ( duration * 1000 );
		}
	}

	this->running = 0;

	// Kill the video thread
	if ( init_video == 0 )
	{
		pthread_mutex_lock( &this->video_mutex );
		pthread_cond_broadcast( &this->video_cond );
		pthread_mutex_unlock( &this->video_mutex );
		pthread_join( thread, NULL );
	}

	while( mlt_deque_count( this->queue ) )
		mlt_frame_close( mlt_deque_pop_back( this->queue ) );

	this->sdl_screen = NULL;
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

	// Lock the display
	//sdl_lock_display();

#ifndef __DARWIN__
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
	consumer_sdl this = parent->child;
	int sdl_started = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "sdl_started" );

	// Stop the consumer
	///mlt_consumer_stop( parent );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the queue
	mlt_deque_close( this->queue );

	// Destroy mutexes
	pthread_mutex_destroy( &this->audio_mutex );
	pthread_cond_destroy( &this->audio_cond );
		
	if ( sdl_started == 0 )
	{
		pthread_mutex_lock( &mlt_sdl_mutex );
		SDL_Quit( );
		pthread_mutex_unlock( &mlt_sdl_mutex );
	}

	// Finally clean up this
	free( this );
}
