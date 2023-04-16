/*
 * link_swresample.c
 * Copyright (C) 2022 Meltytech, LLC
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

#include "common.h"
#include "common_swr.h"

#include <framework/mlt_frame.h>
#include <framework/mlt_link.h>
#include <framework/mlt_log.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private Types
#define FUTURE_FRAMES 1

typedef struct
{
    mlt_position expected_frame;
    mlt_position continuity_frame;
} private_data;

static void destroy_swr_data(mlt_swr_private_data *swr)
{
    if (swr) {
        mlt_free_swr_context(swr);
        free(swr);
    }
}

static void link_configure(mlt_link self, mlt_profile chain_profile)
{
    // Operate at the same frame rate as the next link
    mlt_service_set_profile(MLT_LINK_SERVICE(self),
                            mlt_service_profile(MLT_PRODUCER_SERVICE(self->next)));
}

static int link_get_audio(mlt_frame frame,
                          void **buffer,
                          mlt_audio_format *format,
                          int *frequency,
                          int *channels,
                          int *samples)
{
    int requested_frequency = *frequency <= 0 ? 48000 : *frequency;
    int requested_samples = *samples;
    mlt_link self = (mlt_link) mlt_frame_pop_audio(frame);
    private_data *pdata = (private_data *) self->child;

    // Validate the request
    *channels = *channels <= 0 ? 2 : *channels;

    int src_frequency = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "audio_frequency");
    src_frequency = src_frequency <= 0 ? *frequency : src_frequency;
    int src_samples = mlt_audio_calculate_frame_samples(mlt_producer_get_fps(
                                                            MLT_LINK_PRODUCER(self)),
                                                        src_frequency,
                                                        mlt_frame_get_position(frame));
    int src_channels = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "audio_channels");
    src_channels = src_channels <= 0 ? *channels : src_channels;

    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    struct mlt_audio_s in;
    struct mlt_audio_s out;

    mlt_audio_set_values(&in, *buffer, src_frequency, mlt_audio_none, src_samples, src_channels);
    mlt_audio_set_values(&out, NULL, requested_frequency, *format, requested_samples, *channels);

    // Get the producer's audio
    int error
        = mlt_frame_get_audio(frame, &in.data, &in.format, &in.frequency, &in.channels, &in.samples);
    if (error || in.format == mlt_audio_none || out.format == mlt_audio_none || in.frequency <= 0
        || out.frequency <= 0 || in.channels <= 0 || out.channels <= 0) {
        // Error situation. Do not attempt to convert.
        mlt_audio_get_values(&in, buffer, frequency, format, samples, channels);
        mlt_log_error(MLT_LINK_SERVICE(self),
                      "Invalid Parameters: %dS - %dHz %dC %s -> %dHz %dC %s\n",
                      in.samples,
                      in.frequency,
                      in.channels,
                      mlt_audio_format_name(in.format),
                      out.frequency,
                      out.channels,
                      mlt_audio_format_name(out.format));
        return error;
    }

    if (in.samples == 0) {
        // Noting to convert.
        return error;
    }

    // Determine the input/output channel layout.
    in.layout = mlt_get_channel_layout_or_default(mlt_properties_get(frame_properties,
                                                                     "channel_layout"),
                                                  in.channels);
    out.layout = mlt_get_channel_layout_or_default(mlt_properties_get(frame_properties,
                                                                      "consumer.channel_layout"),
                                                   out.channels);

    if (in.format == out.format && in.frequency == out.frequency && in.channels == out.channels
        && in.layout == out.layout) {
        // No change necessary
        mlt_audio_get_values(&in, buffer, frequency, format, samples, channels);
        return error;
    }

    mlt_service_lock(MLT_LINK_SERVICE(self));

    mlt_swr_private_data *swr = NULL;
    int cache_miss = 0;
    mlt_cache_item cache_item = mlt_service_cache_get(MLT_LINK_SERVICE(self), "link_swresample");
    swr = mlt_cache_item_data(cache_item, NULL);
    if (!cache_item) {
        cache_miss = 1;
    }

    // Detect configuration change
    if (cache_miss || swr->in_format != in.format || swr->out_format != out.format
        || swr->in_frequency != in.frequency || swr->out_frequency != out.frequency
        || swr->in_channels != in.channels || swr->out_channels != out.channels
        || swr->in_layout != in.layout || swr->out_layout != out.layout
        || pdata->expected_frame != mlt_frame_get_position(frame)) {
        mlt_cache_item_close(cache_item);
        swr = calloc(1, sizeof(mlt_swr_private_data));
        // Save the configuration
        swr->in_format = in.format;
        swr->out_format = out.format;
        swr->in_frequency = in.frequency;
        swr->out_frequency = out.frequency;
        swr->in_channels = in.channels;
        swr->out_channels = out.channels;
        swr->in_layout = in.layout;
        swr->out_layout = out.layout;
        // Reconfigure the context
        error = mlt_configure_swr_context(MLT_LINK_SERVICE(self), swr);
        mlt_service_cache_put(MLT_LINK_SERVICE(self),
                              "link_swresample",
                              swr,
                              0,
                              (mlt_destructor) destroy_swr_data);
        cache_item = mlt_service_cache_get(MLT_LINK_SERVICE(self), "link_swresample");
        swr = mlt_cache_item_data(cache_item, NULL);
        pdata->continuity_frame = mlt_frame_get_position(frame);
        pdata->expected_frame = mlt_frame_get_position(frame);
    }

    if (swr && !error) {
        int total_received_samples = 0;
        out.samples = requested_samples;
        mlt_audio_alloc_data(&out);

        if (pdata->continuity_frame == mlt_frame_get_position(frame)) {
            // This is the nominal case when sample rate is not changing
            mlt_audio_get_planes(&in, swr->in_buffers);
            mlt_audio_get_planes(&out, swr->out_buffers);
            total_received_samples = swr_convert(swr->ctx,
                                                 swr->out_buffers,
                                                 out.samples,
                                                 (const uint8_t **) swr->in_buffers,
                                                 in.samples);
            if (total_received_samples < 0) {
                mlt_log_error(MLT_LINK_SERVICE(self),
                              "swr_convert() failed. Needed: %d\tIn: %d\tOut: %d\n",
                              out.samples,
                              in.samples,
                              total_received_samples);
                error = 1;
            }
            pdata->continuity_frame++;
        }

        while (total_received_samples < requested_samples && !error) {
            // The input frame is insufficient to fill the output frame.
            // This happens when sample rate conversion is occurring.
            // Request data from future frames.
            mlt_properties unique_properties
                = mlt_frame_get_unique_properties(frame, MLT_LINK_SERVICE(self));
            if (!unique_properties) {
                error = 1;
                break;
            }
            char key[19];
            int frame_delta = mlt_frame_get_position(frame) - mlt_frame_original_position(frame);
            sprintf(key, "%d", pdata->continuity_frame - frame_delta);
            mlt_frame src_frame = (mlt_frame) mlt_properties_get_data(unique_properties, key, NULL);
            if (!src_frame) {
                mlt_log_error(MLT_LINK_SERVICE(self), "Frame not found: %s\n", key);
                break;
            }

            //  Get the audio from the in frame
            in.samples = mlt_audio_calculate_frame_samples(mlt_producer_get_fps(
                                                               MLT_LINK_PRODUCER(self)),
                                                           in.frequency,
                                                           pdata->continuity_frame);
            in.format = mlt_audio_none;
            error = mlt_frame_get_audio(src_frame,
                                        &in.data,
                                        &in.format,
                                        &in.frequency,
                                        &in.channels,
                                        &in.samples);
            if (error) {
                break;
            }

            // Set up the SWR buffer for the audio from the in frame
            mlt_audio_get_planes(&in, swr->in_buffers);

            // Set up the SWR buffer for the audio from the out frame,
            // shifting according to what has already been received.
            int plane_count = mlt_audio_plane_count(&out);
            int plane_size = mlt_audio_plane_size(&out);
            int out_step_size = plane_size / out.samples;
            int p = 0;
            for (p = 0; p < plane_count; p++) {
                uint8_t *pAudio = (uint8_t *) out.data + (out_step_size * total_received_samples);
                swr->out_buffers[p] = pAudio + (p * plane_size);
            }

            int samples_needed = requested_samples - total_received_samples;
            int received_samples = swr_convert(swr->ctx,
                                               swr->out_buffers,
                                               samples_needed,
                                               (const uint8_t **) swr->in_buffers,
                                               in.samples);
            if (received_samples < 0) {
                mlt_log_error(MLT_LINK_SERVICE(self),
                              "swr_convert() failed. Needed: %d\tIn: %d\tOut: %d\n",
                              samples_needed,
                              in.samples,
                              received_samples);
                error = 1;
            } else {
                total_received_samples += received_samples;
            }
            pdata->continuity_frame++;
        }

        if (total_received_samples == 0) {
            mlt_log_info(MLT_LINK_SERVICE(self), "Failed to get any samples - return silence\n");
            mlt_audio_silence(&out, out.samples, 0);
        } else if (total_received_samples < out.samples) {
            // Duplicate samples to return the exact number requested.
            mlt_audio_copy(&out,
                           &out,
                           total_received_samples,
                           0,
                           out.samples - total_received_samples);
        }
        mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
        mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);
        mlt_properties_set(frame_properties,
                           "channel_layout",
                           mlt_audio_channel_layout_name(out.layout));
        pdata->expected_frame = mlt_frame_get_position(frame) + 1;
    }

    mlt_cache_item_close(cache_item);
    mlt_service_unlock(MLT_LINK_SERVICE(self));
    return error;
}

static int link_get_frame(mlt_link self, mlt_frame_ptr frame, int index)
{
    int error = 0;
    mlt_position frame_pos = mlt_producer_position(MLT_LINK_PRODUCER(self));

    mlt_producer_seek(self->next, frame_pos);
    error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), frame, index);
    if (error) {
        return error;
    }

    mlt_properties unique_properties = mlt_frame_unique_properties(*frame, MLT_LINK_SERVICE(self));

    // Pass future frames
    int i = 0;
    for (i = 0; i < FUTURE_FRAMES; i++) {
        mlt_position future_pos = frame_pos + i + 1;
        mlt_frame future_frame = NULL;
        mlt_producer_seek(self->next, future_pos);
        error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), &future_frame, index);
        if (error) {
            mlt_log_error(MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int) future_pos);
        }
        char key[19];
        sprintf(key, "%d", (int) future_pos);
        mlt_properties_set_data(unique_properties,
                                key,
                                future_frame,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);
    }

    mlt_frame_push_audio(*frame, (void *) self);
    mlt_frame_push_audio(*frame, link_get_audio);

    mlt_producer_prepare_next(MLT_LINK_PRODUCER(self));

    return error;
}

static void link_close(mlt_link self)
{
    if (self) {
        mlt_service_cache_purge(MLT_LINK_SERVICE(self));
        free(self->child);
        self->close = NULL;
        self->child = NULL;
        mlt_link_close(self);
        free(self);
    }
}

mlt_link link_swresample_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_link self = mlt_link_init();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (self && pdata) {
        pdata->continuity_frame = -1;
        pdata->expected_frame = -1;
        self->child = pdata;

        // Callback registration
        self->configure = link_configure;
        self->get_frame = link_get_frame;
        self->close = link_close;
    } else {
        if (pdata) {
            free(pdata);
        }

        if (self) {
            mlt_link_close(self);
            self = NULL;
        }
    }
    return self;
}
