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

    void DeinterlacerSourceDiscontinuity()
    {
        Profile profile;
        profile.set_width(32);
        profile.set_height(16);
        profile.set_frame_rate(25, 1);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QByteArray path = dir.filePath("interlaced.mkv").toUtf8();
        {
            // Encode an interlaced clip: black, except a white flash at frame 25
            Producer blipflash(profile, "blipflash");
            if (!blipflash.is_valid())
                QSKIP("blipflash producer not available");
            blipflash.set_in_and_out(0, 29);
            Consumer encoder(profile, "avformat", path.constData());
            encoder.set("vcodec", "mpeg2video");
            encoder.set("an", 1);
            encoder.set("progressive", 0);
            encoder.set("top_field_first", 1);
            encoder.set("real_time", 0);
            encoder.set("terminate_on_pause", 1);
            encoder.connect(blipflash);
            encoder.run();
        }
        // Loader constructor: avdeinterlace is attached as a normalizer link
        Chain chain(profile, path.constData());
        QVERIFY(chain.is_valid());
        bool hasDeinterlacer = false;
        for (int i = 0; i < chain.link_count(); i++) {
            QScopedPointer<Link> link(chain.link(i));
            hasDeinterlacer |= qstrcmp(link->get("mlt_service"), "avdeinterlace") == 0;
        }
        QVERIFY(hasDeinterlacer);
        Playlist playlist(profile);
        // Two cuts of the same chain on black frame 24; stale look-ahead would return frame 25.
        playlist.append(chain, 24, 24);
        playlist.append(chain, 24, 24);
        for (int position = 0; position < 2; position++) {
            playlist.seek(position);
            QScopedPointer<Frame> frame(playlist.get_frame());
            frame->set("consumer.progressive", 1);
            frame->set("consumer.deinterlacer", "yadif");
            mlt_image_format format = mlt_image_yuv422;
            int width = 32;
            int height = 16;
            uint8_t *image = frame->get_image(format, width, height);
            QVERIFY(image != nullptr);
            QCOMPARE(image[0], uint8_t(16));
        }
    }
};

QTEST_APPLESS_MAIN(TestModAvformat)

#include "test_mod_avformat.moc"
