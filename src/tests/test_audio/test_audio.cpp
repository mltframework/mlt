/*
 * Copyright (C) 2020 Meltytech, LLC
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
};

QTEST_APPLESS_MAIN(TestAudio)

#include "test_audio.moc"
