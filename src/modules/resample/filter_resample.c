/*
 * filter_resample.c -- adjust audio sample frequency
 * Copyright (C) 2003-2021 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <samplerate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROCESS_BUFF_SIZE (10000 * sizeof(float))

// Private Types
typedef struct
{
    SRC_STATE *s;
    int error;
    int channels;
    float buff[PROCESS_BUFF_SIZE];
    int leftover_samples;
} private_data;

/** Get the audio.
*/

static int resample_get_audio(mlt_frame frame,
                              void **buffer,
                              mlt_audio_format *format,
                              int *frequency,
                              int *channels,
                              int *samples)
{
    mlt_filter filter = mlt_frame_pop_audio(frame);
    private_data *pdata = (private_data *) filter->child;
    struct mlt_audio_s in;
    struct mlt_audio_s out;
    mlt_audio_set_values(&out, NULL, *frequency, *format, *samples, *channels);

    // Apply user requested rate - else will normalize to consumer requested rate;
    if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "frequency")) {
        out.frequency = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "frequency");
    }

    // Get the producer's audio
    int error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (error || *format == mlt_audio_none || *frequency <= 0 || out.frequency <= 0
        || *channels <= 0) {
        // Error situation. Do not attempt to convert.
        mlt_log_error(MLT_FILTER_SERVICE(filter),
                      "Invalid Parameters: %dS - %dHz %dC %s -> %dHz %dC %s\n",
                      *samples,
                      *frequency,
                      *channels,
                      mlt_audio_format_name(*format),
                      out.frequency,
                      out.channels,
                      mlt_audio_format_name(out.format));
        return error;
    }

    if (*samples == 0) {
        // Noting to convert. No error message needed.
        return error;
    }

    if (*frequency == out.frequency && !pdata) {
        // No frequency change, and there is no stored state.
        return error;
    }

    // *Proceed to convert the sampling frequency*

    // The converter operates on interleaved float. Convert the samples if necessary
    if (*format != mlt_audio_f32le) {
        // Do not convert to float unless we need to change the rate
        frame->convert_audio(frame, buffer, format, mlt_audio_f32le);
    }

    // Set up audio structures and allocate output buffer
    mlt_audio_set_values(&in, *buffer, *frequency, *format, *samples, *channels);
    out.format = in.format;
    out.channels = in.channels;
    mlt_audio_alloc_data(&out);
    mlt_log_debug(MLT_FILTER_SERVICE(filter), "%dHz -> %dHz\n", in.frequency, out.frequency);

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    // Create the private data if it does not exist
    if (!pdata) {
        pdata = (private_data *) calloc(1, sizeof(private_data));
        pdata->s = NULL;
        pdata->channels = 0;
        pdata->leftover_samples = 0;
        filter->child = pdata;
    }

    // Recreate the resampler if necessary
    if (!pdata->s || pdata->channels != in.channels) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Create resample state %d channels\n",
                      in.channels);
        pdata->s = src_delete(pdata->s);
        pdata->s = src_new(SRC_SINC_BEST_QUALITY, in.channels, &pdata->error);
        pdata->channels = in.channels;
    }

    int total_consumed_samples = 0;
    int consumed_samples = 0;
    int received_samples = 0;
    int process_buff_samples = PROCESS_BUFF_SIZE / sizeof(float) / in.channels;

    // First copy samples that are leftover from the previous frame
    if (pdata->leftover_samples) {
        int samples_to_copy = pdata->leftover_samples;
        if (samples_to_copy > out.samples) {
            samples_to_copy = out.samples;
        }
        memcpy(out.data, pdata->buff, samples_to_copy * out.channels * sizeof(float));
        received_samples += samples_to_copy;
        pdata->leftover_samples -= samples_to_copy;
    }

    // Process all input samples
    while (total_consumed_samples < in.samples || received_samples < out.samples) {
        if (pdata->leftover_samples) {
            mlt_log_error(MLT_FILTER_SERVICE(filter),
                          "Discard leftover samples %d\n",
                          pdata->leftover_samples);
            pdata->leftover_samples = 0;
        }

        if (consumed_samples >= in.samples) {
            // Continue to repeat input samples into the resampler until it
            // provides the desired number of samples out.
            consumed_samples = 0;
            mlt_log_debug(MLT_FILTER_SERVICE(filter), "Repeat samples\n");
        }

        SRC_DATA data;
        data.end_of_input = 0;
        data.src_ratio = (double) out.frequency / (double) in.frequency;
        data.data_in = (float *) in.data + (consumed_samples * in.channels);
        data.input_frames = in.samples - consumed_samples;
        data.data_out = pdata->buff;
        data.output_frames = process_buff_samples;
        if (total_consumed_samples >= in.samples) {
            // All input samples have been read once.
            // Limit the output frames to the minimum necessary to fill the output frame.
            // Limit the input frames to 1 at a time to minimize the duplicated samples.
            // Sometimes one input frame can cause many frames to be output from the resampler.
            data.input_frames = 1;
            if (data.output_frames > (out.samples - received_samples)) {
                data.output_frames = out.samples - received_samples;
            }
        }

        // Resample the audio
        src_set_ratio(pdata->s, data.src_ratio);
        error = src_process(pdata->s, &data);
        if (error) {
            mlt_log_error(MLT_FILTER_SERVICE(filter),
                          "%s %d,%d,%d\n",
                          src_strerror(error),
                          in.frequency,
                          in.samples,
                          out.frequency);
            break;
        }

        // Copy resampled samples from buff to output
        if (data.output_frames_gen) {
            float *dst = (float *) out.data + (received_samples * out.channels);
            int samples_to_copy = data.output_frames_gen;
            if (samples_to_copy > (out.samples - received_samples)) {
                samples_to_copy = out.samples - received_samples;
            }
            int bytes_to_copy = samples_to_copy * out.channels * sizeof(float);
            memcpy(dst, pdata->buff, bytes_to_copy);
            if (samples_to_copy < data.output_frames_gen) {
                // Move leftover samples to the beginning of buff to use next time
                pdata->leftover_samples = data.output_frames_gen - samples_to_copy;
                float *src = pdata->buff + (samples_to_copy * out.channels);
                memmove(pdata->buff, src, pdata->leftover_samples * out.channels * sizeof(float));
            }
            received_samples += samples_to_copy;
        }
        consumed_samples += data.input_frames_used;
        total_consumed_samples += data.input_frames_used;
    }

    mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
    mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    if (mlt_frame_is_test_audio(frame) == 0) {
        mlt_frame_push_audio(frame, filter);
        mlt_frame_push_audio(frame, resample_get_audio);
    }

    return frame;
}

static void close_filter(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;
    if (pdata) {
        if (pdata->s) {
            src_delete(pdata->s);
        }
        free(pdata);
        filter->child = NULL;
    }
}

/** Constructor for the filter.
*/

mlt_filter filter_resample_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter) {
        filter->process = filter_process;
        filter->close = close_filter;
        filter->child = NULL;
    } else {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to initialize\n");
    }
    return filter;
}
