/*
 * Copyright (C) 2019-2026 Meltytech, LLC
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

#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestPlaylist : public QObject
{
    Q_OBJECT
    Profile profile;

public:
    TestPlaylist()
        : profile("dv_pal")
    {
        Factory::init();
    }

private Q_SLOTS:

    void Append()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());
        Producer p(profile, "noise");
        QVERIFY(p.is_valid());
        QCOMPARE(pl.count(), 0);
        int result = pl.append(p);
        QCOMPARE(result, 0); // Success
        QCOMPARE(pl.count(), 1);
    }

    void Remove()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());
        Producer p(profile, "noise");
        QVERIFY(p.is_valid());
        QCOMPARE(pl.count(), 0);
        pl.append(p);
        QCOMPARE(pl.count(), 1);
        int result = pl.remove(0);
        QCOMPARE(result, 0); // Success
        QCOMPARE(pl.count(), 0);
    }

    void RemoveErrorOnInvalidIndex()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());
        Producer p(profile, "noise");
        QVERIFY(p.is_valid());
        QCOMPARE(pl.count(), 0);
        pl.append(p);
        QCOMPARE(pl.count(), 1);
        int result = pl.remove(10); // Invalid index
        QCOMPARE(result, 1);        // Fail
        QCOMPARE(pl.count(), 1);
    }

    void Move()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());

        // Initial order: 1, 2, 3
        Producer p1(profile, "noise");
        p1.set("id", 1);
        QVERIFY(p1.is_valid());
        pl.append(p1);
        Producer p2(profile, "noise");
        p2.set("id", 2);
        QVERIFY(p2.is_valid());
        pl.append(p2);
        Producer p3(profile, "noise");
        p3.set("id", 3);
        QVERIFY(p3.is_valid());
        pl.append(p3);
        QCOMPARE(pl.count(), 3);

        // Move first to last
        int result = pl.move(0, 2);
        QCOMPARE(result, 0); // Success
        QCOMPARE(pl.count(), 3);

        // New order: 2, 3, 1
        Producer *pp1 = pl.get_clip(0);
        Producer *pp2 = pl.get_clip(1);
        Producer *pp3 = pl.get_clip(2);
        QCOMPARE(pp1->parent().get_int("id"), 2);
        QCOMPARE(pp2->parent().get_int("id"), 3);
        QCOMPARE(pp3->parent().get_int("id"), 1);
        delete pp1;
        delete pp2;
        delete pp3;
    }

    void MoveCorrectInvalidIndex()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());

        // Initial order: 1, 2, 3
        Producer p1(profile, "noise");
        p1.set("id", 1);
        QVERIFY(p1.is_valid());
        pl.append(p1);
        Producer p2(profile, "noise");
        p2.set("id", 2);
        QVERIFY(p2.is_valid());
        pl.append(p2);
        Producer p3(profile, "noise");
        p3.set("id", 3);
        QVERIFY(p3.is_valid());
        pl.append(p3);
        QCOMPARE(pl.count(), 3);

        // Move first to an invalid index
        int result = pl.move(0, 10); // 10 is invalid
        // This is still successful because move() corrects the index to be the last entry.
        QCOMPARE(result, 0); // Success
        QCOMPARE(pl.count(), 3);

        // The first will be moved to the last after move() corrects the index.
        Producer *pp1 = pl.get_clip(0);
        Producer *pp2 = pl.get_clip(1);
        Producer *pp3 = pl.get_clip(2);
        QCOMPARE(pp1->parent().get_int("id"), 2);
        QCOMPARE(pp2->parent().get_int("id"), 3);
        QCOMPARE(pp3->parent().get_int("id"), 1);
        delete pp1;
        delete pp2;
        delete pp3;
    }

    void Reorder()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());

        // Initial order: 1, 2, 3
        Producer p1(profile, "noise");
        p1.set("id", 1);
        QVERIFY(p1.is_valid());
        pl.append(p1);
        Producer p2(profile, "noise");
        p2.set("id", 2);
        QVERIFY(p2.is_valid());
        pl.append(p2);
        Producer p3(profile, "noise");
        p3.set("id", 3);
        QVERIFY(p3.is_valid());
        pl.append(p3);
        QCOMPARE(pl.count(), 3);

        // Reverse the order of the clips.
        int new_order[] = {2, 1, 0};
        int result = pl.reorder(new_order);
        QCOMPARE(result, 0); // Success
        QCOMPARE(pl.count(), 3);

        // The order will be reversed.
        Producer *pp1 = pl.get_clip(0);
        Producer *pp2 = pl.get_clip(1);
        Producer *pp3 = pl.get_clip(2);
        QCOMPARE(pp1->parent().get_int("id"), 3);
        QCOMPARE(pp2->parent().get_int("id"), 2);
        QCOMPARE(pp3->parent().get_int("id"), 1);
        delete pp1;
        delete pp2;
        delete pp3;
    }

    void ReorderErrorOnDuplicate()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());

        // Initial order: 1, 2, 3
        Producer p1(profile, "noise");
        p1.set("id", 1);
        QVERIFY(p1.is_valid());
        pl.append(p1);
        Producer p2(profile, "noise");
        p2.set("id", 2);
        QVERIFY(p2.is_valid());
        pl.append(p2);
        Producer p3(profile, "noise");
        p3.set("id", 3);
        QVERIFY(p3.is_valid());
        pl.append(p3);
        QCOMPARE(pl.count(), 3);

        // Provide an order with duplicate indices.
        int new_order[] = {2, 2, 2};
        int result = pl.reorder(new_order);
        QCOMPARE(result, 1); // Fail
        QCOMPARE(pl.count(), 3);

        // The order will unchanged.
        Producer *pp1 = pl.get_clip(0);
        Producer *pp2 = pl.get_clip(1);
        Producer *pp3 = pl.get_clip(2);
        QCOMPARE(pp1->parent().get_int("id"), 1);
        QCOMPARE(pp2->parent().get_int("id"), 2);
        QCOMPARE(pp3->parent().get_int("id"), 3);
        delete pp1;
        delete pp2;
        delete pp3;
    }

    void ReorderErrorOnInvalidIndex()
    {
        Playlist pl(profile);
        QVERIFY(pl.is_valid());

        // Initial order: 1, 2, 3
        Producer p1(profile, "noise");
        p1.set("id", 1);
        QVERIFY(p1.is_valid());
        pl.append(p1);
        Producer p2(profile, "noise");
        p2.set("id", 2);
        QVERIFY(p2.is_valid());
        pl.append(p2);
        Producer p3(profile, "noise");
        p3.set("id", 3);
        QVERIFY(p3.is_valid());
        pl.append(p3);
        QCOMPARE(pl.count(), 3);

        // Provide an order with an invalid index.
        int new_order[] = {0, 1, 10};
        int result = pl.reorder(new_order);
        QCOMPARE(result, 1); // Fail
        QCOMPARE(pl.count(), 3);

        // The order will unchanged.
        Producer *pp1 = pl.get_clip(0);
        Producer *pp2 = pl.get_clip(1);
        Producer *pp3 = pl.get_clip(2);
        QCOMPARE(pp1->parent().get_int("id"), 1);
        QCOMPARE(pp2->parent().get_int("id"), 2);
        QCOMPARE(pp3->parent().get_int("id"), 3);
        delete pp1;
        delete pp2;
        delete pp3;
    }

    void FxCutFrameReceivesProducerMeta()
    {
        Producer p(profile, "color", "0x00000000");
        QVERIFY(p.is_valid());
        p.set("mlt_image_format", "rgba");
        p.set("meta.fx_cut", 1);
        p.set("meta.test.flag", 1);
        p.set("set.consumer.dummy", 1);
        p.set_in_and_out(0, 9);

        Playlist pl(profile);
        pl.append(p);

        Frame *frame = pl.get_frame();
        QVERIFY(frame != NULL);
        QCOMPARE(frame->get_int("fx_cut"), 1);
        QCOMPARE(frame->get_int("meta.test.flag"), 1);
        QCOMPARE(frame->get_int("consumer.dummy"), 1);
        delete frame;
    }

    void RefreshDetectsCutChange()
    {
        Producer p(profile, "color", "red");
        p.set("length", 100);
        p.set_in_and_out(0, 99);
        Playlist pl(profile);
        for (int i = 0; i < 4; i++)
            pl.append(p, 0, 9);
        QCOMPARE(pl.get_playtime(), 40);

        // A cut does not notify the playlist when its points change.
        Producer *cut = pl.get_clip(0);
        QVERIFY(cut != nullptr);
        cut->set_in_and_out(0, 4);
        delete cut;
        QCOMPARE(pl.clip_length(0), 10);

        // The next edit refreshes the playlist from the producers.
        pl.blank(9);
        QCOMPARE(pl.clip_length(0), 5);
        QCOMPARE(pl.clip_start(1), 5);
        QCOMPARE(pl.get_playtime(), 45);

        cut = pl.get_clip(2);
        QVERIFY(cut != nullptr);
        cut->set_in_and_out(2, 4);
        delete cut;
        QCOMPARE(pl.resize_clip(1, 0, 19), 0);
        QCOMPARE(pl.clip_length(1), 20);
        QCOMPARE(pl.clip_length(2), 3);
        QCOMPARE(pl.clip_start(3), 28);
        QCOMPARE(pl.get_playtime(), 48);

        // So does a change of the parent producer.
        cut = pl.get_clip(3);
        QVERIFY(cut != nullptr);
        cut->set_in_and_out(0, 0);
        delete cut;
        p.set("out", 98);
        QCOMPARE(pl.clip_length(3), 1);
        QCOMPARE(pl.get_playtime(), 39);
    }

    void BlankStatus()
    {
        Producer p(profile, "color", "red");
        p.set("length", 100);
        p.set_in_and_out(0, 99);
        Playlist pl(profile);
        pl.blank(4);
        pl.append(p, 0, 9);
        pl.blank(4);
        pl.blank(2);
        pl.append(p, 0, 9);
        pl.blank(6);
        QCOMPARE(pl.count(), 6);
        QVERIFY(pl.is_blank(0));
        QVERIFY(!pl.is_blank(1));
        QVERIFY(pl.is_blank(2));
        QVERIFY(pl.is_blank(3));
        QVERIFY(!pl.is_blank(4));
        QVERIFY(pl.is_blank(5));
        QVERIFY(pl.is_blank(-1));
        QVERIFY(pl.is_blank(6));
        QVERIFY(pl.is_blank_at(0));
        QVERIFY(!pl.is_blank_at(5));
        QVERIFY(pl.is_blank_at(15));
        QVERIFY(pl.is_blank_at(1000));

        pl.consolidate_blanks(0);
        QCOMPARE(pl.count(), 4);
        QVERIFY(pl.is_blank(0));
        QVERIFY(!pl.is_blank(1));
        QVERIFY(pl.is_blank(2));
        QCOMPARE(pl.clip_length(2), 8);
        QVERIFY(!pl.is_blank(3));
        QCOMPARE(pl.get_playtime(), 33);

        // A clip replaced by a blank is merged with its neighbours.
        Producer *clip = pl.replace_with_blank(3);
        QVERIFY(clip != nullptr);
        delete clip;
        QVERIFY(pl.is_blank(3));
        pl.consolidate_blanks(1);
        QCOMPARE(pl.count(), 3);
        QCOMPARE(pl.clip_length(2), 18);
        pl.consolidate_blanks(0);
        QCOMPARE(pl.count(), 2);
        QCOMPARE(pl.get_playtime(), 15);
    }
};

QTEST_APPLESS_MAIN(TestPlaylist)

#include "test_playlist.moc"
