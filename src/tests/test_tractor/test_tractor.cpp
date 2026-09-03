/*
 * Copyright (C) 2015-2026 Meltytech, LLC
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

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
using namespace Mlt;

static Producer makeColor(Profile &profile, const char *color)
{
    Producer p(profile, "color", color);
    p.set("length", 10);
    p.set_in_and_out(0, 9);
    return p;
}

static Producer makeFxCut(Profile &profile, Filter &filter)
{
    Producer fx = makeColor(profile, "0x00000000");
    fx.set("mlt_image_format", "rgba");
    fx.set("meta.fx_cut", 1);
    fx.attach(filter);
    return fx;
}

static bool sampleCenterRgb(Frame *frame, int &r, int &g, int &b)
{
    mlt_image_format fmt = mlt_image_rgb;
    int w = 0, h = 0;
    uint8_t *image = frame->get_image(fmt, w, h, 0);
    if (!image || w < 2 || h < 1)
        return false;
    int x = (w / 2) & ~1;
    int y = h / 2;
    if (fmt == mlt_image_rgb) {
        int i = (y * w + x) * 3;
        r = image[i];
        g = image[i + 1];
        b = image[i + 2];
    } else if (fmt == mlt_image_rgba) {
        int i = (y * w + x) * 4;
        r = image[i];
        g = image[i + 1];
        b = image[i + 2];
    } else if (fmt == mlt_image_yuv422) {
        int off = y * w * 2 + x * 2;
        int yy = image[off];
        int u = image[off + 1];
        int v = image[off + 3];
        r = qBound(0, (int) (1.164 * (yy - 16) + 1.596 * (v - 128)), 255);
        g = qBound(0, (int) (1.164 * (yy - 16) - 0.813 * (v - 128) - 0.391 * (u - 128)), 255);
        b = qBound(0, (int) (1.164 * (yy - 16) + 2.018 * (u - 128)), 255);
    } else {
        return false;
    }
    return true;
}

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

    void ConvertImagePropagatesThroughMultitrack()
    {
        // The tractor's mlt_frame_prepend_image_from_service path calls
        // mlt_frame_copy_convert_image to propagate converters from each track
        // frame onto the merged frame.
        Tractor t(profile);
        QVERIFY(t.is_valid());

        // Wrap the track producer in loader so that image converter filters
        // (avcolor_space, imageconvert) are attached and pushed onto track frames.
        Producer p1(profile, "loader", "noise");
        QVERIFY(p1.is_valid());
        t.set_track(p1, 0);

        mlt_frame merged = NULL;
        mlt_service_get_frame(MLT_PRODUCER_SERVICE(t.get_producer()), &merged, 0);
        QVERIFY(merged != NULL);

        // Calling get_image triggers the tractor's from-service image callback,
        // which pulls track frames through the loader filter chain (causing converters
        // to be pushed onto them), then copies them onto the merged frame via
        // mlt_frame_copy_convert_image.
        uint8_t *image = NULL;
        mlt_image_format fmt = mlt_image_rgb;
        int width = 0, height = 0;
        mlt_frame_get_image(merged, &image, &fmt, &width, &height, 0);

        QVERIFY(mlt_frame_has_convert_image(merged));

        mlt_frame_close(merged);
    }

    void FxCutAppliesToTrackBelow()
    {
        Transition blend(profile, "composite");
        Filter brightness(profile, "brightness");
        if (!blend.is_valid() || !brightness.is_valid())
            QSKIP("composite or brightness not available");
        brightness.set("level", 0.0);

        Producer red = makeColor(profile, "0xff0000ff");
        QVERIFY(red.is_valid());
        Producer fx = makeFxCut(profile, brightness);
        QVERIFY(fx.is_valid());

        Playlist track0(profile);
        Playlist track1(profile);
        track0.append(red);
        track1.append(fx);

        Tractor t(profile);
        t.set_track(track0, 0);
        t.set_track(track1, 1);
        t.plant_transition(blend, 0, 1);

        Frame *frame = t.get_frame();
        QVERIFY(frame != NULL);
        int r = 0, g = 0, b = 0;
        QVERIFY(sampleCenterRgb(frame, r, g, b));
        // brightness level=0 turns the lower red clip black.
        QString pixel = QString("rgb=%1,%2,%3").arg(r).arg(g).arg(b);
        QVERIFY2(r < 40, qPrintable(pixel));
        QVERIFY2(g < 40, qPrintable(pixel));
        QVERIFY2(b < 40, qPrintable(pixel));
        delete frame;
    }

    void FxCutAppliesToTrackBelowWithReversedTracks()
    {
        Transition blend(profile, "composite");
        Filter brightness(profile, "brightness");
        if (!blend.is_valid() || !brightness.is_valid())
            QSKIP("composite or brightness not available");
        brightness.set("level", 0.0);

        Producer red = makeColor(profile, "0xff0000ff");
        QVERIFY(red.is_valid());
        Producer fx = makeFxCut(profile, brightness);
        QVERIFY(fx.is_valid());

        Playlist track0(profile);
        Playlist track1(profile);
        track0.append(red);
        track1.append(fx);

        Tractor t(profile);
        t.set_track(track0, 0);
        t.set_track(track1, 1);
        // a_track > b_track: reverse_order must still wrap the lower clip.
        t.plant_transition(blend, 1, 0);

        Frame *frame = t.get_frame();
        QVERIFY(frame != NULL);
        int r = 0, g = 0, b = 0;
        QVERIFY(sampleCenterRgb(frame, r, g, b));
        QString pixel = QString("rgb=%1,%2,%3").arg(r).arg(g).arg(b);
        QVERIFY2(r < 40, qPrintable(pixel));
        QVERIFY2(g < 40, qPrintable(pixel));
        QVERIFY2(b < 40, qPrintable(pixel));
        delete frame;
    }

    void FxCutAppliesToTrackBelowWithoutTransition()
    {
        Filter brightness(profile, "brightness");
        if (!brightness.is_valid())
            QSKIP("brightness not available");
        brightness.set("level", 0.0);

        Producer red = makeColor(profile, "0xff0000ff");
        QVERIFY(red.is_valid());
        Producer fx = makeFxCut(profile, brightness);
        QVERIFY(fx.is_valid());

        Playlist track0(profile);
        Playlist track1(profile);
        track0.append(red);
        track1.append(fx);

        Tractor t(profile);
        t.set_track(track0, 0);
        t.set_track(track1, 1);

        Frame *frame = t.get_frame();
        QVERIFY(frame != NULL);
        int r = 0, g = 0, b = 0;
        QVERIFY(sampleCenterRgb(frame, r, g, b));
        // Classic tractor stacking: fx_cut filters the lower track with no blend.
        QString pixel = QString("rgb=%1,%2,%3").arg(r).arg(g).arg(b);
        QVERIFY2(r < 40, qPrintable(pixel));
        QVERIFY2(g < 40, qPrintable(pixel));
        QVERIFY2(b < 40, qPrintable(pixel));
        delete frame;
    }

    void FxCutDoesNotAffectTrackAbove()
    {
        Transition blendFx(profile, "composite");
        Transition blendOverlay(profile, "composite");
        Filter brightness(profile, "brightness");
        if (!blendFx.is_valid() || !blendOverlay.is_valid() || !brightness.is_valid())
            QSKIP("composite or brightness not available");
        brightness.set("level", 0.0);

        Producer red = makeColor(profile, "0xff0000ff");
        QVERIFY(red.is_valid());
        Producer fx = makeFxCut(profile, brightness);
        QVERIFY(fx.is_valid());
        Producer green = makeColor(profile, "0x00ff00ff");
        QVERIFY(green.is_valid());

        Playlist track0(profile);
        Playlist track1(profile);
        Playlist track2(profile);
        track0.append(red);
        track1.append(fx);
        track2.append(green);

        Tractor t(profile);
        t.set_track(track0, 0);
        t.set_track(track1, 1);
        t.set_track(track2, 2);
        // Same a_track layout as Shotcut: both overlays blend onto the bottom video.
        t.plant_transition(blendFx, 0, 1);
        t.plant_transition(blendOverlay, 0, 2);

        Frame *frame = t.get_frame();
        QVERIFY(frame != NULL);
        int r = 0, g = 0, b = 0;
        QVERIFY(sampleCenterRgb(frame, r, g, b));
        // Opaque green overlay must stay green (not darkened by the fx_cut).
        QString pixel = QString("rgb=%1,%2,%3").arg(r).arg(g).arg(b);
        QVERIFY2(r < 40, qPrintable(pixel));
        QVERIFY2(g > 200, qPrintable(pixel));
        QVERIFY2(b < 40, qPrintable(pixel));
        delete frame;
    }

    void FxCutDoesNotAffectTrackAboveWithReversedTracks()
    {
        Transition blendFx(profile, "composite");
        Transition blendOverlay(profile, "composite");
        Filter brightness(profile, "brightness");
        if (!blendFx.is_valid() || !blendOverlay.is_valid() || !brightness.is_valid())
            QSKIP("composite or brightness not available");
        brightness.set("level", 0.0);

        Producer red = makeColor(profile, "0xff0000ff");
        QVERIFY(red.is_valid());
        Producer fx = makeFxCut(profile, brightness);
        QVERIFY(fx.is_valid());
        Producer green = makeColor(profile, "0x00ff00ff");
        QVERIFY(green.is_valid());

        Playlist track0(profile);
        Playlist track1(profile);
        Playlist track2(profile);
        track0.append(red);
        track1.append(fx);
        track2.append(green);

        Tractor t(profile);
        t.set_track(track0, 0);
        t.set_track(track1, 1);
        t.set_track(track2, 2);
        // Reversed fx blend: wrap must still hide the fx_cut so stacking does
        // not grade the overlay.
        t.plant_transition(blendFx, 1, 0);
        t.plant_transition(blendOverlay, 0, 2);

        Frame *frame = t.get_frame();
        QVERIFY(frame != NULL);
        int r = 0, g = 0, b = 0;
        QVERIFY(sampleCenterRgb(frame, r, g, b));
        QString pixel = QString("rgb=%1,%2,%3").arg(r).arg(g).arg(b);
        QVERIFY2(r < 40, qPrintable(pixel));
        QVERIFY2(g > 200, qPrintable(pixel));
        QVERIFY2(b < 40, qPrintable(pixel));
        delete frame;
    }

    void FxCutAloneDoesNotCrash()
    {
        Transition blend(profile, "composite");
        Filter brightness(profile, "brightness");
        if (!blend.is_valid() || !brightness.is_valid())
            QSKIP("composite or brightness not available");
        brightness.set("level", 0.0);

        Producer fx = makeFxCut(profile, brightness);
        QVERIFY(fx.is_valid());

        Playlist track0(profile);
        track0.append(fx);

        Tractor t(profile);
        t.set_track(track0, 0);

        Frame *frame = t.get_frame();
        QVERIFY(frame != NULL);
        mlt_image_format fmt = mlt_image_rgb;
        int w = 0, h = 0;
        uint8_t *image = frame->get_image(fmt, w, h, 0);
        QVERIFY(image != NULL);
        delete frame;
    }
};

QTEST_APPLESS_MAIN(TestTractor)

#include "test_tractor.moc"
