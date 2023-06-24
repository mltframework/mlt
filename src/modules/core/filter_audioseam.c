/*
 * filter_audioseam.c -- smooth seams between clips in a playlist
 * Copyright (C) 2023 Meltytech, LLC
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

#include <framework/mlt.h>

#include <math.h>
#include <stdlib.h>

typedef struct
{
    struct mlt_audio_s prev_audio;
} private_data;

static float db_delta(float a, float b)
{
    float dba = 0;
    float dbb = 0;
    const float essentially_zero = 0.001;
    // Calculate db from zero
    if (fabs(a) > essentially_zero) {
        dba = log10(fabs(a)) * 20;
    }
    if (fabs(b) > essentially_zero) {
        dbb = log10(fabs(b)) * 20;
    }
    // Apply sign
    if (a < 0) {
        dba *= -1.0;
    }
    if (b < 0) {
        dba *= -1;
    }
    return dba - dbb;
}

static int filter_get_audio(mlt_frame frame,
                            void **buffer,
                            mlt_audio_format *format,
                            int *frequency,
                            int *channels,
                            int *samples)
{
    mlt_filter filter = mlt_frame_pop_audio(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    private_data *pdata = (private_data *) filter->child;
    int clip_position = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                               "meta.playlist.clip_position");
    int clip_length = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                             "meta.playlist.clip_length");

    if (clip_length == 0 || (clip_position != 0 && clip_position != (clip_length - 1))) {
        // Only operate on the first and last frame of every clip
        return mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    }

    *format = mlt_audio_f32le;
    int ret = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (ret != 0) {
        return ret;
    }

    struct mlt_audio_s curr_audio;
    mlt_audio_set_values(&curr_audio, *buffer, *frequency, *format, *samples, *channels);

    if (clip_position == 0) {
        if (!pdata->prev_audio.data) {
            mlt_log_debug(MLT_FILTER_SERVICE(filter), "Missing previous audio\n");
        } else {
            float *prev_data = pdata->prev_audio.data;
            float *curr_data = curr_audio.data;
            float level_delta = db_delta(prev_data[pdata->prev_audio.samples - 1], curr_data[0]);
            double discontinuity_threshold = mlt_properties_get_double(filter_properties,
                                                                       "discontinuity_threshold");
            if (fabs(level_delta) > discontinuity_threshold) {
                // We have decided to create a transition with the previous frame.
                // Reverse the prevous frame and use the reversed samples as faux
                // data that is continuous from the prevous frame.
                // Mix/fade the reversed previous samples with the new samples to create a transition.
                mlt_audio_reverse(&pdata->prev_audio);
                int fade_samples = 1000;
                if (fade_samples > curr_audio.samples) {
                    fade_samples = curr_audio.samples;
                }
                if (fade_samples > pdata->prev_audio.samples) {
                    fade_samples = pdata->prev_audio.samples;
                }
                for (int c = 0; c < curr_audio.channels; c++) {
                    curr_data = (float *) curr_audio.data + c;
                    prev_data = (float *) pdata->prev_audio.data + c;
                    for (int i = 0; i < fade_samples; i++) {
                        float mix = (1.0 / fade_samples) * (float) (fade_samples - i);
                        *curr_data = (*prev_data * mix) + (*curr_data * (1.0 - mix));
                        curr_data += curr_audio.channels;
                        prev_data += curr_audio.channels;
                    }
                }
                // If this flag is set, it must be cleared so that other services will know it can't be ignored.
                mlt_properties_clear(MLT_FRAME_PROPERTIES(frame), "test_audio");
                // Increment the counter
                mlt_properties_set_int(filter_properties,
                                       "seam_count",
                                       mlt_properties_get_int(filter_properties, "seam_count") + 1);
            }
        }
        mlt_audio_free_data(&pdata->prev_audio);
    } else if (clip_position == (clip_length - 1)) {
        // Save the samples of the last frame to be used to mix with the first frame of the next clip.
        mlt_audio_set_values(&pdata->prev_audio, NULL, *frequency, *format, *samples, *channels);
        mlt_audio_alloc_data(&pdata->prev_audio);
        mlt_audio_copy(&pdata->prev_audio, &curr_audio, *samples, 0, 0);
    }

    return 0;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    int clip_position = mlt_properties_get_int(frame_properties, "meta.playlist.clip_position");
    int clip_length = mlt_properties_get_int(frame_properties, "meta.playlist.clip_length");

    // Only operate on the first and last frame of every clip
    if (clip_length > 0 && (clip_position == 0 || clip_position == (clip_length - 1))) {
        // Be sure to process blanks in a playlist
        mlt_properties_clear(frame_properties, "test_audio");
        mlt_frame_push_audio(frame, filter);
        mlt_frame_push_audio(frame, filter_get_audio);
    }
    return frame;
}

static void filter_close(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;

    if (pdata) {
        mlt_audio_free_data(&pdata->prev_audio);
    }
    free(pdata);
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

mlt_filter filter_audioseam_init(mlt_profile profile,
                                 mlt_service_type type,
                                 const char *id,
                                 char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (filter && pdata) {
        filter->close = filter_close;
        filter->process = filter_process;
        filter->child = pdata;
    } else {
        mlt_filter_close(filter);
        filter = NULL;
        free(pdata);
    }

    return filter;
}
