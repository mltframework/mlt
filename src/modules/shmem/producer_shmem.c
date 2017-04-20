/*
 * producer_shmem.c -- input through shared memory interface
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
	struct mlt_producer_s parent;
	int f_running, f_exit, f_timeout;
	char* arg;
	pthread_t th;
	shmem_xchg_t* shared;
} producer_shmem_t;

static int get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	void *src;
	size_t size;
	mlt_properties fprops = MLT_FRAME_PROPERTIES( frame );
	shmem_frame_t* pshf = mlt_properties_get_data( fprops, "shf", NULL );

	src = shmem_block_attach( pshf->shmid.audio, &size );
	mlt_frame_set_audio( frame, (uint8_t*) src, pshf->aformat, size, (mlt_destructor)shmem_block_detach_destroy );

	*buffer = (int16_t *)src;
	*format = pshf->aformat;
	*frequency = pshf->frequency;
	*channels = pshf->channels;
	*samples = pshf->samples;

	return 0;
}

static int get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	void *src;
	size_t size;
	mlt_properties fprops = MLT_FRAME_PROPERTIES( frame );
	shmem_frame_t* pshf = mlt_properties_get_data( fprops, "shf", NULL );

	src = shmem_block_attach( pshf->shmid.image, &size );
	mlt_frame_set_image( frame, (uint8_t*) src, size, (mlt_destructor)shmem_block_detach_destroy );

	*buffer = (uint8_t *)src;
	*format = pshf->iformat;
	*width = pshf->width;
	*height = pshf->height;

	if ( pshf->shmid.alpha )
	{
		src = shmem_block_attach( pshf->shmid.alpha, &size );
		mlt_frame_set_alpha( frame, (uint8_t*) src, size, (mlt_destructor)shmem_block_detach_destroy );
	}

	return 0;
}


static int get_frame( mlt_producer producer, mlt_frame_ptr pframe, int index )
{
	int i, f_present = 0;
	shmem_frame_t shf;
	struct timeval now;
	struct timespec tm;
	double fps = mlt_producer_get_fps( producer );
	mlt_position position = mlt_producer_position( producer );
	producer_shmem_t* self = ( producer_shmem_t* )producer->child;
	mlt_frame frame;

	// Wait if queue is empty
	pthread_mutex_lock( &self->shared->lock );
	while ( !self->f_timeout && !self->shared->queue.len )
	{
		// Wait up to twice frame duration
		gettimeofday( &now, NULL );
		uint64_t usec = now.tv_sec * 1000000 + now.tv_usec;
		usec += 2000000LL / fps;
		tm.tv_sec = usec / 1000000LL;
		tm.tv_nsec = (usec % 1000000LL) * 1000LL;
		if ( pthread_cond_timedwait( &self->shared->cond, &self->shared->lock, &tm ) )
		// Stop waiting if error (timed out)
			break;
	}
	mlt_log_debug( producer, "%s:%d self->shared->queue.len=%d\n",
		__FUNCTION__, __LINE__, self->shared->queue.len );
	if ( self->shared->queue.len )
	{
		f_present = 1;
		shf = self->shared->queue.frames[ 0 ];
		for( i = 0; i < ( SHMEM_QUEUE_SIZE -  1 ); i++ )
			self->shared->queue.frames[ i ] = self->shared->queue.frames[ i + 1 ];
		self->shared->queue.len--;
		pthread_cond_signal( &self->shared->cond );
	};
	pthread_mutex_unlock( &self->shared->lock );

	*pframe = frame = mlt_frame_init( MLT_PRODUCER_SERVICE(producer) );

	if ( f_present )
	{
		shmem_frame_t* pshf = (shmem_frame_t*) mlt_pool_alloc( sizeof( shmem_frame_t ) );

		*pshf = shf;

		mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "shf", (void *)pshf, 0, mlt_pool_release, NULL );

		// Add audio and video getters
		if ( shf.shmid.audio )
			mlt_frame_push_audio( frame, (void*) get_audio );
		if ( shf.shmid.image )
			mlt_frame_push_get_image( frame, get_image );

		mlt_frame_set_position( frame, position );

		self->f_timeout = 0;
	}
	else
		self->f_timeout++;

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_shmem_close( mlt_producer producer )
{
	free( producer->child );
	producer->close = NULL;
	mlt_producer_close( producer );
}

/** Initialise the producer.
 */
mlt_producer producer_shmem_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// allocate/attach to shared region
	shmem_xchg_t* ctx = shmem_xchg_create(arg);
	if ( !ctx )
		return NULL;

	// Allocate the consumer
	producer_shmem_t* self = ( producer_shmem_t* )calloc( 1, sizeof( producer_shmem_t ) );

	mlt_log_debug( NULL, "%s: entering id=[%s], arg=[%s]\n", __FUNCTION__, id, arg );

	// If allocated
	if ( self && !mlt_producer_init( &self->parent, self ) )
	{
		mlt_producer parent = &self->parent;
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );

		// Setup context
		self->arg = strdup( arg );
		self->shared = ctx;

		// Set callbacks
		parent->close = (mlt_destructor) producer_shmem_close;
		parent->get_frame = get_frame;

		// These properties effectively make it infinite.
		mlt_properties_set_int( properties, "length", INT_MAX );
		mlt_properties_set_int( properties, "out", INT_MAX - 1 );
		mlt_properties_set( properties, "eof", "loop" );

		return parent;
	}

	free( self );

	return NULL;
}
