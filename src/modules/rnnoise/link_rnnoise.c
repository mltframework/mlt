/*
 * link_rnnoise.c -- noise reduction link using RNNoise
 * Copyright (C) 2026 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <framework/mlt.h>
#include <math.h>
#include <rnnoise.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RNNOISE_RATE 48000
#define MAX_CHANNELS 8
#define MIN_RNNOISE_FRAMES 3
#define RNNOISE_STARTUP_DROP_FRAMES 2
// Buffer sizes: at 24fps/48kHz a frame has ~2002 samples.
// With 1 future frame we can have up to ~4004 input samples at once.
// RNNoise frame = 480. Max chunks = ceil(4004/480) = 9 → max out = 9*480 = 4320.
// Use 8192 per channel for headroom.
#define BUF_CAPACITY 8192

typedef struct
{
    // RNNoise per-channel state
    DenoiseState *states[MAX_CHANNELS];
    int n_channels;
    int frequency;

    // Input carry buffer: samples waiting to form the next 480-sample RNNoise chunk.
    // in_carry_count < 480 (a full chunk is processed immediately).
    float in_carry[MAX_CHANNELS][480];
    int in_carry_count;

    // Output carry buffer: processed samples waiting to be delivered.
    // out_carry_count holds how many valid samples are stored from index 0.
    float *out_carry[MAX_CHANNELS];
    int out_carry_count;

    // Continuity tracking
    mlt_position expected_frame;
    mlt_position continuity_frame;
    int continuity_sample; // sample offset within continuity_frame's audio

    // After reset, RNNoise emits two startup frames that are effectively silence.
    // Consume those frames internally so this link adds no output delay.
    int startup_drop_frames;
} private_data;

static void reset_state(mlt_link self)
{
    private_data *pdata = (private_data *) self->child;

    // Destroy existing DenoiseState objects
    for (int c = 0; c < MAX_CHANNELS; c++) {
        if (pdata->states[c]) {
            rnnoise_destroy(pdata->states[c]);
            pdata->states[c] = NULL;
        }
    }

    pdata->n_channels = 0;
    pdata->frequency = 0;
    pdata->in_carry_count = 0;
    pdata->out_carry_count = 0;
    pdata->startup_drop_frames = RNNOISE_STARTUP_DROP_FRAMES;
    pdata->continuity_frame = -1;
    pdata->continuity_sample = 0;
    pdata->expected_frame = -1;
}

static void ensure_states(mlt_link self, int n_channels)
{
    private_data *pdata = (private_data *) self->child;

    if (pdata->n_channels == n_channels)
        return;

    // Destroy old states
    for (int c = 0; c < MAX_CHANNELS; c++) {
        if (pdata->states[c]) {
            rnnoise_destroy(pdata->states[c]);
            pdata->states[c] = NULL;
        }
    }

    // Create new states
    for (int c = 0; c < n_channels && c < MAX_CHANNELS; c++) {
        pdata->states[c] = rnnoise_create(NULL);
    }

    pdata->n_channels = n_channels;
    pdata->in_carry_count = 0;
    pdata->out_carry_count = 0;
    pdata->startup_drop_frames = RNNOISE_STARTUP_DROP_FRAMES;
}

// Copy samples from src (planar float) channel c, starting at sample_offset,
// up to n_src_samples total in src, into dst starting at dst_offset.
// Returns number of samples copied.
static int copy_samples(float *dst,
                        int dst_offset,
                        const float *src_plane, // src for channel c: src + c*src_total_samples
                        int src_offset,
                        int src_total_samples,
                        int count)
{
    int available = src_total_samples - src_offset;
    int n = count < available ? count : available;
    if (n <= 0)
        return 0;
    memcpy(dst + dst_offset, src_plane + src_offset, n * sizeof(float));
    return n;
}

static int link_get_audio(mlt_frame frame,
                          void **buffer,
                          mlt_audio_format *format,
                          int *frequency,
                          int *channels,
                          int *samples)
{
    mlt_link self = (mlt_link) mlt_frame_pop_audio(frame);
    private_data *pdata = (private_data *) self->child;
    int error = 0;
    double link_fps = mlt_producer_get_fps(MLT_LINK_PRODUCER(self));
    if (link_fps <= 0.0)
        link_fps = 25.0;
    mlt_position frame_pos = mlt_frame_get_position(frame);

    // Force 48kHz float for RNNoise
    *frequency = RNNOISE_RATE;
    *format = mlt_audio_float;
    *channels = *channels <= 0 ? 2 : *channels;
    if (*samples <= 0)
        *samples = mlt_audio_calculate_frame_samples(link_fps, RNNOISE_RATE, frame_pos);

    mlt_service_lock(MLT_LINK_SERVICE(self));

    // Detect seek: if not the expected frame, reset everything
    if (pdata->expected_frame != frame_pos) {
        reset_state(self);
        pdata->continuity_frame = frame_pos;
        pdata->continuity_sample = 0;
        pdata->expected_frame = frame_pos;
    }

    // Get current frame's audio (cached after first call)
    struct mlt_audio_s cur_audio;
    mlt_audio_set_values(&cur_audio, NULL, *frequency, *format, *samples, *channels);
    error = mlt_frame_get_audio(frame,
                                &cur_audio.data,
                                &cur_audio.format,
                                &cur_audio.frequency,
                                &cur_audio.channels,
                                &cur_audio.samples);
    if (error || !cur_audio.data || cur_audio.samples <= 0) {
        mlt_service_unlock(MLT_LINK_SERVICE(self));
        return error;
    }

    if (cur_audio.channels > MAX_CHANNELS) {
        mlt_log_warning(MLT_LINK_SERVICE(self),
                        "RNNoise link supports up to %d channels, got %d; bypassing\n",
                        MAX_CHANNELS,
                        cur_audio.channels);
        *buffer = cur_audio.data;
        *frequency = cur_audio.frequency;
        *format = cur_audio.format;
        *channels = cur_audio.channels;
        *samples = cur_audio.samples;
        pdata->expected_frame = frame_pos + 1;
        mlt_service_unlock(MLT_LINK_SERVICE(self));
        return 0;
    }

    // Ensure correct number of RNNoise states
    ensure_states(self, cur_audio.channels);
    *channels = cur_audio.channels;

    // Get unique_properties for future frame lookup
    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_LINK_SERVICE(self));

    // The RNNoise frame size is always 480
    const int rnn_frame = rnnoise_get_frame_size();

    // Allocate output buffer
    struct mlt_audio_s out;
    mlt_audio_set_values(&out,
                         NULL,
                         RNNOISE_RATE,
                         cur_audio.format,
                         cur_audio.samples,
                         cur_audio.channels);
    mlt_audio_alloc_data(&out);
    if (!out.data) {
        mlt_service_unlock(MLT_LINK_SERVICE(self));
        return 1;
    }

    float mix = mlt_properties_get_double(MLT_LINK_PROPERTIES(self), "mix");
    mix = CLAMP(mix, 0.0f, 1.0f);

    // We fill out from out_carry, then generate more by feeding RNNoise chunks.
    int out_delivered = 0;

    while (out_delivered < out.samples) {
        // Drain the output carry buffer first
        if (pdata->out_carry_count > 0) {
            int n_take = out.samples - out_delivered;
            if (n_take > pdata->out_carry_count)
                n_take = pdata->out_carry_count;
            for (int c = 0; c < *channels && c < MAX_CHANNELS; c++) {
                if (pdata->out_carry[c]) {
                    memcpy((float *) out.data + c * out.samples + out_delivered,
                           pdata->out_carry[c],
                           n_take * sizeof(float));
                    // Shift remaining
                    int remaining = pdata->out_carry_count - n_take;
                    if (remaining > 0)
                        memmove(pdata->out_carry[c],
                                pdata->out_carry[c] + n_take,
                                remaining * sizeof(float));
                }
            }
            pdata->out_carry_count -= n_take;
            out_delivered += n_take;
            continue;
        }

        // Need to process more RNNoise frames to fill out_carry.
        // First fill in_carry to 480 samples, crossing as many future MLT
        // frames as needed to satisfy this RNNoise chunk.
        while (pdata->in_carry_count < rnn_frame) {
            // Determine source frame and audio
            float *src_data = NULL;
            int src_total_samples = 0;
            int src_channels = *channels;

            if (pdata->continuity_frame == frame_pos) {
                // Use current frame's audio
                src_data = (float *) cur_audio.data;
                src_total_samples = cur_audio.samples;
            } else {
                // Look up future frame from unique_properties
                if (!unique_properties) {
                    break;
                }
                char key[19];
                int frame_delta = frame_pos - mlt_frame_original_position(frame);
                sprintf(key, "%d", (int) (pdata->continuity_frame - frame_delta));
                mlt_frame src_frame = (mlt_frame) mlt_properties_get_data(unique_properties,
                                                                          key,
                                                                          NULL);
                if (!src_frame) {
                    break;
                }

                // Get audio from the future frame (may be cached)
                struct mlt_audio_s future_audio;
                int future_samples = mlt_audio_calculate_frame_samples(link_fps,
                                                                       RNNOISE_RATE,
                                                                       pdata->continuity_frame);
                mlt_audio_set_values(&future_audio,
                                     NULL,
                                     RNNOISE_RATE,
                                     mlt_audio_float,
                                     future_samples,
                                     *channels);
                int ferr = mlt_frame_get_audio(src_frame,
                                               &future_audio.data,
                                               &future_audio.format,
                                               &future_audio.frequency,
                                               &future_audio.channels,
                                               &future_audio.samples);
                if (ferr || !future_audio.data || future_audio.samples <= 0) {
                    break;
                }
                src_data = (float *) future_audio.data;
                src_total_samples = future_audio.samples;
                src_channels = future_audio.channels;
            }

            // Copy as many samples as possible into in_carry (up to rnn_frame)
            int needed = rnn_frame - pdata->in_carry_count;
            int copied_any = 0;
            for (int c = 0; c < *channels && c < MAX_CHANNELS; c++) {
                int n = 0;
                if (c < src_channels) {
                    float *src_plane = src_data + c * src_total_samples;
                    n = copy_samples(pdata->in_carry[c],
                                     pdata->in_carry_count,
                                     src_plane,
                                     pdata->continuity_sample,
                                     src_total_samples,
                                     needed);
                } else {
                    // Missing source channels are treated as silence.
                    memset(pdata->in_carry[c] + pdata->in_carry_count, 0, needed * sizeof(float));
                    n = needed;
                }
                if (c == 0)
                    copied_any = n; // use channel 0 to track
            }

            if (copied_any <= 0) {
                // Skip empty source frame and continue to the next one.
                pdata->continuity_frame++;
                pdata->continuity_sample = 0;
                continue;
            }

            pdata->in_carry_count += copied_any;
            pdata->continuity_sample += copied_any;

            // Check if we've exhausted this frame's audio
            if (pdata->continuity_sample >= src_total_samples) {
                pdata->continuity_frame++;
                pdata->continuity_sample = 0;
            }
        }

        if (pdata->in_carry_count < rnn_frame) {
            // Not enough input to complete a chunk — zero-pad the remainder
            for (int c = 0; c < *channels && c < MAX_CHANNELS; c++) {
                memset(pdata->in_carry[c] + pdata->in_carry_count,
                       0,
                       (rnn_frame - pdata->in_carry_count) * sizeof(float));
            }
            pdata->in_carry_count = rnn_frame;
        }

        // Process one 480-sample RNNoise chunk per channel
        float rnn_in[480];
        float rnn_out[480];

        // Ensure out_carry buffers are allocated
        for (int c = 0; c < *channels && c < MAX_CHANNELS; c++) {
            if (!pdata->out_carry[c]) {
                pdata->out_carry[c] = (float *) calloc(BUF_CAPACITY, sizeof(float));
                if (!pdata->out_carry[c]) {
                    error = 1;
                    goto done;
                }
            }
        }

        int drop_chunk = pdata->startup_drop_frames > 0;
        int out_base = pdata->out_carry_count;
        if (!drop_chunk && out_base + rnn_frame > BUF_CAPACITY) {
            // Buffer overflow safeguard — should not happen with BUF_CAPACITY=8192
            error = 1;
            goto done;
        }

        for (int c = 0; c < *channels && c < MAX_CHANNELS; c++) {
            // Scale up for RNNoise (expects ±32768)
            for (int s = 0; s < rnn_frame; s++)
                rnn_in[s] = pdata->in_carry[c][s] * 32768.0f;

            if (pdata->states[c]) {
                rnnoise_process_frame(pdata->states[c], rnn_out, rnn_in);
            } else {
                mlt_log_error(MLT_LINK_SERVICE(self),
                              "Missing RNNoise state for channel %d; bypassing denoise\n",
                              c);
                memcpy(rnn_out, rnn_in, sizeof(rnn_out));
            }

            // Scale back and apply wet/dry mix with aligned dry signal.
            // Startup RNNoise delay is compensated by dropping first two output chunks.
            for (int s = 0; s < rnn_frame; s++) {
                float wet = rnn_out[s] / 32768.0f;
                float dry = pdata->in_carry[c][s];
                if (!drop_chunk)
                    pdata->out_carry[c][out_base + s] = dry + mix * (wet - dry);
            }
        }

        if (drop_chunk)
            pdata->startup_drop_frames--;
        else
            pdata->out_carry_count += rnn_frame;
        pdata->in_carry_count = 0;
    }

done:
    if (error) {
        // Return silence on error
        mlt_audio_silence(&out, out.samples, 0);
    }

    int out_size = mlt_audio_format_size(out.format, out.samples, out.channels);
    mlt_frame_set_audio(frame, out.data, out.format, out_size, out.release_data);
    mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);

    pdata->expected_frame = frame_pos + 1;

    mlt_service_unlock(MLT_LINK_SERVICE(self));
    return error;
}

static int link_get_frame(mlt_link self, mlt_frame_ptr frame, int index)
{
    int error = 0;
    private_data *pdata = (private_data *) self->child;
    mlt_position frame_pos = mlt_producer_position(MLT_LINK_PRODUCER(self));
    double fps = mlt_producer_get_fps(MLT_LINK_PRODUCER(self));
    if (fps <= 0.0)
        fps = 25.0;
    int rnn_frame = rnnoise_get_frame_size();
    int needed_samples = MIN_RNNOISE_FRAMES * rnn_frame;
    int frame_samples = mlt_audio_calculate_frame_samples(fps, RNNOISE_RATE, frame_pos);
    int startup_drop = pdata ? pdata->startup_drop_frames : RNNOISE_STARTUP_DROP_FRAMES;
    int output_coverage_samples = frame_samples + (startup_drop * rnn_frame);
    if (output_coverage_samples > needed_samples)
        needed_samples = output_coverage_samples;
    int available_samples = mlt_audio_calculate_frame_samples(fps, RNNOISE_RATE, frame_pos);
    int future_frames_needed = 0;

    while (available_samples < needed_samples) {
        mlt_position future_pos = frame_pos + future_frames_needed + 1;
        available_samples += mlt_audio_calculate_frame_samples(fps, RNNOISE_RATE, future_pos);
        future_frames_needed++;
    }

    mlt_producer_seek(self->next, frame_pos);
    error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), frame, index);
    if (error) {
        return error;
    }

    mlt_properties unique_properties = mlt_frame_unique_properties(*frame, MLT_LINK_SERVICE(self));

    // Fetch and store enough future frames to provide at least 3 RNNoise chunks.
    for (int i = 0; i < future_frames_needed; i++) {
        mlt_position future_pos = frame_pos + i + 1;
        mlt_frame future_frame = NULL;
        mlt_producer_seek(self->next, future_pos);
        error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), &future_frame, index);
        if (error) {
            mlt_log_error(MLT_LINK_SERVICE(self),
                          "Error getting future frame: %d\n",
                          (int) future_pos);
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

static void link_configure(mlt_link self, mlt_profile chain_profile)
{
    // Operate at the same frame rate as the next link in the chain
    mlt_service_set_profile(MLT_LINK_SERVICE(self),
                            mlt_service_profile(MLT_PRODUCER_SERVICE(self->next)));
}

static void link_close(mlt_link self)
{
    if (self) {
        private_data *pdata = (private_data *) self->child;
        if (pdata) {
            for (int c = 0; c < MAX_CHANNELS; c++) {
                if (pdata->states[c]) {
                    rnnoise_destroy(pdata->states[c]);
                }
                free(pdata->out_carry[c]);
            }
            free(pdata);
        }
        self->child = NULL;
        self->close = NULL;
        mlt_link_close(self);
        free(self);
    }
}

mlt_link link_rnnoise_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_link self = mlt_link_init();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (self && pdata) {
        pdata->expected_frame = -1;
        pdata->continuity_frame = -1;
        pdata->continuity_sample = 0;
        self->child = pdata;

        // Set default property
        mlt_properties_set_double(MLT_LINK_PROPERTIES(self), "mix", 1.0);

        // Register callbacks
        self->configure = link_configure;
        self->get_frame = link_get_frame;
        self->close = link_close;
    } else {
        free(pdata);
        if (self) {
            mlt_link_close(self);
            self = NULL;
        }
    }

    return self;
}
