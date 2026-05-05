/*
 * Copyright (C) 2026 Julius Künzel <julius.kuenzel@kde.org>
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <QtTest>

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
using namespace Mlt;

class TestModAvformat : public QObject
{
    Q_OBJECT

public:
    TestModAvformat() { Factory::init(); }

    ~TestModAvformat() {}

private Q_SLOTS:
    void BasicAvformatProducer()
    {
        Profile profile;
        Producer producer(profile, "avformat:blue.mpg");
    }

    void BasicAvformatNovalidateProducer()
    {
        Profile profile;
        Producer producer(profile, "avformat-novalidate:blue.mpg");
    }

    void LoaderAttachesImageConverters()
    {
        // The loader producer must attach at least one image converter (avcolor_space or
        // imageconvert) via loader.ini image_convert so that frames produced through it
        // have mlt_frame_has_convert_image() == true.
        Profile profile;
        // Use loader explicitly wrapping the color producer so no file system access is needed.
        mlt_producer raw = mlt_factory_producer(profile.get_profile(), "loader", "color:black");
        QVERIFY(raw != NULL);

        mlt_frame frame = NULL;
        mlt_service_get_frame(MLT_PRODUCER_SERVICE(raw), &frame, 0);
        QVERIFY(frame != NULL);

        QVERIFY(mlt_frame_has_convert_image(frame));

        mlt_frame_close(frame);
        mlt_producer_close(raw);
    }

    void LoaderNoGlSkipsMovitConverter()
    {
        // loader-nogl must not attach movit.convert but still attach a CPU converter.
        Profile profile;
        mlt_producer raw = mlt_factory_producer(profile.get_profile(), "loader-nogl", "color:black");
        QVERIFY(raw != NULL);

        mlt_frame frame = NULL;
        mlt_service_get_frame(MLT_PRODUCER_SERVICE(raw), &frame, 0);
        QVERIFY(frame != NULL);
        QVERIFY(mlt_frame_has_convert_image(frame));

        // Verify movit.convert is not among the attached filters.
        mlt_service svc = MLT_PRODUCER_SERVICE(raw);
        int count = mlt_service_filter_count(svc);
        for (int i = 0; i < count; i++) {
            mlt_filter f = mlt_service_filter(svc, i);
            const char *id = mlt_properties_get(MLT_FILTER_PROPERTIES(f), "mlt_service");
            QVERIFY(qstrcmp(id, "movit.convert") != 0);
        }

        mlt_frame_close(frame);
        mlt_producer_close(raw);
    }
};

QTEST_APPLESS_MAIN(TestModAvformat)

#include "test_mod_avformat.moc"
