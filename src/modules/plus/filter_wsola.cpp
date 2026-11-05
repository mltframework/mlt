/*
 * filter_wsola.cpp -- pitch preserving speed change with WSOLA
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "wsola/wsola_algorithm.h"

#include <framework/mlt.h>
#include <framework/mlt_log.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

// Private Types
typedef struct
{
    wsola::AudioRendererAlgorithm *algo;
    int out_frequency;
    mlt_position expected_position;
    double out_remainder;
} private_data;

static const int MAX_CHANNELS = 32;

static int wsola_get_audio(mlt_frame frame,
                           void **buffer,
                           mlt_audio_format *format,
                           int *frequency,
                           int *channels,
                           int *samples)
{
    mlt_filter filter = static_cast<mlt_filter>(mlt_frame_pop_audio(frame));
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    private_data *pdata = (private_data *) filter->child;

    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_FILTER_SERVICE(filter));
    if (!unique_properties) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Missing unique_properites\n");
        return mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    }
    double speed = mlt_properties_get_double(unique_properties, "speed");

    // Get the producer's audio
    *format = mlt_audio_float;
    int error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (error || *samples <= 0 || *frequency <= 0)
        return error;
    if (*channels > MAX_CHANNELS) {
        mlt_log_error(MLT_FILTER_SERVICE(filter),
                      "Too many channels: %d > %d\n",
                      *channels,
                      MAX_CHANNELS);
        return error;
    }

    // Make sure the audio is in the correct format
    // This is useful if the filter is encapsulated in a producer and does not
    // have a normalizing filter before it.
    if (*format != mlt_audio_float && frame->convert_audio != NULL) {
        frame->convert_audio(frame, buffer, format, mlt_audio_float);
    }
    if (*format != mlt_audio_float || !(speed > 0.0))
        return error;

    // The input is the original audio labeled with frequency * speed. Restore
    // the original frequency, and derive the WSOLA rate from the two integer
    // frequencies so that the produced sample count and the amount of input
    // consumed by WSOLA stay in exact proportion (no drift of the queue).
    int out_frequency = std::max(1, (int) std::lround(*frequency / speed));
    double rate = (double) *frequency / (double) out_frequency;

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    wsola::AudioRendererAlgorithm *algo = pdata->algo;
    if (!algo || algo->channels() != *channels || pdata->out_frequency != out_frequency) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Create a new WSOLA instance\t%d\t%d\t%f\n",
                      *channels,
                      out_frequency,
                      rate);
        delete algo;
        algo = new wsola::AudioRendererAlgorithm;
        // The algorithm works on the original time base.
        algo->Initialize(*channels, out_frequency);
        pdata->algo = algo;
        pdata->out_frequency = out_frequency;
        pdata->out_remainder = 0.0;
        pdata->expected_position = -1;
    }

    // Start over on seek: stale input would otherwise be played out.
    mlt_position position = mlt_frame_get_position(frame);
    if (position != pdata->expected_position && pdata->expected_position != -1) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Discontinuity %d -> %d, flushing\n",
                      (int) pdata->expected_position,
                      (int) position);
        algo->FlushBuffers();
        pdata->out_remainder = 0.0;
    }
    pdata->expected_position = position + 1;

    // Queue the input.
    struct mlt_audio_s in;
    mlt_audio_set_values(&in, *buffer, *frequency, *format, *samples, *channels);
    const float *in_planes[MAX_CHANNELS];
    for (int i = 0; i < in.channels; i++) {
        in_planes[i] = ((float *) in.data) + (size_t) in.samples * i;
    }
    algo->EnqueueBuffer(in_planes, in.samples);

    // Same duration as the input: samples / frequency.
    double out_exact = in.samples / rate + pdata->out_remainder;
    int out_samples = (int) std::floor(out_exact);
    pdata->out_remainder = out_exact - out_samples;

    struct mlt_audio_s out;
    mlt_audio_set_values(&out, NULL, out_frequency, *format, out_samples, *channels);
    if (out_samples > 0) {
        mlt_audio_alloc_data(&out);
        wsola::AudioBus::ChannelVector out_planes;
        for (int i = 0; i < out.channels; i++) {
            out_planes.push_back(((float *) out.data) + (size_t) out.samples * i);
        }
        auto bus = wsola::AudioBus::CreateWrapper(out_planes, out.samples);
        int rendered = algo->FillBuffer(bus.get(), 0, out.samples, rate);
        if (rendered < out.samples) {
            // Only expected while WSOLA fills its look-ahead after a (re)start:
            // the missing part becomes leading silence.
            int missing = out.samples - rendered;
            mlt_log_debug(MLT_FILTER_SERVICE(filter), "Underrun: %d samples\n", missing);
            for (int i = 0; i < out.channels; i++) {
                float *plane = out_planes[i];
                std::memmove(plane + missing, plane, sizeof(float) * rendered);
                std::fill_n(plane, missing, 0.0f);
            }
        }
        mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
    } else {
        out.data = *buffer;
    }
    mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);

    // Report the latency.
    double latency = algo->DelayInFrames(rate) * 1000.0 / (double) out_frequency;
    mlt_properties_set_double(filter_properties, "latency", latency);

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "In: %d@%d\tOut: %d@%d\tRate: %f\tLatency: %fms\n",
                  in.samples,
                  in.frequency,
                  out_samples,
                  out_frequency,
                  rate,
                  latency);
    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

    // Save the speed on the frame to be used in wsola_get_audio
    double speed = mlt_properties_get_double(filter_properties, "speed");
    if (!(speed > 0.0) /*also catches nan*/) {
        speed = 1.0;
    }
    speed = std::clamp(speed, 0.0625, 16.0);
    mlt_properties unique_properties = mlt_frame_unique_properties(frame,
                                                                   MLT_FILTER_SERVICE(filter));
    mlt_properties_set_double(unique_properties, "speed", speed);

    mlt_frame_push_audio(frame, (void *) filter);
    mlt_frame_push_audio(frame, (void *) wsola_get_audio);

    return frame;
}

static void close_filter(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;
    if (pdata) {
        delete pdata->algo;
        free(pdata);
        filter->child = NULL;
    }
}

extern "C" {

mlt_filter filter_wsola_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (filter && pdata) {
        pdata->algo = NULL;
        pdata->expected_position = -1;
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(filter), "speed", arg ? atof(arg) : 1.0);

        filter->process = filter_process;
        filter->close = close_filter;
        filter->child = pdata;
    } else {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to initialize\n");

        if (filter) {
            mlt_filter_close(filter);
        }
        free(pdata);
        filter = NULL;
    }
    return filter;
}
}
