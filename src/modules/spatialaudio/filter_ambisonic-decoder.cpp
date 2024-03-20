/*
 * filter_ambisonic-decoder.cpp -- decode ambisonic audio to speaker channels
 * Copyright (C) 2024 Meltytech, LLC
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
#include <spatialaudio/Ambisonics.h>

static const auto MAX_CHANNELS = 6;
static const auto AMBISONICS_BLOCK_SIZE = 1024;
static const auto AMBISONICS_ORDER = 1;
static const auto AMBISONICS_1_CHANNELS = 4;

extern "C" {
static mlt_frame process(mlt_filter filter, mlt_frame frame);
static void close_filter(mlt_filter filter);
}

class SpatialAudio
{
private:
    mlt_filter m_filter;
    CAmbisonicBinauralizer binauralizer;
    unsigned tailLength;
    CAmbisonicDecoder decoder;
    CAmbisonicProcessor processor;
    CAmbisonicZoomer zoomer;
    float *speakers[MAX_CHANNELS];

public:
    SpatialAudio()
    {
        m_filter = mlt_filter_new();
        if (m_filter) {
            m_filter->child = this;
            m_filter->close = close_filter;
            m_filter->process = process;
        }
    }

    ~SpatialAudio()
    {
        m_filter->child = nullptr;
        m_filter->close = nullptr;
        m_filter->parent.close = nullptr;
        mlt_service_close(MLT_FILTER_SERVICE(m_filter));
        m_filter = nullptr;
    }

    mlt_filter filter() { return m_filter; }
    mlt_properties properties() { return MLT_FILTER_PROPERTIES(filter()); }
    double getDouble(const char *name, int position, int length)
    {
        return mlt_properties_anim_get_double(properties(), name, position, length);
    }

    bool getAudio(mlt_frame frame, float *buffer, int samples, int channels)
    {
        bool error = false;
        bool binaural = channels >= 2 && mlt_properties_get_int(properties(), "binaural");

        // First time setup
        if (binaural && !binauralizer.GetChannelCount()) {
            mlt_log_verbose(MLT_FILTER_SERVICE(filter()),
                            "configuring spatial audio binauralizer\n");
            error = !binauralizer.Configure(AMBISONICS_ORDER,
                                            true,
                                            mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                                                   "audio_frequency"),
                                            samples,
                                            tailLength);
            if (!error) {
                binauralizer.Reset();
            } else {
                mlt_log_error(MLT_FILTER_SERVICE(filter()),
                              "failed to configure spatial audio binauralizer\n");
            }
        } else if (!binaural && !decoder.GetChannelCount()) {
            mlt_log_verbose(MLT_FILTER_SERVICE(filter()),
                            "configuring spatial audio decoder for %d channels\n",
                            channels);
            error = !decoder.Configure(AMBISONICS_ORDER,
                                       true,
                                       AMBISONICS_BLOCK_SIZE,
                                       channels == 6   ? kAmblib_51
                                       : channels == 2 ? kAmblib_Stereo
                                       : channels == 4 ? kAmblib_Quad
                                                       : kAmblib_CustomSpeakerSetUp,
                                       channels);
            if (!error) {
                mlt_log_verbose(MLT_FILTER_SERVICE(filter()),
                                "configuring spatial audio processor\n");
                error = !processor.Configure(AMBISONICS_ORDER, true, AMBISONICS_BLOCK_SIZE, 0);
                if (!error) {
                    mlt_log_verbose(MLT_FILTER_SERVICE(filter()),
                                    "configuring spatial audio zoomer\n");
                    error = !zoomer.Configure(AMBISONICS_ORDER, true, AMBISONICS_BLOCK_SIZE, 0);
                    if (error) {
                        mlt_log_error(MLT_FILTER_SERVICE(filter()),
                                      "failed to configure spatial audio zoomer\n");
                    }
                } else {
                    mlt_log_error(MLT_FILTER_SERVICE(filter()),
                                  "failed to configure spatial audio processor\n");
                }
            } else {
                mlt_log_error(MLT_FILTER_SERVICE(filter()),
                              "failed to configure spatial audio decoder\n");
            }
        }

        // Processing
        if (!error) {
            CBFormat bformat;
            bformat.Configure(AMBISONICS_ORDER, true, samples);
            for (unsigned i = 0; i < AMBISONICS_1_CHANNELS; ++i)
                bformat.InsertStream(&buffer[samples * i], i, samples);

            if (!binaural) {
                mlt_position position = mlt_filter_get_position(filter(), frame);
                mlt_position length = mlt_filter_get_length2(filter(), frame);
                processor.SetOrientation({-DegreesToRadians(getDouble("yaw", position, length)),
                                          DegreesToRadians(getDouble("pitch", position, length)),
                                          DegreesToRadians(getDouble("roll", position, length))});
                processor.Refresh();
                processor.Process(&bformat, samples);
                zoomer.SetZoom(getDouble("zoom", position, length));
                zoomer.Refresh();
                zoomer.Process(&bformat, samples);
            }

            if (channels == 6) {
                // libspatialaudio has a different channel order for 5.1
                speakers[0] = &buffer[samples * 0]; // left
                speakers[1] = &buffer[samples * 1]; // right
                speakers[2] = &buffer[samples * 4]; // center
                speakers[3] = &buffer[samples * 5]; // LFE (subwoofer)
                speakers[4] = &buffer[samples * 2]; // left surround
                speakers[5] = &buffer[samples * 3]; // right surround
                decoder.Process(&bformat, samples, speakers);
            } else if (channels == 4 && mlt_properties_get_int(properties(), "ambisonic")) {
                for (int i = 0; i < channels; ++i)
                    bformat.ExtractStream(&buffer[samples * i], i, samples);
            } else {
                for (int i = 0; i < channels; ++i)
                    speakers[i] = &buffer[samples * i];
                if (binaural) {
                    binauralizer.Process(&bformat, speakers, samples);
                    if (channels > 2) {
                        for (int i = 2; i < channels; ++i)
                            ::memset(speakers[i], 0, samples * sizeof(float));
                    }
                } else {
                    decoder.Process(&bformat, samples, speakers);
                }
            }
        }
        return error;
    }
};

extern "C" {

static int get_audio(mlt_frame frame,
                     void **buffer,
                     mlt_audio_format *format,
                     int *frequency,
                     int *channels,
                     int *samples)
{
    auto error = 0;
    // Get the filter service
    mlt_filter filter = (mlt_filter) mlt_frame_pop_audio(frame);

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    // Get the producer's audio
    if (*channels > 1) {
        *format = mlt_audio_float;
        int requestChannels = MAX(AMBISONICS_1_CHANNELS, *channels);
        auto properties = MLT_FRAME_PROPERTIES(frame);
        auto channel_layout = mlt_audio_channel_layout_id(
            mlt_properties_get(properties, "consumer.channel_layout"));
        mlt_properties_set(MLT_FRAME_PROPERTIES(frame),
                           "consumer.channel_layout",
                           mlt_audio_channel_layout_name(mlt_channel_independent));

        error = mlt_frame_get_audio(frame, buffer, format, frequency, &requestChannels, samples);

        // Restore the saved channel layout
        mlt_properties_set(properties,
                           "consumer.channel_layout",
                           mlt_audio_channel_layout_name(channel_layout));

        if (!error && requestChannels >= AMBISONICS_1_CHANNELS) {
            auto spatial = static_cast<SpatialAudio *>(filter->child);
            error = spatial->getAudio(frame, static_cast<float *>(*buffer), *samples, *channels);
        }
    } else {
        // pass through mono
        error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    }

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    return error;
}

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
    if (mlt_frame_is_test_audio(frame) == 0) {
        // Add the filter to the frame
        mlt_frame_push_audio(frame, filter);
        mlt_frame_push_audio(frame, (void *) get_audio);
    }
    return frame;
}

static void close_filter(mlt_filter filter)
{
    delete reinterpret_cast<SpatialAudio *>(filter->child);
}

mlt_filter filter_ambisonic_decoder_init(mlt_profile profile,
                                         mlt_service_type type,
                                         const char *id,
                                         char *arg)
{
    mlt_filter filter = nullptr;
    auto *spatial = new SpatialAudio;

    if (spatial) {
        filter = spatial->filter();
        if (!filter)
            delete spatial;
    }
    return filter;
}

} // extern
