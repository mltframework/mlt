/*
 * consumer_shmem.c -- output through shared memory interface
 * Copyright (C) 2016 Maksym Veremeyenko <verem@m1stereo.tv>
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_profile.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include "factory.h"

typedef struct
{
	struct mlt_consumer_s parent;
	int f_running, f_exit;
	char* arg;
	pthread_t th;
	shmem_xchg_t* shared;
} consumer_shmem_t;

static void* consumer_shmem_feeder( void* p )
{
	int r;
	mlt_consumer consumer = p;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	consumer_shmem_t* self = ( consumer_shmem_t* )consumer->child;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	while ( !self->f_exit )
	{
		mlt_frame frm = NULL;

		pthread_mutex_lock( &self->shared->lock );

		while ( !self->f_exit && self->shared->queue.len >= SHMEM_QUEUE_SIZE )
		{
			struct timespec ts;
			struct timeval tv;
			gettimeofday( &tv, NULL );

			tv.tv_usec += 10000LL;
			ts.tv_nsec = 1000LL * ( tv.tv_usec % 1000000LL );
			ts.tv_sec = tv.tv_sec + tv.tv_usec / 1000000LL;

			pthread_cond_timedwait( &self->shared->cond, &self->shared->lock, &ts );
		}

		pthread_mutex_unlock( &self->shared->lock );

		if ( self->f_exit || self->shared->queue.len >= SHMEM_QUEUE_SIZE )
			continue;

		frm = mlt_consumer_rt_frame( consumer );
		if ( frm )
		{
			shmem_frame_t shf;
			void *src, *dst;
			mlt_properties fprops = MLT_FRAME_PROPERTIES( frm );

			shf.width = 0;
			shf.height = 0;
			shf.shmid.image = 0;
			shf.shmid.alpha = 0;
			shf.shmid.audio = 0;

			shf.frequency = mlt_properties_get_int( fprops, "frequency" );

			shf.channels = mlt_properties_get_int( properties, "channels" )
				? mlt_properties_get_int( properties, "channels" )
				: mlt_properties_get_int( fprops, "channels" );

			shf.samples = 0;

			shf.iformat = mlt_properties_get_int( fprops, "keyer" )
				? mlt_image_rgb24a
				: mlt_image_yuv422;

			shf.aformat = mlt_audio_s16;

			if ( mlt_properties_get_int( fprops, "rendered" ) &&
				!mlt_frame_get_image( frm, (uint8_t **)&src, &shf.iformat, &shf.width, &shf.height, 0 ) )
			{
				mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s:%d: width=%d, height=%d\n",
					__FUNCTION__, __LINE__, shf.width, shf.height );

				// find if it is already shared memory attached
				r = shmem_block_lookup(src);
				if ( r > 0 )
				{
					// set image to NULL
					mlt_frame_set_image( frm, NULL, 0, NULL );

					// save shared mem id
					shf.shmid.image = r;

					// detach
					shmem_block_detach( src );
				}
				else
				{
					// get image size
					int image_size = mlt_image_format_size( shf.iformat, shf.width, shf.height, NULL );

					// create shared block and attach
					shf.shmid.image = shmem_block_create_attach( image_size, &dst );

					// copy video data
					memcpy( dst, src, image_size );

					// detach
					shmem_block_detach( dst );
				}

				src = mlt_frame_get_alpha( frm );
				if ( src )
				{
					// find if it is already shared memory attached
					r = shmem_block_lookup(src);
					if ( r > 0 )
					{
						// set image to NULL
						mlt_frame_set_alpha( frm, NULL, 0, NULL );

						// save shared mem id
						shf.shmid.alpha = r;

						// detach
						shmem_block_detach( src );
					}
					else
					{
						// get image size
						int alpha_size = shf.width * shf.height;

						// create shared block and attach
						shf.shmid.alpha = shmem_block_create_attach( alpha_size, &dst );

						// copy video data
						memcpy( dst, src, alpha_size );

						// detach
						shmem_block_detach( dst );
					}
				}

				if ( !mlt_frame_get_audio( frm, &src, &shf.aformat, &shf.frequency, &shf.channels, &shf.samples ) )
				{
					// find if it is already shared memory attached
					r = shmem_block_lookup(src);
					if ( r > 0 )
					{
						// set audio to NULL
						mlt_frame_set_audio( frm, NULL, 0, 0, NULL );

						// save shared mem id
						shf.shmid.audio = r;

						// detach
						shmem_block_detach( src );
					}
					else
					{
						// get audio size
						int audio_size = mlt_audio_format_size( shf.aformat, shf.samples, shf.channels );

						// create shared block and attach
						shf.shmid.audio = shmem_block_create_attach( audio_size, &dst );

						// copy video data
						memcpy( dst, src, audio_size );

						// detach
						shmem_block_detach( dst );
					}
				}
			}

			/* put frame into queue */
			pthread_mutex_lock( &self->shared->lock );
			self->shared->queue.frames[ self->shared->queue.len ] = shf;
			self->shared->queue.len++;
			mlt_log_debug( consumer, "%s:%d: self->shared->queue.len=%d\n",
				__FUNCTION__, __LINE__, self->shared->queue.len );
			pthread_cond_signal( &self->shared->cond );
			pthread_mutex_unlock( &self->shared->lock );
		}

		if ( frm )
		{
			mlt_events_fire( properties, "consumer-frame-show", frm, NULL );
			mlt_frame_close( frm );
		}
	}

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );

	return NULL;
}

static int consumer_shmem_start( mlt_consumer consumer )
{
	int r = 0;

	mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "%s: entering\n", __FUNCTION__ );

	consumer_shmem_t* self = ( consumer_shmem_t* )consumer->child;

	if ( !self->f_running )
	{
		// set flags
		self->f_exit = 0;

		pthread_create( &self->th, NULL, consumer_shmem_feeder, consumer );

		// set flags
		self->f_running = 1;
	}

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );

	return r;
}

static int consumer_shmem_stop( mlt_consumer consumer )
{
	int r = 0;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	consumer_shmem_t* self = ( consumer_shmem_t* )consumer->child;

	if ( self->f_running )
	{
		// rise flags
		self->f_exit = 1;

		// wait for thread
		pthread_join( self->th, NULL );

		// hide flags
		self->f_running = 0;
	}

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );

	return r;
}

static int consumer_shmem_is_stopped( mlt_consumer consumer )
{
	consumer_shmem_t* self = ( consumer_shmem_t* )consumer->child;
	return !self->f_running;
}

static void consumer_shmem_close( mlt_consumer consumer )
{
	consumer_shmem_t* self = ( consumer_shmem_t* )consumer->child;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	// Stop the consumer
	mlt_consumer_stop( consumer );

	// Close the parent
	consumer->close = NULL;
	mlt_consumer_close( consumer );

	// free context
	if ( self->arg )
		free( self->arg );

	free( self );

	mlt_log_debug( NULL, "%s: exiting\n", __FUNCTION__ );
}

/** Initialise the consumer.
 */
mlt_consumer consumer_shmem_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// allocate/attach to shared region
	shmem_xchg_t* ctx = shmem_xchg_create(arg);
	if ( !ctx )
		return NULL;

	// Allocate the consumer
	consumer_shmem_t* self = ( consumer_shmem_t* )calloc( 1, sizeof( consumer_shmem_t ) );

	mlt_log_debug( NULL, "%s: entering id=[%s], arg=[%s]\n", __FUNCTION__, id, arg );

	// If allocated
	if ( self && !mlt_consumer_init( &self->parent, self, profile ) )
	{
		mlt_consumer parent = &self->parent;

		// Setup context
		self->arg = strdup( arg );
		self->shared = ctx;

		// Setup callbacks
		parent->close = consumer_shmem_close;
		parent->start = consumer_shmem_start;
		parent->stop = consumer_shmem_stop;
		parent->is_stopped = consumer_shmem_is_stopped;

		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		mlt_properties_set( properties, "deinterlace_method", "onefield" );

		return parent;
	}

	free( self );

	return NULL;
}
