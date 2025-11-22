/*
 * filter_rbpitch.c -- adjust audio pitch
 * Copyright (C) 2020-2024 Meltytech, LLC
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

#include <framework/mlt.h>
#include <framework/mlt_log.h>

#include <rubberband/RubberBandStretcher.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace RubberBand;

// Private Types
typedef struct
{
    RubberBandStretcher *s;
    int rubberband_frequency;
    uint64_t in_samples;
    uint64_t out_samples;
} private_data;

static int rbpitch_get_audio(mlt_frame frame,
                             void **buffer,
                             mlt_audio_format *format,
                             int *frequency,
                             int *channels,
                             int *samples)
{
    mlt_filter filter = static_cast<mlt_filter>(mlt_frame_pop_audio(frame));
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    private_data *pdata = (private_data *) filter->child;
    if (*channels > (int) MAX_CHANNELS) {
        mlt_log_error(MLT_FILTER_SERVICE(filter),
                      "Too many channels requested: %d > %d\n",
                      *channels,
                      (int) MAX_CHANNELS);
        return mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    }

    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_FILTER_SERVICE(filter));
    if (!unique_properties) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Missing unique_properites\n");
        return mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    }

    // Get the producer's audio
    int requested_frequency = *frequency;
    int requested_samples = *samples;
    *format = mlt_audio_float;
    int error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (error)
        return error;

    // Make sure the audio is in the correct format
    // This is useful if the filter is encapsulated in a producer and does not
    // have a normalizing filter before it.
    if (*format != mlt_audio_float && frame->convert_audio != NULL) {
        frame->convert_audio(frame, buffer, format, mlt_audio_float);
    }

    // Sanity check parameters
    // rubberband library crashes have been seen with a very large scale factor
    // or very small sampling frequency. Very small scale factor and very high
    // sampling frequency can result in too much audio lag.
    // Disallow these extreme scenarios for now. Maybe it will be improved in
    // the future.
    double pitchscale = mlt_properties_get_double(unique_properties, "pitchscale");
    pitchscale = CLAMP(pitchscale, 0.05, 50.0);
    double timeratio = 1.0;
    int stretch = mlt_properties_get_int(unique_properties, "stretch");
    int rubberband_frequency = *frequency;
    if (stretch) {
        rubberband_frequency = requested_frequency;
        timeratio = (double) requested_samples / (double) *samples;
    }
    rubberband_frequency = CLAMP(rubberband_frequency, 10000, 300000);

    // Protect the RubberBandStretcher instance.
    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    // Configure the stretcher.
    RubberBandStretcher *s = pdata->s;
    if (!s || s->available() == -1 || (int) s->getChannelCount() != *channels
        || pdata->rubberband_frequency != rubberband_frequency) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Create a new stretcher\t%d\t%d\t%f\n",
                      *channels,
                      rubberband_frequency,
                      pitchscale);
        delete s;
        // Create a rubberband instance
        RubberBandStretcher::Options options = RubberBandStretcher::OptionProcessRealTime;
#if RUBBERBAND_API_MAJOR_VERSION >= 2 && RUBBERBAND_API_MINOR_VERSION >= 7
        // Use the higher quality engine if available.
        options |= RubberBandStretcher::OptionEngineFiner;
#endif
        s = new RubberBandStretcher(rubberband_frequency, *channels, options, 1.0, pitchscale);
        pdata->s = s;
        pdata->rubberband_frequency = rubberband_frequency;
        pdata->in_samples = 0;
        pdata->out_samples = 0;
    }

    s->setPitchScale(pitchscale);
    if (pitchscale >= 0.5 && pitchscale <= 2.0) {
        // Pitch adjustment < 200%
#if RUBBERBAND_API_MAJOR_VERSION <= 2 && RUBBERBAND_API_MINOR_VERSION < 7
        s->setPitchOption(RubberBandStretcher::OptionPitchHighQuality);
#endif
        s->setTransientsOption(RubberBandStretcher::OptionTransientsCrisp);
    } else {
        // Pitch adjustment > 200%
        // "HighConsistency" and "Smooth" options help to avoid large memory
        // consumption and crashes that can occur for large pitch adjustments.
#if RUBBERBAND_API_MAJOR_VERSION <= 2 && RUBBERBAND_API_MINOR_VERSION < 7
        s->setPitchOption(RubberBandStretcher::OptionPitchHighConsistency);
#endif
        s->setTransientsOption(RubberBandStretcher::OptionTransientsSmooth);
    }
    s->setTimeRatio(timeratio);

    // Configure input and output buffers and counters.
    int consumed_samples = 0;
    int total_consumed_samples = 0;
    int received_samples = 0;
    struct mlt_audio_s in;
    struct mlt_audio_s out;
    mlt_audio_set_values(&in, *buffer, *frequency, *format, *samples, *channels);
    if (stretch) {
        *frequency = requested_frequency;
        *samples = requested_samples;
    }
    mlt_audio_set_values(&out, NULL, *frequency, *format, *samples, *channels);
    mlt_audio_alloc_data(&out);

    // Process all input samples
    while (true) {
        // Send more samples to the stretcher
        if (consumed_samples == in.samples) {
            // Continue to repeat input samples into the stretcher until it
            // provides the desired number of samples out.
            consumed_samples = 0;
            mlt_log_debug(MLT_FILTER_SERVICE(filter), "Repeat samples\n");
        }
        int process_samples = std::min(in.samples - consumed_samples, (int) s->getSamplesRequired());
        if (process_samples == 0 && received_samples == out.samples
            && total_consumed_samples < in.samples) {
            // No more out samples are needed, but input samples are still available.
            // Send the final input samples for processing.
            process_samples = in.samples - total_consumed_samples;
        }
        if (process_samples > 0) {
            float *in_planes[MAX_CHANNELS];
            for (int i = 0; i < in.channels; i++) {
                in_planes[i] = ((float *) in.data) + (in.samples * i) + consumed_samples;
            }
            s->process(in_planes, process_samples, false);
            consumed_samples += process_samples;
            total_consumed_samples += process_samples;
            pdata->in_samples += process_samples;
        }

        // Receive samples from the stretcher
        int retrieve_samples = std::min(out.samples - received_samples, s->available());
        if (retrieve_samples > 0) {
            float *out_planes[MAX_CHANNELS];
            for (int i = 0; i < out.channels; i++) {
                out_planes[i] = ((float *) out.data) + (out.samples * i) + received_samples;
            }
            retrieve_samples = (int) s->retrieve(out_planes, retrieve_samples);
            received_samples += retrieve_samples;
            pdata->out_samples += retrieve_samples;
        }

        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Process: %d\t Retrieve: %d\n",
                      process_samples,
                      retrieve_samples);

        if (received_samples == out.samples && total_consumed_samples >= in.samples) {
            // There is nothing more to do;
            break;
        }
    }

    // Save the processed samples.
    mlt_audio_shrink(&out, received_samples);
    mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
    mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);

    // Report the latency.
    double latency = (double) (pdata->in_samples - pdata->out_samples) * 1000.0
                     / (double) *frequency;
    mlt_properties_set_double(filter_properties, "latency", latency);

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "Requested: %d\tReceived: %d\tSent: %d\tLatency: %d(%fms)\n",
                  requested_samples,
                  in.samples,
                  out.samples,
                  (int) (pdata->in_samples - pdata->out_samples),
                  latency);
    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    // Determine the pitchscale
    double pitchscale = 1.0;
    if (mlt_properties_exists(filter_properties, "pitchscale")) {
        pitchscale
            = mlt_properties_anim_get_double(filter_properties, "pitchscale", position, length);
    } else {
        double octaveshift
            = mlt_properties_anim_get_double(filter_properties, "octaveshift", position, length);
        pitchscale = pow(2, octaveshift);
    }
    if (pitchscale <= 0.0 || /*check for nan:*/ pitchscale != pitchscale) {
        pitchscale = 1.0;
    }

    // Save the pitchscale on the frame to be used in rbpitch_get_audio
    mlt_properties unique_properties = mlt_frame_unique_properties(frame,
                                                                   MLT_FILTER_SERVICE(filter));
    mlt_properties_set_double(unique_properties, "pitchscale", pitchscale);
    mlt_properties_set_int(unique_properties,
                           "stretch",
                           mlt_properties_get_int(filter_properties, "stretch"));

    mlt_frame_push_audio(frame, (void *) filter);
    mlt_frame_push_audio(frame, (void *) rbpitch_get_audio);

    return frame;
}

static void close_filter(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;
    if (pdata) {
        RubberBandStretcher *s = static_cast<RubberBandStretcher *>(pdata->s);
        if (s) {
            delete s;
        }
        free(pdata);
        filter->child = NULL;
    }
}

extern "C" {

mlt_filter filter_rbpitch_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (filter && pdata) {
        pdata->s = NULL;
        pdata->rubberband_frequency = 0;
        pdata->in_samples = 0;
        pdata->out_samples = 0;

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
