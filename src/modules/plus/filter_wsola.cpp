/*
 * filter_wsola.cpp -- pitch change with WSOLA
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

#include "wsola/sinc_resampler.h"
#include "wsola/wsola_algorithm.h"

#include <framework/mlt.h>
#include <framework/mlt_log.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

// The pitch is changed in two steps, the same way as the rbpitch filter but with WSOLA:
//
// 1. A sinc resampler plays the input pitchscale times faster: the pitch changes and the
//    duration changes by 1 / pitchscale.
// 2. WSOLA time stretches the result back to the requested number of samples, preserving the
//    new pitch.
//
// Resampling first means that WSOLA always works on audio whose real sample rate is the
// frequency it is labeled with, so its windows have the intended length.

static const int MAX_CHANNELS = 32;
static const double MIN_PITCHSCALE = 0.0625;
static const double MAX_PITCHSCALE = 16.0;
// Frames that the resamplers request from the input at a time.
static const int REQUEST_FRAMES = wsola::SincResampler::kSmallRequestSize;

typedef struct private_data_s
{
    std::unique_ptr<wsola::AudioRendererAlgorithm> algo;
    std::vector<std::unique_ptr<wsola::SincResampler>> resamplers;
    // Input not yet read by the resamplers, one vector per channel.
    std::vector<std::vector<float>> input;
    // The block that the resampler of the first channel read last, for the other channels.
    std::vector<std::vector<float>> request;
    int channels = 0;
    int frequency = 0;
    double pitchscale = 1.0;
    mlt_position expected_position = -1;
} private_data;

// Reads REQUEST_FRAMES frames of input for one resampler. All the resamplers use the same ratio
// and are called in the same order, so they read at the same points: the first one takes the
// block out of the input and the others copy their channel from it.
static void read_input(private_data *pdata, int channel, float *destination, int frames)
{
    if (channel == 0) {
        int available = std::min(frames, (int) pdata->input[0].size());
        for (int c = 0; c < pdata->channels; c++) {
            std::vector<float> &input = pdata->input[c];
            std::vector<float> &request = pdata->request[c];
            request.assign(frames, 0.0f);
            std::copy_n(input.begin(), available, request.begin());
            input.erase(input.begin(), input.begin() + available);
        }
    }
    std::copy_n(pdata->request[channel].begin(), frames, destination);
}

// Starts over: drops any buffered audio and primes the pipeline. WSOLA gets one block of
// silence, as resampled, to absorb the input that the resamplers hold back (up to one block),
// so that it does not run out of input later.
static void reset_pipeline(private_data *pdata)
{
    for (auto &input : pdata->input)
        input.clear();
    for (auto &resampler : pdata->resamplers) {
        resampler->Flush();
        resampler->PrimeWithSilence();
    }
    pdata->algo->FlushBuffers();
    int silence = (int) std::ceil(REQUEST_FRAMES / pdata->pitchscale);
    std::vector<float> zeros((size_t) silence * pdata->channels, 0.0f);
    pdata->algo->EnqueueInterleaved(zeros.data(), silence);
}

static void create_pipeline(private_data *pdata, int channels, int frequency, double pitchscale)
{
    pdata->channels = channels;
    pdata->frequency = frequency;
    pdata->pitchscale = pitchscale;
    pdata->algo = std::make_unique<wsola::AudioRendererAlgorithm>();
    pdata->algo->Initialize(channels, frequency);
    pdata->resamplers.clear();
    for (int c = 0; c < channels; c++) {
        pdata->resamplers.push_back(
            std::make_unique<wsola::SincResampler>(pitchscale,
                                                   REQUEST_FRAMES,
                                                   [pdata, c](float *destination, int frames) {
                                                       read_input(pdata, c, destination, frames);
                                                   }));
    }
    pdata->input.assign(channels, {});
    pdata->request.assign(channels, {});
    reset_pipeline(pdata);
    pdata->expected_position = -1;
}

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

    // Get the producer's audio
    int requested_frequency = *frequency;
    int requested_samples = *samples;
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
    if (*format != mlt_audio_float)
        return error;

    double pitchscale = mlt_properties_get_double(unique_properties, "pitchscale");
    pitchscale = std::clamp(pitchscale, MIN_PITCHSCALE, MAX_PITCHSCALE);

    // Without stretch, the output has as many samples as the input, at the same frequency.
    // With stretch, the input is stretched to the requested samples and frequency.
    int stretch = mlt_properties_get_int(unique_properties, "stretch");
    int out_samples = *samples;
    int out_frequency = *frequency;
    if (stretch && requested_samples > 0 && requested_frequency > 0) {
        out_samples = requested_samples;
        out_frequency = requested_frequency;
    }
    // The resampled audio has samples / pitchscale frames; WSOLA turns them into out_samples.
    double rate = *samples / pitchscale / out_samples;

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    if (!pdata->algo || pdata->channels != *channels || pdata->frequency != *frequency) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Create a new pipeline\t%d\t%d\t%f\n",
                      *channels,
                      *frequency,
                      pitchscale);
        create_pipeline(pdata, *channels, *frequency, pitchscale);
    } else if (pdata->pitchscale != pitchscale) {
        pdata->pitchscale = pitchscale;
        for (auto &resampler : pdata->resamplers)
            resampler->SetRatio(pitchscale);
    }

    // Start over on seek: stale input would otherwise be played out.
    mlt_position position = mlt_frame_get_position(frame);
    if (position != pdata->expected_position && pdata->expected_position != -1) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter),
                      "Discontinuity %d -> %d, flushing\n",
                      (int) pdata->expected_position,
                      (int) position);
        reset_pipeline(pdata);
    }
    pdata->expected_position = position + 1;

    // 1. Resample. Each call of ChunkSize() frames reads at most one block of input, so keep
    // going while a whole block is available.
    const float *in = (const float *) *buffer;
    for (int c = 0; c < *channels; c++) {
        const float *plane = in + (size_t) *samples * c;
        pdata->input[c].insert(pdata->input[c].end(), plane, plane + *samples);
    }
    std::vector<std::vector<float>> resampled(*channels);
    int resampled_frames = 0;
    while ((int) pdata->input[0].size() >= REQUEST_FRAMES) {
        int chunk = std::max(1, pdata->resamplers[0]->ChunkSize());
        for (int c = 0; c < *channels; c++) {
            resampled[c].resize(resampled_frames + chunk);
            pdata->resamplers[c]->Resample(resampled[c].data() + resampled_frames, chunk);
        }
        resampled_frames += chunk;
    }
    if (resampled_frames > 0) {
        const float *planes[MAX_CHANNELS];
        for (int c = 0; c < *channels; c++)
            planes[c] = resampled[c].data();
        pdata->algo->EnqueueBuffer(planes, resampled_frames);
    }

    // 2. Time stretch.
    struct mlt_audio_s out;
    mlt_audio_set_values(&out, NULL, out_frequency, *format, out_samples, *channels);
    mlt_audio_alloc_data(&out);
    wsola::AudioBus::ChannelVector out_planes;
    for (int c = 0; c < out.channels; c++)
        out_planes.push_back(((float *) out.data) + (size_t) out.samples * c);
    auto bus = wsola::AudioBus::CreateWrapper(out_planes, out.samples);
    int rendered = pdata->algo->FillBuffer(bus.get(), 0, out.samples, rate);
    if (rendered < out.samples) {
        // Only expected while WSOLA fills its look-ahead after a (re)start:
        // the missing part becomes leading silence.
        int missing = out.samples - rendered;
        mlt_log_debug(MLT_FILTER_SERVICE(filter), "Underrun: %d samples\n", missing);
        for (int c = 0; c < out.channels; c++) {
            float *plane = out_planes[c];
            std::memmove(plane + missing, plane, sizeof(float) * rendered);
            std::fill_n(plane, missing, 0.0f);
        }
    }
    int in_samples = *samples;
    mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
    mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);

    // Report the latency: what WSOLA holds, plus the input that the resamplers hold, which
    // becomes 1 / (pitchscale * rate) output frames per frame.
    double held_input = pdata->input[0].size() + pdata->resamplers[0]->BufferedFrames();
    double latency_frames = pdata->algo->DelayInFrames(rate) + held_input / (pitchscale * rate);
    double latency = latency_frames * 1000.0 / (double) out_frequency;
    mlt_properties_set_double(filter_properties, "latency", latency);

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "In: %d\tOut: %d@%d\tPitch: %f\tRate: %f\tLatency: %fms\n",
                  in_samples,
                  out_samples,
                  out_frequency,
                  pitchscale,
                  rate,
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
    if (!(pitchscale > 0.0) /*also catches nan*/) {
        pitchscale = 1.0;
    }

    // Save the pitchscale on the frame to be used in wsola_get_audio
    mlt_properties unique_properties = mlt_frame_unique_properties(frame,
                                                                   MLT_FILTER_SERVICE(filter));
    mlt_properties_set_double(unique_properties, "pitchscale", pitchscale);
    mlt_properties_set_int(unique_properties,
                           "stretch",
                           mlt_properties_get_int(filter_properties, "stretch"));

    mlt_frame_push_audio(frame, (void *) filter);
    mlt_frame_push_audio(frame, (void *) wsola_get_audio);

    return frame;
}

static void close_filter(mlt_filter filter)
{
    delete (private_data *) filter->child;
    filter->child = NULL;
}

extern "C" {

mlt_filter filter_wsola_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        filter->close = close_filter;
        filter->child = new private_data;
    } else {
        mlt_log_error(NULL, "[filter wsola] Failed to initialize\n");
    }
    return filter;
}
}
