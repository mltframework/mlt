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

#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

#include <atomic>
#include <cstdio>

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
using namespace Mlt;

static void countEvent(mlt_properties, void *data, mlt_event_data)
{
    auto calls = static_cast<std::atomic<int> *>(data);
    ++*calls;
}

static void onRunningChanged(mlt_properties owner, void *data, mlt_event_data eventData)
{
    if (qstrcmp(mlt_event_data_to_string(eventData), "running") == 0
        && mlt_properties_get_int(owner, "running"))
        countEvent(owner, data, eventData);
}

class TestModAvformat : public QObject
{
    Q_OBJECT

public:
    TestModAvformat() { Factory::init(); }

    ~TestModAvformat() {}

private:
    // A deadlocked export cannot be safely stopped as a thread. Run it in a child
    // process so the parent can terminate it on timeout without hanging the suite.
    // The environment variable identifies the child and provides its temporary
    // directory, preventing it from recursively starting another subprocess.
    void runExportInSubprocess()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QProcess child;
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert("MLT_TEST_EXPORT_COMPLETION_CHILD", directory.path());
        child.setProcessEnvironment(environment);
        child.start(QCoreApplication::applicationFilePath(),
                    {QString::fromLatin1(QTest::currentTestFunction()),
                     QStringLiteral("-o"),
                     directory.filePath("result.txt") + QStringLiteral(",txt")});
        QVERIFY2(child.waitForStarted(), qPrintable(child.errorString()));
        bool finished = child.waitForFinished(15000);
        if (!finished) {
            child.kill();
            child.waitForFinished();
        }
        QByteArray output = child.readAllStandardOutput() + child.readAllStandardError();
        QFile result(directory.filePath("result.txt"));
        if (result.open(QIODevice::ReadOnly))
            output += result.readAll();
        if (!finished || child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0)
            fprintf(stderr, "Export test did not complete successfully:\n%s", output.constData());
        QVERIFY2(finished, "Export test did not finish within 15 seconds");
        QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                 output.constData());
    }

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

    // Exercise serialized start/stop cycles with each framework rendering mode.
    void RepeatedStartStop_data()
    {
        QTest::addColumn<int>("realTime");
        QTest::newRow("synchronous") << 0;
        QTest::newRow("read-ahead") << -1;
        QTest::newRow("parallel-render") << -2;
    }

    // Each stop must join the worker and leave the consumer ready for another start.
    void RepeatedStartStop()
    {
        QFETCH(int, realTime);
        Profile profile;
        profile.set_width(32);
        profile.set_height(16);
        Producer producer(profile, "color", "black");
        Consumer consumer(profile, "avformat", "unused");
        QVERIFY(producer.is_valid());
        QVERIFY(consumer.is_valid());
        producer.set("eof", "loop");
        consumer.set("f", "null");
        consumer.set("vcodec", "rawvideo");
        consumer.set("an", 1);
        consumer.set("real_time", realTime);
        consumer.connect(producer);

        for (int i = 0; i < 3; ++i) {
            QCOMPARE(consumer.start(), 0);
            QCOMPARE(consumer.stop(), 0);
            QVERIFY(consumer.is_stopped());
        }
    }

    // Startup failure must deliver a fatal event before start returns an error,
    // and the same consumer must be reusable after stop and configuration correction.
    void StartupFailureAndRecovery()
    {
        Profile profile;
        Consumer consumer(profile, "avformat", "unused");
        QVERIFY(consumer.is_valid());
        consumer.set("f", "null");
        consumer.set("an", 1);
        consumer.set("vn", 1);
        consumer.set("real_time", -1);
        std::atomic<int> calls{0};
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer.get_consumer());
        mlt_event event = mlt_events_listen(properties, &calls, "consumer-fatal-error", countEvent);
        QVERIFY(event != nullptr);
        QVERIFY(consumer.start() != 0);
        QCOMPARE(calls.load(), 1);
        QVERIFY(consumer.is_valid());
        QVERIFY(consumer.is_stopped());
        mlt_events_disconnect(properties, &calls);
        QCOMPARE(consumer.stop(), 0);

        consumer.set("vn", 0);
        consumer.set("vcodec", "rawvideo");
        Producer producer(profile, "color", "black");
        QVERIFY(producer.is_valid());
        consumer.connect(producer);
        QCOMPARE(consumer.start(), 0);
        QCOMPARE(consumer.stop(), 0);
    }

    // Failed startup must return without deadlocking and retain both notifications.
    void ExportMissingDirectoryWithFatalAndCompletionWait()
    {
        if (!qEnvironmentVariableIsSet("MLT_TEST_EXPORT_COMPLETION_CHILD")) {
            runExportInSubprocess();
            return;
        }

        QDir directory(qEnvironmentVariable("MLT_TEST_EXPORT_COMPLETION_CHILD"));
        Profile profile;
        profile.set_width(32);
        profile.set_height(16);
        Producer producer(profile, "color", "black");
        Consumer consumer(profile,
                          "avformat",
                          directory.filePath("missing/export.nut").toUtf8().constData());
        QVERIFY(producer.is_valid());
        QVERIFY(consumer.is_valid());
        producer.set_in_and_out(0, 4);
        consumer.set("f", "nut");
        consumer.set("vcodec", "rawvideo");
        consumer.set("an", 1);
        consumer.set("real_time", 0);
        consumer.connect(producer);

        mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer.get_consumer());
        mlt_event fatal = mlt_events_setup_wait_for(properties, "consumer-fatal-error");
        mlt_event stopped = mlt_events_setup_wait_for(properties, "consumer-stopped");
        QVERIFY(fatal != nullptr);
        QVERIFY(stopped != nullptr);
        QVERIFY(consumer.start() != 0);
        mlt_events_wait_for(properties, fatal);
        mlt_events_wait_for(properties, stopped);
        QCOMPARE(consumer.stop(), 0);
        mlt_events_close_wait_for(properties, fatal);
        mlt_events_close_wait_for(properties, stopped);
    }

    // A format-list request returns without creating a worker, but must still
    // restore the framework property listener before the consumer is reused.
    void FormatListRestoresPropertyListener()
    {
        Profile profile;
        Consumer consumer(profile, "avformat", "unused");
        QVERIFY(consumer.is_valid());
        consumer.set("real_time", 0);
        consumer.set("f", "list");
        QVERIFY(consumer.start() != 0);
        QCOMPARE(consumer.stop(), 0);
        consumer.set("width", 32);
        QCOMPARE(mlt_service_profile(MLT_CONSUMER_SERVICE(consumer.get_consumer()))->width, 32);
    }

    // A listener must observe the worker setting "running" before start returns,
    // and the consumer must remain usable for a subsequent stop.
    void RunningPropertyIsReportedBeforeStartReturns()
    {
        Profile profile;
        profile.set_width(32);
        profile.set_height(16);
        Producer producer(profile, "color", "black");
        Consumer consumer(profile, "avformat", "unused");
        QVERIFY(producer.is_valid());
        QVERIFY(consumer.is_valid());
        producer.set("eof", "loop");
        consumer.set("f", "null");
        consumer.set("vcodec", "rawvideo");
        consumer.set("an", 1);
        consumer.set("real_time", 0);
        consumer.connect(producer);

        std::atomic<int> calls{0};
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer.get_consumer());
        QVERIFY(mlt_events_listen(properties, &calls, "property-changed", onRunningChanged));
        QCOMPARE(consumer.start(), 0);
        QCOMPARE(calls.load(), 1);
        QVERIFY(consumer.is_valid());
        QCOMPARE(consumer.stop(), 0);
        mlt_events_disconnect(properties, &calls);
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

    // Sample the center pixel after a decode. Limited-range red and blue survive
    // the round trip well enough to tell the two source frames apart.
    static int centerColor(Producer &producer, int position)
    {
        producer.seek(position);
        QScopedPointer<Frame> frame(producer.get_frame());
        if (!frame)
            return -1;
        mlt_image_format format = mlt_image_rgb;
        int width = 0;
        int height = 0;
        uint8_t *image = frame->get_image(format, width, height);
        if (!image || width < 1 || height < 1)
            return -1;
        const uint8_t *pixel = image + ((height / 2) * width + width / 2) * 3;
        if (pixel[0] > 150 && pixel[1] < 80 && pixel[2] < 80)
            return 0;
        if (pixel[2] > 150 && pixel[0] < 80 && pixel[1] < 80)
            return 1;
        return -1;
    }

    static void setSquare(Profile &profile)
    {
        profile.set_width(32);
        profile.set_height(16);
        profile.set_progressive(1);
        profile.set_sample_aspect(1, 1);
        profile.set_display_aspect(2, 1);
        profile.set_colorspace(709);
        profile.set_explicit(1);
    }

    static bool encodeTwoFrames(Profile &profile, const QByteArray &path)
    {
        Producer red(profile, "color", "red");
        Producer blue(profile, "color", "blue");
        if (!red.is_valid() || !blue.is_valid())
            return false;
        Playlist playlist(profile);
        playlist.append(red, 0, 0);
        playlist.append(blue, 0, 0);
        Consumer encoder(profile, "avformat", path.constData());
        if (!encoder.is_valid())
            return false;
        // MPEG-2 cannot store a rate below 1 fps. libx264 can, and it is what
        // the slow-clip repro uses.
        encoder.set("vcodec", "libx264");
        encoder.set("pix_fmt", "yuv420p");
        encoder.set("preset", "ultrafast");
        encoder.set("an", 1);
        encoder.set("progressive", 1);
        encoder.set("g", 1);
        encoder.set("real_time", 0);
        encoder.set("terminate_on_pause", 1);
        encoder.connect(playlist);
        encoder.run();
        const QFileInfo info(QString::fromUtf8(path));
        return info.exists() && info.size() > 0;
    }

    void SlowFrameHoldAndFastRounding()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // Two source frames at 1/2 fps: red, then blue. Each source frame is
        // two seconds, so a 25 fps project holds red through frame 49.
        Profile slow;
        setSquare(slow);
        slow.set_frame_rate(1, 2);
        const QByteArray slowPath = dir.filePath("slow.mp4").toUtf8();
        if (!encodeTwoFrames(slow, slowPath))
            QSKIP("libx264 did not produce a file");

        Profile project;
        setSquare(project);
        project.set_frame_rate(25, 1);
        Producer slowClip(project, "avformat", slowPath.constData());
        QVERIFY(slowClip.is_valid());
        const double probedRate = slowClip.get_double("meta.media.0.stream.frame_rate");
        QVERIFY(probedRate > 0.0 && probedRate < 1.0);
        QVERIFY(slowClip.get_length() >= 100);
        QCOMPARE(centerColor(slowClip, 0), 0);
        // frame_rate_num is stored when the decoder opens, during the first image.
        const double slowRate = slowClip.get_double("meta.media.frame_rate_num")
                                / slowClip.get_double("meta.media.frame_rate_den");
        QVERIFY(slowRate > 0.0 && slowRate < 1.0);
        QCOMPARE(centerColor(slowClip, 49), 0);
        QCOMPARE(centerColor(slowClip, 50), 1);
        QCOMPARE(centerColor(slowClip, 99), 1);

        // 24 fps in a 30 fps project. Nearest-frame rounding shows the second
        // source frame at project frame 1. Holding for the full source frame
        // would still show the first.
        Profile film;
        setSquare(film);
        film.set_frame_rate(24, 1);
        const QByteArray filmPath = dir.filePath("film.mp4").toUtf8();
        if (!encodeTwoFrames(film, filmPath))
            QSKIP("libx264 did not produce a file");
        Profile video;
        setSquare(video);
        video.set_frame_rate(30, 1);
        Producer filmClip(video, "avformat", filmPath.constData());
        QVERIFY(filmClip.is_valid());
        QVERIFY(filmClip.get_double("meta.media.0.stream.frame_rate") >= 1.0);
        QVERIFY(filmClip.get_length() >= 2);
        QCOMPARE(centerColor(filmClip, 0), 0);
        const double filmRate = filmClip.get_double("meta.media.frame_rate_num")
                                / filmClip.get_double("meta.media.frame_rate_den");
        QVERIFY(filmRate >= 1.0);
        QCOMPARE(centerColor(filmClip, 1), 1);
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

QTEST_GUILESS_MAIN(TestModAvformat)

#include "test_mod_avformat.moc"
