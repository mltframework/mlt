/*
 * filter_mono.c -- mix all channels to a mono signal across n channels
 * Copyright (C) 2003-2025 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Get the audio.
*/

static int filter_get_audio(mlt_frame frame,
                            void **buffer,
                            mlt_audio_format *format,
                            int *frequency,
                            int *channels,
                            int *samples)
{
    mlt_filter filter = (mlt_filter) mlt_frame_pop_audio(frame);
    mlt_properties properties = mlt_filter_properties(filter);
    int channels_out = mlt_properties_get_int(properties, "channels");
    int input_chmask = mlt_properties_get_int64(properties, "input_chmask");
    int output_chmask = mlt_properties_get_int64(properties, "output_chmask");
    int i, j, size;

    // Get the producer's audio
    mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);

    if (channels_out == -1 || channels_out > *channels)
        channels_out = *channels;
    size = mlt_audio_format_size(*format, *samples, channels_out);

    switch (*format) {
    case mlt_audio_u8: {
        uint8_t *new_buffer = mlt_pool_alloc(size);
        memcpy(new_buffer, *buffer, size);
        for (i = 0; i < *samples; i++) {
            uint8_t mixdown = 0;
            for (j = 0; j < *channels; j++) {
                if (input_chmask & (1 << j))
                    mixdown += ((uint8_t *) *buffer)[(i * *channels) + j];
            }
            for (j = 0; j < channels_out; j++) {
                if (output_chmask & (1 << j))
                    new_buffer[(i * channels_out) + j] = mixdown;
            }
        }
        *buffer = new_buffer;
        break;
    }
    case mlt_audio_s16: {
        int16_t *new_buffer = mlt_pool_alloc(size);
        memcpy(new_buffer, *buffer, size);
        for (i = 0; i < *samples; i++) {
            int16_t mixdown = 0;
            for (j = 0; j < *channels; j++) {
                if (input_chmask & (1 << j))
                    mixdown += ((int16_t *) *buffer)[(i * *channels) + j];
            }
            for (j = 0; j < channels_out; j++) {
                if (output_chmask & (1 << j))
                    new_buffer[(i * channels_out) + j] = mixdown;
            }
        }
        *buffer = new_buffer;
        break;
    }
    case mlt_audio_s32le: {
        int32_t *new_buffer = mlt_pool_alloc(size);
        memcpy(new_buffer, *buffer, size);
        for (i = 0; i < *samples; i++) {
            int32_t mixdown = 0;
            for (j = 0; j < *channels; j++) {
                if (input_chmask & (1 << j))
                    mixdown += ((int32_t *) *buffer)[(i * *channels) + j];
            }
            for (j = 0; j < channels_out; j++) {
                if (output_chmask & (1 << j))
                    new_buffer[(i * channels_out) + j] = mixdown;
            }
        }
        *buffer = new_buffer;
        break;
    }
    case mlt_audio_f32le: {
        float *new_buffer = mlt_pool_alloc(size);
        memcpy(new_buffer, *buffer, size);
        for (i = 0; i < *samples; i++) {
            float mixdown = 0;
            for (j = 0; j < *channels; j++) {
                if (input_chmask & (1 << j))
                    mixdown += ((float *) *buffer)[(i * *channels) + j];
            }
            for (j = 0; j < channels_out; j++) {
                if (output_chmask & (1 << j))
                    new_buffer[(i * channels_out) + j] = mixdown;
            }
        }
        *buffer = new_buffer;
        break;
    }
    case mlt_audio_s32: {
        int32_t *new_buffer = mlt_pool_alloc(size);
        memcpy(new_buffer, *buffer, size);
        for (i = 0; i < *samples; i++) {
            int32_t mixdown = 0;
            for (j = 0; j < *channels; j++) {
                if (input_chmask & (1 << j))
                    mixdown += ((int32_t *) *buffer)[(j * *samples) + i];
            }
            for (j = 0; j < channels_out; j++) {
                if (output_chmask & (1 << j))
                    new_buffer[(j * *samples) + i] = mixdown;
            }
        }
        *buffer = new_buffer;
        break;
    }
    case mlt_audio_float: {
        float *new_buffer = mlt_pool_alloc(size);
        memcpy(new_buffer, *buffer, size);
        for (i = 0; i < *samples; i++) {
            float mixdown = 0;
            for (j = 0; j < *channels; j++) {
                if (input_chmask & (1 << j))
                    mixdown += ((float *) *buffer)[(j * *samples) + i];
            }
            for (j = 0; j < channels_out; j++) {
                if (output_chmask & (1 << j))
                    new_buffer[(j * *samples) + i] = mixdown;
            }
        }
        *buffer = new_buffer;
        break;
    }
    default:
        mlt_log_error(NULL, "[filter mono] Invalid audio format\n");
        break;
    }
    if (size > *samples * channels_out) {
        mlt_frame_set_audio(frame, *buffer, *format, size, mlt_pool_release);
        *channels = channels_out;
    }

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Override the get_audio method
    mlt_frame_push_audio(frame, filter);
    mlt_frame_push_audio(frame, filter_get_audio);

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mono_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        if (arg != NULL)
            mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "channels", atoi(arg));
        else
            mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "channels", -1);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "input_chmask", 3);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "output_chmask", -1);
    }
    return filter;
}
