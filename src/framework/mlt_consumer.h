/**
 * \file mlt_consumer.h
 * \brief abstraction for all consumer services
 * \see mlt_consumer_s
 *
 * Copyright (C) 2003-2015 Meltytech, LLC
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

#ifndef MLT_CONSUMER_H
#define MLT_CONSUMER_H

#include "mlt_service.h"
#include "mlt_events.h"
#include <pthread.h>

/** \brief Consumer abstract service class
 *
 * A consumer is a service that pulls audio and video from the connected
 * producers, filters, and transitions. Typically a consumer is used to
 * output audio and/or video to a device, file, or socket.
 *
 * \extends mlt_service_s
 * \properties \em rescale the scaling algorithm to pass on to all scaling
 * filters, defaults to "bilinear"
 * \properties \em buffer the number of frames to use in the asynchronous
 * render thread, defaults to 25
 * \properties \em prefill the number of frames to render before commencing
 * output when real_time <> 0, defaults to the size of buffer
 * \properties \em drop_max the maximum number of consecutively dropped frames, defaults to 5
 * \properties \em frequency the audio sample rate to use in Hertz, defaults to 48000
 * \properties \em channels the number of audio channels to use, defaults to 2
 * \properties \em real_time the asynchronous behavior: 1 (default) for asynchronous
 * with frame dropping, -1 for asynchronous without frame dropping, 0 to disable (synchronous)
 * \properties \em test_card the name of a resource to use as the test card, defaults to
 * environment variable MLT_TEST_CARD. If undefined, the hard-coded default test card is
 * white silence. A test card is what appears when nothing is produced.
 * \event \em consumer-frame-show Subclass implementations fire this immediately after showing a frame
 * or when a frame should be shown (if audio-only consumer).
 * \event \em consumer-frame-render The base class fires this immediately before rendering a frame.
 * \event \em consumer-thread-create Override the implementation of creating and
 *   starting a thread by listening and responding to this (real_time 1 or -1 only).
 * \event \em consumer-thread-join Override the implementation of waiting and
 *   joining a terminated thread  by listening and responding to this (real_time 1 or -1 only).
 * \event \em consumer-thread-started The base class fires when beginning execution of a rendering thread.
 * \event \em consumer-thread-stopped The base class fires when a rendering thread has ended.
 * \event \em consumer-stopping This is fired when stop was requested, but before render threads are joined.
 * \event \em consumer-stopped This is fired when the subclass implementation calls mlt_consumer_stopped().
 * \properties \em fps video frames per second as floating point (read only)
 * \properties \em frame_rate_num the numerator of the video frame rate, overrides \p mlt_profile_s
 * \properties \em frame_rate_den the denominator of the video frame rate, overrides \p mlt_profile_s
 * \properties \em width the horizontal video resolution, overrides \p mlt_profile_s
 * \properties \em height the vertical video resolution, overrides \p mlt_profile_s
 * \properties \em progressive a flag that indicates if the video is interlaced
 * or progressive, overrides \p mlt_profile_s
 * \properties \em aspect_ratio the video sample (pixel) aspect ratio as floating point (read only)
 * \properties \em sample_aspect_num the numerator of the sample aspect ratio, overrides \p mlt_profile_s
 * \properties \em sample_aspect_den the denominator of the sample aspect ratio, overrides \p mlt_profile_s
 * \properties \em display_ratio the video frame aspect ratio as floating point (read only)
 * \properties \em display_aspect_num the numerator of the video frame aspect ratio, overrides \p mlt_profile_s
 * \properties \em display_aspect_den the denominator of the video frame aspect ratio, overrides \p mlt_profile_s
 * \properties \em priority the OS scheduling priority for the render threads when real_time is not 0.
 * \properties \em top_field_first when not progressive, whether interlace field order is top-field-first, defaults to 0.
 *   Set this to -1 if the consumer does not care about the field order.
 * \properties \em mlt_image_format the image format to request in rendering threads, defaults to yuv422
 * \properties \em mlt_audio_format the audio format to request in rendering threads, defaults to S16
 * \properties \em audio_off set non-zero to disable audio processing
 * \properties \em video_off set non-zero to disable video processing
 * \properties \em drop_count the number of video frames not rendered since starting consumer
 */

struct mlt_consumer_s
{
	/** A consumer is a service. */
	struct mlt_service_s parent;

	/** Start the consumer to pull frames (virtual function).
	 *
	 * \param mlt_consumer a consumer
	 * \return true if there was an error
	 */
	int ( *start )( mlt_consumer );

	/** Stop the consumer (virtual function).
	 *
	 * \param mlt_consumer a consumer
	 * \return true if there was an error
	 */
	int ( *stop )( mlt_consumer );

	/** Get whether the consumer is running or stopped (virtual function).
	 *
	 * \param mlt_consumer a consumer
	 * \return true if the consumer is stopped
	 */
	int ( *is_stopped )( mlt_consumer );

	/** Purge the consumer of buffered data (virtual function).
	 *
	 * \param mlt_consumer a consumer
	 */
	void ( *purge )( mlt_consumer );

	/** The destructor virtual function
	 *
	 * \param mlt_consumer a consumer
	 */
	void ( *close )( mlt_consumer );

	void *local; /**< \private instance object */
	void *child; /**< \private the object of a subclass */
};

#define MLT_CONSUMER_SERVICE( consumer )	( &( consumer )->parent )
#define MLT_CONSUMER_PROPERTIES( consumer )	MLT_SERVICE_PROPERTIES( MLT_CONSUMER_SERVICE( consumer ) )

extern int mlt_consumer_init( mlt_consumer self, void *child, mlt_profile profile );
extern mlt_consumer mlt_consumer_new( mlt_profile profile );
extern mlt_service mlt_consumer_service( mlt_consumer self );
extern mlt_properties mlt_consumer_properties( mlt_consumer self );
extern int mlt_consumer_connect( mlt_consumer self, mlt_service producer );
extern int mlt_consumer_start( mlt_consumer self );
extern void mlt_consumer_purge( mlt_consumer self );
extern int mlt_consumer_put_frame( mlt_consumer self, mlt_frame frame );
extern mlt_frame mlt_consumer_get_frame( mlt_consumer self );
extern mlt_frame mlt_consumer_rt_frame( mlt_consumer self );
extern int mlt_consumer_stop( mlt_consumer self );
extern int mlt_consumer_is_stopped( mlt_consumer self );
extern void mlt_consumer_stopped( mlt_consumer self );
extern void mlt_consumer_close( mlt_consumer );
extern mlt_position mlt_consumer_position( mlt_consumer );

#endif
