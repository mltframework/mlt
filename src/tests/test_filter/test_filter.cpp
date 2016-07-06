/*
 * Copyright (C) 2015 Brian Matherly <code@brianmatherly.com>
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

class TestFilter: public QObject
{
    Q_OBJECT
    locale_t locale;

public:
    TestFilter() {
#if defined(__linux__) || defined(__APPLE__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        Factory::init();
    }

    ~TestFilter() {
#if defined(__linux__) || defined(__APPLE__)
        freelocale(locale);
#endif
    }

private Q_SLOTS:
    void ProcessModifiesFrame()
    {
        Profile profile("dv_ntsc");
        Producer producer(profile, "noise", NULL);
        Filter filter(profile, "resize");
        int width = 0;
        int height = 0;
        mlt_image_format format = mlt_image_rgb24;

        // Get a frame from the producer
        Frame* frame = producer.get_frame();

        // Get the default image size: width should match profile
        frame->get_image(format, width, height, 0);
        QCOMPARE(width, 720);

        // Without applying the filter, the width request is not honored.
        width = 360;
        frame->get_image(format, width, height, 0);
        QCOMPARE(width, 720);

        // Apply the filter and the requested width will be provided
        width = 360;
        filter.process(*frame);
        frame->get_image(format, width, height, 0);
        QCOMPARE(width, 360);

        delete frame;
    }

};

QTEST_APPLESS_MAIN(TestFilter)

#include "test_filter.moc"

