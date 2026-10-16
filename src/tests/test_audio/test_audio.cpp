/*
 * Copyright (C) 2020-2026 Meltytech, LLC
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

#include <QString>
#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestAudio : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void DefaultConstructor()
    {
        Audio a;
        QVERIFY(a.samples() == 0);
    }

    void ConstructFromAudio()
    {
        mlt_audio audio = mlt_audio_new();
        audio->samples = 500;
        Audio a(audio);
        QVERIFY(a.samples() == 500);
    }

    void GetSetData()
    {
        Audio a;
        void *data = malloc(500);
        a.set_data(data);
        QVERIFY(a.data() == data);
        free(data);
        a.set_data(nullptr);
    }

    void ShrinkPlanarAudio_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<int>("samples");
        QTest::addColumn<QList<int>>("expected");

        const QList<int> unchanged = {11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34};
        const QList<int> shrunk = {11, 12, 21, 22, 31, 32};
        for (auto format : {mlt_audio_s32, mlt_audio_float}) {
            const QString name = mlt_audio_format_name(format);
            QTest::newRow(qPrintable(name + "-negative")) << int(format) << -1 << unchanged;
            QTest::newRow(qPrintable(name + "-same")) << int(format) << 4 << unchanged;
            QTest::newRow(qPrintable(name + "-larger")) << int(format) << 5 << unchanged;
            QTest::newRow(qPrintable(name + "-smaller")) << int(format) << 2 << shrunk;
            QTest::newRow(qPrintable(name + "-zero")) << int(format) << 0 << QList<int>();
        }
    }

    void ShrinkPlanarAudio()
    {
        QFETCH(int, format);
        QFETCH(int, samples);
        QFETCH(QList<int>, expected);

        int32_t data[] = {11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34};
        struct mlt_audio_s audio = {};
        mlt_audio_set_values(&audio, data, 48000, mlt_audio_format(format), 4, 3);

        mlt_audio_shrink(&audio, samples);

        QCOMPARE(audio.samples, int(expected.size()) / 3);
        for (int i = 0; i < expected.size(); i++)
            QCOMPARE(data[i], expected[i]);
    }
};

QTEST_APPLESS_MAIN(TestAudio)

#include "test_audio.moc"
