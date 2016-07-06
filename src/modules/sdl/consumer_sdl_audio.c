/*
 * consumer_sdl_audio.c -- A Simple DirectMedia Layer audio-only consumer
 * Copyright (C) 2009-2014 Meltytech, LLC
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
	int playing;

	pthread_cond_t refresh_cond;
	pthread_mutex_t refresh_mutex;
	int refresh_count;
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
static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer self, char *name );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_sdl_audio_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
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
		
		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;
		parent->purge = consumer_purge;

		// Initialize the refresh handler
		pthread_cond_init( &self->refresh_cond, NULL );
		pthread_mutex_init( &self->refresh_mutex, NULL );
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( parent ), self, "property-changed", ( mlt_listener )consumer_refresh_cb );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( self );

	// Indicate failure
	return NULL;
}

static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer parent, char *name )
{
	if ( !strcmp( name, "refresh" ) )
	{
		consumer_sdl self = parent->child;
		pthread_mutex_lock( &self->refresh_mutex );
		if ( self->refresh_count < 2 )
			self->refresh_count = self->refresh_count <= 0 ? 1 : self->refresh_count + 1;
		pthread_cond_broadcast( &self->refresh_cond );
		pthread_mutex_unlock( &self->refresh_mutex );
	}
}

int consumer_start( mlt_consumer parent )
{
	consumer_sdl self = parent->child;

	if ( !self->running )
	{
		consumer_stop( parent );

		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		char *audio_driver = mlt_properties_get( properties, "audio_driver" );
		char *audio_device = mlt_properties_get( properties, "audio_device" );

		if ( audio_driver && strcmp( audio_driver, "" ) )
			setenv( "SDL_AUDIODRIVER", audio_driver, 1 );

		if ( audio_device && strcmp( audio_device, "" ) )
			setenv( "AUDIODEV", audio_device, 1 );

		pthread_mutex_lock( &mlt_sdl_mutex );
		int ret = SDL_Init( SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE );
		pthread_mutex_unlock( &mlt_sdl_mutex );
		if ( ret < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(parent), "Failed to initialize SDL: %s\n", SDL_GetError() );
			return -1;
		}

		self->running = 1;
		self->joined = 0;
		pthread_create( &self->thread, NULL, consumer_thread, self );
	}

	return 0;
}

int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl self = parent->child;

	if ( self->running && !self->joined )
	{
		// Kill the thread and clean up
		self->joined = 1;
		self->running = 0;

		// Unlatch the consumer thread
		pthread_mutex_lock( &self->refresh_mutex );
		pthread_cond_broadcast( &self->refresh_cond );
		pthread_mutex_unlock( &self->refresh_mutex );

		// Cleanup the main thread
#ifndef _WIN32
		if ( self->thread )
#endif
			pthread_join( self->thread, NULL );

		// Unlatch the video thread
		pthread_mutex_lock( &self->video_mutex );
		pthread_cond_broadcast( &self->video_cond );
		pthread_mutex_unlock( &self->video_mutex );

		// Unlatch the audio callback
		pthread_mutex_lock( &self->audio_mutex );
		pthread_cond_broadcast( &self->audio_cond );
		pthread_mutex_unlock( &self->audio_mutex );

		SDL_QuitSubSystem( SDL_INIT_AUDIO );
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
		mlt_frame frame = MLT_FRAME( mlt_deque_peek_back( self->queue ) );
		// When playing rewind or fast forward then we need to keep one
		// frame in the queue to prevent playback stalling.
		double speed = frame? mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), "_speed" ) : 0;
		int n = ( speed == 0.0 || speed == 1.0 ) ? 0 : 1;
		while ( mlt_deque_count( self->queue ) > n )
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

	pthread_mutex_lock( &self->audio_mutex );

	// Block until audio received
#ifdef __APPLE__
	while ( self->running && len > self->audio_avail )
		pthread_cond_wait( &self->audio_cond, &self->audio_mutex );
#endif

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
		SDL_MixAudio( stream, self->audio_buffer, self->audio_avail,
			( int )( ( float )SDL_MIX_MAXVOLUME * volume ) );

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
	// Get the properties of this consumer
	mlt_properties properties = self->properties;
	mlt_audio_format afmt = mlt_audio_s16;

	// Set the preferred params of the test card signal
	int channels = mlt_properties_get_int( properties, "channels" );
	int frequency = mlt_properties_get_int( properties, "frequency" );
	int scrub = mlt_properties_get_int( properties, "scrub_audio" );
	static int counter = 0;

	int samples = mlt_sample_calculator( mlt_properties_get_double( self->properties, "fps" ), frequency, counter++ );
	
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, (void**) &pcm, &afmt, &frequency, &channels, &samples );
	*duration = ( ( samples * 1000 ) / frequency );

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
		request.channels = channels;
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
		bytes = ( samples * channels * 2 );
		pthread_mutex_lock( &self->audio_mutex );
		while ( self->running && bytes > ( sizeof( self->audio_buffer) - self->audio_avail ) )
			pthread_cond_wait( &self->audio_cond, &self->audio_mutex );
		if ( self->running )
		{
			if ( scrub || mlt_properties_get_double( properties, "_speed" ) == 1 )
				memcpy( &self->audio_buffer[ self->audio_avail ], pcm, bytes );
			else
				memset( &self->audio_buffer[ self->audio_avail ], 0, bytes );
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
	mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
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
		if ( mlt_properties_get_int( properties, "rendered" ) == 1 )
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

		// This frame can now be closed
		mlt_frame_close( next );
		next = NULL;
	}

	// This consumer is stopping. But audio has already been played for all
	// the frames in the queue. Spit out all the frames so that the display has
	// the option to catch up with the audio.
	if ( next != NULL ) {
		consumer_play_video( self, next );
		mlt_frame_close( next );
		next = NULL;
	}
	while ( mlt_deque_count( self->queue ) > 0 ) {
		next = mlt_deque_pop_front( self->queue );
		consumer_play_video( self, next );
		mlt_frame_close( next );
		next = NULL;
	}

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

	// Get the properties
	mlt_properties consumer_props = MLT_CONSUMER_PROPERTIES( consumer );

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
//	int last_position = -1;

	pthread_mutex_lock( &self->refresh_mutex );
	self->refresh_count = 0;
	pthread_mutex_unlock( &self->refresh_mutex );

	// Loop until told not to
	while( self->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_rt_frame( consumer );

		// Ensure that we have a frame
		if ( frame )
		{
			// Get the frame properties
			properties =  MLT_FRAME_PROPERTIES( frame );

			// Get the speed of the frame
			double speed = mlt_properties_get_double( properties, "_speed" );

			// Clear refresh
			mlt_events_block( consumer_props, consumer_props );
			mlt_properties_set_int( consumer_props, "refresh", 0 );
			mlt_events_unblock( consumer_props, consumer_props );

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
			mlt_properties_set_int( properties, "playtime", playtime );

			while ( self->running && speed != 0 && mlt_deque_count( self->queue ) > 15 )
				nanosleep( &tm, NULL );

			// Push this frame to the back of the queue
			if ( self->running && speed )
			{
				pthread_mutex_lock( &self->video_mutex );
				if ( self->is_purge && speed == 1.0 )
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
			else if ( self->running )
			{
				pthread_mutex_lock( &self->refresh_mutex );
				consumer_play_video( self, frame );
				mlt_frame_close( frame );
				frame = NULL;
				self->refresh_count --;
				if ( self->refresh_count <= 0 )
				{
					pthread_cond_wait( &self->refresh_cond, &self->refresh_mutex );
				}
				pthread_mutex_unlock( &self->refresh_mutex );
			}

			// Optimisation to reduce latency
			if ( speed == 1.0 )
			{
                // TODO: disabled due to misbehavior on parallel-consumer
//				if ( last_position != -1 && last_position + 1 != mlt_frame_get_position( frame ) )
//					mlt_consumer_purge( consumer );
//				last_position = mlt_frame_get_position( frame );
			}
			else
			{
				mlt_consumer_purge( consumer );
//				last_position = -1;
			}
		}
	}

	// Kill the video thread
	if ( init_video == 0 )
	{
		pthread_mutex_lock( &self->video_mutex );
		pthread_cond_broadcast( &self->video_cond );
		pthread_mutex_unlock( &self->video_mutex );
		pthread_join( thread, NULL );
	}

	if ( frame )
	{
		// The video thread has cleared out the queue. But the audio was played
		// for this frame. So play the video before stopping so the display has
		// the option to catch up with the audio.
		consumer_play_video( self, frame );
		mlt_frame_close( frame );
		frame = NULL;
	}

	self->audio_avail = 0;

	return NULL;
}

/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl self = parent->child;

	// Stop the consumer
	mlt_consumer_stop( parent );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the queue
	mlt_deque_close( self->queue );

	// Destroy mutexes
	pthread_mutex_destroy( &self->audio_mutex );
	pthread_cond_destroy( &self->audio_cond );
	pthread_mutex_destroy( &self->video_mutex );
	pthread_cond_destroy( &self->video_cond );
	pthread_mutex_destroy( &self->refresh_mutex );
	pthread_cond_destroy( &self->refresh_cond );

	// Finally clean up this
	free( self );
}
