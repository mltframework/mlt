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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
#include <string.h>
#include <QtTest>
using namespace Mlt;

class TestAudioConvert : public QObject
{
    Q_OBJECT

public:
    TestAudioConvert() { Factory::init(); }

    ~TestAudioConvert() { Factory::close(); }

private:
    /** Run one conversion through the audioconvert filter and copy the result out.
     *
     * The filter installs itself as the frame's convert_audio callback, which is
     * the same entry point mlt_frame_get_audio() uses.
     */
    void convert(const void *in,
                 mlt_audio_format from,
                 void *out,
                 mlt_audio_format to,
                 int samples,
                 int channels)
    {
        int in_size = mlt_audio_format_size(from, samples, channels);
        int out_size = mlt_audio_format_size(to, samples, channels);
        Profile profile;
        mlt_frame frame = mlt_frame_init(NULL);
        QVERIFY(frame != NULL);
        mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
        mlt_properties_set_int(properties, "audio_channels", channels);
        mlt_properties_set_int(properties, "audio_samples", samples);

        void *buffer = mlt_pool_alloc(in_size);
        memcpy(buffer, in, in_size);
        mlt_frame_set_audio(frame, buffer, from, in_size, mlt_pool_release);

        mlt_filter filter = mlt_factory_filter(profile.get_profile(), "audioconvert", NULL);
        QVERIFY(filter != NULL);
        mlt_filter_process(filter, frame);
        QVERIFY(frame->convert_audio != NULL);

        mlt_audio_format format = from;
        QCOMPARE(frame->convert_audio(frame, &buffer, &format, to), 0);
        QCOMPARE((int) format, (int) to);
        memcpy(out, buffer, out_size);

        mlt_filter_close(filter);
        mlt_frame_close(frame);
    }

private Q_SLOTS:

    /** Every s16 value must survive a trip through interleaved float.
     *
     * This failed before the conversion was made symmetric: float -> s16 scaled
     * by 32767 while s16 -> float divided by 32768, and the result was truncated
     * rather than rounded, so 65535 of the 65536 values came back 1 LSB low.
     */
    void S16ToF32leRoundTripIsLossless()
    {
        const int samples = 65536;
        QVector<int16_t> in(samples), out(samples);
        QVector<float> mid(samples);
        for (int i = 0; i < samples; i++)
            in[i] = (int16_t) (i - 32768);

        convert(in.data(), mlt_audio_s16, mid.data(), mlt_audio_f32le, samples, 1);
        convert(mid.data(), mlt_audio_f32le, out.data(), mlt_audio_s16, samples, 1);

        for (int i = 0; i < samples; i++)
            if (in[i] != out[i])
                QFAIL(qPrintable(
                    QString("sample %1: %2 came back as %3").arg(i).arg(in[i]).arg(out[i])));
    }

    /** The same, through the planar float format, which takes a different path. */
    void S16ToFloatRoundTripIsLossless()
    {
        const int channels = 2;
        const int samples = 32768;
        const int count = samples * channels;
        QVector<int16_t> in(count), out(count);
        QVector<float> mid(count);
        for (int i = 0; i < count; i++)
            in[i] = (int16_t) (i - 32768);

        convert(in.data(), mlt_audio_s16, mid.data(), mlt_audio_float, samples, channels);
        convert(mid.data(), mlt_audio_float, out.data(), mlt_audio_s16, samples, channels);

        for (int i = 0; i < count; i++)
            if (in[i] != out[i])
                QFAIL(qPrintable(
                    QString("sample %1: %2 came back as %3").arg(i).arg(in[i]).arg(out[i])));
    }

    /** u8 is full scale at +/-128, the same as the u8 -> s16 path assumes. */
    void U8RoundTripIsLossless()
    {
        const int samples = 256;
        QVector<uint8_t> in(samples), out(samples);
        QVector<float> mid(samples);
        for (int i = 0; i < samples; i++)
            in[i] = (uint8_t) i;

        convert(in.data(), mlt_audio_u8, mid.data(), mlt_audio_f32le, samples, 1);
        convert(mid.data(), mlt_audio_f32le, out.data(), mlt_audio_u8, samples, 1);

        for (int i = 0; i < samples; i++)
            if (in[i] != out[i])
                QFAIL(qPrintable(
                    QString("sample %1: %2 came back as %3").arg(i).arg(in[i]).arg(out[i])));
    }

    /** Full scale and beyond must saturate, never wrap. */
    void FloatToIntegerSaturates()
    {
        const float in[4] = {1.0f, -1.0f, 2.0f, -2.0f};
        int16_t s16[4];
        int32_t s32[4];
        uint8_t u8[4];

        convert(in, mlt_audio_f32le, s16, mlt_audio_s16, 4, 1);
        QCOMPARE(s16[0], (int16_t) 32767);
        QCOMPARE(s16[1], (int16_t) -32768);
        QCOMPARE(s16[2], (int16_t) 32767);
        QCOMPARE(s16[3], (int16_t) -32768);

        convert(in, mlt_audio_f32le, s32, mlt_audio_s32le, 4, 1);
        QCOMPARE(s32[0], (int32_t) 2147483647);
        QCOMPARE(s32[1], (int32_t) -2147483647 - 1);

        convert(in, mlt_audio_f32le, u8, mlt_audio_u8, 4, 1);
        QCOMPARE(u8[0], (uint8_t) 255);
        QCOMPARE(u8[1], (uint8_t) 0);
    }

    /** Equal positive and negative inputs must give equal magnitudes.
     *
     * The old s32 path scaled positives by 2147483647 and negatives by
     * 2147483648, which is an asymmetric gain and so generates even harmonics.
     */
    void FloatToIntegerIsSymmetric()
    {
        const float in[2] = {0.5f, -0.5f};
        int16_t s16[2];
        int32_t s32[2];

        convert(in, mlt_audio_f32le, s16, mlt_audio_s16, 2, 1);
        QCOMPARE(s16[0], (int16_t) 16384);
        QCOMPARE(s16[1], (int16_t) -16384);

        convert(in, mlt_audio_f32le, s32, mlt_audio_s32le, 2, 1);
        QCOMPARE(s32[0], (int32_t) 1073741824);
        QCOMPARE(s32[1], (int32_t) -1073741824);
    }

    /** Narrowing must round to nearest, not truncate toward zero. */
    void FloatToIntegerRoundsToNearest()
    {
        const float in[2] = {100.6f / 32768.0f, -100.6f / 32768.0f};
        int16_t s16[2];

        convert(in, mlt_audio_f32le, s16, mlt_audio_s16, 2, 1);
        QCOMPARE(s16[0], (int16_t) 101);
        QCOMPARE(s16[1], (int16_t) -101);
    }
};

QTEST_APPLESS_MAIN(TestAudioConvert)

#include "test_audioconvert.moc"
