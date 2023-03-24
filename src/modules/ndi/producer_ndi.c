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

#define __STDC_FORMAT_MACROS /* see inttypes.h */
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <framework/mlt.h>

#include "Processing.NDI.Lib.h"

#include "factory.h"

#define NDI_TIMEBASE 10000000LL

typedef struct
{
    mlt_producer parent;
    int f_running, f_exit, f_timeout;
    char *arg;
    pthread_t th;
    int count;
    mlt_deque a_queue, v_queue;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    NDIlib_recv_instance_t recv;
    int v_queue_limit, a_queue_limit, v_prefill;
} producer_ndi_t;

static void *producer_ndi_feeder(void *p)
{
    mlt_producer producer = p;
    int ndi_src_idx;
    const NDIlib_source_t *ndi_srcs = NULL;
    NDIlib_video_frame_t *video = NULL;
    NDIlib_audio_frame_t audio;
    NDIlib_metadata_frame_t meta;
    producer_ndi_t *self = (producer_ndi_t *) producer->child;

    mlt_log_debug(MLT_PRODUCER_SERVICE(producer), "%s: entering\n", __FUNCTION__);

    // find the source
    const NDIlib_find_create_t find_create_desc = {.show_local_sources = true,
                                                   .p_groups = NULL,
                                                   .p_extra_ips = NULL};

    // create a finder
    NDIlib_find_instance_t ndi_find = NDIlib_find_create2(&find_create_desc);
    if (!ndi_find) {
        mlt_log_error(MLT_PRODUCER_SERVICE(producer),
                      "%s: NDIlib_find_create failed\n",
                      __FUNCTION__);
        return NULL;
    }

    // wait for source
    mlt_log_debug(MLT_PRODUCER_SERVICE(producer),
                  "%s: waiting for sources, searching for [%s]\n",
                  __FUNCTION__,
                  self->arg);
    for (ndi_src_idx = -1; ndi_src_idx == -1 && !self->f_exit;) {
        unsigned int j, f, n;

        // wait sources list changed
        f = NDIlib_find_wait_for_sources(ndi_find, 500);
        mlt_log_debug(MLT_PRODUCER_SERVICE(producer),
                      "%s: NDIlib_find_wait_for_sources=%d\n",
                      __FUNCTION__,
                      f);
        if (!f) {
            mlt_log_debug(MLT_PRODUCER_SERVICE(producer), "%s: no more sources\n", __FUNCTION__);
            break;
        }

        // check if request present in sources list
        ndi_srcs = NDIlib_find_get_current_sources(ndi_find, &n);
        mlt_log_debug(MLT_PRODUCER_SERVICE(producer), "%s: found %d sources\n", __FUNCTION__, n);
        for (j = 0; j < n && ndi_src_idx == -1; j++)
            if (!strcmp(self->arg, ndi_srcs[j].p_ndi_name))
                ndi_src_idx = j;
    }

    // exit if nothing
    if (self->f_exit || ndi_src_idx == -1) {
        mlt_log_error(MLT_PRODUCER_SERVICE(producer),
                      "%s: exiting, self->f_exit=%d\n",
                      __FUNCTION__,
                      self->f_exit);

        NDIlib_find_destroy(ndi_find);

        return NULL;
    }

    mlt_log_debug(MLT_PRODUCER_SERVICE(producer),
                  "%s: source [%s] found\n",
                  __FUNCTION__,
                  self->arg);

    // create receiver description
    NDIlib_recv_create_t recv_create_desc = {ndi_srcs[ndi_src_idx],
                                             NDIlib_recv_color_format_e_UYVY_RGBA,
                                             NDIlib_recv_bandwidth_highest,
                                             true};

    // Create the receiver
    self->recv = NDIlib_recv_create2(&recv_create_desc);
    NDIlib_find_destroy(ndi_find);
    if (!self->recv) {
        mlt_log_error(MLT_PRODUCER_SERVICE(producer),
                      "%s: exiting, NDIlib_recv_create2 failed\n",
                      __FUNCTION__);
        return 0;
    }

    // set tally
    const NDIlib_tally_t tally_state = {true, false};
    NDIlib_recv_set_tally(self->recv, &tally_state);

    while (!self->f_exit) {
        NDIlib_frame_type_e t;
        NDIlib_audio_frame_interleaved_16s_t *audio_packet, *audio_packet_prev;

        if (!video)
            video = mlt_pool_alloc(sizeof(NDIlib_video_frame_t));

        t = NDIlib_recv_capture(self->recv, video, &audio, &meta, 10);

        mlt_log_debug(NULL, "%s:%d: NDIlib_recv_capture=%d\n", __FILE__, __LINE__, t);

        switch (t) {
        case NDIlib_frame_type_none:
            break;

        case NDIlib_frame_type_video:
            pthread_mutex_lock(&self->lock);
            mlt_deque_push_back(self->v_queue, video);
            if (mlt_deque_count(self->v_queue) >= self->v_queue_limit) {
                video = mlt_deque_pop_front(self->v_queue);
                NDIlib_recv_free_video(self->recv, video);
            } else
                video = NULL;
            pthread_cond_broadcast(&self->cond);
            pthread_mutex_unlock(&self->lock);
            break;

        case NDIlib_frame_type_audio:
            // convert to 16s interleaved
            audio_packet = mlt_pool_alloc(sizeof(NDIlib_audio_frame_interleaved_16s_t));
            audio_packet->reference_level = 0;
            audio_packet->p_data = mlt_pool_alloc(2 * audio.no_channels * audio.no_samples);
            NDIlib_util_audio_to_interleaved_16s(&audio, audio_packet);
            NDIlib_recv_free_audio(self->recv, &audio);

            // store into queue
            pthread_mutex_lock(&self->lock);

            // check if it continues prev packet
            audio_packet_prev = mlt_deque_pop_back(self->a_queue);
            if (audio_packet_prev) {
                int64_t n = audio_packet_prev->timecode
                            + NDI_TIMEBASE * audio_packet_prev->no_samples
                                  / audio_packet_prev->sample_rate,
                        d = audio_packet->timecode - n;

                if (d && llabs(d) < 2) {
                    mlt_log_debug(NULL,
                                  "%s:%d: audio_packet_prev->timecode=%" PRId64
                                  ", audio_packet->timecode=%" PRId64 ", n=%" PRId64 ", d=%" PRId64
                                  ", audio_packet_prev->no_samples=%d\n",
                                  __FILE__,
                                  __LINE__,
                                  audio_packet_prev->timecode,
                                  audio_packet->timecode,
                                  n,
                                  d,
                                  audio_packet_prev->no_samples);

                    audio_packet_prev->p_data = mlt_pool_realloc(audio_packet_prev->p_data,
                                                                 (audio_packet_prev->no_samples
                                                                  + audio_packet->no_samples)
                                                                     * audio_packet->no_channels
                                                                     * 2);

                    memcpy((unsigned char *) audio_packet_prev->p_data
                               + 2 * audio_packet_prev->no_channels * audio_packet_prev->no_samples,
                           audio_packet->p_data,
                           2 * audio_packet->no_channels * audio_packet->no_samples);

                    audio_packet_prev->no_samples += audio_packet->no_samples;

                    mlt_pool_release(audio_packet->p_data);
                    mlt_pool_release(audio_packet);
                    audio_packet = NULL;
                }

                mlt_deque_push_back(self->a_queue, audio_packet_prev);
            }
            if (audio_packet)
                mlt_deque_push_back(self->a_queue, audio_packet);
            if (mlt_deque_count(self->a_queue) >= self->a_queue_limit) {
                audio_packet = mlt_deque_pop_front(self->a_queue);
                mlt_pool_release(audio_packet->p_data);
                mlt_pool_release(audio_packet);
            }
            pthread_cond_broadcast(&self->cond);
            pthread_mutex_unlock(&self->lock);
            break;

        case NDIlib_frame_type_metadata:
            NDIlib_recv_free_metadata(self->recv, &meta);
            break;

        case NDIlib_frame_type_error:
            mlt_log_error(MLT_PRODUCER_SERVICE(producer),
                          "%s: NDIlib_recv_capture failed\n",
                          __FUNCTION__);
            break;
        }
    }

    if (video)
        mlt_pool_release(video);

    mlt_log_debug(MLT_PRODUCER_SERVICE(producer), "%s: exiting\n", __FUNCTION__);

    return NULL;
}

static int get_audio(mlt_frame frame,
                     int16_t **buffer,
                     mlt_audio_format *format,
                     int *frequency,
                     int *channels,
                     int *samples)
{
    mlt_properties fprops = MLT_FRAME_PROPERTIES(frame);
    NDIlib_recv_instance_t recv = mlt_properties_get_data(fprops, "ndi_recv", NULL);
    NDIlib_audio_frame_interleaved_16s_t *audio = mlt_properties_get_data(fprops, "ndi_audio", NULL);

    mlt_log_debug(NULL, "%s:%d: recv=%p, audio=%p\n", __FILE__, __LINE__, recv, audio);

    if (recv && audio) {
        size_t size;
        *format = mlt_audio_s16;
        *frequency = audio->sample_rate;
        *channels = audio->no_channels;
        *samples = audio->no_samples;
        size = 2 * (*samples) * (*channels);
        *buffer = audio->p_data;
        mlt_frame_set_audio(frame, (uint8_t *) *buffer, *format, size, NULL);
    }

    return 0;
}

static int get_image(mlt_frame frame,
                     uint8_t **buffer,
                     mlt_image_format *format,
                     int *width,
                     int *height,
                     int writable)
{
    mlt_properties fprops = MLT_FRAME_PROPERTIES(frame);
    NDIlib_recv_instance_t recv = mlt_properties_get_data(fprops, "ndi_recv", NULL);
    NDIlib_video_frame_t *video = mlt_properties_get_data(fprops, "ndi_video", NULL);

    mlt_log_debug(NULL, "%s:%d: recv=%p, video=%p\n", __FILE__, __LINE__, recv, video);

    if (recv && video) {
        uint8_t *dst = NULL;
        size_t size;
        int j, dst_stride = 0, stride;

        *width = video->xres;
        *height = video->yres;

        if (NDIlib_FourCC_type_UYVY == video->FourCC || NDIlib_FourCC_type_UYVA == video->FourCC) {
            dst_stride = 2 * video->xres;
            *format = mlt_image_yuv422;
        } else if (NDIlib_FourCC_type_RGBA == video->FourCC
                   || NDIlib_FourCC_type_RGBX == video->FourCC) {
            dst_stride = 4 * video->xres;
            *format = mlt_image_rgba;
        } else
            *format = mlt_image_none;

        size = mlt_image_format_size(*format, *width, *height, NULL);

        dst = mlt_pool_alloc(size);

        stride = (dst_stride > video->line_stride_in_bytes) ? video->line_stride_in_bytes
                                                            : dst_stride;
        mlt_log_debug(NULL, "%s: stride=%d\n", __FUNCTION__, stride);

        if (NDIlib_FourCC_type_UYVY == video->FourCC || NDIlib_FourCC_type_UYVA == video->FourCC)
            for (j = 0; j < video->yres; j++)
                swab2(video->p_data + j * video->line_stride_in_bytes, dst + j * dst_stride, stride);
        else if (NDIlib_FourCC_type_RGBA == video->FourCC
                 || NDIlib_FourCC_type_RGBX == video->FourCC)
            for (j = 0; j < video->yres; j++)
                memcpy(dst + j * dst_stride,
                       video->p_data + j * video->line_stride_in_bytes,
                       stride);

        if (dst) {
            mlt_frame_set_image(frame, (uint8_t *) dst, size, (mlt_destructor) mlt_pool_release);
            *buffer = dst;
        }

        if (NDIlib_FourCC_type_UYVA == video->FourCC) {
            uint8_t *src = video->p_data + (*height) * video->line_stride_in_bytes;

            size = (*width) * (*height);
            dst = mlt_pool_alloc(size);

            dst_stride = *width;
            stride = (dst_stride > (video->line_stride_in_bytes / 2))
                         ? (video->line_stride_in_bytes / 2)
                         : dst_stride;
            for (j = 0; j < video->yres; j++)
                memcpy(dst + j * dst_stride, src + j * (video->line_stride_in_bytes / 2), stride);

            mlt_frame_set_alpha(frame, (uint8_t *) dst, size, (mlt_destructor) mlt_pool_release);
        }

        mlt_properties_set_int(fprops,
                               "progressive",
                               (video->frame_format_type == NDIlib_frame_format_type_progressive));
        mlt_properties_set_int(fprops,
                               "top_field_first",
                               (video->frame_format_type == NDIlib_frame_format_type_interleaved));

        NDIlib_recv_free_video(recv, video);
    }

    return 0;
}

static int get_frame(mlt_producer producer, mlt_frame_ptr pframe, int index)
{
    mlt_frame frame = NULL;
    NDIlib_audio_frame_interleaved_16s_t *audio_frame = NULL;
    NDIlib_video_frame_t *video = NULL;
    double fps = mlt_producer_get_fps(producer);
    mlt_position position = mlt_producer_position(producer);
    producer_ndi_t *self = (producer_ndi_t *) producer->child;

    pthread_mutex_lock(&self->lock);

    mlt_log_debug(NULL, "%s:%d: entering %s\n", __FILE__, __LINE__, __FUNCTION__);

    // run thread
    if (!self->f_running) {
        // set flags
        self->f_exit = 0;

        pthread_create(&self->th, NULL, producer_ndi_feeder, producer);

        // set flags
        self->f_running = 1;
    }

    mlt_log_debug(NULL,
                  "%s:%d: audio_cnt=%d, video_cnt=%d\n",
                  __FILE__,
                  __LINE__,
                  mlt_deque_count(self->a_queue),
                  mlt_deque_count(self->v_queue));

    // wait for prefill
    if (mlt_deque_count(self->v_queue) < self->v_prefill) {
        struct timespec tm;

        // Wait
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_nsec += self->v_prefill * 1000000000LL / fps;
        tm.tv_sec += tm.tv_nsec / 1000000000LL;
        tm.tv_nsec %= 1000000000LL;
        pthread_cond_timedwait(&self->cond, &self->lock, &tm);
    }

    // pop frame to use
    if (mlt_deque_count(self->v_queue) >= self->v_prefill)
        video = (NDIlib_video_frame_t *) mlt_deque_pop_front(self->v_queue);

    if (video) {
        int64_t video_timecode_out, video_dur;

        mlt_log_debug(NULL,
                      "%s:%d: video: timecode=%" PRId64 "\n",
                      __FILE__,
                      __LINE__,
                      video->timecode);
        video_dur = NDI_TIMEBASE * video->frame_rate_D / video->frame_rate_N;
        video_timecode_out = video->timecode + video_dur;

        // deal with audio
        while (1) {
            NDIlib_audio_frame_interleaved_16s_t *audio_packet;
            int64_t audio_packet_dur, audio_packet_timecode_out, T1, T2, dst_offset, src_offset,
                duration;

            // pop audio packet
            audio_packet = (NDIlib_audio_frame_interleaved_16s_t *) mlt_deque_pop_front(
                self->a_queue);

            // check if audio present
            if (!audio_packet)
                break;

            // check if packet in a future
            if (video_timecode_out < audio_packet->timecode) {
                // push it back to queue
                mlt_deque_push_front(self->a_queue, audio_packet);
                break;
            }

            // calc packet
            audio_packet_dur = NDI_TIMEBASE * audio_packet->no_samples / audio_packet->sample_rate;
            audio_packet_timecode_out = audio_packet_dur + audio_packet->timecode;

            // check if packet in the past
            if (audio_packet_timecode_out < video->timecode) {
                mlt_pool_release(audio_packet->p_data);
                mlt_pool_release(audio_packet);
                continue;
            }

            // allocate new audio frame
            if (!audio_frame) {
                audio_frame = (NDIlib_audio_frame_interleaved_16s_t *) mlt_pool_alloc(
                    sizeof(NDIlib_audio_frame_interleaved_16s_t));
                audio_frame->timecode = video->timecode;
                audio_frame->no_channels = audio_packet->no_channels;
                audio_frame->sample_rate = audio_packet->sample_rate;
                audio_frame->no_samples = audio_packet->sample_rate * video->frame_rate_D
                                          / video->frame_rate_N;
                audio_frame->p_data = (short *) mlt_pool_alloc(2 * audio_frame->no_samples
                                                               * audio_frame->no_channels);
            }

            // copy data of overlapping region
            T1 = MAX(audio_packet->timecode, video->timecode);
            T2 = MIN(audio_packet_timecode_out, video_timecode_out);
            dst_offset = (T1 - audio_frame->timecode) * audio_frame->sample_rate / NDI_TIMEBASE;
            src_offset = (T1 - audio_packet->timecode) * audio_packet->sample_rate / NDI_TIMEBASE;
            duration = (T2 - T1) * audio_frame->sample_rate / NDI_TIMEBASE;
            ;

            memcpy((unsigned char *) audio_frame->p_data + 2 * audio_frame->no_channels * dst_offset,
                   (unsigned char *) audio_packet->p_data
                       + 2 * audio_packet->no_channels * src_offset,
                   2 * audio_packet->no_channels * duration);

            // save packet back if it will be used later or clear it
            if (video_timecode_out < audio_packet_timecode_out) {
                // push it back to queue
                mlt_deque_push_front(self->a_queue, audio_packet);
                break;
            }

            // free packet data
            mlt_pool_release(audio_packet->p_data);
            mlt_pool_release(audio_packet);
        }
    }

    pthread_mutex_unlock(&self->lock);

    *pframe = frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
    if (frame) {
        mlt_properties p = MLT_FRAME_PROPERTIES(frame);

        mlt_properties_set_data(p, "ndi_recv", (void *) self->recv, 0, NULL, NULL);

        if (video) {
            mlt_properties_set_data(p, "ndi_video", (void *) video, 0, mlt_pool_release, NULL);
            mlt_frame_push_get_image(frame, get_image);
        } else
            mlt_log_error(NULL, "%s:%d: NO VIDEO\n", __FILE__, __LINE__);

        if (audio_frame) {
            mlt_properties_set_data(p, "ndi_audio", (void *) audio_frame, 0, mlt_pool_release, NULL);
            mlt_properties_set_data(p,
                                    "ndi_audio_data",
                                    (void *) audio_frame->p_data,
                                    0,
                                    mlt_pool_release,
                                    NULL);
            mlt_frame_push_audio(frame, (void *) get_audio);
        }

        self->f_timeout = 0;
    }

    mlt_frame_set_position(frame, position);

    // Calculate the next timecode
    mlt_producer_prepare_next(producer);

    return 0;
}

static void producer_ndi_close(mlt_producer producer)
{
    producer_ndi_t *self = (producer_ndi_t *) producer->child;

    mlt_log_debug(MLT_PRODUCER_SERVICE(producer), "%s: entering\n", __FUNCTION__);

    if (self->f_running) {
        // rise flags
        self->f_exit = 1;

        // signal threads
        pthread_cond_broadcast(&self->cond);

        // wait for thread
        pthread_join(self->th, NULL);

        // hide flags
        self->f_running = 0;

        // dequeue audio frames
        while (mlt_deque_count(self->a_queue)) {
            NDIlib_audio_frame_interleaved_16s_t *audio
                = (NDIlib_audio_frame_interleaved_16s_t *) mlt_deque_pop_front(self->a_queue);
            mlt_pool_release(audio->p_data);
            mlt_pool_release(audio);
        }

        // dequeue video frames
        while (mlt_deque_count(self->v_queue)) {
            NDIlib_video_frame_t *video = (NDIlib_video_frame_t *) mlt_deque_pop_front(
                self->v_queue);
            NDIlib_recv_free_video(self->recv, video);
            mlt_pool_release(video);
        }

        // close receiver
        NDIlib_recv_destroy(self->recv);
    }

    mlt_deque_close(self->a_queue);
    mlt_deque_close(self->v_queue);
    pthread_mutex_destroy(&self->lock);
    pthread_cond_destroy(&self->cond);

    free(producer->child);
    producer->close = NULL;
    mlt_producer_close(producer);

    mlt_log_debug(NULL, "%s: exiting\n", __FUNCTION__);
}

/** Initialise the producer.
 */
mlt_producer producer_ndi_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    // Allocate the producer
    producer_ndi_t *self = (producer_ndi_t *) calloc(1, sizeof(producer_ndi_t));
    mlt_producer parent = (mlt_producer) calloc(1, sizeof(*parent));

    mlt_log_debug(NULL, "%s: entering id=[%s], arg=[%s]\n", __FUNCTION__, id, arg);

    // If allocated
    if (self && !mlt_producer_init(parent, self)) {
        self->parent = parent;
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(parent);

        // Setup context
        self->arg = strdup(arg);
        pthread_mutex_init(&self->lock, NULL);
        pthread_cond_init(&self->cond, NULL);
        self->v_queue = mlt_deque_init();
        self->a_queue = mlt_deque_init();
        self->v_queue_limit = 6;
        self->a_queue_limit = 6;
        self->v_prefill = 2;

        // Set callbacks
        parent->close = (mlt_destructor) producer_ndi_close;
        parent->get_frame = get_frame;

        // These properties effectively make it infinite.
        mlt_properties_set_int(properties, "length", INT_MAX);
        mlt_properties_set_int(properties, "out", INT_MAX - 1);
        mlt_properties_set(properties, "eof", "loop");

        return parent;
    }

    free(self);

    return NULL;
}
