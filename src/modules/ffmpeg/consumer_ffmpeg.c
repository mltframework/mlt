/*
 * consumer_ffmpeg.c -- an ffmpeg consumer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "consumer_ffmpeg.h"
#include <framework/mlt_frame.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/** This classes definition.
*/

typedef struct consumer_ffmpeg_s *consumer_ffmpeg;

struct consumer_ffmpeg_s
{
	struct mlt_consumer_s parent;
	mlt_properties properties;
	int format;
	int video;
	pthread_t thread;
	int running;
	uint8_t audio_buffer[ 4096 * 3 ];
	int audio_avail;
	pthread_mutex_t audio_mutex;
	pthread_cond_t audio_cond;
	int window_width;
	int window_height;
	float aspect_ratio;
	int width;
	int height;
	int playing;
	mlt_frame *queue;
	int size;
	int count;
	uint8_t *buffer;
};

/** Forward references to static functions.
*/

static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_ffmpeg_init( char *arg )
{
	// Create the consumer object
	consumer_ffmpeg this = calloc( sizeof( struct consumer_ffmpeg_s ), 1 );

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

		// This is the initialisation of the consumer
		this->running = 1;
		pthread_mutex_init( &this->audio_mutex, NULL );
		pthread_cond_init( &this->audio_cond, NULL);
		
		// process actual param
		if ( arg == NULL || !strcmp( arg, "-" ) )
		{
			mlt_properties_set( this->properties, "video_file", "-" );
			mlt_properties_set( this->properties, "video_format", "dv" );
		}
		else
		{
			mlt_properties_set( this->properties, "video_file", arg );
			mlt_properties_set( this->properties, "video_format", "" );
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

static void sdl_fill_audio( void *udata, uint8_t *stream, int len )
{
	consumer_ffmpeg this = udata;

	pthread_mutex_lock( &this->audio_mutex );

	// Block until audio received
	while ( this->running && len > this->audio_avail )
		pthread_cond_wait( &this->audio_cond, &this->audio_mutex );

	if ( this->audio_avail >= len )
	{
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

		// No audio left
		this->audio_avail = 0;
	}

	pthread_cond_broadcast( &this->audio_cond );
	pthread_mutex_unlock( &this->audio_mutex );
}

static int consumer_play_audio( consumer_ffmpeg this, mlt_frame frame, int init_audio )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;
	mlt_audio_format afmt = mlt_audio_pcm;
	int channels;
	int samples;
	int frequency;
	int16_t *pcm;
	int bytes;

	mlt_frame_get_audio( frame, &pcm, &afmt, &frequency, &channels, &samples );

	if ( mlt_properties_get_int( properties, "audio_off" ) )
		return init_audio;

	if ( init_audio == 0 )
	{
		bytes = ( samples * channels * 2 );
		pthread_mutex_lock( &this->audio_mutex );
		while ( bytes > ( sizeof( this->audio_buffer) - this->audio_avail ) )
			pthread_cond_wait( &this->audio_cond, &this->audio_mutex );
		mlt_properties properties = mlt_frame_properties( frame );
		if ( mlt_properties_get_double( properties, "speed" ) == 1 )
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

static int consumer_play_video( consumer_ffmpeg this, mlt_frame frame )
{
	// Get the properties of this consumer
	mlt_properties properties = this->properties;

	if ( mlt_properties_get_int( properties, "video_off" ) )
	{
		mlt_frame_close( frame );
		return 0;
	}

	if ( this->count == this->size )
	{
		this->size += 25;
		this->queue = realloc( this->queue, sizeof( mlt_frame ) * this->size );
	}
	this->queue[ this->count ++ ] = frame;

	// We're working on the oldest frame now
	frame = this->queue[ 0 ];

	// Shunt the frames in the queue down
	int i = 0;
	for ( i = 1; i < this->count; i ++ )
		this->queue[ i - 1 ] = this->queue[ i ];
	this->count --;

	return 0;
}

/** Threaded wrapper for pipe.
*/

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_ffmpeg this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;

	// Get the service assoicated to the consumer
	mlt_service service = mlt_consumer_service( consumer );

	// Define a frame pointer
	mlt_frame frame;

	// internal intialization
	int init_audio = 1;

	// Loop until told not to
	while( this->running )
	{
		// Get a frame from the service (should never return anything other than 0)
		if ( mlt_service_get_frame( service, &frame, 0 ) == 0 )
		{
			init_audio = consumer_play_audio( this, frame, init_audio );
			consumer_play_video( this, frame );
		}
	}

	return NULL;
}

/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_ffmpeg this = parent->child;

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

