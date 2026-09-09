/*
 * producer_tone.c -- audio tone generating producer
 * Copyright (C) 2014-2026 Meltytech, LLC
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t float_to_s32(float value)
{
    value = CLAMP(value, -1.0f, 1.0f);
    int64_t pcm = (value > 0.0f ? INT32_MAX : -(int64_t) INT32_MIN) * value;
    return CLAMP(pcm, INT32_MIN, INT32_MAX);
}

static int producer_get_audio(mlt_frame frame,
                              void **buffer,
                              mlt_audio_format *format,
                              int *frequency,
                              int *channels,
                              int *samples)
{
    mlt_producer producer = mlt_frame_pop_audio(frame);
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    double fps = mlt_producer_get_fps(producer);
    mlt_position position = mlt_frame_get_position(frame);
    mlt_position length = mlt_producer_get_length(producer);

    // Correct the returns if necessary
    *frequency = *frequency <= 0 ? 48000 : *frequency;
    *channels = *channels <= 0 ? 2 : *channels;
    *samples = *samples <= 0 ? mlt_audio_calculate_frame_samples(fps, *frequency, position)
                             : *samples;
    if (*format != mlt_audio_s16 && *format != mlt_audio_s32 && *format != mlt_audio_float
        && *format != mlt_audio_s32le && *format != mlt_audio_f32le && *format != mlt_audio_u8)
        *format = mlt_audio_float;

    // Allocate the buffer
    int size = mlt_audio_format_size(*format, *samples, *channels);
    *buffer = mlt_pool_alloc(size);

    // Fill the buffer
    int s = 0;
    int c = 0;
    long double first_sample = mlt_audio_calculate_samples_to_position(fps, *frequency, position);
    float a = mlt_properties_anim_get_double(producer_properties, "level", position, length);
    long double f
        = mlt_properties_anim_get_double(producer_properties, "frequency", position, length);
    long double p = mlt_properties_anim_get_double(producer_properties, "phase", position, length);
    p = (M_PI / 180) * p;  // Convert from degrees to radians
    a = pow(10, a / 20.0); // Convert from dB to amplitude

    for (s = 0; s < *samples; s++) {
        long double t = (first_sample + s) / *frequency;
        float value = a * sin(2 * M_PI * f * t + p);

        switch (*format) {
        case mlt_audio_s16: {
            int16_t *sample_ptr = (int16_t *) *buffer + s * *channels;
            for (c = 0; c < *channels; c++)
                *sample_ptr++ = 32767 * CLAMP(value, -1.0f, 1.0f);
            break;
        }
        case mlt_audio_s32: {
            int32_t *sample_ptr = (int32_t *) *buffer + s;
            for (c = 0; c < *channels; c++) {
                *sample_ptr = float_to_s32(value);
                sample_ptr += *samples;
            }
            break;
        }
        case mlt_audio_float: {
            float *sample_ptr = (float *) *buffer + s;
            for (c = 0; c < *channels; c++) {
                *sample_ptr = value;
                sample_ptr += *samples;
            }
            break;
        }
        case mlt_audio_s32le: {
            int32_t *sample_ptr = (int32_t *) *buffer + s * *channels;
            int32_t pcm = float_to_s32(value);
            for (c = 0; c < *channels; c++)
                *sample_ptr++ = pcm;
            break;
        }
        case mlt_audio_f32le: {
            float *sample_ptr = (float *) *buffer + s * *channels;
            for (c = 0; c < *channels; c++)
                *sample_ptr++ = value;
            break;
        }
        case mlt_audio_u8: {
            uint8_t *sample_ptr = (uint8_t *) *buffer + s * *channels;
            uint8_t pcm = (127 * CLAMP(value, -1.0f, 1.0f)) + 128;
            for (c = 0; c < *channels; c++)
                *sample_ptr++ = pcm;
            break;
        }
        default:
            break;
        }
    }

    // Set the buffer for destruction
    mlt_frame_set_audio(frame, *buffer, *format, size, mlt_pool_release);

    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties_set(frame_properties,
                       "channel_layout",
                       mlt_properties_get(frame_properties, "consumer.channel_layout"));

    return 0;
}

static int producer_get_frame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    // Generate a frame
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));

    if (*frame != NULL) {
        // Update time code on the frame
        mlt_frame_set_position(*frame, mlt_producer_position(producer));

        // Configure callbacks
        mlt_frame_push_audio(*frame, producer);
        mlt_frame_push_audio(*frame, producer_get_audio);
    }

    // Calculate the next time code
    mlt_producer_prepare_next(producer);

    return 0;
}

static void producer_close(mlt_producer this)
{
    this->close = NULL;
    mlt_producer_close(this);
    free(this);
}

mlt_producer producer_tone_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    // Create a new producer object
    mlt_producer producer = mlt_producer_new(profile);
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);

    // Initialize the producer
    if (producer) {
        mlt_properties_set_double(producer_properties, "frequency", 1000.0);
        mlt_properties_set_double(producer_properties, "phase", 0.0);
        mlt_properties_set_double(producer_properties, "level", 0.0);

        // Callback registration
        producer->get_frame = producer_get_frame;
        producer->close = (mlt_destructor) producer_close;
    }

    return producer;
}
