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

#include "test_placebo_deinterlace.h"

using namespace Mlt;
using namespace PlaceboDeinterlaceTest;

void TestPlaceboDeinterlace::BypassAndWeave()
{
    const QByteArray original = pixelsFor(40, 200, 128);
    for (const char *method : {"none", "weave", "bob"}) {
        auto filter = newFilter();
        auto frame = imageFrame(original, 3);
        QVERIFY(filter && frame);
        mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
        requestDeinterlace(props, method);
        if (!strcmp(method, "bob"))
            mlt_properties_set_int(props, "progressive", 1);
        mlt_filter_process(filter.get(), frame.get());
        Result result = readImage(frame.get());
        QCOMPARE(result.error, 0);
        QCOMPARE(result.testImage, 0);
        QCOMPARE(result.format, mlt_image_rgba);
        QCOMPARE(result.pixels, original);
        QCOMPARE(result.progressive, strcmp(method, "none") != 0 ? 1 : 0);
    }
    auto filter = newFilter();
    auto frame = imageFrame(original, 3);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
    mlt_properties_set_int(props, "consumer.progressive", 0);
    mlt_properties_set(props, "consumer.deinterlacer", "bob");
    mlt_filter_process(filter.get(), frame.get());
    Result result = readImage(frame.get());
    QCOMPARE(result.pixels, original);
    QCOMPARE(result.progressive, 0);

    frame = imageFrame(original, 3);
    props = MLT_FRAME_PROPERTIES(frame.get());
    mlt_properties_set_int(props, "consumer.progressive", 1);
    mlt_properties_set_int(props, "test_image", 1);
    mlt_properties_set(props, "consumer.deinterlacer", "weave");
    mlt_filter_process(filter.get(), frame.get());
    result = readImage(frame.get());
    QCOMPARE(result.pixels, original);
    QCOMPARE(result.progressive, 0);
    QCOMPARE(result.testImage, 1);
}

void TestPlaceboDeinterlace::BobFieldOrder()
{
    for (bool topFirst : {true, false}) {
        Result result = render("bob", 40, 200, topFirst);
        QCOMPARE(result.error, 0);
        QCOMPARE(result.testImage, 0);
        QCOMPARE(result.format, mlt_image_rgba);
        QCOMPARE(result.width, imageWidth);
        QCOMPARE(result.height, imageHeight);
        QCOMPARE(result.progressive, 1);
        QVERIFY(!result.pixels.isEmpty());
        for (int y = 0; y < imageHeight; y++)
            QVERIFY2(qAbs(redAt(result, 3, y) - (topFirst ? 40 : 200)) <= 1,
                     qPrintable(QString("row %1: %2").arg(y).arg(redAt(result, 3, y))));
    }
}

void TestPlaceboDeinterlace::LogsMethodChanges()
{
    auto filter = newFilter();
    QVERIFY(filter);
    for (const char *method : {"bob", "yadif", "weave", "none", "bob"}) {
        auto frame = imageFrame(pixelsFor(40, 200), 3);
        QVERIFY(frame);
        mlt_filter_process(filter.get(), frame.get());
        requestDeinterlace(frame.get(), method);
        Result result = readImage(frame.get());
        QCOMPARE(result.error, 0);
        QCOMPARE(result.progressive, strcmp(method, "none") ? 1 : 0);
    }
    std::lock_guard<std::mutex> lock(logMutex);
    QCOMPARE(loggedVerbose,
             QByteArray("Deinterlacing with GPU bob\n"
                        "Deinterlacing with GPU weave\n"
                        "Deinterlacing with GPU bob\n"));
}

void TestPlaceboDeinterlace::UnknownMethodChanges()
{
    auto filter = newFilter();
    QVERIFY(filter);
    for (const char *method : {"unknown-first", "unknown-first", "unknown-second", "bob"}) {
        auto frame = imageFrame(pixelsFor(40, 200), 3);
        QVERIFY(frame);
        requestDeinterlace(frame.get(), method);
        mlt_filter_process(filter.get(), frame.get());
        Result result = readImage(frame.get());
        QVERIFY_DEINTERLACED(result);
    }
    std::lock_guard<std::mutex> lock(logMutex);
    QCOMPARE(loggedWarnings,
             QByteArray("Deinterlacer unknown-first is unavailable; using GPU bob\n"
                        "Deinterlacer unknown-second is unavailable; using GPU bob\n"));
    QCOMPARE(loggedVerbose, QByteArray("Deinterlacing with GPU bob\n"));
}

void TestPlaceboDeinterlace::PreserveUpstreamConverters()
{
    for (const char *method : {"none", "bob"}) {
        auto source = imageFrame(pixelsFor(40, 200), 3);
        FramePtr frame(mlt_frame_init(nullptr), mlt_frame_close);
        auto filter = newFilter();
        QVERIFY(source && frame && filter);
        mlt_frame_append_convert_image(source.get(), upstreamConvert);
        QCOMPARE(mlt_frame_prepend_image_from_service(frame.get(), source.get()), 0);
        requestDeinterlace(frame.get(), method);
        mlt_filter_process(filter.get(), frame.get());
        Result result = readImage(frame.get());
        QCOMPARE(result.error, 0);
        QCOMPARE(result.format, mlt_image_rgba);
        upstreamConversions = 0;
        result = readImage(frame.get(), mlt_image_rgba64);
        QCOMPARE(result.error, 0);
        QCOMPARE(result.format, mlt_image_rgba64);
        QCOMPARE(upstreamConversions.load(), 1);
    }
}

void TestPlaceboDeinterlace::HighBitDepth()
{
    for (mlt_image_format input : {mlt_image_yuv420p10, mlt_image_yuv444p10, mlt_image_yuv422p16}) {
        FilterPtr avcolor(mlt_factory_filter(profile.get_profile(), "avcolor_space", nullptr),
                          mlt_filter_close);
        if (!avcolor)
            QSKIP("avcolor_space is unavailable");
        auto original = imageFrame(highDepthPixels(input), 3, false, true, input);
        QVERIFY(original);
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(original.get()), "full_range", 1);
        mlt_filter_process(avcolor.get(), original.get());
        Result expected = readImage(original.get(), mlt_image_rgba64);
        QCOMPARE(expected.error, 0);
        QCOMPARE(expected.format, mlt_image_rgba64);
        QVERIFY(!expected.pixels.isEmpty());
        mlt_properties_set(MLT_FRAME_PROPERTIES(original.get()), "consumer.color_range", "full");
        Result expectedYuv = readImage(original.get(), input);
        QCOMPARE(expectedYuv.format, input);
        QVERIFY(!expectedYuv.pixels.isEmpty());

        auto frame = imageFrame(highDepthPixels(input), 3, false, true, input);
        auto filter = newFilter();
        FilterPtr scale(mlt_factory_filter(profile.get_profile(), "placebo.scale", nullptr),
                        mlt_filter_close);
        QVERIFY(frame && filter && scale);
        mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
        mlt_properties_set_int(props, "full_range", 1);
        requestDeinterlace(props, "bob");
        mlt_properties_set(props, "consumer.rescale", "none");
        auto *alpha = static_cast<uint8_t *>(mlt_pool_alloc(imageWidth * imageHeight));
        QVERIFY(alpha);
        memset(alpha, 173, imageWidth * imageHeight);
        mlt_frame_set_alpha(frame.get(), alpha, imageWidth * imageHeight, mlt_pool_release);
        mlt_filter_process(avcolor.get(), frame.get());
        mlt_filter_process(filter.get(), frame.get());
        mlt_filter_process(scale.get(), frame.get());
        Result gpu = readImage(frame.get(), mlt_image_private);
        QCOMPARE(gpu.error, 0);
        QCOMPARE(gpu.testImage, 0);
        QCOMPARE(gpu.format, mlt_image_private);
        Result actual = readImage(frame.get(), mlt_image_rgba64);
        QCOMPARE(actual.error, 0);
        QCOMPARE(actual.format, mlt_image_rgba64);
        QVERIFY(!actual.pixels.isEmpty());
        QVERIFY2(qAbs(int(component16(actual, 3, 3, 0)) - int(component16(expected, 3, 2, 0))) <= 4,
                 qPrintable(QString("format %1: actual=%2 expected=%3")
                                .arg(input)
                                .arg(component16(actual, 3, 3, 0))
                                .arg(component16(expected, 3, 2, 0))));
        QCOMPARE(component16(actual, 3, 3, 3), uint16_t(173 << 8));

        frame = imageFrame(highDepthPixels(input), 3, false, true, input);
        props = MLT_FRAME_PROPERTIES(frame.get());
        mlt_properties_set_int(props, "full_range", 1);
        requestDeinterlace(props, "bob");
        mlt_properties_set(props, "consumer.color_range", "full");
        mlt_filter_process(avcolor.get(), frame.get());
        mlt_filter_process(filter.get(), frame.get());
        gpu = readImage(frame.get(), mlt_image_private);
        QCOMPARE(gpu.format, mlt_image_private);
        Result yuv = readImage(frame.get(), input);
        QCOMPARE(yuv.error, 0);
        QCOMPARE(yuv.testImage, 0);
        QCOMPARE(yuv.format, input);
        QVERIFY(!yuv.pixels.isEmpty());
        uint16_t luma = 0;
        uint16_t expectedLuma = 0;
        memcpy(&luma, yuv.pixels.constData() + 2 * (3 * imageWidth + 3), 2);
        memcpy(&expectedLuma, expectedYuv.pixels.constData() + 2 * (2 * imageWidth + 3), 2);
        QVERIFY(qAbs(int(expectedLuma) - (input == mlt_image_yuv422p16 ? 32000 : 493))
                <= (input == mlt_image_yuv422p16 ? 256 : 2));
        QVERIFY2(qAbs(int(luma) - int(expectedLuma)) <= 2,
                 qPrintable(QString("format %1: luma=%2 expected=%3")
                                .arg(input)
                                .arg(luma)
                                .arg(expectedLuma)));
    }
}

void TestPlaceboDeinterlace::PrivateOutputAndAlpha()
{
    auto filter = newFilter();
    auto frame = imageFrame(pixelsFor(40, 200, 128), 3);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
    requestDeinterlace(props, "bob");
    mlt_filter_process(filter.get(), frame.get());
    Result privateResult = readImage(frame.get(), mlt_image_private);
    QCOMPARE(privateResult.error, 0);
    QCOMPARE(privateResult.format, mlt_image_private);
    QCOMPARE(QByteArray(mlt_properties_get(props, "mlt_image_private")), QByteArray("placebo"));
    QCOMPARE(mlt_image_color_trc_id(mlt_properties_get(props, "color_trc")), mlt_color_trc_linear);
    Result pixels = readImage(frame.get());
    QCOMPARE(pixels.error, 0);
    QCOMPARE(pixels.format, mlt_image_rgba);
    QVERIFY(!pixels.pixels.isEmpty());
    QCOMPARE(static_cast<unsigned char>(pixels.pixels[3]), 128);
    QVERIFY(qAbs(redAt(pixels, 3, 3) - 40) <= 1);
}

void TestPlaceboDeinterlace::Rgba64AndTransfer()
{
    auto filter = newFilter();
    auto frame = imageFrame(rgba64Pixels(), 3, false, true, mlt_image_rgba64);
    QVERIFY(filter && frame);
    mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
    mlt_properties_set_int(props, "color_trc", mlt_color_trc_smpte2084);
    requestDeinterlace(props, "bob");
    mlt_filter_process(filter.get(), frame.get());
    Result result = readImage(frame.get(), mlt_image_rgba64);
    QCOMPARE(result.error, 0);
    QCOMPARE(result.testImage, 0);
    QCOMPARE(result.format, mlt_image_rgba64);
    QCOMPARE(result.progressive, 1);
    QCOMPARE(mlt_image_color_trc_id(mlt_properties_get(props, "color_trc")),
             mlt_color_trc_smpte2084);
    QVERIFY(!result.pixels.isEmpty());
    const uint16_t expected[] = {12345, 23456, 34567, 11111};
    for (int component = 0; component < 4; component++)
        QVERIFY(qAbs(int(component16(result, 3, 3, component)) - expected[component]) <= 1);
}

void TestPlaceboDeinterlace::VulkanHighDepthAndTransfer()
{
    if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
        QSKIP("The published placebo backend is not Vulkan");
    for (bool differentFrameTransfer : {false, true}) {
        auto filter = newFilter();
        auto frame = imageFrame(rgba64Pixels(), 3, false, true, mlt_image_rgba64);
        QVERIFY(filter && frame);
        mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
        mlt_properties_set_int(props, "color_trc", mlt_color_trc_bt709);
        requestDeinterlace(props, "bob");
        mlt_filter_process(filter.get(), frame.get());
        Result output = readImage(frame.get(), mlt_image_vulkan);
        QCOMPARE(output.format, mlt_image_vulkan);
        QCOMPARE(output.testImage, 0);
        auto *image = static_cast<mlt_vulkan_image *>(
            mlt_properties_get_data(props, "image", nullptr));
        QVERIFY(image);
        QCOMPARE(image->bit_depth, 16);
        mlt_properties_set_int(props, "progressive", 0);
        if (differentFrameTransfer)
            mlt_properties_set_int(props, "color_trc", mlt_color_trc_linear);
        mlt_filter_process(filter.get(), frame.get());
        output = readImage(frame.get(), mlt_image_vulkan);
        QCOMPARE(output.format, mlt_image_vulkan);
        QCOMPARE(output.testImage, 0);
        image = static_cast<mlt_vulkan_image *>(mlt_properties_get_data(props, "image", nullptr));
        QVERIFY(image);
        QCOMPARE(image->bit_depth, 16);
        QCOMPARE(image->color_trc, int(mlt_color_trc_bt709));
        mlt_properties_set_int(props, "progressive", 0);
        mlt_filter_process(filter.get(), frame.get());
        Result pixels = readImage(frame.get(), mlt_image_rgba64);
        QCOMPARE(pixels.format, mlt_image_rgba64);
        QCOMPARE(pixels.testImage, 0);
        QVERIFY(!pixels.pixels.isEmpty());
        const uint16_t expected[] = {12345, 23456, 34567, 11111};
        for (int component = 0; component < 4; component++)
            QVERIFY2(qAbs(int(component16(pixels, 3, 3, component)) - expected[component]) <= 4,
                     qPrintable(QString("component %1: %2")
                                    .arg(component)
                                    .arg(component16(pixels, 3, 3, component))));
    }
}

void TestPlaceboDeinterlace::ProgressiveVulkanOutput()
{
    if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
        QSKIP("The published placebo backend is not Vulkan");
    QByteArray pixels = rgba64Pixels();
    const uint16_t opaque = 65535;
    for (int i = 0; i < imageWidth * imageHeight; i++)
        memcpy(pixels.data() + 8 * i + 6, &opaque, sizeof(opaque));
    for (auto requested : {mlt_image_rgba, mlt_image_rgba64, mlt_image_private, mlt_image_vulkan}) {
        auto native = imageFrame(pixels, 3, true, true, mlt_image_rgba64);
        auto filter = newFilter();
        FramePtr frame(mlt_frame_init(nullptr), mlt_frame_close);
        QVERIFY(native && filter && frame);
        mlt_filter_process(publisher.get(), native.get());
        Result uploaded = readImage(native.get(), mlt_image_vulkan);
        QCOMPARE(uploaded.error, 0);
        QCOMPARE(uploaded.format, mlt_image_vulkan);
        QCOMPARE(mlt_frame_prepend_image_from_service(frame.get(), native.get()), 0);
        requestDeinterlace(frame.get(), "yadif");
        mlt_filter_process(filter.get(), frame.get());
        clearLogs();
        Result result = readImage(frame.get(), requested);
        QCOMPARE(result.error, 0);
        QCOMPARE(result.testImage, 0);
        QCOMPARE(result.progressive, 1);
        if (requested == mlt_image_private || requested == mlt_image_vulkan) {
            QCOMPARE(result.format, mlt_image_vulkan);
            continue;
        }
        QCOMPARE(result.format, requested);
        QVERIFY(!result.pixels.isEmpty());
        Result expected = readImage(native.get(), mlt_image_vulkan);
        QCOMPARE(expected.error, 0);
        mlt_filter_process(publisher.get(), native.get());
        expected = readImage(native.get(), requested);
        QCOMPARE(expected.format, requested);
        QVERIFY(!expected.pixels.isEmpty());
        if (requested == mlt_image_rgba64) {
            for (int y : {2, 3})
                for (int component = 0; component < 4; component++)
                    QVERIFY(qAbs(int(component16(result, 3, y, component))
                                 - int(component16(expected, 3, y, component)))
                            <= 4);
            QVERIFY(component16(result, 3, 3, 0) > component16(result, 3, 2, 0) + 30000);
        } else {
            for (int y : {2, 3})
                QVERIFY(qAbs(redAt(result, 3, y) - redAt(expected, 3, y)) <= 1);
            QVERIFY2(redAt(result, 3, 3) > redAt(result, 3, 2),
                     qPrintable(QString("odd=%1 even=%2")
                                    .arg(redAt(result, 3, 3))
                                    .arg(redAt(result, 3, 2))));
        }
        std::lock_guard<std::mutex> lock(logMutex);
        QVERIFY(!loggedVerbose.contains("Deinterlacing"));
        QVERIFY(!loggedWarnings.contains("Deinterlacing"));
    }
}

void TestPlaceboDeinterlace::ConcurrentFrames()
{
    auto filter = newFilter();
    QVERIFY(filter);
    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 6; i++) {
        threads.emplace_back([&, i]() {
            for (int n = 0; n < 3; n++) {
                auto frame = imageFrame(pixelsFor(40, 200), i * 3 + n, false, i % 2 == 0);
                if (!frame) {
                    failures++;
                    continue;
                }
                requestDeinterlace(frame.get(), "bob");
                mlt_filter_process(filter.get(), frame.get());
                Result result = readImage(frame.get());
                if (result.error || result.testImage || result.pixels.isEmpty()
                    || qAbs(redAt(result, 3, 3) - (i % 2 == 0 ? 40 : 200)) > 1)
                    failures++;
            }
        });
    }
    for (auto &thread : threads)
        thread.join();
    QCOMPARE(failures.load(), 0);
}
