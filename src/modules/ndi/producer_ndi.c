/*
 * producer_ndi.c -- output through NewTek NDI
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

#define __STDC_FORMAT_MACROS  /* see inttypes.h */
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>

#include <framework/mlt.h>

#include "Processing.NDI.Lib.h"

#include "factory.h"

typedef struct
{
	mlt_producer parent;
	int f_running, f_exit, f_timeout;
	char* arg;
	pthread_t th;
	int count;
	mlt_deque a_queue, v_queue;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	NDIlib_recv_instance_t recv;
	int v_queue_limit, a_queue_limit;
} producer_ndi_t;

static void* producer_ndi_feeder( void* p )
{
	int i;
	mlt_producer producer = p;
	const NDIlib_source_t* ndi_srcs = NULL;
	NDIlib_video_frame_t* video = NULL;
	NDIlib_audio_frame_t* audio = NULL;
	NDIlib_metadata_frame_t* meta = NULL;
	producer_ndi_t* self = ( producer_ndi_t* )producer->child;

	mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "%s: entering\n", __FUNCTION__ );

	// find the source
	const NDIlib_find_create_t find_create_desc = { true, NULL };

	// create a finder
	NDIlib_find_instance_t ndi_find = NDIlib_find_create( &find_create_desc );
	if ( !ndi_find )
	{
		mlt_log_error( MLT_PRODUCER_SERVICE( producer ), "%s: NDIlib_find_create failed\n", __FUNCTION__ );
		return NULL;
	}

	// wait for source
	mlt_log_debug( MLT_PRODUCER_SERVICE( producer ), "%s: waiting for source [%s]\n", __FUNCTION__, self->arg );
	for ( i = -1; !self->f_exit && -1 == i; )
	{
		unsigned int c = 0, j;

		// wait for sources
		ndi_srcs = NDIlib_find_get_sources( ndi_find, &c, 100 );

		// check if requested src found
		for ( j = 0; j < c && -1 == i; j++ )
			if ( !strcmp( self->arg, ndi_srcs[j].p_ndi_name ) )
				i = j;

		// free if not found
		if ( -1 == i && ndi_srcs )
		{
			ndi_srcs = NULL;
		}
	}

	// exit if nothing
	if ( self->f_exit || -1 == i )
	{
		mlt_log_error( MLT_PRODUCER_SERVICE( producer ), "%s: exiting, self->f_exit=%d, i=%d\n",
			__FUNCTION__, self->f_exit, i );

		NDIlib_find_destroy( ndi_find );

		return NULL;
	}

	mlt_log_debug( MLT_PRODUCER_SERVICE( producer ), "%s: source [%s] found\n", __FUNCTION__, self->arg );

	// create receiver description
	NDIlib_recv_create_t recv_create_desc =
	{
		ndi_srcs[ i ],
		NDIlib_recv_color_format_e_UYVY_RGBA,
		NDIlib_recv_bandwidth_highest,
		true
	};

	// Create the receiver
	self->recv = NDIlib_recv_create2( &recv_create_desc );
	NDIlib_find_destroy( ndi_find );
	if ( !self->recv )
	{

		mlt_log_error( MLT_PRODUCER_SERVICE( producer ), "%s: exiting, NDIlib_recv_create2 failed\n", __FUNCTION__ );
		return 0;
	}

	// set tally
	const NDIlib_tally_t tally_state = { true, false };
	NDIlib_recv_set_tally( self->recv, &tally_state );

	while( !self->f_exit )
	{
		NDIlib_frame_type_e t;

		if ( !video )
			video = mlt_pool_alloc( sizeof( NDIlib_video_frame_t ) );
		if ( !audio )
			audio = mlt_pool_alloc( sizeof( NDIlib_audio_frame_t ) );
		if ( !meta )
			meta = mlt_pool_alloc( sizeof( NDIlib_metadata_frame_t ) );

////fprintf(stderr, "%s:%d: NDIlib_recv_capture....\n", __FUNCTION__, __LINE__ );
		t = NDIlib_recv_capture(self->recv, video, audio, meta, 10 );
////fprintf(stderr, "%s:%d: NDIlib_recv_capture=%d\n", __FUNCTION__, __LINE__, t );

		mlt_log_debug( NULL, "%s:%d: NDIlib_recv_capture=%d\n", __FILE__, __LINE__, t );

		switch( t )
		{
			case NDIlib_frame_type_none:
				break;

			case NDIlib_frame_type_video:
////fprintf(stderr, "%s:%d: locking....\n", __FUNCTION__, __LINE__ );
				pthread_mutex_lock( &self->lock );
////fprintf(stderr, "%s:%d: locked\n", __FUNCTION__, __LINE__ );
				mlt_deque_push_back( self->v_queue, video );
				if ( mlt_deque_count( self->v_queue ) >= self->v_queue_limit )
				{
					video = mlt_deque_pop_front( self->v_queue );
					NDIlib_recv_free_video( self->recv, video );
				}
				else
					video = NULL;
				pthread_cond_broadcast( &self->cond );
				pthread_mutex_unlock( &self->lock );
////fprintf(stderr, "%s:%d: unlocked\n", __FUNCTION__, __LINE__ );
				break;

			case NDIlib_frame_type_audio:
////fprintf(stderr, "%s:%d: locking....\n", __FUNCTION__, __LINE__ );
				pthread_mutex_lock( &self->lock );
////fprintf(stderr, "%s:%d: locked\n", __FUNCTION__, __LINE__ );
				mlt_deque_push_back( self->a_queue, audio );
				if ( mlt_deque_count( self->a_queue ) >= self->a_queue_limit )
				{
					audio = mlt_deque_pop_front( self->a_queue );
					NDIlib_recv_free_audio( self->recv, audio );
				}
				else
					audio = NULL;
				pthread_cond_broadcast( &self->cond );
				pthread_mutex_unlock( &self->lock );
////fprintf(stderr, "%s:%d: unlocked\n", __FUNCTION__, __LINE__ );
				break;

			case NDIlib_frame_type_metadata:
				NDIlib_recv_free_metadata( self->recv, meta );
				break;

			case NDIlib_frame_type_error:
				mlt_log_error( MLT_PRODUCER_SERVICE( producer ),
					"%s: NDIlib_recv_capture failed\n", __FUNCTION__ );
				break;
		}
	}

	if ( video )
		mlt_pool_release( video );
	if ( audio )
		mlt_pool_release( audio );
	if ( meta )
		mlt_pool_release( meta );

	mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "%s: exiting\n", __FUNCTION__ );

	return NULL;
}

static int get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_properties fprops = MLT_FRAME_PROPERTIES( frame );
	NDIlib_recv_instance_t recv = mlt_properties_get_data( fprops, "ndi_recv", NULL );
	NDIlib_audio_frame_t* audio = mlt_properties_get_data( fprops, "ndi_audio", NULL );

	mlt_log_debug( NULL, "%s:%d: recv=%p, audio=%p\n", __FILE__, __LINE__, recv, audio );

	if ( recv && audio )
	{
		size_t size;
		NDIlib_audio_frame_interleaved_16s_t dst;

		*format = mlt_audio_s16;
		*frequency = audio->sample_rate;
		*channels = audio->no_channels;
		*samples = audio->no_samples;

		size = 2 * ( *samples ) * ( *channels );
		*buffer = mlt_pool_alloc( size );
		mlt_frame_set_audio( frame, (uint8_t*) *buffer, *format, size, (mlt_destructor)mlt_pool_release );

		dst.reference_level = 0;
		dst.p_data = *buffer;
		NDIlib_util_audio_to_interleaved_16s( audio, &dst );

		NDIlib_recv_free_audio( recv, audio );
	}

	return 0;
}

static int get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties fprops = MLT_FRAME_PROPERTIES( frame );
	NDIlib_recv_instance_t recv = mlt_properties_get_data( fprops, "ndi_recv", NULL );
	NDIlib_video_frame_t* video = mlt_properties_get_data( fprops, "ndi_video", NULL );

	mlt_log_debug( NULL, "%s:%d: recv=%p, video=%p\n", __FILE__, __LINE__, recv, video );

	if ( recv && video )
	{
		uint8_t *dst = NULL;
		size_t size;
		int j, dst_stride = 0, stride;

		*width = video->xres;
		*height = video->yres;

		if ( NDIlib_FourCC_type_UYVY == video->FourCC  || NDIlib_FourCC_type_UYVA == video->FourCC )
		{
			dst_stride = 2 * video->xres;
			*format = mlt_image_yuv422;
		}
		else if ( NDIlib_FourCC_type_RGBA == video->FourCC || NDIlib_FourCC_type_RGBX == video->FourCC )
		{
			dst_stride = 4 * video->xres;
			*format = mlt_image_rgb24a;
		}
		else
			*format = mlt_image_none;

		size = mlt_image_format_size( *format, *width, *height, NULL );

		dst = mlt_pool_alloc( size );

		stride = ( dst_stride > video->line_stride_in_bytes ) ? video->line_stride_in_bytes : dst_stride;
		mlt_log_debug( NULL, "%s: stride=%d\n", __FUNCTION__, stride );

		if ( NDIlib_FourCC_type_UYVY == video->FourCC  || NDIlib_FourCC_type_UYVA == video->FourCC )
			for( j = 0; j < video->yres; j++)
				swab2( video->p_data + j * video->line_stride_in_bytes, dst + j * dst_stride, stride );
		else if ( NDIlib_FourCC_type_RGBA == video->FourCC || NDIlib_FourCC_type_RGBX == video->FourCC )
			for( j = 0; j < video->yres; j++)
				memcpy( dst + j * dst_stride, video->p_data + j * video->line_stride_in_bytes, stride );

		if ( dst )
		{
			mlt_frame_set_image( frame, (uint8_t*) dst, size, (mlt_destructor)mlt_pool_release );
			*buffer = dst;
		}

		if ( NDIlib_FourCC_type_UYVA == video->FourCC )
		{
			uint8_t *src = video->p_data + (*height) * video->line_stride_in_bytes;

			size = (*width) * (*height);
			dst = mlt_pool_alloc( size );

			dst_stride = *width;
			stride = ( dst_stride > ( video->line_stride_in_bytes / 2) ) ? ( video->line_stride_in_bytes / 2 ) : dst_stride;
			for( j = 0; j < video->yres; j++)
				memcpy( dst + j * dst_stride, src + j * ( video->line_stride_in_bytes / 2 ), stride );

			mlt_frame_set_alpha( frame, (uint8_t*) dst, size, (mlt_destructor)mlt_pool_release );
		}

		mlt_properties_set_int( fprops, "progressive",
			( video->frame_format_type == NDIlib_frame_format_type_progressive ) );
		mlt_properties_set_int( fprops, "top_field_first",
			( video->frame_format_type == NDIlib_frame_format_type_interleaved ) );

		NDIlib_recv_free_video( recv, video );
	}

	return 0;
}

static int get_frame( mlt_producer producer, mlt_frame_ptr pframe, int index )
{
	mlt_frame frame = NULL;
	NDIlib_audio_frame_t* audio = NULL;
	NDIlib_video_frame_t* video = NULL;
	double fps = mlt_producer_get_fps( producer );
	mlt_position position = mlt_producer_position( producer );
	producer_ndi_t* self = ( producer_ndi_t* )producer->child;

	pthread_mutex_lock( &self->lock );

	mlt_log_debug( NULL, "%s:%d: entering %s\n", __FILE__, __LINE__, __FUNCTION__ );

	// run thread
	if ( !self->f_running )
	{
		// set flags
		self->f_exit = 0;

		pthread_create( &self->th, NULL, producer_ndi_feeder, producer );

		// set flags
		self->f_running = 1;
	}

	while ( !self->f_exit ) //&& !mlt_deque_count( self->queue ) )
	{
		mlt_log_debug( NULL, "%s:%d: audio=%p, video=%p, audio_cnt=%d, video_cnt=%d\n",
			__FILE__, __LINE__, audio, video,
			mlt_deque_count( self->a_queue ), mlt_deque_count( self->v_queue ));

		if ( !video )
			video = (NDIlib_video_frame_t*)mlt_deque_pop_front( self->v_queue );

		if ( !video )
		{
			int r;
			struct timespec tm;

			// Wait
			clock_gettime(CLOCK_REALTIME, &tm);
			tm.tv_nsec += 2LL * 1000000000LL / fps;
			tm.tv_sec += tm.tv_nsec / 1000000000LL;
			tm.tv_nsec %= 1000000000LL;
			r = pthread_cond_timedwait( &self->cond, &self->lock, &tm );
			if( !r )
				continue;

			mlt_log_warning( NULL, "%s:%d: pthread_cond_timedwait()=%d\n", __FILE__, __LINE__, r );

			break;
		}

		if ( !audio )
			audio = (NDIlib_audio_frame_t*)mlt_deque_pop_front( self->a_queue );

		if ( !audio )
		{
			// send muted video frame
//			if( mlt_deque_count( self->v_queue ) )
				break;

			// push back video frame
//			mlt_deque_push_front( self->v_queue, video );
//			video = NULL;
		}
		else
		{
			// drop audio frame
			if ( audio->timecode < video->timecode )
			{
				NDIlib_recv_free_audio( self->recv, audio );
				mlt_pool_release( audio );
				mlt_log_warning( NULL, "%s:%d: dropped audio frame\n", __FILE__, __LINE__ );
				audio = NULL;
				continue;
			}

			// send muted video frame, but save current audio chunk
			if ( audio->timecode > video->timecode )
			{
				mlt_deque_push_front( self->a_queue, audio );
				audio = NULL;
				break;
			}

			// sync frame
			if ( audio && audio->timecode == video->timecode )
				break;
		}
	}

	pthread_mutex_unlock( &self->lock );

	*pframe = frame = mlt_frame_init( MLT_PRODUCER_SERVICE(producer) );
	if ( frame )
	{
		mlt_properties p = MLT_FRAME_PROPERTIES( frame );

		mlt_properties_set_data( p, "ndi_recv", (void *)self->recv, 0, NULL, NULL );

////fprintf(stderr, "%s:%d: audio=%p, video=%p, audio_cnt=%d, video_cnt=%d\n",
////__FUNCTION__, __LINE__, audio, video,  mlt_deque_count( self->a_queue ), mlt_deque_count( self->v_queue ));

		if ( video )
		{
			mlt_properties_set_data( p, "ndi_video", (void *)video, 0, mlt_pool_release, NULL );
			mlt_frame_push_get_image( frame, get_image );
		}
		else
			mlt_log_error( NULL, "%s:%d: NO VIDEO\n", __FILE__, __LINE__ );

		if ( audio )
		{
			mlt_properties_set_data( p, "ndi_audio", (void *)audio, 0, mlt_pool_release, NULL );
			mlt_frame_push_audio( frame, (void*) get_audio );
		}

		self->f_timeout = 0;
	}

	mlt_frame_set_position( frame, position );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_ndi_close( mlt_producer producer )
{
	producer_ndi_t* self = ( producer_ndi_t* )producer->child;

	mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "%s: entering\n", __FUNCTION__ );

	if ( self->f_running )
	{
		// rise flags
		self->f_exit = 1;

		// signal threads
		pthread_cond_broadcast( &self->cond );

		// wait for thread
		pthread_join( self->th, NULL );

		// hide flags
		self->f_running = 0;

		// dequeue audio frames
		while( mlt_deque_count( self->a_queue ) )
		{
			NDIlib_audio_frame_t* audio = (NDIlib_audio_frame_t*)mlt_deque_pop_front( self->a_queue );
			NDIlib_recv_free_audio( self->recv, audio );
			mlt_pool_release( audio );
		}

		// dequeue video frames
		while( mlt_deque_count( self->v_queue ) )
		{
			NDIlib_video_frame_t* video = (NDIlib_video_frame_t*)mlt_deque_pop_front( self->v_queue );
			NDIlib_recv_free_video( self->recv, video );
			mlt_pool_release( video );
		}

		// close receiver
		NDIlib_recv_destroy( self->recv );
	}

	mlt_deque_close( self->a_queue );
	mlt_deque_close( self->v_queue );
	pthread_mutex_destroy( &self->lock );
	pthread_cond_destroy( &self->cond );

	free( producer->child );
	producer->close = NULL;
	mlt_producer_close( producer );

	mlt_log_debug( NULL, "%s: exiting\n", __FUNCTION__ );
}

/** Initialise the producer.
 */
mlt_producer producer_ndi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the producer
	producer_ndi_t* self = ( producer_ndi_t* )calloc( 1, sizeof( producer_ndi_t ) );
	mlt_producer parent = ( mlt_producer )calloc( 1, sizeof( *parent ) );

	mlt_log_debug( NULL, "%s: entering id=[%s], arg=[%s]\n", __FUNCTION__, id, arg );

	// If allocated
	if ( self && !mlt_producer_init( parent, self ) )
	{
		self->parent = parent;
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );

		// Setup context
		self->arg = strdup( arg );
		pthread_mutex_init( &self->lock, NULL );
		pthread_cond_init( &self->cond, NULL );
		self->v_queue = mlt_deque_init();
		self->a_queue = mlt_deque_init();
		self->v_queue_limit = 6;
		self->a_queue_limit = 6;

		// Set callbacks
		parent->close = (mlt_destructor) producer_ndi_close;
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
