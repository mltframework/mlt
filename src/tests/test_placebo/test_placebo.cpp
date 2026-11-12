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
#include <QtTest>

#include <cstring>
#include <memory>
#include <vector>

using namespace Mlt;

class TestPlacebo : public QObject
{
    Q_OBJECT
    Profile profile;
    std::unique_ptr<Filter> renderer;

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("MLT_PLACEBO_IDLE_MS", "200"); // short, so the idle thread acts quickly
        Factory::init();
        profile.set_width(8);
        profile.set_height(8);
        profile.set_progressive(1);
        profile.set_colorspace(709);
        profile.set_sample_aspect(1, 1);
        profile.set_display_aspect(1, 1);
        profile.set_explicit(1);
        renderer = std::make_unique<Filter>(profile, "placebo.render");
        QVERIFY(renderer->is_valid());
    }

    void CpuRoundTrip_data()
    {
        QTest::addColumn<int>("imageFormat");
        QTest::addColumn<int>("transfer");
        for (auto format : {mlt_image_rgba, mlt_image_rgba64}) {
            for (auto transfer :
                 {mlt_color_trc_bt709, mlt_color_trc_linear, mlt_color_trc_smpte2084})
                QTest::newRow(qPrintable(QString("%1-%2").arg(format).arg(transfer)))
                    << int(format) << int(transfer);
        }
    }

    void CpuRoundTrip()
    {
        QFETCH(int, imageFormat);
        QFETCH(int, transfer);
        auto format = static_cast<mlt_image_format>(imageFormat);
        const int size = mlt_image_format_size(format, 8, 8, nullptr);
        auto *pixels = static_cast<uint8_t *>(mlt_pool_alloc(size));
        QVERIFY(pixels);
        QByteArray expected(size, '\0');
        for (int i = 0; i < 64; i++) {
            if (format == mlt_image_rgba64) {
                const uint16_t rgba[] = {12345, 23456, 34567, 65535};
                memcpy(expected.data() + 8 * i, rgba, sizeof(rgba));
            } else {
                const uint8_t rgba[] = {48, 96, 144, 255};
                memcpy(expected.data() + 4 * i, rgba, sizeof(rgba));
            }
        }
        memcpy(pixels, expected.constData(), size);
        mlt_frame raw = mlt_frame_init(nullptr);
        if (!raw) {
            mlt_pool_release(pixels);
            QFAIL("Failed to allocate frame");
        }
        Frame frame(raw);
        mlt_frame_close(raw);
        if (mlt_frame_set_image(frame.get_frame(), pixels, size, mlt_pool_release)) {
            mlt_pool_release(pixels);
            QFAIL("Failed to install CPU image");
        }
        frame.set("format", imageFormat);
        frame.set("width", 8);
        frame.set("height", 8);
        frame.set("colorspace", 709);
        frame.set("color_trc", transfer);
        renderer->process(frame);
        int width = 8;
        int height = 8;
        uint8_t *output = frame.get_image(format, width, height);
        QVERIFY(output);
        QCOMPARE(int(format), imageFormat);
        QCOMPARE(frame.get_int("test_image"), 0);
        QCOMPARE(width, 8);
        QCOMPARE(height, 8);
        QCOMPARE(mlt_image_color_trc_id(frame.get("color_trc")), transfer);
        for (int i = 0; i < size; i += format == mlt_image_rgba64 ? 2 : 1) {
            if (format == mlt_image_rgba64) {
                uint16_t actual, wanted;
                memcpy(&actual, output + i, 2);
                memcpy(&wanted, expected.constData() + i, 2);
                QVERIFY2(qAbs(int(actual) - int(wanted)) <= 4,
                         qPrintable(
                             QString("offset %1: %2 expected %3").arg(i).arg(actual).arg(wanted)));
            } else {
                QVERIFY(qAbs(int(output[i]) - int(static_cast<uint8_t>(expected[i]))) <= 1);
            }
        }
    }

    void FrameSequence_data()
    {
        QTest::addColumn<int>("imageFormat");
        QTest::addColumn<bool>("overlap");
        for (bool overlap : {false, true}) {
            QTest::newRow(overlap ? "private-overlapping" : "private-sequential")
                << int(mlt_image_private) << overlap;
            QTest::newRow(overlap ? "vulkan-overlapping" : "vulkan-sequential")
                << int(mlt_image_vulkan) << overlap;
        }
    }

    // Frames of different colors keep their own content whether or not their
    // textures were recycled from earlier frames or are alive at the same time.
    void FrameSequence()
    {
        QFETCH(int, imageFormat);
        QFETCH(bool, overlap);
        if (imageFormat == mlt_image_vulkan
            && !mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
            QSKIP("The published placebo backend is not Vulkan");

        const char *colors[] = {"#ff0000", "#00ff00", "#0000ff"};
        std::vector<std::unique_ptr<Frame>> frames;
        for (int i = 0; i < 9; i++) {
            if (!overlap)
                frames.clear(); // closing a frame returns its texture for the next one
            std::unique_ptr<Frame> frame
                = renderedColorFrame(colors[i % 3], 8, 8, mlt_image_format(imageFormat));
            QVERIFY(frame);
            if (!overlap)
                QCOMPARE(centerChannel(*frame, 8, 8), i % 3);
            frames.push_back(std::move(frame));
            if (overlap && frames.size() == 3) {
                for (int j = 0; j < 3; j++)
                    QCOMPARE(centerChannel(*frames[j], 8, 8), j);
                frames.clear();
            }
        }
    }

    // More distinct sizes than the renderer cache holds, twice, so entries are
    // evicted and then created again.
    void RendererCacheEviction()
    {
        for (int pass = 0; pass < 2; pass++) {
            for (int i = 0; i < 24; i++) {
                int width = 16 + 2 * i;
                int height = 8 + i;
                std::unique_ptr<Frame> frame
                    = renderedColorFrame("#00ff00", width, height, mlt_image_private);
                QVERIFY(frame);
                QCOMPARE(centerChannel(*frame, width, height), 1);
            }
        }
    }

    // Textures and renderers idle for longer than the limit are released by the
    // idle thread, so the next frame has to create them again.
    void RecreateAfterIdle()
    {
        mlt_properties global = mlt_global_properties();
        int before = mlt_properties_get_int(global, "placebo.trimmed");

        for (int pass = 0; pass < 2; pass++) {
            std::unique_ptr<Frame> frame = renderedColorFrame("#0000ff", 8, 8, mlt_image_private);
            QVERIFY(frame);
            QCOMPARE(centerChannel(*frame, 8, 8), 2);
            frame.reset();
            if (pass == 0)
                QTRY_VERIFY_WITH_TIMEOUT(mlt_properties_get_int(global, "placebo.trimmed") > before,
                                         5000);
        }
    }

    void ScaleIdentity_data()
    {
        QTest::addColumn<int>("outWidth");
        QTest::addColumn<int>("outHeight");
        QTest::addColumn<double>("aspect");
        QTest::addColumn<bool>("crop");
        QTest::addColumn<int>("inputFormat");
        QTest::addColumn<int>("requestedFormat");
        QTest::addColumn<bool>("passThrough");
        QTest::newRow("identity") << 8 << 8 << 1.0 << false << int(mlt_image_vulkan)
                                  << int(mlt_image_vulkan) << true;
        QTest::newRow("private-identity")
            << 8 << 8 << 1.0 << false << int(mlt_image_private) << int(mlt_image_private) << true;
        QTest::newRow("resize") << 4 << 4 << 1.0 << false << int(mlt_image_vulkan)
                                << int(mlt_image_vulkan) << false;
        QTest::newRow("letterbox")
            << 8 << 8 << 2.0 << false << int(mlt_image_vulkan) << int(mlt_image_vulkan) << false;
        QTest::newRow("crop") << 8 << 8 << 1.0 << true << int(mlt_image_vulkan)
                              << int(mlt_image_vulkan) << false;
        QTest::newRow("cpu-format")
            << 8 << 8 << 1.0 << false << int(mlt_image_vulkan) << int(mlt_image_rgba) << false;
    }

    void ScaleIdentity()
    {
        QFETCH(int, outWidth);
        QFETCH(int, outHeight);
        QFETCH(double, aspect);
        QFETCH(bool, crop);
        QFETCH(int, inputFormat);
        QFETCH(int, requestedFormat);
        QFETCH(bool, passThrough);
        if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
            QSKIP("The published placebo backend is not Vulkan");

        Filter scale(profile, "placebo.scale");
        QVERIFY(scale.is_valid());
        Producer producer(profile, "color", "#ff0000");
        QVERIFY(producer.is_valid());
        std::unique_ptr<Frame> frame(producer.get_frame());
        QVERIFY(frame && frame->is_valid());
        frame->set("color_trc", "bt709");
        renderer->process(*frame);
        mlt_image_format format = static_cast<mlt_image_format>(inputFormat);
        int width = 8;
        int height = 8;
        uint8_t *input = frame->get_image(format, width, height);
        QVERIFY(input);
        QCOMPARE(int(format), inputFormat);
        if (crop)
            reinterpret_cast<mlt_vulkan_image *>(input)->crop_left = 1;
        frame->set("meta.media.width", 8);
        frame->set("meta.media.height", 8);
        frame->set("consumer.rescale", "bilinear");
        mlt_frame_set_aspect_ratio(frame->get_frame(), aspect);
        scale.process(*frame);

        format = static_cast<mlt_image_format>(requestedFormat);
        width = outWidth;
        height = outHeight;
        uint8_t *output = frame->get_image(format, width, height);
        QVERIFY(output);
        QCOMPARE(int(format), requestedFormat);
        QCOMPARE(width, outWidth);
        QCOMPARE(height, outHeight);
        QCOMPARE(output == input, passThrough);
        if (aspect != 1.0) {
            QCOMPARE(mlt_frame_get_aspect_ratio(frame->get_frame()), 1.0);
            renderer->process(*frame);
            format = mlt_image_rgba;
            output = frame->get_image(format, width, height);
            QVERIFY(output);
            QCOMPARE(format, mlt_image_rgba);
            QVERIFY(output[0] < 4 && output[1] < 4 && output[2] < 4);
            int center = 4 * (4 * width + 4);
            QVERIFY(output[center] > 200 && output[center + 1] < 20 && output[center + 2] < 20);
        }
    }

    void VulkanSampleDepth_data()
    {
        QTest::addColumn<int>("depth");
        for (int depth : {8, 10, 12, 16})
            QTest::newRow(qPrintable(QString::number(depth))) << depth;
    }

    void VulkanSampleDepth()
    {
        QFETCH(int, depth);
        if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
            QSKIP("The published placebo backend is not Vulkan");
        Filter scale(profile, "avfilter.scale_vulkan");
        if (!scale.is_valid())
            QSKIP("FFmpeg's Vulkan scaler is unavailable");
        scale.set("av.format", "rgba64le");
        scale.set("av.w", 8);
        scale.set("av.h", 8);
        scale.set("av.scaler", "nearest");

        int reference = 0;
        for (bool adjustDepth : {false, true}) {
            Producer producer(profile, "color", "#7f7f7f");
            QVERIFY(producer.is_valid());
            std::unique_ptr<Frame> frame(producer.get_frame());
            QVERIFY(frame && frame->is_valid());
            frame->set("color_trc", "bt709");
            renderer->process(*frame);
            scale.process(*frame);
            mlt_image_format format = mlt_image_vulkan;
            int width = 8;
            int height = 8;
            uint8_t *bytes = frame->get_image(format, width, height);
            QVERIFY(bytes);
            QCOMPARE(format, mlt_image_vulkan);
            auto *image = reinterpret_cast<mlt_vulkan_image *>(bytes);
            QCOMPARE(image->bit_depth, 16);
            image->alpha = 0;
            if (adjustDepth) {
                image->bit_depth = depth;
                image->bit_shift = 16 - depth;
            }
            renderer->process(*frame);
            format = mlt_image_rgba64;
            bytes = frame->get_image(format, width, height);
            QVERIFY(bytes);
            QCOMPARE(format, mlt_image_rgba64);
            QCOMPARE(frame->get_int("test_image"), 0);
            uint16_t rgba[4];
            memcpy(rgba, bytes + 8 * (3 * width + 3), sizeof(rgba));
            if (!adjustDepth) {
                reference = rgba[0];
                QVERIFY(reference > 30000 && reference < 35000);
                continue;
            }
            const int expected = qRound(double(reference) * 65535
                                        / (((1 << depth) - 1) * (1 << (16 - depth))));
            for (int component = 0; component < 3; component++)
                QVERIFY2(qAbs(int(rgba[component]) - expected) <= 4,
                         qPrintable(QString("depth %1 component %2: %3 expected %4")
                                        .arg(depth)
                                        .arg(component)
                                        .arg(rgba[component])
                                        .arg(expected)));
        }
    }

private:
    // A color frame run through the render filter and requested in the format.
    std::unique_ptr<Frame> renderedColorFrame(const char *color,
                                              int width,
                                              int height,
                                              mlt_image_format format)
    {
        Producer producer(profile, "color", color);
        std::unique_ptr<Frame> frame(producer.get_frame());
        if (!frame || !frame->is_valid())
            return nullptr;
        frame->set("color_trc", "bt709");
        renderer->process(*frame);
        if (!frame->get_image(format, width, height))
            return nullptr;
        return frame;
    }

    // The channel (0 red, 1 green, 2 blue) that is the only one lit at the center
    // of the frame, or -1.
    int centerChannel(Frame &frame, int width, int height)
    {
        renderer->process(frame);
        mlt_image_format format = mlt_image_rgba;
        const uint8_t *image = frame.get_image(format, width, height);
        if (!image || format != mlt_image_rgba)
            return -1;
        const uint8_t *center = image + 4 * (height / 2 * width + width / 2);
        int lit = -1;
        for (int i = 0; i < 3; i++) {
            if (center[i] > 200 && lit < 0)
                lit = i;
            else if (center[i] >= 40)
                return -1;
        }
        return lit;
    }
};

QTEST_APPLESS_MAIN(TestPlacebo)

#include "test_placebo.moc"
