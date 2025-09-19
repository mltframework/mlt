/*
 * Copyright (C) 2021 Meltytech, LLC
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

class TestImage : public QObject
{
    Q_OBJECT

public:
    TestImage() { Factory::init(); }

private Q_SLOTS:

    void DefaultConstructor()
    {
        Image i;
        QVERIFY(i.width() == 0);
    }

    void ConstructFromImage()
    {
        mlt_image image = mlt_image_new();
        image->width = 500;
        Image i(image);
        QCOMPARE(i.width(), 500);
    }

    void ConstructAndAlloc()
    {
        Image i(1920, 1080, mlt_image_rgb);
        QVERIFY(i.plane(0) != nullptr);
    }

    void PlaneAndStrideRgb()
    {
        Image i(1920, 1080, mlt_image_rgb);
        QVERIFY(i.plane(0) != nullptr);
        QCOMPARE(i.stride(0), 1920 * 3);
        QVERIFY(i.plane(1) == nullptr);
        QCOMPARE(i.stride(1), 0);
        QVERIFY(i.plane(2) == nullptr);
        QCOMPARE(i.stride(2), 0);
        QVERIFY(i.plane(3) == nullptr);
        QCOMPARE(i.stride(3), 0);
    }

    void PlaneAndStrideRgba()
    {
        Image i(1920, 1080, mlt_image_rgba);
        QVERIFY(i.plane(0) != nullptr);
        QCOMPARE(i.stride(0), 1920 * 4);
        QVERIFY(i.plane(1) == nullptr);
        QCOMPARE(i.stride(1), 0);
        QVERIFY(i.plane(2) == nullptr);
        QCOMPARE(i.stride(2), 0);
        QVERIFY(i.plane(3) == nullptr);
        QCOMPARE(i.stride(3), 0);
    }

    void PlaneAndStrideYuv422()
    {
        Image i(1920, 1080, mlt_image_yuv422);
        QVERIFY(i.plane(0) != nullptr);
        QCOMPARE(i.stride(0), 1920 * 2);
        QVERIFY(i.plane(1) == nullptr);
        QCOMPARE(i.stride(1), 0);
        QVERIFY(i.plane(2) == nullptr);
        QCOMPARE(i.stride(2), 0);
        QVERIFY(i.plane(3) == nullptr);
        QCOMPARE(i.stride(3), 0);
    }

    void PlaneAndStrideYuv420p()
    {
        Image i(1920, 1080, mlt_image_yuv420p);
        QVERIFY(i.plane(0) != nullptr);
        QCOMPARE(i.stride(0), 1920);
        QVERIFY(i.plane(1) != nullptr);
        QCOMPARE(i.stride(1), 1920 / 2);
        QVERIFY(i.plane(2) != nullptr);
        QCOMPARE(i.stride(2), 1920 / 2);
        QVERIFY(i.plane(3) == nullptr);
        QCOMPARE(i.stride(3), 0);
    }

    void PlaneAndStrideYuv422p16()
    {
        Image i(1920, 1080, mlt_image_yuv420p);
        QVERIFY(i.plane(0) != nullptr);
        QCOMPARE(i.stride(0), 1920);
        QVERIFY(i.plane(1) != nullptr);
        QCOMPARE(i.stride(1), 1920 / 2);
        QVERIFY(i.plane(2) != nullptr);
        QCOMPARE(i.stride(2), 1920 / 2);
        QVERIFY(i.plane(3) == nullptr);
        QCOMPARE(i.stride(3), 0);
    }

    void GetSetColorspace()
    {
        Image i(1920, 1080, mlt_image_rgb);
        i.set_colorspace(mlt_colorspace_bt709);
        QCOMPARE(i.colorspace(), mlt_colorspace_bt709);
    }

    void InitAlpha()
    {
        Image i(1920, 1080, mlt_image_rgb);
        i.init_alpha();
        QVERIFY(i.plane(3) != nullptr);
    }

    void ColorTrc()
    {
        // Test conversion from short name
        QCOMPARE(mlt_color_trc_bt709, mlt_image_color_trc_id("bt709"));
        QCOMPARE(mlt_color_trc_linear, mlt_image_color_trc_id("linear"));
        // Test conversion from number
        QCOMPARE(mlt_color_trc_bt709, mlt_image_color_trc_id("1"));
        QCOMPARE(mlt_color_trc_linear, mlt_image_color_trc_id("8"));
    }

    void Colorspace()
    {
        // Test conversion from short name
        QCOMPARE(mlt_image_colorspace_id("bt709"), mlt_colorspace_bt709);
        QCOMPARE(mlt_image_colorspace_id("bt470bg"), mlt_colorspace_bt470bg);
        QCOMPARE(mlt_image_colorspace_id("bt601"), mlt_colorspace_bt601);
        QCOMPARE(mlt_image_colorspace_id("rgb"), mlt_colorspace_rgb);
        // Test conversion from number
        QCOMPARE(mlt_image_colorspace_id("709"), mlt_colorspace_bt709);
        QCOMPARE(mlt_image_colorspace_id("470"), mlt_colorspace_bt470bg);
        QCOMPARE(mlt_image_colorspace_id("601"), mlt_colorspace_bt601);
        QCOMPARE(mlt_image_colorspace_id("0"), mlt_colorspace_rgb);
    }

    void ColorPrimaries()
    {
        // Test conversion from short name
        QCOMPARE(mlt_image_color_pri_id("bt709"), mlt_color_pri_bt709);
        QCOMPARE(mlt_image_color_pri_id("bt470bg"), mlt_color_pri_bt470bg);
        QCOMPARE(mlt_image_color_pri_id("smpte170m"), mlt_color_pri_smpte170m);
        QCOMPARE(mlt_image_color_pri_id("bt2020"), mlt_color_pri_bt2020);
        // Test conversion from number
        QCOMPARE(mlt_image_color_pri_id("1"), mlt_color_pri_bt709);
        QCOMPARE(mlt_image_color_pri_id("5"), mlt_color_pri_bt470bg);
        QCOMPARE(mlt_image_color_pri_id("6"), mlt_color_pri_smpte170m);
        QCOMPARE(mlt_image_color_pri_id("9"), mlt_color_pri_bt2020);
    }
};

QTEST_APPLESS_MAIN(TestImage)

#include "test_image.moc"
