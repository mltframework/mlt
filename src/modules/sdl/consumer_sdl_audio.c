/*
 * consumer_sdl_audio.c -- A Simple DirectMedia Layer audio-only consumer
 * Copyright (C) 2009, 2010 Ushodaya Enterprises Limited
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
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer this, char *name );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_sdl_audio_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
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
		mlt_properties_set( this->properties, "deinterlace_method", "onefield" );

		// Default buffer for low latency
		mlt_properties_set_int( this->properties, "buffer", 1 );

		// Default audio buffer
		mlt_properties_set_int( this->properties, "audio_buffer", 2048 );

		// Ensure we don't join on a non-running object
		this->joined = 1;
		
		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;

		// Initialize the refresh handler
		pthread_cond_init( &this->refresh_cond, NULL );
		pthread_mutex_init( &this->refresh_mutex, NULL );
		mlt_events_listen( MLT_CONSUMER_PROPERTIES( parent ), this, "property-changed", ( mlt_listener )consumer_refresh_cb );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer parent, char *name )
{
	if ( !strcmp( name, "refresh" ) )
	{
		consumer_sdl this = parent->child;
		pthread_mutex_lock( &this->refresh_mutex );
		this->refresh_count = this->refresh_count <= 0 ? 1 : this->refresh_count + 1;
		pthread_cond_broadcast( &this->refresh_cond );
		pthread_mutex_unlock( &this->refresh_mutex );
	}
}

int consumer_start( mlt_consumer parent )
{
	consumer_sdl this = parent->child;

	if ( !this->running )
	{
		consumer_stop( parent );

		pthread_mutex_lock( &mlt_sdl_mutex );
		int ret = SDL_Init( SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE );
		pthread_mutex_unlock( &mlt_sdl_mutex );
		if ( ret < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(parent), "Failed to initialize SDL: %s\n", SDL_GetError() );
			return -1;
		}

		this->running = 1;
		this->joined = 0;
		pthread_create( &this->thread, NULL, consumer_thread, this );
	}

	return 0;
}

int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_sdl this = parent->child;

	if ( this->running && !this->joined )
	{
		// Kill the thread and clean up
		this->joined = 1;
		this->running = 0;

		// Unlatch the consumer thread
		pthread_mutex_lock( &this->refresh_mutex );
		pthread_cond_broadcast( &this->refresh_cond );
		pthread_mutex_unlock( &this->refresh_mutex );

		// Cleanup the main thread
#ifndef WIN32
		if ( this->thread )
#endif
			pthread_join( this->thread, NULL );

		// Unlatch the video thread
		pthread_mutex_lock( &this->video_mutex );
		pthread_cond_broadcast( &this->video_cond );
		pthread_mutex_unlock( &this->video_mutex );

		// Unlatch the audio callback
		pthread_mutex_lock( &this->audio_mutex );
		pthread_cond_broadcast( &this->audio_cond );
		pthread_mutex_unlock( &this->audio_mutex );

		if ( this->playing )
			SDL_QuitSubSystem( SDL_INIT_AUDIO );
	}

	return 0;
}

int consumer_is_stopped( mlt_consumer parent )
{
	consumer_sdl this = parent->child;
	return !this->running;
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
	int frequency = mlt_properties_get_int( properties, "frequency" );
	static int counter = 0;

	int samples = mlt_sample_calculator( mlt_properties_get_double( this->properties, "fps" ), frequency, counter++ );
	
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, (void**) &pcm, &afmt, &frequency, &channels, &samples );
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

		int audio_buffer = mlt_properties_get_int( properties, "audio_buffer" );

		// specify audio format
		memset( &request, 0, sizeof( SDL_AudioSpec ) );
		this->playing = 0;
		request.freq = frequency;
		request.format = AUDIO_S16SYS;
		request.channels = channels;
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
		bytes = ( samples * channels * 2 );
		pthread_mutex_lock( &this->audio_mutex );
		while ( this->running && bytes > ( sizeof( this->audio_buffer) - this->audio_avail ) )
			pthread_cond_wait( &this->audio_cond, &this->audio_mutex );
		if ( this->running )
		{
			if ( mlt_properties_get_double( properties, "_speed" ) == 1 )
				memcpy( &this->audio_buffer[ this->audio_avail ], pcm, bytes );
			else
				memset( &this->audio_buffer[ this->audio_avail ], 0, bytes );
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
	if ( this->running && !mlt_consumer_is_stopped( &this->parent ) )
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

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
	this->refresh_count = 0;

	// Loop until told not to
	while( this->running )
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

			// Get refresh request for the current frame
			int refresh = mlt_properties_get_int( consumer_props, "refresh" );

			// Clear refresh
			mlt_events_block( consumer_props, consumer_props );
			mlt_properties_set_int( consumer_props, "refresh", 0 );
			mlt_events_unblock( consumer_props, consumer_props );

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

			while ( this->running && speed != 0 && mlt_deque_count( this->queue ) > 15 )
				nanosleep( &tm, NULL );

			// Push this frame to the back of the queue
			if ( this->running && speed )
			{
				pthread_mutex_lock( &this->video_mutex );
				mlt_deque_push_back( this->queue, frame );
				pthread_cond_broadcast( &this->video_cond );
				pthread_mutex_unlock( &this->video_mutex );

				// Calculate the next playtime
				playtime += ( duration * 1000 );
			}
			else if ( this->running )
			{
				pthread_mutex_lock( &this->refresh_mutex );
				if ( refresh == 0 && this->refresh_count <= 0 )
				{
					consumer_play_video( this, frame );
					pthread_cond_wait( &this->refresh_cond, &this->refresh_mutex );
				}
				mlt_frame_close( frame );
				this->refresh_count --;
				pthread_mutex_unlock( &this->refresh_mutex );
			}
			else
			{
				mlt_frame_close( frame );
				frame = NULL;
			}

			// Optimisation to reduce latency
			if ( frame && speed == 1.0 )
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
		pthread_mutex_lock( &this->video_mutex );
		pthread_cond_broadcast( &this->video_cond );
		pthread_mutex_unlock( &this->video_mutex );
		pthread_join( thread, NULL );
	}

	while( mlt_deque_count( this->queue ) )
		mlt_frame_close( mlt_deque_pop_back( this->queue ) );

	this->audio_avail = 0;

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

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the queue
	mlt_deque_close( this->queue );

	// Destroy mutexes
	pthread_mutex_destroy( &this->audio_mutex );
	pthread_cond_destroy( &this->audio_cond );
	pthread_mutex_destroy( &this->video_mutex );
	pthread_cond_destroy( &this->video_cond );
	pthread_mutex_destroy( &this->refresh_mutex );
	pthread_cond_destroy( &this->refresh_cond );

	// Finally clean up this
	free( this );
}
