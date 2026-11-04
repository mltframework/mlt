/*
 * Copyright (C) 2026 Matheus Salvalagio
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
    // Plays the producer from its first frame and returns the first channel.
    std::vector<float> playAudio(Producer &producer, int *audioFrequency = nullptr)
    {
        std::vector<float> audio;
        producer.seek(0);
        Profile *profile = producer.profile();
        const double fps = profile->fps();
        for (int i = 0; i < FRAMES; i++) {
            std::unique_ptr<Frame> frame(producer.get_frame());
            mlt_audio_format format = mlt_audio_float;
            int frequency = FREQUENCY;
            int channels = CHANNELS;
            int samples = mlt_audio_calculate_frame_samples(fps, FREQUENCY, i);
            const float *data = static_cast<const float *>(
                frame->get_audio(format, frequency, channels, samples));
            if (data && i >= SETTLE_FRAMES && format == mlt_audio_float) {
                audio.insert(audio.end(), data, data + samples);
                if (audioFrequency)
                    *audioFrequency = frequency;
            }
            producer.seek(i + 1);
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
        QCOMPARE(frequency, FREQUENCY);
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

    void SeekDoesNotChangePitch()
    {
        Profile profile("dv_pal");
        std::unique_ptr<Producer> producer(createProducer(profile, 2.0, true));
        QVERIFY(producer->is_valid());
        producer->set("warp_pitch_method", "wsola");

        playAudio(*producer);
        producer->seek(10);
        int frequency = 0;
        const std::vector<float> audio = playAudio(*producer, &frequency);
        const double measured = pitch(audio, frequency);
        QVERIFY2(qAbs(measured - TONE) < TONE * 0.02, qPrintable(QString::number(measured)));
    }
};

QTEST_APPLESS_MAIN(TestWsola)

#include "test_wsola.moc"
