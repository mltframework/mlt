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

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
#include <string.h>
#include <QtTest>

using namespace Mlt;

namespace {

struct Pixel
{
    int r;
    int g;
    int b;
    int a;
};

mlt_frame solidFrame(int width, int height, const uint8_t *pixels)
{
    mlt_frame frame = mlt_frame_init(NULL);
    const int count = width * height;
    uint8_t *image = (uint8_t *) mlt_pool_alloc(count * 4);
    memcpy(image, pixels, count * 4);
    mlt_frame_set_image(frame, image, count * 4, mlt_pool_release);
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties_set_int(properties, "format", mlt_image_rgba);
    mlt_properties_set_int(properties, "width", width);
    mlt_properties_set_int(properties, "height", height);
    return frame;
}

} // namespace

class TestPlaceboBlend : public QObject
{
    Q_OBJECT
    Profile profile;

public:
    TestPlaceboBlend()
    {
        Factory::init();
        profile.set_width(8);
        profile.set_height(8);
        profile.set_progressive(1);
        profile.set_colorspace(709);
        profile.set_sample_aspect(1, 1);
        profile.set_display_aspect(1, 1);
        profile.set_explicit(1);
    }

private:
    bool available()
    {
        Transition probe(profile, "placebo.blend");
        return probe.is_valid();
    }

    Pixel blend(int mode, Pixel bottom, Pixel top, const char *frameMode)
    {
        Pixel out = {-1, -1, -1, -1};
        const uint8_t bottomPx[4]
            = {(uint8_t) bottom.r, (uint8_t) bottom.g, (uint8_t) bottom.b, (uint8_t) bottom.a};
        const uint8_t topPx[4]
            = {(uint8_t) top.r, (uint8_t) top.g, (uint8_t) top.b, (uint8_t) top.a};
        mlt_frame a = solidFrame(1, 1, bottomPx);
        mlt_frame b = solidFrame(1, 1, topPx);
        if (frameMode)
            mlt_properties_set(MLT_FRAME_PROPERTIES(b), "placebo.blend.mode", frameMode);

        mlt_transition transition = mlt_factory_transition(profile.get_profile(),
                                                           "placebo.blend",
                                                           NULL);
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "mode", mode);
        mlt_transition_process(transition, a, b);

        mlt_image_format format = mlt_image_rgba;
        int width = 0;
        int height = 0;
        uint8_t *image = NULL;
        if (!mlt_frame_get_image(a, &image, &format, &width, &height, 0) && image
            && format == mlt_image_rgba) {
            out.r = image[0];
            out.g = image[1];
            out.b = image[2];
            out.a = image[3];
        }
        mlt_transition_close(transition);
        mlt_frame_close(b);
        mlt_frame_close(a);
        return out;
    }

    void expect(Pixel got, Pixel want, int tolerance, const char *label)
    {
        const bool close = qAbs(got.r - want.r) <= tolerance && qAbs(got.g - want.g) <= tolerance
                           && qAbs(got.b - want.b) <= tolerance
                           && qAbs(got.a - want.a) <= tolerance;
        QVERIFY2(close,
                 qPrintable(QString("%1 got (%2,%3,%4,%5) want (%6,%7,%8,%9) ±%10")
                                .arg(label)
                                .arg(got.r)
                                .arg(got.g)
                                .arg(got.b)
                                .arg(got.a)
                                .arg(want.r)
                                .arg(want.g)
                                .arg(want.b)
                                .arg(want.a)
                                .arg(tolerance)));
    }

private Q_SLOTS:
    void SourceOverPartialAlpha()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        // Half red over opaque green. Straight source-over rounds to this.
        expect(blend(0, {0, 255, 0, 255}, {255, 0, 0, 128}, NULL), {128, 127, 0, 255}, 2, "over");
    }

    void TransparentTopLeavesBottom()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        const Pixel bottom = {10, 20, 30, 200};
        expect(blend(0, bottom, {255, 255, 255, 0}, NULL), bottom, 1, "transparent top");
    }

    void OpaqueTopReplacesBottom()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        expect(blend(0, {0, 255, 0, 255}, {0, 0, 255, 255}, NULL),
               {0, 0, 255, 255},
               1,
               "opaque top");
    }

    void SourceAndDestination()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        const Pixel bottom = {0, 255, 0, 255};
        const Pixel top = {255, 0, 0, 128};
        expect(blend(3, bottom, top, NULL), top, 1, "source");
        expect(blend(4, bottom, top, NULL), bottom, 1, "destination");
    }

    void MultiplyScreenDarkenLighten()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        expect(blend(13, {255, 0, 0, 255}, {0, 255, 0, 255}, NULL), {0, 0, 0, 255}, 1, "multiply");
        expect(blend(14, {128, 128, 128, 255}, {128, 128, 128, 255}, NULL),
               {192, 192, 192, 255},
               2,
               "screen");
        expect(blend(16, {200, 40, 100, 255}, {40, 180, 100, 255}, NULL),
               {40, 40, 100, 255},
               1,
               "darken");
        expect(blend(17, {200, 40, 100, 255}, {40, 180, 100, 255}, NULL),
               {200, 180, 100, 255},
               1,
               "lighten");
    }

    void ClearXorAndPlus()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        expect(blend(2, {0, 255, 0, 255}, {255, 0, 0, 255}, NULL), {0, 0, 0, 0}, 1, "clear");
        expect(blend(11, {0, 255, 0, 255}, {255, 0, 0, 255}, NULL), {0, 0, 0, 0}, 1, "xor");
        expect(blend(12, {200, 0, 0, 255}, {100, 0, 0, 255}, NULL), {255, 0, 0, 255}, 1, "plus");
    }

    void UnknownModeIsSourceOver()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        expect(blend(99, {0, 255, 0, 255}, {255, 0, 0, 128}, NULL),
               {128, 127, 0, 255},
               2,
               "mode 99");
    }

    void PerPixel()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        const uint8_t bottomPx[] = {0, 255, 0, 255, 0, 0, 255, 255};
        const uint8_t topPx[] = {255, 0, 0, 128, 255, 255, 255, 0};
        mlt_frame a = solidFrame(2, 1, bottomPx);
        mlt_frame b = solidFrame(2, 1, topPx);
        mlt_transition transition = mlt_factory_transition(profile.get_profile(),
                                                           "placebo.blend",
                                                           NULL);
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "mode", 0);
        mlt_transition_process(transition, a, b);

        mlt_image_format format = mlt_image_rgba;
        int width = 0;
        int height = 0;
        uint8_t *image = NULL;
        const int error = mlt_frame_get_image(a, &image, &format, &width, &height, 0);
        QVERIFY2(!error && image && format == mlt_image_rgba && width == 2 && height == 1,
                 "per-pixel blend did not return rgba");
        expect({image[0], image[1], image[2], image[3]}, {128, 127, 0, 255}, 2, "left");
        expect({image[4], image[5], image[6], image[7]}, {0, 0, 255, 255}, 1, "right");
        mlt_transition_close(transition);
        mlt_frame_close(b);
        mlt_frame_close(a);
    }

    void FrameModeOverridesTransition()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        // Transition mode is source-over. The upper frame asks for destination.
        const Pixel bottom = {0, 255, 0, 255};
        expect(blend(0, bottom, {255, 0, 0, 128}, "4"), bottom, 1, "frame mode");
    }

    void BlendModeFilter()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        const uint8_t bottomPx[4] = {0, 255, 0, 255};
        const uint8_t topPx[4] = {255, 0, 0, 128};
        mlt_frame a = solidFrame(1, 1, bottomPx);
        mlt_frame b = solidFrame(1, 1, topPx);
        mlt_filter filter = mlt_factory_filter(profile.get_profile(), "placebo.blend_mode", "3");
        QVERIFY(filter);
        mlt_filter_process(filter, b);

        mlt_transition transition = mlt_factory_transition(profile.get_profile(),
                                                           "placebo.blend",
                                                           NULL);
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "mode", 0);
        mlt_transition_process(transition, a, b);

        mlt_image_format format = mlt_image_rgba;
        int width = 0;
        int height = 0;
        uint8_t *image = NULL;
        QVERIFY(!mlt_frame_get_image(a, &image, &format, &width, &height, 0));
        expect({image[0], image[1], image[2], image[3]}, {255, 0, 0, 128}, 1, "blend_mode filter");
        mlt_filter_close(filter);
        mlt_transition_close(transition);
        mlt_frame_close(b);
        mlt_frame_close(a);
    }

    void LinearLightKeepsPartialCoverage()
    {
        if (!available())
            QSKIP("placebo.blend is unavailable");
        // Publish the GPU before the loader builds color_transform and placebo.scale.
        Transition anchor(profile, "placebo.blend");
        QVERIFY(anchor.is_valid());

        Producer bottom(profile, "loader", "color:0x00ff00ff");
        Producer top(profile, "loader", "color:0xff000080");
        QVERIFY(bottom.is_valid());
        QVERIFY(top.is_valid());
        Frame *a = bottom.get_frame();
        Frame *b = top.get_frame();
        QVERIFY(a && b);
        a->set("consumer.mlt_color_trc", "linear");
        b->set("consumer.mlt_color_trc", "linear");

        mlt_transition transition = mlt_factory_transition(profile.get_profile(),
                                                           "placebo.blend",
                                                           NULL);
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "mode", 0);
        mlt_transition_process(transition, a->get_frame(), b->get_frame());

        mlt_image_format format = mlt_image_rgba;
        int width = 0;
        int height = 0;
        uint8_t *image = a->get_image(format, width, height, 0);
        QVERIFY(image);
        QVERIFY(format == mlt_image_rgba);
        // Full red at half coverage over full green, in linear light, is still
        // about half of each. A flattened top would be an opaque dark red card.
        expect({image[0], image[1], image[2], image[3]}, {128, 128, 0, 255}, 3, "linear over");
        mlt_transition_close(transition);
        delete b;
        delete a;
    }
};

QTEST_APPLESS_MAIN(TestPlaceboBlend)

#include "test_placebo_blend.moc"
