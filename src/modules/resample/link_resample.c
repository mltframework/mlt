/*
 * link_resample.c
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

#include <framework/mlt_frame.h>
#include <framework/mlt_link.h>
#include <framework/mlt_log.h>

#include <samplerate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private Types
#define FRAME_CACHE_SIZE 3

typedef struct
{
    // Used by get_frame
    mlt_frame frame_cache[FRAME_CACHE_SIZE];
    // Used by get_audio
    mlt_position expected_frame;
    mlt_position continuity_frame;
    int continuity_sample;
    SRC_STATE *s;
    int channels;
} private_data;

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
    int src_frequency = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "audio_frequency");
    src_frequency = src_frequency <= 0 ? *frequency : src_frequency;
    int src_samples = mlt_audio_calculate_frame_samples(mlt_producer_get_fps(
                                                            MLT_LINK_PRODUCER(self)),
                                                        src_frequency,
                                                        mlt_frame_get_position(frame));

    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    struct mlt_audio_s in;
    struct mlt_audio_s out;

    mlt_audio_set_values(&in, *buffer, src_frequency, *format, src_samples, *channels);
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

    if (in.frequency == requested_frequency && !pdata->s) {
        // No change necessary
        mlt_audio_get_values(&in, buffer, frequency, format, samples, channels);
        return error;
    }

    // Set up audio structures and allocate output buffer
    in.format = mlt_audio_f32le;
    out.format = mlt_audio_f32le;
    out.channels = in.channels;
    mlt_audio_alloc_data(&out);
    mlt_log_debug(MLT_LINK_SERVICE(self), "%dHz -> %dHz\n", in.frequency, out.frequency);

    mlt_service_lock(MLT_LINK_SERVICE(self));

    // Recreate the resampler if necessary
    if (!pdata->s || pdata->channels != in.channels
        || pdata->expected_frame != mlt_frame_get_position(frame)) {
        mlt_log_info(MLT_LINK_SERVICE(self), "%dHz -> %dHz\n", in.frequency, out.frequency);
        pdata->s = src_delete(pdata->s);
        pdata->s = src_new(SRC_SINC_BEST_QUALITY, in.channels, &error);
        pdata->channels = in.channels;
        pdata->expected_frame = mlt_frame_get_position(frame);
        pdata->continuity_frame = mlt_frame_get_position(frame);
        pdata->continuity_sample = 0;
    }

    int received_samples = 0;

    while (received_samples < out.samples && !error) {
        mlt_frame src_frame = NULL;
        if (pdata->continuity_frame == mlt_frame_get_position(frame)) {
            src_frame = frame;
        } else {
            // The input frame is insufficient to fill the output frame.
            // Request audio from future frames.
            mlt_properties unique_properties
                = mlt_frame_get_unique_properties(frame, MLT_LINK_SERVICE(self));
            if (!unique_properties) {
                error = 1;
                break;
            }
            char key[19];
            int frame_delta = mlt_frame_get_position(frame) - mlt_frame_original_position(frame);
            sprintf(key, "%d", pdata->continuity_frame - frame_delta);
            src_frame = (mlt_frame) mlt_properties_get_data(unique_properties, key, NULL);
        }

        if (!src_frame) {
            mlt_log_error(MLT_LINK_SERVICE(self), "Frame not found: %d\n", pdata->continuity_frame);
            error = 1;
            break;
        }

        in.samples = mlt_audio_calculate_frame_samples(mlt_producer_get_fps(MLT_LINK_PRODUCER(self)),
                                                       in.frequency,
                                                       pdata->continuity_frame);
        error = mlt_frame_get_audio(src_frame,
                                    &in.data,
                                    &in.format,
                                    &in.frequency,
                                    &in.channels,
                                    &in.samples);
        if (error) {
            mlt_log_error(MLT_LINK_SERVICE(self),
                          "Unable to get audio for %d\n",
                          pdata->continuity_frame);
            break;
        }

        while (pdata->continuity_sample < in.samples && received_samples < out.samples) {
            SRC_DATA data;
            data.end_of_input = 0;
            data.src_ratio = (double) out.frequency / (double) in.frequency;
            data.data_out = (float *) out.data + (received_samples * out.channels);
            data.output_frames = out.samples - received_samples;
            data.data_in = (float *) in.data + (pdata->continuity_sample * in.channels);
            // Calculate the fewest samples that can be sent to the resampler to get the needed output.
            // Round down just to be sure. This is done to reduce the latency through the resampler and
            // to borrow as few samples from future frames as possible.
            data.input_frames = ((data.output_frames * in.frequency) / out.frequency) - 1;
            if (data.input_frames > (in.samples - pdata->continuity_sample)) {
                data.input_frames = in.samples - pdata->continuity_sample;
            }
            if (data.input_frames <= 0) {
                data.input_frames = 1;
            }

            // Resample the audio
            src_set_ratio(pdata->s, data.src_ratio);
            error = src_process(pdata->s, &data);
            if (error) {
                mlt_log_error(MLT_LINK_SERVICE(self),
                              "%s %d,%d,%d\n",
                              src_strerror(error),
                              in.frequency,
                              in.samples,
                              out.frequency);
                break;
            }
            received_samples += data.output_frames_gen;
            pdata->continuity_sample += data.input_frames_used;
        }
        if (pdata->continuity_sample >= in.samples) {
            // All the samples from this frame are used.
            pdata->continuity_sample = 0;
            pdata->continuity_frame++;
        }
    }

    if (received_samples == 0) {
        mlt_log_info(MLT_LINK_SERVICE(self), "Failed to get any samples - return silence\n");
        mlt_audio_silence(&out, out.samples, 0);
    } else if (received_samples < out.samples) {
        // Duplicate samples to return the exact number requested.
        mlt_audio_copy(&out, &out, received_samples, 0, out.samples - received_samples);
    }

    mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
    mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);
    mlt_properties_set(frame_properties,
                       "channel_layout",
                       mlt_audio_channel_layout_name(out.layout));
    pdata->expected_frame = mlt_frame_get_position(frame) + 1;

    mlt_service_unlock(MLT_LINK_SERVICE(self));
    return error;
}

static int link_get_frame(mlt_link self, mlt_frame_ptr frame, int index)
{
    int result = 0;
    private_data *pdata = (private_data *) self->child;
    mlt_position frame_pos = mlt_producer_position(MLT_LINK_PRODUCER(self));

    // Cycle out the first frame in the cache
    mlt_frame_close(pdata->frame_cache[0]);
    // Shift all the frames or get new
    int i = 0;
    for (i = 0; i < FRAME_CACHE_SIZE - 1; i++) {
        mlt_position pos = frame_pos + i;
        mlt_frame next_frame = pdata->frame_cache[i + 1];
        if (next_frame && mlt_frame_get_position(next_frame) == pos) {
            // Shift the frame if it is correct
            pdata->frame_cache[i] = next_frame;
        } else {
            // Get a new frame if the next cache frame is not the needed frame
            mlt_frame_close(next_frame);
            mlt_producer_seek(self->next, pos);
            result = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next),
                                           &pdata->frame_cache[i],
                                           index);
            if (result) {
                mlt_log_error(MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int) pos);
            }
        }
    }
    // Get the new last frame in the cache
    mlt_producer_seek(self->next, frame_pos + i);
    result = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), &pdata->frame_cache[i], index);
    if (result) {
        mlt_log_error(MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int) frame_pos + i);
    }

    *frame = pdata->frame_cache[0];
    mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(*frame));

    // Attach the next frames to the current frame in case they are needed for sample rate conversion
    mlt_properties unique_properties = mlt_frame_unique_properties(*frame, MLT_LINK_SERVICE(self));
    for (int i = 1; i < FRAME_CACHE_SIZE; i++) {
        char key[19];
        sprintf(key, "%d", (int) mlt_frame_get_position(pdata->frame_cache[i]));
        mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(pdata->frame_cache[i]));
        mlt_properties_set_data(unique_properties,
                                key,
                                pdata->frame_cache[i],
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);
    }
    mlt_frame_push_audio(*frame, (void *) self);
    mlt_frame_push_audio(*frame, link_get_audio);

    mlt_producer_prepare_next(MLT_LINK_PRODUCER(self));

    return result;
}

static void link_close(mlt_link self)
{
    if (self) {
        private_data *pdata = (private_data *) self->child;
        if (pdata) {
            src_delete(pdata->s);
            free(pdata);
        }
        self->close = NULL;
        self->child = NULL;
        mlt_link_close(self);
        free(self);
    }
}

mlt_link link_resample_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
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
