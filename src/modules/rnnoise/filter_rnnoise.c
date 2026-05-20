/*
 * filter_rnnoise.c -- audio noise reduction using RNNoise
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <rnnoise.h>

#include <stdlib.h>
#include <string.h>

#define MAX_CHANNELS 8
#define RNNOISE_RATE 48000

// Input carry: at most rnn_frame-1 = 479 unprocessed samples per channel.
#define IN_CARRY_CAPACITY 480
// Output carry: at 24fps/48kHz a frame is ~2000 samples.
// One extra RNNoise chunk (480 samples) above n_samples is the maximum excess.
// 1024 is ample.
#define OUT_CARRY_CAPACITY 1024

typedef struct
{
    DenoiseState *states[MAX_CHANNELS];
    int n_channels;
    int frequency;

    // Input carry: raw (unprocessed) samples left over from the previous frame
    // that didn't form a complete 480-sample RNNoise chunk.
    // in_carry_count < rnn_frame always.
    float in_carry[MAX_CHANNELS][IN_CARRY_CAPACITY];
    int in_carry_count;

    // Output carry: already-processed, mix-applied samples that belong to
    // future MLT frames due to the output count exceeding n_samples.
    // out_carry_count is typically small (< rnn_frame).
    float *out_carry[MAX_CHANNELS];
    int out_carry_count;

    // Frame-position continuity tracking.
    mlt_position expected_frame;

    // Dry-signal delay ring buffer for wet/dry mix alignment.
    // RNNoise synthesizes from the previous call's FFT, which itself analyzed
    // the frame before that ([analysis_mem, in]), so the output at call N
    // reconstructs in_{N-2} — exactly 2 * rnn_frame = 960 samples of delay.
    // We delay the dry signal by the same amount so both are aligned.
    float dry_ring[MAX_CHANNELS][960];
    int dry_ring_pos;
} private_data;

static void reset_all(mlt_filter filter, private_data *pdata, int channels)
{
    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "reset (channels=%d in_carry=%d out_carry=%d)\n",
                  channels,
                  pdata->in_carry_count,
                  pdata->out_carry_count);
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (pdata->states[i]) {
            rnnoise_destroy(pdata->states[i]);
            pdata->states[i] = NULL;
        }
    }
    for (int i = 0; i < channels && i < MAX_CHANNELS; i++) {
        pdata->states[i] = rnnoise_create(NULL);
        if (!pdata->states[i]) {
            mlt_log_error(MLT_FILTER_SERVICE(filter),
                          "Failed to create RNNoise state for channel %d\n",
                          i);
        }
    }
    pdata->n_channels = channels;
    pdata->in_carry_count = 0;
    pdata->out_carry_count = 0;
    memset(pdata->dry_ring, 0, sizeof(pdata->dry_ring));
    pdata->dry_ring_pos = 0;
}

static int rnnoise_get_audio(mlt_frame frame,
                             void **buffer,
                             mlt_audio_format *format,
                             int *frequency,
                             int *channels,
                             int *samples)
{
    mlt_filter filter = mlt_frame_pop_audio(frame);
    mlt_properties filter_props = MLT_FILTER_PROPERTIES(filter);
    private_data *pdata = (private_data *) filter->child;

    // RNNoise requires 48 kHz float input
    *frequency = RNNOISE_RATE;
    *format = mlt_audio_float;

    int error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (error || *samples == 0)
        return error;

    if (*format != mlt_audio_float && frame->convert_audio != NULL)
        frame->convert_audio(frame, buffer, format, mlt_audio_float);

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    const int n_samples = *samples;
    const int ch = *channels;
    const int rnn_frame = rnnoise_get_frame_size(); // always 480
    mlt_position frame_pos = mlt_frame_get_position(frame);

    // Detect format change or seek discontinuity -> full reset
    if (pdata->n_channels != ch || pdata->frequency != *frequency
        || pdata->expected_frame != frame_pos) {
        reset_all(filter, pdata, ch);
        pdata->frequency = *frequency;
    }

    // Ensure output carry buffers are allocated
    for (int c = 0; c < ch && c < MAX_CHANNELS; c++) {
        if (!pdata->out_carry[c]) {
            pdata->out_carry[c] = (float *) calloc(OUT_CARRY_CAPACITY, sizeof(float));
            if (!pdata->out_carry[c]) {
                mlt_log_error(MLT_FILTER_SERVICE(filter),
                              "Failed to allocate output carry buffer for channel %d\n",
                              c);
                mlt_service_unlock(MLT_FILTER_SERVICE(filter));
                return 1;
            }
        }
    }

    double mix = mlt_properties_get_double(filter_props, "mix");
    if (mix < 0.0)
        mix = 0.0;
    if (mix > 1.0)
        mix = 1.0;

    // ------------------------------------------------------------------
    // Algorithm: input-carry + output-carry
    //
    // Input carry (raw, unprocessed, < rnn_frame samples):
    //   Leftover samples from the previous frame that did not form a
    //   complete 480-sample RNNoise chunk. Prepended to this frame's input
    //   before processing. RNNoise is NEVER fed silence after the first frame.
    //
    // On the very first call (in_carry_count == 0, out_carry_count == 0):
    //   Prepend `pad` silence samples so that total input is a multiple of
    //   rnn_frame. This introduces a one-time delay of `pad` samples and
    //   ensures in_carry_count remains 0 afterwards (pad is chosen to align).
    //   Because all chunks on the first frame use real audio (after the
    //   initial silence prefix), no silence is ever fed again.
    //
    // Output carry (processed, mix-applied):
    //   When the number of processed samples exceeds n_samples, the excess
    //   is stored here and prepended to the next frame's output.
    //
    // The cycle for n_samples=2000, rnn_frame=480:
    //   Frame 1:  in_carry=0, pad=400, total=2400, chunks=5, out=2400,
    //             out_carry=400, in_carry=0
    //   Frame 2:  in_carry=0, pad=0, total=2000, chunks=4, out=1920,
    //             avail=400+1920=2320, deliver 2000, out_carry=320
    //   ...
    //   Frame 6:  avail=80+1920=2000, deliver 2000, out_carry=0
    //   Frame 7:  same as frame 2
    //   (RNNoise is never fed silence after frame 1's initial pad)
    // ------------------------------------------------------------------

    // On first call, compute silence prefix so (pad + n_samples) % rnn_frame == 0
    int pad = 0;
    if (pdata->in_carry_count == 0 && pdata->out_carry_count == 0) {
        int remainder = n_samples % rnn_frame;
        if (remainder != 0)
            pad = rnn_frame - remainder;
    }

    // Total input fed to RNNoise this call:
    //   [silence(pad)] ++ [in_carry] ++ [in_ch]
    // But after the first frame, pad==0 always (see below).
    int total_in = pad + pdata->in_carry_count + n_samples;
    int n_chunks = total_in / rnn_frame; // floor: only complete chunks, no silence padding
    int new_in_carry = total_in - n_chunks * rnn_frame; // leftover raw input (< rnn_frame)

    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "frame=%d n_samples=%d pad=%d in_carry=%d out_carry=%d "
                  "n_chunks=%d new_in_carry=%d\n",
                  (int) frame_pos,
                  n_samples,
                  pad,
                  pdata->in_carry_count,
                  pdata->out_carry_count,
                  n_chunks,
                  new_in_carry);

    float *src = (float *) *buffer;

    // Allocate output buffer
    struct mlt_audio_s out;
    mlt_audio_set_values(&out, NULL, *frequency, mlt_audio_float, n_samples, ch);
    mlt_audio_alloc_data(&out);
    if (!out.data) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate output audio buffer\n");
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));
        return 1;
    }
    float *dst = (float *) out.data;

    // Scratch buffers for one RNNoise chunk (stack)
    float frame_in[480];
    float frame_out[480];

    // We build a new input carry as we go (same across all channels,
    // but stored per-channel for the raw values).
    // in_carry_new_count is computed on channel 0 and reused.
    int new_in_carry_count = 0;
    int new_out_carry_count = 0;

    // Snapshot ring position so each channel processes the same virtual positions.
    const int ring_size = 2 * rnn_frame;
    int ring_start = pdata->dry_ring_pos;

    for (int c = 0; c < ch && c < MAX_CHANNELS; c++) {
        pdata->dry_ring_pos = ring_start; // reset for each channel
        float *in_ch = src + (size_t) c * n_samples;
        float *out_ch = dst + (size_t) c * n_samples;
        float *out_carry_ch = pdata->out_carry[c];

        // 1. Drain output carry into out_ch
        int from_out_carry = pdata->out_carry_count < n_samples ? pdata->out_carry_count
                                                                : n_samples;
        if (c == 0 && pdata->out_carry_count > n_samples)
            mlt_log_warning(MLT_FILTER_SERVICE(filter),
                            "frame=%d ch=%d out_carry_count=%d > n_samples=%d\n",
                            (int) frame_pos,
                            c,
                            pdata->out_carry_count,
                            n_samples);
        memcpy(out_ch, out_carry_ch, from_out_carry * sizeof(float));
        int out_carry_left = pdata->out_carry_count - from_out_carry;
        if (out_carry_left > 0)
            memmove(out_carry_ch, out_carry_ch + from_out_carry, out_carry_left * sizeof(float));

        int out_pos = from_out_carry;   // next write index in out_ch
        int carry_pos = out_carry_left; // next write index in out_carry_ch

        // 2. Process n_chunks complete RNNoise frames.
        //    Virtual read position into the combined input stream:
        //      negative indices          → silence (pad prefix, first frame only)
        //      [0 .. in_carry_count)     → pdata->in_carry[c]
        //      [in_carry_count .. total) → in_ch
        int in_virtual = -pad; // starts in the silence region on first frame

        for (int k = 0; k < n_chunks; k++) {
            // Build one rnn_frame-sample input
            for (int s = 0; s < rnn_frame; s++) {
                int p = in_virtual + s;
                float val;
                if (p < 0) {
                    val = 0.0f; // silence prefix (first frame only)
                } else if (p < pdata->in_carry_count) {
                    val = pdata->in_carry[c][p] * 32768.0f;
                } else {
                    int q = p - pdata->in_carry_count;
                    val = (q < n_samples) ? in_ch[q] * 32768.0f : 0.0f;
                }
                frame_in[s] = val;
            }
            rnnoise_process_frame(pdata->states[c], frame_out, frame_in);

            // Distribute output to out_ch or out_carry
            for (int s = 0; s < rnn_frame; s++) {
                int p = in_virtual + s;
                float raw;
                if (p < 0) {
                    raw = 0.0f;
                } else if (p < pdata->in_carry_count) {
                    raw = pdata->in_carry[c][p];
                } else {
                    int q = p - pdata->in_carry_count;
                    raw = (q < n_samples) ? in_ch[q] : 0.0f;
                }
                float denoised = frame_out[s] / 32768.0f;
                // Delay dry by 2*rnn_frame to match RNNoise's two-frame internal delay.
                int ring_idx = pdata->dry_ring_pos % ring_size;
                float delayed_raw = pdata->dry_ring[c][ring_idx]; // read (2*rnn_frame ago)
                pdata->dry_ring[c][ring_idx] = raw;               // write current
                pdata->dry_ring_pos++;
                float mixed = delayed_raw * (1.0f - (float) mix) + denoised * (float) mix;

                if (out_pos < n_samples) {
                    out_ch[out_pos++] = mixed;
                } else {
                    if (carry_pos < OUT_CARRY_CAPACITY) {
                        out_carry_ch[carry_pos++] = mixed;
                    } else {
                        mlt_log_warning(MLT_FILTER_SERVICE(filter),
                                        "frame=%d ch=%d out_carry overflow at pos=%d\n",
                                        (int) frame_pos,
                                        c,
                                        carry_pos);
                    }
                }
            }
            in_virtual += rnn_frame;
        }

        // 3. Save leftover raw input into in_carry (the samples not yet fed to RNNoise)
        //    These are the last new_in_carry samples of the combined input stream,
        //    which all come from in_ch (pad is only on first frame and aligns perfectly).
        int in_carry_src = n_samples - new_in_carry; // start of leftover in in_ch
        for (int s = 0; s < new_in_carry; s++)
            pdata->in_carry[c][s] = in_ch[in_carry_src + s];

        if (c == 0) {
            if (out_pos != n_samples)
                mlt_log_warning(MLT_FILTER_SERVICE(filter),
                                "frame=%d ch=%d output not fully filled: "
                                "out_pos=%d n_samples=%d (gap=%d)\n",
                                (int) frame_pos,
                                c,
                                out_pos,
                                n_samples,
                                n_samples - out_pos);
            new_in_carry_count = new_in_carry;
            new_out_carry_count = carry_pos;
        }
    }

    pdata->in_carry_count = new_in_carry_count;
    pdata->out_carry_count = new_out_carry_count;
    pdata->expected_frame = frame_pos + 1;

    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "frame=%d new_in_carry=%d new_out_carry=%d\n",
                  (int) frame_pos,
                  new_in_carry_count,
                  new_out_carry_count);

    mlt_frame_set_audio(frame, out.data, mlt_audio_float, 0, out.release_data);
    *buffer = out.data;
    *format = mlt_audio_float;

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));
    return 0;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_audio(frame, filter);
    mlt_frame_push_audio(frame, (void *) rnnoise_get_audio);
    return frame;
}

static void close_filter(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;
    if (pdata) {
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (pdata->states[i]) {
                rnnoise_destroy(pdata->states[i]);
                pdata->states[i] = NULL;
            }
            free(pdata->out_carry[i]);
        }
        free(pdata);
        filter->child = NULL;
    }
}

mlt_filter filter_rnnoise_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (filter && pdata) {
        pdata->expected_frame = -1;
        filter->process = filter_process;
        filter->close = close_filter;
        filter->child = pdata;
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "mix", 1.0);
    } else {
        mlt_log_error(NULL, "RNNoise filter: failed to allocate resources\n");
        if (filter)
            mlt_filter_close(filter);
        free(pdata);
        filter = NULL;
    }
    return filter;
}
