/*
 * filter_ambisonic-encoder.cpp -- position mono and stero sound in ambisonic space
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

// static const auto MAX_CHANNELS = 6;
static const auto AMBISONICS_ORDER = 1;
static const auto AMBISONICS_1_CHANNELS = 4;

extern "C" {
static mlt_frame process(mlt_filter filter, mlt_frame frame);
static void close_filter(mlt_filter filter);
}

class SpatialAudioEncoder
{
private:
    mlt_filter m_filter;
    CAmbisonicEncoder encoder;

public:
    SpatialAudioEncoder()
    {
        m_filter = mlt_filter_new();
        if (m_filter) {
            m_filter->child = this;
            m_filter->close = close_filter;
            m_filter->process = process;
        }
    }

    ~SpatialAudioEncoder()
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

        // First time setup
        if (!encoder.GetOrder()) {
            mlt_log_verbose(MLT_FILTER_SERVICE(filter()), "configuring spatial audio encoder\n");
            error = !encoder.Configure(AMBISONICS_ORDER, true, 0);
            if (error) {
                mlt_log_error(MLT_FILTER_SERVICE(filter()),
                              "failed to configure spatial audio encoder\n");
            }
        }

        // Processing
        if (!error) {
            CBFormat bformat;
            PolarPoint polar;
            mlt_position position = mlt_filter_get_position(filter(), frame);
            mlt_position length = mlt_filter_get_length2(filter(), frame);

            bformat.Configure(AMBISONICS_ORDER, true, samples);
            polar.fAzimuth = -DegreesToRadians(getDouble("azimuth", position, length));
            polar.fElevation = DegreesToRadians(getDouble("elevation", position, length));
            polar.fDistance = getDouble("distance", position, length);
            encoder.SetPosition(polar, 1.f);
            encoder.Refresh();
            encoder.Process(buffer, samples, &bformat);
            for (int i = 0; i < AMBISONICS_1_CHANNELS; ++i)
                bformat.ExtractStream(&buffer[samples * i], i, samples);
            for (int i = AMBISONICS_1_CHANNELS; i < channels; ++i)
                ::memset(&buffer[samples * i], 0, samples * sizeof(float));
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
    if (*channels >= AMBISONICS_1_CHANNELS) {
        *format = mlt_audio_float;
        auto properties = MLT_FRAME_PROPERTIES(frame);
        auto channel_layout = mlt_audio_channel_layout_id(
            mlt_properties_get(properties, "consumer.channel_layout"));
        mlt_properties_set(MLT_FRAME_PROPERTIES(frame),
                           "consumer.channel_layout",
                           mlt_audio_channel_layout_name(mlt_channel_independent));

        error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);

        // Restore the saved channel layout
        mlt_properties_set(properties,
                           "consumer.channel_layout",
                           mlt_audio_channel_layout_name(channel_layout));

        if (!error && *channels >= AMBISONICS_1_CHANNELS) {
            auto spatial = static_cast<SpatialAudioEncoder *>(filter->child);
            error = spatial->getAudio(frame, static_cast<float *>(*buffer), *samples, *channels);
        }
    } else {
        // pass through mono and stereo
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
    delete reinterpret_cast<SpatialAudioEncoder *>(filter->child);
}

mlt_filter filter_ambisonic_encoder_init(mlt_profile profile,
                                         mlt_service_type type,
                                         const char *id,
                                         char *arg)
{
    mlt_filter filter = nullptr;
    auto *spatial = new SpatialAudioEncoder;

    if (spatial) {
        filter = spatial->filter();
        if (!filter)
            delete spatial;
    }
    return filter;
}

} // extern
