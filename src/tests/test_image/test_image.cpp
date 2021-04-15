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
	TestImage()
    {
        Factory::init();
    }

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
		Image i(1920, 1080, mlt_image_rgb );
		QVERIFY(i.plane(0) != nullptr);
	}

	void PlaneAndStrideRgb24()
	{
		Image i(1920, 1080, mlt_image_rgb );
		QVERIFY(i.plane(0) != nullptr);
		QCOMPARE(i.stride(0), 1920 * 3);
		QVERIFY(i.plane(1) == nullptr);
		QCOMPARE(i.stride(1), 0);
		QVERIFY(i.plane(2) == nullptr);
		QCOMPARE(i.stride(2), 0);
		QVERIFY(i.plane(3) == nullptr);
		QCOMPARE(i.stride(3), 0);
	}

	void PlaneAndStrideRgb24a()
	{
		Image i(1920, 1080, mlt_image_rgba );
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
		Image i(1920, 1080, mlt_image_yuv422 );
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
		Image i(1920, 1080, mlt_image_yuv420p );
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
		Image i(1920, 1080, mlt_image_yuv420p );
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
		Image i(1920, 1080, mlt_image_rgb );
		i.set_colorspace(mlt_colorspace_bt709);
		QCOMPARE(i.colorspace(), mlt_colorspace_bt709);
	}

	void InitAlpha()
	{
		Image i(1920, 1080, mlt_image_rgb );
		i.init_alpha();
		QVERIFY(i.plane(3) != nullptr);
	}
};

QTEST_APPLESS_MAIN(TestImage)

#include "test_image.moc"
