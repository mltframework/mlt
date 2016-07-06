/*
 * consumer_jack.c -- a JACK audio consumer
 * Copyright (C) 2011-2014 Meltytech, LLC
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

#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>

#define BUFFER_LEN (204800 * 6)

pthread_mutex_t g_activate_mutex = PTHREAD_MUTEX_INITIALIZER;

/** This classes definition.
*/

typedef struct consumer_jack_s *consumer_jack;

struct consumer_jack_s
{
	struct mlt_consumer_s parent;
	jack_client_t *jack;
	mlt_deque queue;
	pthread_t thread;
	int joined;
	int running;
	pthread_mutex_t video_mutex;
	pthread_cond_t video_cond;
	int playing;

	pthread_cond_t refresh_cond;
	pthread_mutex_t refresh_mutex;
	int refresh_count;
	int counter;
	jack_ringbuffer_t **ringbuffers;
	jack_port_t **ports;
};

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static void consumer_refresh_cb( mlt_consumer sdl, mlt_consumer parent, char *name );
static int jack_process( jack_nframes_t frames, void * data );

/** Constructor
*/

mlt_consumer consumer_jack_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	consumer_jack self = calloc( 1, sizeof( struct consumer_jack_s ) );

	// If no malloc'd and consumer init ok
	if ( self != NULL && mlt_consumer_init( &self->parent, self, profile ) == 0 )
	{
		char name[14];

		snprintf( name, sizeof( name ), "mlt%d", getpid() );
		if (( self->jack = jack_client_open( name, JackNullOption, NULL ) ))
		{
			jack_set_process_callback( self->jack, jack_process, self );

			// Create the queue
			self->queue = mlt_deque_init( );

			// Get the parent consumer object
			mlt_consumer parent = &self->parent;

			// We have stuff to clean up, so override the close method
			parent->close = consumer_close;

			// get a handle on properties
			mlt_service service = MLT_CONSUMER_SERVICE( parent );
			mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

			// This is the initialisation of the consumer
			pthread_mutex_init( &self->video_mutex, NULL );
			pthread_cond_init( &self->video_cond, NULL);

			// Default scaler (for now we'll use nearest)
			mlt_properties_set( properties, "rescale", "nearest" );
			mlt_properties_set( properties, "deinterlace_method", "onefield" );

			// Default buffer for low latency
			mlt_properties_set_int( properties, "buffer", 1 );

			// Set frequency from JACK
			mlt_properties_set_int( properties, "frequency", (int) jack_get_sample_rate( self->jack ) );

			// Set default volume
			mlt_properties_set_double( properties, "volume", 1.0 );

			// Ensure we don't join on a non-running object
			self->joined = 1;

			// Allow thread to be started/stopped
			parent->start = consumer_start;
			parent->stop = consumer_stop;
			parent->is_stopped = consumer_is_stopped;

			// Initialize the refresh handler
			pthread_cond_init( &self->refresh_cond, NULL );
			pthread_mutex_init( &self->refresh_mutex, NULL );
			mlt_events_listen( MLT_CONSUMER_PROPERTIES( parent ), self, "property-changed", ( mlt_listener )consumer_refresh_cb );

			// Return the consumer produced
			return parent;
		}
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
		consumer_jack self = parent->child;
		pthread_mutex_lock( &self->refresh_mutex );
		self->refresh_count = self->refresh_count <= 0 ? 1 : self->refresh_count + 1;
		pthread_cond_broadcast( &self->refresh_cond );
		pthread_mutex_unlock( &self->refresh_mutex );
	}
}

static int consumer_start( mlt_consumer parent )
{
	consumer_jack self = parent->child;

	if ( !self->running )
	{
		consumer_stop( parent );
		self->running = 1;
		self->joined = 0;
		pthread_create( &self->thread, NULL, consumer_thread, self );
	}

	return 0;
}

static int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_jack self = parent->child;

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

		// Cleanup JACK
		if ( self->playing )
			jack_deactivate( self->jack );
		if ( self->ringbuffers )
		{
			int n = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( parent ), "channels" );
			while ( n-- )
			{
				jack_ringbuffer_free( self->ringbuffers[n] );
				jack_port_unregister( self->jack, self->ports[n] );
			}
			mlt_pool_release( self->ringbuffers );
		}
		self->ringbuffers = NULL;
		if ( self->ports )
			mlt_pool_release( self->ports );
		self->ports = NULL;
	}

	return 0;
}

static int consumer_is_stopped( mlt_consumer parent )
{
	consumer_jack self = parent->child;
	return !self->running;
}

static int jack_process( jack_nframes_t frames, void * data )
{
	int error = 0;
	consumer_jack self = (consumer_jack) data;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( &self->parent );
	int channels = mlt_properties_get_int( properties, "channels" );
	int i;

	if ( !self->ringbuffers )
		return 1;

	for ( i = 0; i < channels; i++ )
	{
		size_t jack_size = ( frames * sizeof(float) );
		size_t ring_size = jack_ringbuffer_read_space( self->ringbuffers[i] );
		char *dest = jack_port_get_buffer( self->ports[i], frames );

		jack_ringbuffer_read( self->ringbuffers[i], dest, ring_size < jack_size ? ring_size : jack_size );
		if ( ring_size < jack_size )
			memset( dest + ring_size, 0, jack_size - ring_size );
	}

	return error;
}

static void initialise_jack_ports( consumer_jack self )
{
	int i;
	char mlt_name[20], con_name[30];
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( &self->parent );
	const char **ports = NULL;

	// Propogate these for the Jack processing callback
	int channels = mlt_properties_get_int( properties, "channels" );

	// Allocate buffers and ports
	self->ringbuffers = mlt_pool_alloc( sizeof( jack_ringbuffer_t *) * channels );
	self->ports = mlt_pool_alloc( sizeof(jack_port_t *) * channels );

	// Start Jack processing - required before registering ports
	pthread_mutex_lock( &g_activate_mutex );
	jack_activate( self->jack );
	pthread_mutex_unlock( &g_activate_mutex );
	self->playing = 1;

	// Register Jack ports
	for ( i = 0; i < channels; i++ )
	{
		self->ringbuffers[i] = jack_ringbuffer_create( BUFFER_LEN * sizeof(float) );
		snprintf( mlt_name, sizeof( mlt_name ), "out_%d", i + 1 );
		self->ports[i] = jack_port_register( self->jack, mlt_name, JACK_DEFAULT_AUDIO_TYPE,
				JackPortIsOutput | JackPortIsTerminal, 0 );
	}

	// Establish connections
	for ( i = 0; i < channels; i++ )
	{
		snprintf( mlt_name, sizeof( mlt_name ), "%s", jack_port_name( self->ports[i] ) );
		if ( mlt_properties_get( properties, con_name ) )
			snprintf( con_name, sizeof( con_name ), "%s", mlt_properties_get( properties, con_name ) );
		else
		{
			if ( !ports )
				ports = jack_get_ports( self->jack, NULL, NULL, JackPortIsPhysical | JackPortIsInput );
			if ( ports )
				strncpy( con_name, ports[i], sizeof( con_name ));
			else
				snprintf( con_name, sizeof( con_name ), "system:playback_%d", i + 1);
			con_name[ sizeof( con_name ) - 1 ] = '\0';
		}
		mlt_log_verbose( NULL, "JACK connect %s to %s\n", mlt_name, con_name );
		jack_connect( self->jack, mlt_name, con_name );
	}
	if ( ports )
		jack_free( ports );
}

static int consumer_play_audio( consumer_jack self, mlt_frame frame, int init_audio, int *duration )
{
	// Get the properties of this consumer
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( &self->parent );
	mlt_audio_format afmt = mlt_audio_float;

	// Set the preferred params of the test card signal
	double speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), "_speed" );
	int channels = mlt_properties_get_int( properties, "channels" );
	int frequency = mlt_properties_get_int( properties, "frequency" );
	int scrub = mlt_properties_get_int( properties, "scrub_audio" );
	int samples = mlt_sample_calculator( mlt_properties_get_double( properties, "fps" ), frequency, self->counter++ );
	float *buffer;

	mlt_frame_get_audio( frame, (void**) &buffer, &afmt, &frequency, &channels, &samples );
	*duration = ( ( samples * 1000 ) / frequency );

	if ( mlt_properties_get_int( properties, "audio_off" ) )
	{
		init_audio = 1;
		return init_audio;
	}

	if ( init_audio == 1 )
	{
		self->playing = 0;
		initialise_jack_ports( self );
		init_audio = 0;
	}

	if ( init_audio == 0 && ( speed == 1.0 || speed == 0.0 ) )
	{
		int i;
		size_t mlt_size = samples * sizeof(float);
		float volume = mlt_properties_get_double( properties, "volume" );

		if ( !scrub && speed == 0.0 )
			volume = 0.0;

		if ( volume != 1.0 )
		{
			float *p = buffer;
			i = samples * channels + 1;
			while (--i)
				*p++ *= volume;
		}

		// Write into output ringbuffer
		for ( i = 0; i < channels; i++ )
		{
			size_t ring_size = jack_ringbuffer_write_space( self->ringbuffers[i] );
			if ( ring_size >= mlt_size )
				jack_ringbuffer_write( self->ringbuffers[i], (char*)( buffer + i * samples ), mlt_size );
		}
	}

	return init_audio;
}

static int consumer_play_video( consumer_jack self, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( &self->parent );
	if ( self->running && !mlt_consumer_is_stopped( &self->parent ) )
		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

	return 0;
}

static void *video_thread( void *arg )
{
	// Identify the arg
	consumer_jack self = arg;

	// Obtain time of thread start
	struct timeval now;
	int64_t start = 0;
	int64_t elapsed = 0;
	struct timespec tm;
	mlt_frame next = NULL;
	mlt_properties properties = NULL;
	double speed = 0;

	// Get real time flag
	int real_time = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( &self->parent ), "real_time" );

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
	consumer_jack self = arg;

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

			// Get refresh request for the current frame
			int refresh = mlt_properties_get_int( consumer_props, "refresh" );

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
				mlt_deque_push_back( self->queue, frame );
				pthread_cond_broadcast( &self->video_cond );
				pthread_mutex_unlock( &self->video_mutex );

				// Calculate the next playtime
				playtime += ( duration * 1000 );
			}
			else if ( self->running )
			{
				pthread_mutex_lock( &self->refresh_mutex );
				if ( refresh == 0 && self->refresh_count <= 0 )
				{
					consumer_play_video( self, frame );
					pthread_cond_wait( &self->refresh_cond, &self->refresh_mutex );
				}
				mlt_frame_close( frame );
				self->refresh_count --;
				pthread_mutex_unlock( &self->refresh_mutex );
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
		pthread_mutex_lock( &self->video_mutex );
		pthread_cond_broadcast( &self->video_cond );
		pthread_mutex_unlock( &self->video_mutex );
		pthread_join( thread, NULL );
	}

	while( mlt_deque_count( self->queue ) )
		mlt_frame_close( mlt_deque_pop_back( self->queue ) );

	return NULL;
}

/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_jack self = parent->child;

	// Stop the consumer
	mlt_consumer_stop( parent );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the queue
	mlt_deque_close( self->queue );

	// Destroy mutexes
	pthread_mutex_destroy( &self->video_mutex );
	pthread_cond_destroy( &self->video_cond );
	pthread_mutex_destroy( &self->refresh_mutex );
	pthread_cond_destroy( &self->refresh_cond );

	// Disconnect from JACK
	jack_client_close( self->jack );

	// Finally deallocate self
	free( self );
}
