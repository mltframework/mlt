/*
 * Copyright (C) 2025 Meltytech, LLC
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

#include <mlt++/Mlt.h>
#include <QtTest>
using namespace Mlt;

class TestMultitrack : public QObject
{
    Q_OBJECT

    Profile profile;

public:
    TestMultitrack()
        : profile("dv_pal")
    {
        Factory::init();
    }

private Q_SLOTS:

    void TestConstructor()
    {
        mlt_multitrack multitrack = mlt_multitrack_init();
        QVERIFY(multitrack != nullptr);

        Multitrack multiTrack(multitrack);
        QVERIFY(multiTrack.get_multitrack() == multitrack);

        mlt_multitrack_close(multitrack);
    }

    void ConnectAndDisconnect()
    {
        mlt_multitrack multitrack = mlt_multitrack_init();
        Producer producer(profile, "color");

        Multitrack multiTrack(multitrack);
        QVERIFY(multiTrack.connect(producer, 0) == 0);
        QCOMPARE(multiTrack.count(), 1);

        QVERIFY(multiTrack.disconnect(0) == 0);
        QCOMPARE(multiTrack.count(), 0);

        mlt_multitrack_close(multitrack);
    }

    void InsertAndClip()
    {
        mlt_multitrack multitrack = mlt_multitrack_init();
        Producer producer(profile, "color");

        Multitrack multiTrack(multitrack);
        QVERIFY(multiTrack.insert(producer, 0) == 0);
        QCOMPARE(multiTrack.count(), 1);

        QVERIFY(multiTrack.clip(::mlt_whence_relative_start, 0) == 0);

        mlt_multitrack_close(multitrack);
    }

    void GetTrack()
    {
        mlt_multitrack multitrack = mlt_multitrack_init();
        Producer producer(profile, "color");

        Multitrack multiTrack(multitrack);
        QVERIFY(multiTrack.insert(producer, 0) == 0);

        Producer *track = multiTrack.track(0);
        QVERIFY(track != nullptr);
        QCOMPARE(track->get_producer(), producer.get_producer());
        delete track;

        mlt_multitrack_close(multitrack);
    }

    void GetFrame()
    {
        mlt_multitrack multitrack = mlt_multitrack_init();
        Producer producer(profile, "color");

        Multitrack multiTrack(multitrack);
        QVERIFY(multiTrack.connect(producer, 0) == 0);

        auto position = 3;
        multiTrack.seek(position);
        auto frame = multiTrack.get_frame();
        QVERIFY(frame != nullptr);
        QCOMPARE(frame->get_position(), position);
        // The color producer created the frame
        QCOMPARE(frame->get_int("progressive"), 1);
        delete frame;

        // Test hiding and muting the producer
        producer.set("hide", 3);
        frame = multiTrack.get_frame();
        QVERIFY(frame != nullptr);
        // Returns a dummy frame
        QCOMPARE(frame->get_int("progressive"), 0);
        delete frame;

        mlt_multitrack_close(multitrack);
    }
};

QTEST_APPLESS_MAIN(TestMultitrack)

#include "test_multitrack.moc"
