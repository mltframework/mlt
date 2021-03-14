/*
 * consumer_sdl.c -- A Simple DirectMedia Layer consumer
 * Copyright (C) 2017-2021 Meltytech, LLC
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

#include "common.h"

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
#include <sys/time.h>
#include <stdatomic.h>

#undef MLT_IMAGE_FORMAT // only yuv422 working currently

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
	atomic_int running;
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
	int out_channels;
	atomic_int playing;
	SDL_Window *sdl_window;
	SDL_Renderer *sdl_renderer;
	SDL_Texture *sdl_texture;
	SDL_Rect sdl_rect;
	uint8_t *buffer;
	int is_purge;
#ifdef _WIN32
	int no_quit_subsystem;
#endif
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_purge( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static int setup_sdl_video( consumer_sdl self );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_sdl2_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
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

		// Default scrub audio
		mlt_properties_set_int( self->properties, "scrub_audio", 1 );

		// Ensure we don't join on a non-running object
		self->joined = 1;
		
		// process actual param
		if ( arg && sscanf( arg, "%dx%d", &self->width, &self->height ) )
		{
			mlt_properties_set_int( self->properties, "resolution", 1 );
		}
		else
		{
			self->width = mlt_properties_get_int( self->properties, "width" );
			self->height = mlt_properties_get_int( self->properties, "height" );
		}
	
		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;
		parent->purge = consumer_purge;

		// Register specific events
		mlt_events_register( self->properties, "consumer-sdl-event" );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( self );

	// Indicate failure
	return NULL;
}

int consumer_start( mlt_consumer parent )
{
	consumer_sdl self = parent->child;

	if ( !self->running )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		int audio_off = mlt_properties_get_int( properties, "audio_off" );
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

		if ( ! mlt_properties_get_int( self->properties, "resolution" ) )
		{
			if ( mlt_properties_get_int( self->properties, "width" ) > 0 )
				self->width = mlt_properties_get_int( self->properties, "width" );
			if ( mlt_properties_get_int( self->properties, "height" ) > 0 )
				self->height = mlt_properties_get_int( self->properties, "height" );
		}

		if ( audio_off == 0 )
			SDL_InitSubSystem( SDL_INIT_AUDIO );

		// Default window size
		if ( mlt_properties_get_int( self->properties, "resolution" ) )
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

		// Initialize SDL video if needed.
		if ( setup_sdl_video(self) )
			return 1;

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

		if ( !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "audio_off" ) )
		{
			pthread_mutex_lock( &self->audio_mutex );
			pthread_cond_broadcast( &self->audio_cond );
			pthread_mutex_unlock( &self->audio_mutex );
		}

#ifndef _WIN32
		if ( self->thread )
#endif
			pthread_join( self->thread, NULL );

		// cleanup SDL
		pthread_mutex_lock( &mlt_sdl_mutex );
		if ( self->sdl_texture )
			SDL_DestroyTexture( self->sdl_texture );
		self->sdl_texture = NULL;
		if ( self->sdl_renderer )
			SDL_DestroyRenderer( self->sdl_renderer );
		self->sdl_renderer = NULL;
		if ( self->sdl_window )
			SDL_DestroyWindow( self->sdl_window );
		self->sdl_window = NULL;
#ifdef _WIN32
		if ( !self->no_quit_subsystem )
#endif
		if ( !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "audio_off" ) )
			SDL_QuitSubSystem( SDL_INIT_AUDIO );
		if ( mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "sdl_started" ) == 0 )
			SDL_Quit( );
		pthread_mutex_unlock( &mlt_sdl_mutex );
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

static void sdl_fill_audio( void *udata, uint8_t *stream, int len )
{
	consumer_sdl self = udata;

	// Get the volume
	double volume = mlt_properties_get_double( self->properties, "volume" );

	// Wipe the stream first
	memset( stream, 0, len );

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
	int frequency = mlt_properties_get_int( properties, "frequency" );
	int scrub = mlt_properties_get_int( properties, "scrub_audio" );
	static int counter = 0;

	int samples = mlt_audio_calculate_frame_samples( mlt_properties_get_double( self->properties, "fps" ), frequency, counter++ );
	int16_t *pcm;
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
		SDL_AudioDeviceID dev;
		int audio_buffer = mlt_properties_get_int( properties, "audio_buffer" );

		// specify audio format
		memset( &request, 0, sizeof( SDL_AudioSpec ) );
		self->playing = 0;
		request.freq = frequency;
		request.format = AUDIO_S16SYS;
		request.channels = mlt_properties_get_int( properties, "channels" );
		request.samples = audio_buffer;
		request.callback = sdl_fill_audio;
		request.userdata = (void *)self;

		dev = sdl2_open_audio( &request, &got );
		if( dev == 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE( self ), "SDL failed to open audio\n" );
			init_audio = 2;
		}
		else
		{
			if( got.channels != request.channels )
			{
				mlt_log_info( MLT_CONSUMER_SERVICE( self ), "Unable to output %d channels. Change to %d\n", request.channels, got.channels );
			}
			mlt_log_info( MLT_CONSUMER_SERVICE( self ), "Audio Opened: driver=%s channels=%d frequency=%d\n", SDL_GetCurrentAudioDriver(), got.channels, got.freq );
			SDL_PauseAudioDevice( dev, 0 );
			init_audio = 0;
			self->out_channels = got.channels;
		}
	}

	if ( init_audio == 0 )
	{
		mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
		int samples_copied = 0;
		int dst_stride = self->out_channels * sizeof( *pcm );

		pthread_mutex_lock( &self->audio_mutex );

		while ( self->running && samples_copied < samples )
		{
			int sample_space = ( sizeof( self->audio_buffer ) - self->audio_avail ) / dst_stride;
			while ( self->running && sample_space == 0 )
			{
				struct timeval now;
				struct timespec tm;

				gettimeofday( &now, NULL );
				tm.tv_sec = now.tv_sec + 1;
				tm.tv_nsec = now.tv_usec * 1000;
				pthread_cond_timedwait( &self->audio_cond, &self->audio_mutex, &tm );
				sample_space = ( sizeof( self->audio_buffer ) - self->audio_avail ) / dst_stride;

				if ( sample_space == 0 && self->running )
				{
					mlt_log_warning( MLT_CONSUMER_SERVICE(&self->parent), "audio timed out\n" );
					pthread_mutex_unlock( &self->audio_mutex );
#ifdef _WIN32
					self->no_quit_subsystem = 1;
#endif
					return 1;
				}
			}
			if ( self->running )
			{
				int samples_to_copy = samples - samples_copied;
				if ( samples_to_copy > sample_space )
				{
					samples_to_copy = sample_space;
				}
				int dst_bytes = samples_to_copy * dst_stride;

				if ( scrub || mlt_properties_get_double( properties, "_speed" ) == 1 )
				{
					if ( channels == self->out_channels )
					{
						memcpy( &self->audio_buffer[ self->audio_avail ], pcm, dst_bytes );
						pcm += samples_to_copy * channels;
					}
					else
					{
						int16_t *dest = (int16_t*) &self->audio_buffer[ self->audio_avail ];
						int i = samples_to_copy + 1;
						while ( --i )
						{
							memcpy( dest, pcm, dst_stride );
							pcm += channels;
							dest += self->out_channels;
						}
					}
				}
				else
				{
					memset( &self->audio_buffer[ self->audio_avail ], 0, dst_bytes );
					pcm += samples_to_copy * channels;
				}
				self->audio_avail += dst_bytes;
				samples_copied += samples_to_copy;
			}
			pthread_cond_broadcast( &self->audio_cond );
		}
		pthread_mutex_unlock( &self->audio_mutex );
	}
	else
	{
		self->playing = 1;
	}

	return init_audio;
}

static int setup_sdl_video( consumer_sdl self )
{
	int error = 0;
	int sdl_flags = SDL_WINDOW_RESIZABLE;
	int texture_format = SDL_PIXELFORMAT_YUY2;

	// Skip this if video is disabled.
	int video_off = mlt_properties_get_int( self->properties, "video_off" );
	int preview_off = mlt_properties_get_int( self->properties, "preview_off" );
	if ( video_off || preview_off )
		return error;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		pthread_mutex_lock( &mlt_sdl_mutex );
		int ret = SDL_Init( SDL_INIT_VIDEO );
		pthread_mutex_unlock( &mlt_sdl_mutex );
		if ( ret < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent), "Failed to initialize SDL: %s\n", SDL_GetError() );
			return -1;
		}
	}

#ifdef MLT_IMAGE_FORMAT
	int image_format = mlt_properties_get_int( self->properties, "mlt_image_format" );

	if ( image_format ) switch ( image_format ) {
	case mlt_image_rgb24:
		texture_format = SDL_PIXELFORMAT_RGB24;
		break;
	case mlt_image_rgb24a:
		texture_format = SDL_PIXELFORMAT_ABGR8888;
		break;
	case mlt_image_yuv420p:
		texture_format = SDL_PIXELFORMAT_IYUV;
		break;
	case mlt_image_yuv422:
		texture_format = SDL_PIXELFORMAT_YUY2;
		break;
	default:
		mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent), "Invalid image format %s\n",
			mlt_image_format_name( image_format ) );
		return -1;
	}
#endif

	if ( mlt_properties_get_int( self->properties, "fullscreen" ) )
	{
		self->window_width = self->width;
		self->window_height = self->height;
		sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		SDL_ShowCursor( SDL_DISABLE );
	}

	pthread_mutex_lock( &mlt_sdl_mutex );
	self->sdl_window = SDL_CreateWindow("MLT", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		self->window_width, self->window_height, sdl_flags);
	self->sdl_renderer = SDL_CreateRenderer(self->sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if ( self->sdl_renderer )
	{
		// Get texture width and height from the profile.
		int width = mlt_properties_get_int( self->properties, "width" );
		int height = mlt_properties_get_int( self->properties, "height" );
		self->sdl_texture = SDL_CreateTexture( self->sdl_renderer, texture_format,
			SDL_TEXTUREACCESS_STREAMING, width, height );
		if ( self->sdl_texture ) {
			SDL_SetRenderDrawColor( self->sdl_renderer, 0, 0, 0, 255);
		} else {
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent), "Failed to create SDL texture: %s\n", SDL_GetError() );
			error = -1;
		}
	} else {
		mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent), "Failed to create SDL renderer: %s\n", SDL_GetError() );
		error = -1;
	}
	pthread_mutex_unlock( &mlt_sdl_mutex );

	return error;
}

static int consumer_play_video( consumer_sdl self, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = self->properties;

#ifdef MLT_IMAGE_FORMAT
	mlt_image_format vfmt = mlt_properties_get_int( properties, "mlt_image_format" );
#else
	mlt_image_format vfmt = mlt_image_yuv422;
#endif
	int width = self->width, height = self->height;
	uint8_t *image;

	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	int display_off = video_off | preview_off;

	if ( self->running && !display_off )
	{
		// Get the image, width and height
		mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );

		if ( self->running )
		{
			// Determine window's new display aspect ratio
			int x = mlt_properties_get_int( properties, "window_width" );
			if ( x && x != self->window_width )
				self->window_width = x;
			x = mlt_properties_get_int( properties, "window_height" );
			if ( x && x != self->window_height )
				self->window_height = x;
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
				self->sdl_rect.w = frame_aspect / this_aspect * self->window_width;
				self->sdl_rect.h = self->window_height;
				if ( self->sdl_rect.w > self->window_width )
				{
					self->sdl_rect.w = self->window_width;
					self->sdl_rect.h = this_aspect / frame_aspect * self->window_height;
				}
			}
			// Special case optimisation to negate odd effect of sample aspect ratio
			// not corresponding exactly with image resolution.
			else if ( (int)( this_aspect * 1000 ) == (int)( display_ratio * 1000 ) ) 
			{
				self->sdl_rect.w = self->window_width;
				self->sdl_rect.h = self->window_height;
			}
			// Use hardware scaler to normalise sample aspect ratio
			else if ( self->window_height * display_ratio > self->window_width )
			{
				self->sdl_rect.w = self->window_width;
				self->sdl_rect.h = self->window_width / display_ratio;
			}
			else
			{
				self->sdl_rect.w = self->window_height * display_ratio;
				self->sdl_rect.h = self->window_height;
			}
			
			self->sdl_rect.x = ( self->window_width - self->sdl_rect.w ) / 2;
			self->sdl_rect.y = ( self->window_height - self->sdl_rect.h ) / 2;
			self->sdl_rect.x -= self->sdl_rect.x % 2;

			mlt_properties_set_int( self->properties, "rect_x", self->sdl_rect.x );
			mlt_properties_set_int( self->properties, "rect_y", self->sdl_rect.y );
			mlt_properties_set_int( self->properties, "rect_w", self->sdl_rect.w );
			mlt_properties_set_int( self->properties, "rect_h", self->sdl_rect.h );
		}

		if ( self->running && image )
		{
			unsigned char* planes[4];
			int strides[4];

			mlt_image_format_planes( vfmt, width, height, image, planes, strides );
			if ( strides[1] ) {
				SDL_UpdateYUVTexture( self->sdl_texture, NULL,
					planes[0], strides[0],
					planes[1], strides[1],
					planes[2], strides[2] );
			} else {
				SDL_UpdateTexture( self->sdl_texture, NULL, planes[0], strides[0] );
			}
			SDL_RenderClear( self->sdl_renderer );
			SDL_RenderCopy( self->sdl_renderer, self->sdl_texture, NULL, &self->sdl_rect );
			SDL_RenderPresent( self->sdl_renderer );
		}

		mlt_events_fire( properties, "consumer-frame-show", mlt_event_data_from_frame(frame) );
	}
	else if ( self->running )
	{
		if ( !video_off ) {
			mlt_image_format preview_format = mlt_properties_get_int( properties, "preview_format" );
			vfmt = preview_format == mlt_image_none ? mlt_image_rgb24a : preview_format;
			mlt_frame_get_image( frame, &image, &vfmt, &width, &height, 0 );
		}
		mlt_events_fire( properties, "consumer-frame-show", mlt_event_data_from_frame(frame) );
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

	// Determine start time
	gettimeofday( &now, NULL );
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

	// internal initialization
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

	pthread_mutex_lock( &self->audio_mutex );
	self->audio_avail = 0;
	pthread_mutex_unlock( &self->audio_mutex );

	return NULL;
}

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
