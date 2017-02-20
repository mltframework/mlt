/*
 * consumer_ndi.c -- output through NewTek NDI
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

#include "factory.h"

#include "Processing.NDI.Lib.h"

typedef struct
{
	struct mlt_consumer_s parent;
	int f_running, f_exit;
	char* arg;
	pthread_t th;
	int count;
	int sliced_swab;
} consumer_ndi_t;

static void* consumer_ndi_feeder( void* p )
{
	int i;
	mlt_frame last = NULL;
	mlt_consumer consumer = p;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	consumer_ndi_t* self = ( consumer_ndi_t* )consumer->child;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	const NDIlib_send_create_t ndi_send_desc = { self->arg, NULL, true, false };

	NDIlib_send_instance_t ndi_send = NDIlib_send_create( &ndi_send_desc );
	if ( !ndi_send )
	{
		mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "%s: NDIlib_send_create failed\n", __FUNCTION__ );

		return NULL;
	}

	char* ndi_con_str = malloc(NDI_CON_STR_MAX);
	strncpy(ndi_con_str, "<ndi_product", NDI_CON_STR_MAX);
	for ( i = 0; i < mlt_properties_count( properties ); i++ )
	{
		char *name = mlt_properties_get_name( properties, i );
		if ( !name || strncmp( name, "meta.attr.", 10 ) )
			continue;
		strncat(ndi_con_str, " \"", NDI_CON_STR_MAX);
		strncat(ndi_con_str, name + 10, NDI_CON_STR_MAX);
		strncat(ndi_con_str, "\"=\"", NDI_CON_STR_MAX);
		strncat(ndi_con_str, mlt_properties_get_value( properties, i ), NDI_CON_STR_MAX);
		strncat(ndi_con_str, "\"", NDI_CON_STR_MAX);
	}
	strncat(ndi_con_str, ">", NDI_CON_STR_MAX);

	const NDIlib_metadata_frame_t ndi_con_type =
	{
		// The length
		(int)strlen(ndi_con_str),
		// Timecode (synthesized for us !)
		NDIlib_send_timecode_synthesize,
		// The string
		(char*)ndi_con_str
	};

	NDIlib_send_add_connection_metadata(ndi_send, &ndi_con_type);

	while ( !self->f_exit )
	{
		mlt_frame frame = mlt_consumer_rt_frame( consumer );

		if ( frame || last )
		{
			mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( consumer ) );
			int m_isKeyer = 0, width = profile->width, height = profile->height;
			uint8_t* image = 0;
			mlt_frame frm = m_isKeyer ? frame : last;
			mlt_image_format format = 0 ? mlt_image_rgb24a : mlt_image_yuv422;
			int rendered = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frm ), "rendered" );

			if ( rendered && !mlt_frame_get_image( frm, &image, &format, &width, &height, 0 ) )
			{
				mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s:%d: width=%d, height=%d\n",
					__FUNCTION__, __LINE__, width, height );

				uint8_t* buffer = 0;
				int64_t timecode;
				int stride = width * ( m_isKeyer? 4 : 2 );
				int progressive = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "progressive" );

				timecode = 10000000LL * (uint64_t)self->count * (uint64_t)profile->frame_rate_den;
				timecode = timecode / (uint64_t)profile->frame_rate_num;

				const NDIlib_video_frame_t ndi_video_frame =
				{
					// Resolution
					width, height,

					// Use YCbCr video
					m_isKeyer
						? NDIlib_FourCC_type_BGRA
						: NDIlib_FourCC_type_UYVY,

					// The frame-eate
					profile->frame_rate_num, profile->frame_rate_den,

					// The aspect ratio
					(double)profile->display_aspect_num / profile->display_aspect_den,

					// This is a progressive frame
					progressive
						? NDIlib_frame_format_type_progressive
						: NDIlib_frame_format_type_interleaved,

					// Timecode
					timecode,

					// The video memory used for this frame
					buffer = (uint8_t*)malloc( height * stride ),

					// The line to line stride of this image
					stride
				};

				if ( !m_isKeyer )
				{
					unsigned char *arg[3] = { image, buffer };
					ssize_t size = stride * height;

					// Normal non-keyer playout - needs byte swapping
					if ( !self->sliced_swab )
						swab2( arg[0], arg[1], size );
					else
					{
						arg[2] = (unsigned char*)size;
						mlt_slices_run_fifo( 0, swab_sliced, arg);
					}
				}
				else if ( !mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "test_image" ) )
				{
					// Normal keyer output
					int y = height + 1;
					uint32_t* s = (uint32_t*) image;
					uint32_t* d = (uint32_t*) buffer;

					// Need to relocate alpha channel RGBA => ARGB
					while ( --y )
					{
						int x = width + 1;
						while ( --x )
						{
							*d++ = ( *s << 8 ) | ( *s >> 24 );
							s++;
						}
					}
				}
				else
				{
					// Keying blank frames - nullify alpha
					memset( buffer, 0, stride * height );
				}

				mlt_audio_format aformat = mlt_audio_s16;
				int frequency = 48000;
				int m_channels = 2;
				int samples = mlt_sample_calculator( mlt_profile_fps( profile ), frequency, self->count );
				int16_t *pcm = 0;

				if ( !mlt_frame_get_audio( frm, (void**) &pcm, &aformat, &frequency, &m_channels, &samples ) )
				{
					// Create an audio buffer
					const NDIlib_audio_frame_interleaved_16s_t audio_data =
					{
						// 48kHz
						frequency,

						// Lets submit stereo although there is nothing limiting us
						m_channels,

						//
						samples,

						// Timecode
						timecode,

						// reference_level
						0,

						// The audio data, interleaved 16bpp
						pcm
					};

					NDIlib_util_send_send_audio_interleaved_16s(ndi_send, &audio_data);
				}

				// We now submit the frame.
				NDIlib_send_send_video(ndi_send, &ndi_video_frame);

				// free buf
				free( ndi_video_frame.p_data );

				self->count++;
			}
		}

		if ( frame )
		{
			if ( last )
				mlt_frame_close( last );

			last = frame;
		}

		mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
	}

	NDIlib_send_destroy( ndi_send );

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );

	if ( last )
		mlt_frame_close( last );

	return NULL;
}

static int consumer_ndi_start( mlt_consumer consumer )
{
	int r = 0;

	mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "%s: entering\n", __FUNCTION__ );

	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Set interlacing properties
	mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( consumer ) );
	mlt_properties_set_int( properties, "top_field_first", !profile->progressive );

	consumer_ndi_t* self = ( consumer_ndi_t* )consumer->child;

	if ( !self->f_running )
	{
		self->sliced_swab = mlt_properties_get_int( properties, "sliced_swab" );

		// set flags
		self->f_exit = 0;

		pthread_create( &self->th, NULL, consumer_ndi_feeder, consumer );

		// set flags
		self->f_running = 1;
	}

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );

	return r;
}

static int consumer_ndi_stop( mlt_consumer consumer )
{
	int r = 0;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	consumer_ndi_t* self = ( consumer_ndi_t* )consumer->child;

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

static int consumer_ndi_is_stopped( mlt_consumer consumer )
{
	consumer_ndi_t* self = ( consumer_ndi_t* )consumer->child;
	return !self->f_running;
}

static void consumer_ndi_close( mlt_consumer consumer )
{
	consumer_ndi_t* self = ( consumer_ndi_t* )consumer->child;

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
mlt_consumer consumer_ndi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the consumer
	consumer_ndi_t* self = ( consumer_ndi_t* )calloc( 1, sizeof( consumer_ndi_t ) );

	mlt_log_debug( NULL, "%s: entering id=[%s], arg=[%s]\n", __FUNCTION__, id, arg );

	// If allocated
	if ( self && !mlt_consumer_init( &self->parent, self, profile ) )
	{
		mlt_consumer parent = &self->parent;

		// Setup context
		if( arg )
			self->arg = strdup( arg );

		// Setup callbacks
		parent->close = consumer_ndi_close;
		parent->start = consumer_ndi_start;
		parent->stop = consumer_ndi_stop;
		parent->is_stopped = consumer_ndi_is_stopped;

		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		mlt_properties_set( properties, "deinterlace_method", "onefield" );


		return parent;
	}

	free( self );

	return NULL;
}
