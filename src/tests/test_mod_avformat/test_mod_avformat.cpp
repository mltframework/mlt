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

#include <atomic>
#include <cstdio>
#include <cstring>
#include <framework/mlt.h>
#include <mlt++/Mlt.h>
using namespace Mlt;

namespace {
std::atomic<void *> loggedProducer;
std::atomic<void *> loggedReference;
std::atomic<int> referenceHardwareFrames;
std::atomic<int> vulkanFrames;
std::atomic<int> vulkanFilters;

void logCallback(void *service, int level, const char *format, va_list args)
{
    if (service == loggedReference.load() && !strcmp(format, "Decoded hardware frame: %s\n"))
        ++referenceHardwareFrames;
    if (service == loggedProducer.load()) {
        va_list copy;
        va_copy(copy, args);
        if (!strcmp(format, "Decoded hardware frame: %s\n")
            && !strcmp(va_arg(copy, const char *), "vulkan"))
            ++vulkanFrames;
        else if (!strcmp(format, "Applying video filters to %s frame\n")
                 && !strcmp(va_arg(copy, const char *), "vulkan"))
            ++vulkanFilters;
        va_end(copy);
    }
    if (level <= mlt_log_get_level())
        vfprintf(stderr, format, args);
}
} // namespace

class TestModAvformat : public QObject
{
    Q_OBJECT

public:
    TestModAvformat()
    {
        Factory::init();
        mlt_log_set_callback(logCallback);
    }

    ~TestModAvformat() { mlt_log_set_callback(nullptr); }

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

    void VideoFilterOutput_data()
    {
        QTest::addColumn<QString>("codec");
        QTest::addColumn<int>("imageFormat");
        QTest::addColumn<bool>("vulkan");
        QTest::addColumn<bool>("invalidGraph");
        QTest::addColumn<int>("angle");
        for (bool vulkan : {false, true}) {
            const QByteArray backend = vulkan ? "vulkan" : "software";
            for (int angle : {90, 180, 270}) {
                const QByteArray name = backend + "-" + QByteArray::number(angle);
                QTest::newRow(("8bit-" + name).constData())
                    << QString("libx264") << int(mlt_image_yuv420p) << vulkan << false << angle;
                QTest::newRow(("10bit-" + name).constData())
                    << QString("libx265") << int(mlt_image_yuv420p10) << vulkan << false << angle;
            }
            QTest::newRow(("invalid-graph-" + backend).constData())
                << QString("libx264") << int(mlt_image_yuv420p) << vulkan << true << 180;
        }
    }

    void VideoFilterOutput()
    {
        QFETCH(QString, codec);
        QFETCH(int, imageFormat);
        QFETCH(bool, vulkan);
        QFETCH(bool, invalidGraph);
        QFETCH(int, angle);
        const QString ffmpeg = QStandardPaths::findExecutable("ffmpeg");
        if (ffmpeg.isEmpty())
            QSKIP("ffmpeg is required to generate the rotation test clip");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("rotation.mp4");
        QProcess encoder;
        encoder
            .start(ffmpeg,
                   {"-v",
                    "error",
                    "-f",
                    "lavfi",
                    "-i",
                    "nullsrc=size=128x128:rate=25,"
                    "geq=lum='if(lt(X,64),if(lt(Y,64),32,96),if(lt(Y,64),160,224))':cb=128:cr=128",
                    "-frames:v",
                    "3",
                    "-c:v",
                    codec,
                    "-pix_fmt",
                    imageFormat == mlt_image_yuv420p10 ? "yuv420p10le" : "yuv420p",
                    "-threads",
                    "1",
                    path});
        QVERIFY(encoder.waitForFinished(30000));
        const QByteArray encoderError = encoder.readAllStandardError();
        if (encoderError.contains("Unknown encoder"))
            QSKIP(encoderError.constData());
        QCOMPARE(encoder.exitStatus(), QProcess::NormalExit);
        QVERIFY2(encoder.exitCode() == 0, encoderError.constData());

        const char *environmentKeys[] = {"MLT_AVFORMAT_HWACCEL",
                                         "MLT_AVFORMAT_HWACCEL_DEVICE",
                                         "MLT_AVFORMAT_HWACCEL_PPS"};
        QList<QByteArray> environmentValues;
        for (const char *key : environmentKeys)
            environmentValues.append(qgetenv(key));
        const auto restoreEnvironment = qScopeGuard([&] {
            for (int i = 0; i < 3; i++) {
                if (environmentValues[i].isNull())
                    qunsetenv(environmentKeys[i]);
                else
                    qputenv(environmentKeys[i], environmentValues[i]);
            }
        });
        for (const char *key : environmentKeys)
            QVERIFY(qunsetenv(key));

        Profile profile;
        const QByteArray resource
            = ("mov:" + path + (vulkan ? "?hwaccel=vulkan" : "?hwaccel=none")).toUtf8();
        Producer producer(profile, "avformat", resource.constData());
        QVERIFY(producer.is_valid());
        producer.set("rotate", angle);
        producer.set("noimagecache", 1);
        Producer reference(profile,
                           "avformat",
                           ("mov:" + path + "?hwaccel=none").toUtf8().constData());
        QVERIFY(reference.is_valid());
        reference.set("rotate", angle);
        reference.set("noimagecache", 1);
        vulkanFrames = 0;
        vulkanFilters = 0;
        referenceHardwareFrames = 0;
        loggedProducer = MLT_PRODUCER_SERVICE(producer.get_producer());
        loggedReference = MLT_PRODUCER_SERVICE(reference.get_producer());
        const auto resetLog = qScopeGuard([] {
            loggedProducer = nullptr;
            loggedReference = nullptr;
        });
        if (invalidGraph)
            producer.set("filtergraph", "mlt_nonexistent_filter");
        for (int position = 0; position < 3; position++) {
            producer.seek(position);
            QScopedPointer<Frame> frame(producer.get_frame());
            QVERIFY(frame);
            mlt_image_format format = mlt_image_none;
            int width = 128, height = 128;
            uint8_t *image = nullptr;
            // Test the producer's error, before mlt_frame_get_image substitutes a test image.
            mlt_get_image getImage = mlt_frame_pop_get_image(frame->get_frame());
            QVERIFY(getImage);
            const int error = getImage(frame->get_frame(), &image, &format, &width, &height, 0);
            if (invalidGraph) {
                QVERIFY(error != 0);
            } else {
                QCOMPARE(error, 0);
                QVERIFY(image);
                QCOMPARE(int(format), imageFormat);
                reference.seek(position);
                QScopedPointer<Frame> expected(reference.get_frame());
                mlt_image_format expectedFormat = mlt_image_none;
                int expectedWidth = 128, expectedHeight = 128;
                const uint8_t *expectedImage = expected->get_image(expectedFormat,
                                                                   expectedWidth,
                                                                   expectedHeight);
                QCOMPARE(referenceHardwareFrames.load(), 0);
                QVERIFY(expectedImage);
                QCOMPARE(format, expectedFormat);
                QCOMPARE(width, expectedWidth);
                QCOMPARE(height, expectedHeight);
                qint64 difference = 0;
                for (int y : {32, 96}) {
                    for (int x : {32, 96}) {
                        const int i = y * width + x;
                        if (format == mlt_image_yuv420p10) {
                            difference += qAbs(
                                int(reinterpret_cast<const uint16_t *>(image)[i])
                                - int(reinterpret_cast<const uint16_t *>(expectedImage)[i]));
                        } else {
                            difference += qAbs(int(image[i]) - int(expectedImage[i]));
                        }
                    }
                }
                QVERIFY2(difference <= (format == mlt_image_yuv420p10 ? 32 : 8),
                         "Rotated quadrant luma differs from software reference");
            }
        }
        if (vulkan && !invalidGraph) {
            if (!vulkanFrames)
                QSKIP("Vulkan hardware decoding is unavailable");
            QCOMPARE(vulkanFilters.load(), 3);
        }
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
        // After a gap, continue the source at the flash frame 25: timeline jumps, source does not.
        playlist.blank(4);
        playlist.append(chain, 25, 26);
        for (int position : {0, 1, 7, 8}) {
            playlist.seek(position);
            QScopedPointer<Frame> frame(playlist.get_frame());
            frame->set("consumer.progressive", 1);
            frame->set("consumer.deinterlacer", "yadif");
            mlt_image_format format = mlt_image_yuv422;
            int width = 32;
            int height = 16;
            uint8_t *image = frame->get_image(format, width, height);
            QVERIFY(image != nullptr);
            QCOMPARE(image[0] > 128, position == 7);
        }
    }
};

QTEST_APPLESS_MAIN(TestModAvformat)

#include "test_mod_avformat.moc"
