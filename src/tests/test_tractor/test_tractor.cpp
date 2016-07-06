/*
 * Copyright (C) 2015 Dan Dennedy <dan@dennedy.org>
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

class TestTractor : public QObject
{
    Q_OBJECT
    Profile profile;

public:
    TestTractor()
        : profile("dv_pal")
    {
        Factory::init();
    }

private Q_SLOTS:

    void CreateSingleTrack()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p(profile, "noise");
        QVERIFY(p.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p, t.count());
        QCOMPARE(t.count(), 1);
    }

    void FailSameProducerNewTrack()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p(profile, "noise");
        QVERIFY(p.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p, t.count());
        QCOMPARE(t.count(), 1);
        int result = t.set_track(p, t.count());
        QCOMPARE(result, 3);
        QCOMPARE(t.count(), 1);
    }

    void CreateMultipleTracks()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);
    }

    void PlantTransitionWorks()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 0, 1);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);
    }

    void InsertTrackBelowAdjustsTransition()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 0, 1);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);

        Producer p3(profile, "noise");
        QVERIFY(p3.is_valid());
        t.insert_track(p3, 0);
        QCOMPARE(t.count(), 3);
        QCOMPARE(trans.get_int("a_track"), 1);
        QCOMPARE(trans.get_int("b_track"), 2);
    }

    void InsertTrackMiddleAdjustsTransition()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 0, 1);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);

        Producer p3(profile, "noise");
        QVERIFY(p3.is_valid());
        t.insert_track(p3, 1);
        QCOMPARE(t.count(), 3);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 2);
    }

    void InsertTrackAboveDoesNotAffectTransition()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 0, 1);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);

        Producer p3(profile, "noise");
        QVERIFY(p3.is_valid());
        t.insert_track(p3, 2);
        QCOMPARE(t.count(), 3);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);
    }

    void RemoveTrackBelowAdjustsTransition()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);
        Producer p3(profile, "noise");
        QVERIFY(p3.is_valid());
        t.set_track(p3, t.count());

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 0, 1);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);

        t.remove_track(0);
        QCOMPARE(t.count(), 2);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 0);
        // This transition is a candidate for removal.
    }

    void RemoveMiddleTrackAdjustsTransition()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);
        Producer p3(profile, "noise");
        QVERIFY(p3.is_valid());
        t.set_track(p3, t.count());

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 0, 2);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 2);

        t.remove_track(1);
        QCOMPARE(t.count(), 2);
        QCOMPARE(trans.get_int("a_track"), 0);
        QCOMPARE(trans.get_int("b_track"), 1);
    }

    void RemoveTrackAboveAdjustsTransition()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);
        Producer p3(profile, "noise");
        QVERIFY(p3.is_valid());
        t.set_track(p3, t.count());

        Transition trans(profile, "mix");
        QVERIFY(trans.is_valid());
        t.plant_transition(trans, 1, 2);
        QCOMPARE(trans.get_int("a_track"), 1);
        QCOMPARE(trans.get_int("b_track"), 2);

        t.remove_track(2);
        QCOMPARE(t.count(), 2);
        QCOMPARE(trans.get_int("a_track"), 1);
        QCOMPARE(trans.get_int("b_track"), 1);
        // This transition is a candidate for removal.
    }

    void PlantFilterWorks()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);

        Filter filter(profile, "crop");
        QVERIFY(filter.is_valid());
        t.plant_filter(filter, 0);
        QCOMPARE(filter.get_track(), 0);
    }

    void InsertTrackBelowAdjustsFilter()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);

        Filter filter(profile, "crop");
        QVERIFY(filter.is_valid());
        t.plant_filter(filter, 0);
        QCOMPARE(filter.get_track(), 0);

        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.insert_track(p2, 0);
        QCOMPARE(t.count(), 2);
        QCOMPARE(filter.get_track(), 1);
    }

    void InsertTrackAboveDoesNotAffectFilter()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);

        Filter filter(profile, "crop");
        QVERIFY(filter.is_valid());
        t.plant_filter(filter, 0);
        QCOMPARE(filter.get_track(), 0);

        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.insert_track(p2, 1);
        QCOMPARE(t.count(), 2);
        QCOMPARE(filter.get_track(), 0);
    }

    void RemoveTrackBelowAdjustsFilter()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Filter filter(profile, "crop");
        QVERIFY(filter.is_valid());
        t.plant_filter(filter, 1);
        QCOMPARE(filter.get_track(), 1);

        t.remove_track(0);
        QCOMPARE(t.count(), 1);
        QCOMPARE(filter.get_track(), 0);
    }

    void RemoveFilteredTrackAdjustsFilter()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Filter filter(profile, "crop");
        QVERIFY(filter.is_valid());
        t.plant_filter(filter, 1);
        QCOMPARE(filter.get_track(), 1);

        t.remove_track(1);
        QCOMPARE(t.count(), 1);
        QCOMPARE(filter.get_track(), 0);
        // This filter is a candidate for removal.
    }

    void RemoveTrackAboveDoesNotAffectFilter()
    {
        Tractor t(profile);
        QVERIFY(t.is_valid());
        Producer p1(profile, "noise");
        QVERIFY(p1.is_valid());
        QCOMPARE(t.count(), 0);
        t.set_track(p1, t.count());
        QCOMPARE(t.count(), 1);
        Producer p2(profile, "noise");
        QVERIFY(p2.is_valid());
        t.set_track(p2, t.count());
        QCOMPARE(t.count(), 2);

        Filter filter(profile, "crop");
        QVERIFY(filter.is_valid());
        t.plant_filter(filter, 0);
        QCOMPARE(filter.get_track(), 0);

        t.remove_track(1);
        QCOMPARE(t.count(), 1);
        QCOMPARE(filter.get_track(), 0);
    }
};

QTEST_APPLESS_MAIN(TestTractor)

#include "test_tractor.moc"
