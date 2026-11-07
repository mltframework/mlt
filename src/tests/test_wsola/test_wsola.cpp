/*
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

#include <QtTest>

extern "C" {
#include <framework/mlt_audio.h>
}
#include <mlt++/Mlt.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
using namespace Mlt;

static const int FREQUENCY = 48000;
static const int CHANNELS = 2;
static const double TONE = 1000.0;
static const int FRAMES = 100;
// WSOLA starts with silence while it fills its look-ahead.
static const int SETTLE_FRAMES = 20;

class TestWsola : public QObject
{
    Q_OBJECT

public:
    TestWsola() { Factory::init(); }

    ~TestWsola() {}

private:
    // Plays the producer from the start position and returns the audio.
    std::vector<float> playAudio(Producer &producer,
                                 int *audioFrequency = nullptr,
                                 int start = 0,
                                 int frames = FRAMES)
    {
        std::vector<float> audio;
        producer.seek(start);
        Profile *profile = producer.profile();
        const double fps = profile->fps();
        for (int i = 0; i < frames; i++) {
            std::unique_ptr<Frame> frame(producer.get_frame());
            mlt_audio_format format = mlt_audio_float;
            int frequency = FREQUENCY;
            int channels = CHANNELS;
            int samples = mlt_audio_calculate_frame_samples(fps, FREQUENCY, start + i);
            const float *data = static_cast<const float *>(
                frame->get_audio(format, frequency, channels, samples));
            if (data && i >= SETTLE_FRAMES && format == mlt_audio_float) {
                audio.insert(audio.end(), data, data + samples);
                if (audioFrequency)
                    *audioFrequency = frequency;
            }
            producer.seek(start + i + 1);
        }
        return audio;
    }

    // Measures the pitch from the rising zero crossings.
    double pitch(const std::vector<float> &audio, int frequency)
    {
        int first = -1;
        int last = -1;
        int crossings = 0;
        for (size_t i = 1; i < audio.size(); i++) {
            if (audio[i - 1] < 0.0f && audio[i] >= 0.0f) {
                if (first < 0)
                    first = int(i);
                else
                    crossings++;
                last = int(i);
            }
        }
        return last > first ? crossings * double(frequency) / (last - first) : 0.0;
    }

    // The longest run of samples that are practically zero: a gap in the audio.
    int longestSilence(const std::vector<float> &audio)
    {
        int longest = 0;
        int run = 0;
        for (float sample : audio) {
            run = std::fabs(sample) < 1e-4f ? run + 1 : 0;
            longest = std::max(longest, run);
        }
        return longest;
    }

    Producer *createProducer(Profile &profile, double speed, bool pitchCompensation)
    {
        auto *producer = new Producer(profile,
                                      "timewarp",
                                      QByteArray::number(speed).append(":tone:").constData());
        producer->set("warp_pitch", pitchCompensation ? 1 : 0);
        return producer;
    }

private Q_SLOTS:
    void FilterIsRegistered()
    {
        Profile profile;
        Filter filter(profile, "wsola");
        QVERIFY(filter.is_valid());
    }

    void PreservesPitch_data()
    {
        QTest::addColumn<double>("speed");
        QTest::newRow("half") << 0.5;
        QTest::newRow("double") << 2.0;
        QTest::newRow("triple") << 3.0;
        // The fastest speed that timewarp supports.
        QTest::newRow("twenty") << 20.0;
    }

    void PreservesPitch()
    {
        QFETCH(double, speed);
        Profile profile("dv_pal");
        std::unique_ptr<Producer> producer(createProducer(profile, speed, true));
        QVERIFY(producer->is_valid());
        producer->set("warp_pitch_method", "wsola");

        int frequency = 0;
        const std::vector<float> audio = playAudio(*producer, &frequency);
        // Like rbpitch, the filter keeps the frequency that timewarp labels its audio with.
        QCOMPARE(frequency, int(FREQUENCY * speed));
        QVERIFY2(longestSilence(audio) < 4, qPrintable(QString::number(longestSilence(audio))));
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - TONE) < TONE * 0.02, qPrintable(QString::number(measured)));
    }

    void WithoutCompensationPitchFollowsSpeed()
    {
        Profile profile("dv_pal");
        std::unique_ptr<Producer> producer(createProducer(profile, 2.0, false));
        QVERIFY(producer->is_valid());
        producer->set("warp_pitch_method", "wsola");

        int frequency = 0;
        const std::vector<float> audio = playAudio(*producer, &frequency);
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - 2.0 * TONE) < TONE * 0.04, qPrintable(QString::number(measured)));
    }

    void ShiftsPitchOnItsOwn_data()
    {
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<double>("value");
        QTest::addColumn<double>("expected");
        QTest::newRow("pitchscale 2") << QByteArray("pitchscale") << 2.0 << 2.0 * TONE;
        QTest::newRow("pitchscale 0.5") << QByteArray("pitchscale") << 0.5 << 0.5 * TONE;
        QTest::newRow("pitchscale 1.5") << QByteArray("pitchscale") << 1.5 << 1.5 * TONE;
        QTest::newRow("pitchscale 0.05") << QByteArray("pitchscale") << 0.05 << 0.05 * TONE;
        QTest::newRow("octaveshift -1") << QByteArray("octaveshift") << -1.0 << 0.5 * TONE;
    }

    void ShiftsPitchOnItsOwn()
    {
        QFETCH(QByteArray, property);
        QFETCH(double, value);
        QFETCH(double, expected);
        Profile profile("dv_pal");
        Producer producer(profile, "tone:");
        QVERIFY(producer.is_valid());
        Filter filter(profile, "wsola");
        QVERIFY(filter.is_valid());
        filter.set(property.constData(), value);
        producer.attach(filter);

        int frequency = 0;
        const std::vector<float> audio = playAudio(producer, &frequency);
        QCOMPARE(frequency, FREQUENCY);
        QCOMPARE(int(audio.size()), int((FRAMES - SETTLE_FRAMES) * FREQUENCY / profile.fps() + 0.5));
        QVERIFY2(longestSilence(audio) < 4, qPrintable(QString::number(longestSilence(audio))));
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - expected) < expected * 0.02, qPrintable(QString::number(measured)));
    }

    void SeekDoesNotChangePitch()
    {
        Profile profile("dv_pal");
        std::unique_ptr<Producer> producer(createProducer(profile, 2.0, true));
        QVERIFY(producer->is_valid());
        producer->set("warp_pitch_method", "wsola");

        playAudio(*producer);
        int frequency = 0;
        const std::vector<float> audio = playAudio(*producer, &frequency, 10);
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - TONE) < TONE * 0.02, qPrintable(QString::number(measured)));
    }

    // A pitch scale close to 1 must still give exactly the requested number of samples,
    // or the queued audio runs out (or piles up) over time.
    void PitchscaleNearOneDoesNotDrift()
    {
        Profile profile("dv_pal");
        Producer producer(profile, "tone:");
        QVERIFY(producer.is_valid());
        Filter filter(profile, "wsola");
        QVERIFY(filter.is_valid());
        filter.set("pitchscale", 1.001);
        producer.attach(filter);

        int frequency = 0;
        const std::vector<float> audio = playAudio(producer, &frequency, 0, 1000);
        QVERIFY2(longestSilence(audio) < 4, qPrintable(QString::number(longestSilence(audio))));
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - 1.001 * TONE) < TONE * 0.02, qPrintable(QString::number(measured)));
    }

    // Without warp_pitch_method, timewarp uses rubberband if it is available
    // and wsola otherwise. Either one keeps the pitch.
    void UnsetMethodPreservesPitch()
    {
        Profile profile("dv_pal");
        std::unique_ptr<Producer> producer(createProducer(profile, 2.0, true));
        QVERIFY(producer->is_valid());

        int frequency = 0;
        const std::vector<float> audio = playAudio(*producer, &frequency);
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - TONE) < TONE * 0.02, qPrintable(QString::number(measured)));
    }
};

QTEST_APPLESS_MAIN(TestWsola)

#include "test_wsola.moc"
