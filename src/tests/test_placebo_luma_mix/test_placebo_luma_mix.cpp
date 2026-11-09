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

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

using namespace Mlt;

namespace {

struct Pixel
{
    int r;
    int g;
    int b;
    int a;
};

struct RenderedImage
{
    int error = 1;
    mlt_image_format format = mlt_image_none;
    int width = 0;
    int height = 0;
    bool testImage = false;
    QByteArray pixels;
};

using FramePtr = std::unique_ptr<mlt_frame_s, decltype(&mlt_frame_close)>;

QByteArray solidPixels(int width, int height, Pixel pixel)
{
    QByteArray pixels(width * height * 4, '\0');
    for (int i = 0; i < width * height; i++) {
        pixels[4 * i] = char(pixel.r);
        pixels[4 * i + 1] = char(pixel.g);
        pixels[4 * i + 2] = char(pixel.b);
        pixels[4 * i + 3] = char(pixel.a);
    }
    return pixels;
}

mlt_frame imageFrame(int width,
                     int height,
                     mlt_image_format format,
                     const QByteArray &pixels,
                     const char *transfer = "linear")
{
    mlt_frame frame = mlt_frame_init(nullptr);
    if (!frame)
        return nullptr;
    auto image = static_cast<uint8_t *>(mlt_pool_alloc(pixels.size()));
    if (!image) {
        mlt_frame_close(frame);
        return nullptr;
    }
    memcpy(image, pixels.constData(), pixels.size());
    if (mlt_frame_set_image(frame, image, pixels.size(), mlt_pool_release)) {
        mlt_pool_release(image);
        mlt_frame_close(frame);
        return nullptr;
    }
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties_set_int(properties, "format", format);
    mlt_properties_set_int(properties, "width", width);
    mlt_properties_set_int(properties, "height", height);
    mlt_properties_set_int(properties, "colorspace", 709);
    mlt_properties_set(properties, "color_trc", transfer);
    return frame;
}

RenderedImage readImage(mlt_frame frame, mlt_image_format requested = mlt_image_rgba)
{
    RenderedImage result;
    result.format = requested;
    uint8_t *image = nullptr;
    result.error
        = mlt_frame_get_image(frame, &image, &result.format, &result.width, &result.height, 0);
    result.testImage = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "test_image");
    if (!result.error && image
        && (result.format == mlt_image_rgba || result.format == mlt_image_rgba64)) {
        int size = mlt_image_format_size(result.format, result.width, result.height, nullptr);
        result.pixels = QByteArray(reinterpret_cast<const char *>(image), size);
    }
    return result;
}

int wipeGetImage(mlt_frame frame,
                 uint8_t **image,
                 mlt_image_format *format,
                 int *width,
                 int *height,
                 int writable)
{
    (void) writable;
    auto producer = static_cast<mlt_producer>(mlt_frame_pop_service(frame));
    const char *pattern = mlt_properties_get(MLT_PRODUCER_PROPERTIES(producer), "_test_pattern");
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    int position = mlt_properties_get_int(properties, "_test_position");
    int bias = mlt_properties_get_int(properties, "_test_bias");
    const int gradient[] = {0, 64, 128, 255};
    const int reversed[] = {255, 128, 64, 0};
    auto pixels = static_cast<uint8_t *>(mlt_pool_alloc(16));
    if (!pixels)
        return 1;

    for (int i = 0; i < 4; i++) {
        int red = gradient[i];
        int alpha = 255;
        if (!strcmp(pattern, "reverse"))
            red = reversed[i];
        else if (!strcmp(pattern, "video"))
            red = position % 2 ? 255 : 0;
        else if (!strcmp(pattern, "alpha")) {
            red = 255;
            alpha = gradient[i];
        }
        red = std::clamp(red + bias, 0, 255);
        pixels[4 * i] = red;
        pixels[4 * i + 1] = !strcmp(pattern, "color") ? 255 - red : red;
        pixels[4 * i + 2] = !strcmp(pattern, "color") && (i % 2) ? 255 : red;
        pixels[4 * i + 3] = alpha;
    }
    if (mlt_frame_set_image(frame, pixels, 16, mlt_pool_release)) {
        mlt_pool_release(pixels);
        return 1;
    }
    *image = pixels;
    *format = mlt_image_rgba;
    *width = 2;
    *height = 2;
    return 0;
}

int wipeGetFrame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    (void) index;
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
    if (!*frame)
        return 1;
    const char *pattern = mlt_properties_get(MLT_PRODUCER_PROPERTIES(producer), "_test_pattern");
    if (!strcmp(pattern, "no_frame"))
        return 1;
    mlt_properties properties = MLT_FRAME_PROPERTIES(*frame);
    mlt_position position = mlt_producer_position(producer);
    mlt_frame_set_position(*frame, position);
    mlt_properties_set_position(properties, "_test_position", position);
    mlt_properties_set_int(properties,
                           "_test_bias",
                           mlt_properties_get_int(MLT_PRODUCER_PROPERTIES(producer), "bias"));
    mlt_properties_set_int(properties, "width", 2);
    mlt_properties_set_int(properties, "height", 2);
    mlt_properties_set_int(properties, "colorspace", 709);
    mlt_properties_set(properties, "color_trc", "linear");
    mlt_frame_push_service(*frame, producer);
    mlt_frame_push_get_image(*frame, wipeGetImage);
    mlt_producer_prepare_next(producer);
    return 0;
}

void *wipeProducerInit(mlt_profile profile,
                       mlt_service_type type,
                       const char *id,
                       const void *argument)
{
    (void) type;
    (void) id;
    const char *pattern = static_cast<const char *>(argument);
    if (!pattern
        || (strcmp(pattern, "gradient") && strcmp(pattern, "reverse") && strcmp(pattern, "color")
            && strcmp(pattern, "alpha") && strcmp(pattern, "video") && strcmp(pattern, "no_frame")))
        return nullptr;
    mlt_producer producer = mlt_producer_new(profile);
    if (!producer)
        return nullptr;
    mlt_properties properties = MLT_PRODUCER_PROPERTIES(producer);
    mlt_properties_set(properties, "_test_pattern", pattern);
    mlt_properties_set_int(properties, "loader_normalized", 1);
    mlt_properties_set_position(properties, "length", 2);
    mlt_producer_set_in_and_out(producer, 0, 1);
    producer->get_frame = wipeGetFrame;
    return producer;
}

int observeGpuImage(mlt_frame frame,
                    uint8_t **image,
                    mlt_image_format *format,
                    int *width,
                    int *height,
                    int writable)
{
    int error = mlt_frame_get_image(frame, image, format, width, height, writable);
    auto observed = static_cast<bool *>(
        mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), "_test_gpu_observed", nullptr));
    if (!error && *image && (*format == mlt_image_private || *format == mlt_image_vulkan))
        *observed = true;
    return error;
}

struct ConsumerCapture
{
    std::atomic<int> frames{0};
    std::atomic<int> vulkanFrames{0};
    std::atomic<int> transitionedFrames{0};
    std::atomic<int> transitionedVulkanFrames{0};
};

bool hasLumaTransition(mlt_frame frame)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    for (int i = 0; i < mlt_properties_count(properties); i++) {
        const char *name = mlt_properties_get_name(properties, i);
        if (name && !strncmp(name, "_placebo_luma_mix.", 18))
            return true;
    }
    return false;
}

bool isVulkanFrame(mlt_frame frame)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    auto image = static_cast<mlt_vulkan_image *>(
        mlt_properties_get_data(properties, "image", nullptr));
    return mlt_properties_get_int(properties, "format") == mlt_image_vulkan && image
           && image->nb_planes > 0 && image->planes[0].image
           && !mlt_properties_get_int(properties, "test_image");
}

void consumerFrameShown(mlt_properties owner, ConsumerCapture *capture, mlt_event_data data)
{
    (void) owner;
    mlt_frame frame = mlt_event_data_to_frame(data);
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    bool transitioned = hasLumaTransition(frame);
    bool transitionedVulkan = transitioned && isVulkanFrame(frame);
    for (int i = 0; i < mlt_properties_count(properties); i++) {
        const char *name = mlt_properties_get_name(properties, i);
        if (name && !strncmp(name, "mlt_tractor ", 12)) {
            auto track = static_cast<mlt_frame>(mlt_properties_get_data_at(properties, i, nullptr));
            if (track && hasLumaTransition(track)) {
                transitioned = true;
                transitionedVulkan |= isVulkanFrame(track);
            }
        }
    }
    bool vulkan = isVulkanFrame(frame);
    if (transitioned)
        capture->transitionedFrames++;
    if (vulkan)
        capture->vulkanFrames++;
    if (transitionedVulkan && vulkan)
        capture->transitionedVulkanFrames++;
    capture->frames++;
}

} // namespace

class TestPlaceboLumaMix : public QObject
{
    Q_OBJECT

    Profile profile;
    std::unique_ptr<Filter> publisher;
    bool gpuAvailable = false;

    RenderedImage render(Transition &transition,
                         int position,
                         Pixel bottom = {0, 0, 0, 255},
                         Pixel top = {255, 255, 255, 255},
                         const char *transfer = "linear")
    {
        FramePtr a(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, bottom), transfer),
                   mlt_frame_close);
        FramePtr b(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, top), transfer),
                   mlt_frame_close);
        if (!a || !b)
            return {};
        mlt_frame_set_position(a.get(), position);
        mlt_transition_process(transition.get_transition(), a.get(), b.get());
        return readImage(a.get());
    }

    void expectGray(const RenderedImage &image,
                    const std::array<int, 4> &expected,
                    const char *label)
    {
        QCOMPARE(image.error, 0);
        QCOMPARE(image.format, mlt_image_rgba);
        QCOMPARE(image.width, 2);
        QCOMPARE(image.height, 2);
        QVERIFY2(!image.testImage, label);
        QCOMPARE(image.pixels.size(), 16);
        const auto *pixels = reinterpret_cast<const uint8_t *>(image.pixels.constData());
        for (int i = 0; i < 4; i++) {
            for (int channel = 0; channel < 3; channel++) {
                QVERIFY2(qAbs(int(pixels[4 * i + channel]) - expected[i]) <= 3,
                         qPrintable(QString("%1 pixel %2 channel %3: got %4, expected %5")
                                        .arg(label)
                                        .arg(i)
                                        .arg(channel)
                                        .arg(pixels[4 * i + channel])
                                        .arg(expected[i])));
            }
            QVERIFY2(qAbs(int(pixels[4 * i + 3]) - 255) <= 3, label);
        }
    }

    void expectPixel(const RenderedImage &image, Pixel expected, const char *label)
    {
        QCOMPARE(image.error, 0);
        QCOMPARE(image.format, mlt_image_rgba);
        QVERIFY2(!image.testImage, label);
        QVERIFY(image.pixels.size() >= 4);
        const auto *p = reinterpret_cast<const uint8_t *>(image.pixels.constData());
        const int want[] = {expected.r, expected.g, expected.b, expected.a};
        for (int channel = 0; channel < 4; channel++) {
            QVERIFY2(qAbs(int(p[channel]) - want[channel]) <= 3,
                     qPrintable(QString("%1 channel %2: got %3, expected %4")
                                    .arg(label)
                                    .arg(channel)
                                    .arg(p[channel])
                                    .arg(want[channel])));
        }
    }

private Q_SLOTS:
    void initTestCase()
    {
        Factory::init();
        profile.set_width(2);
        profile.set_height(2);
        profile.set_progressive(1);
        profile.set_colorspace(709);
        profile.set_sample_aspect(1, 1);
        profile.set_display_aspect(1, 1);
        profile.set_explicit(1);
        mlt_repository_register(mlt_factory_repository(),
                                mlt_service_producer_type,
                                "test_luma_wipe",
                                wipeProducerInit);
        publisher = std::make_unique<Filter>(profile, "placebo.render");
        QVERIFY(publisher->is_valid());
        Filter probe(profile, "placebo.scale");
        gpuAvailable = probe.is_valid();
    }

    void RegistrationAndMetadata()
    {
        mlt_repository repository = mlt_factory_repository();
        QVERIFY(mlt_properties_exists(mlt_repository_transitions(repository), "placebo.luma_mix"));
        QVERIFY(mlt_properties_exists(mlt_repository_filters(repository), "placebo.scale"));
        std::unique_ptr<Properties> metadata(
            Factory::init()->metadata(mlt_service_transition_type, "placebo.luma_mix"));
        QVERIFY(metadata && metadata->is_valid());
        QCOMPARE(QString::fromUtf8(metadata->get("identifier")), QString("placebo.luma_mix"));
        auto parameters = static_cast<mlt_properties>(metadata->get_data("parameters"));
        QVERIFY(parameters);
        QCOMPARE(mlt_properties_count(parameters), 5);
        const char *names[] = {"resource", "softness", "reverse", "invert", "producer.*"};
        for (int i = 0; i < 5; i++) {
            auto parameter = static_cast<mlt_properties>(
                mlt_properties_get_data_at(parameters, i, nullptr));
            QVERIFY(parameter);
            QCOMPARE(QString::fromUtf8(mlt_properties_get(parameter, "identifier")),
                     QString::fromUtf8(names[i]));
            QCOMPARE(QString::fromUtf8(mlt_properties_get(parameter, "readonly")), QString("no"));
            if (i == 1)
                QCOMPARE(QString::fromUtf8(mlt_properties_get(parameter, "animation")),
                         QString("yes"));
        }
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        QCOMPARE(QString::fromUtf8(transition.get("resource")), QString("test_luma_wipe:gradient"));
        QCOMPARE(transition.get_int("_transition_type"), 1);
    }

    void DissolveEasing_data()
    {
        QTest::addColumn<int>("reverse");
        QTest::addColumn<int>("invert");
        QTest::addColumn<int>("middle");
        QTest::newRow("normal") << 0 << 0 << 64;
        QTest::newRow("reverse") << 1 << 0 << 191;
        QTest::newRow("invert") << 0 << 1 << 191;
        QTest::newRow("reverse-invert") << 1 << 1 << 64;
    }

    void DissolveEasing()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        QFETCH(int, reverse);
        QFETCH(int, invert);
        QFETCH(int, middle);
        Transition transition(profile, "placebo.luma_mix");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        transition.set("reverse", reverse);
        transition.set("invert", invert);
        int first = reverse ? 255 : 0;
        int last = reverse ? 0 : 255;
        expectGray(render(transition, 0), {first, first, first, first}, "dissolve start");
        expectGray(render(transition, 50), {middle, middle, middle, middle}, "eased dissolve");
        expectGray(render(transition, 100), {last, last, last, last}, "dissolve end");
    }

    void HardWipe_data()
    {
        QTest::addColumn<int>("reverse");
        QTest::addColumn<int>("invert");
        QTest::addColumn<int>("p0");
        QTest::addColumn<int>("p1");
        QTest::addColumn<int>("p2");
        QTest::addColumn<int>("p3");
        QTest::newRow("normal") << 0 << 0 << 255 << 0 << 0 << 0;
        QTest::newRow("reverse") << 1 << 0 << 255 << 255 << 255 << 0;
        QTest::newRow("invert") << 0 << 1 << 0 << 0 << 255 << 255;
        QTest::newRow("reverse-invert") << 1 << 1 << 0 << 0 << 0 << 255;
    }

    void HardWipe()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        QFETCH(int, reverse);
        QFETCH(int, invert);
        QFETCH(int, p0);
        QFETCH(int, p1);
        QFETCH(int, p2);
        QFETCH(int, p3);
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        transition.set("reverse", reverse);
        transition.set("invert", invert);
        const int first = reverse ? 255 : 0;
        const int last = reverse ? 0 : 255;
        expectGray(render(transition, 0), {first, first, first, first}, "wipe start");
        expectGray(render(transition, 49), {p0, p1, p2, p3}, "hard wipe");
        expectGray(render(transition, 100), {last, last, last, last}, "wipe end");
    }

    void SoftnessAndMaskChannels()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        transition.set("softness", 0.9999);
        const std::array<int, 4> soft = {128, 64, 0, 0};
        expectGray(render(transition, 50), soft, "soft gray mask");
        transition.set("invert", 1);
        expectGray(render(transition, 50), {128, 191, 255, 255}, "inverted soft mask");
        transition.set("reverse", 1);
        expectGray(render(transition, 50), {0, 0, 1, 128}, "reversed inverted soft mask");
        transition.set("invert", 0);
        transition.set("reverse", 0);
        transition.set("resource", "test_luma_wipe:color");
        expectGray(render(transition, 50), soft, "red-only mask");
        transition.set("resource", "test_luma_wipe:alpha");
        expectGray(render(transition, 50), soft, "premultiplied mask");
        transition.set("resource", "test_luma_wipe:gradient");
        transition.set("softness", "0=0;50=0.9999;99=0.9999");
        expectGray(render(transition, 50), soft, "animated softness");
        transition.set("softness", "0=0;99=0");
        expectGray(render(transition, 50), {255, 0, 0, 0}, "animated hard wipe");
    }

    void PartialAlphaAndColorTransfer()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        expectPixel(render(transition, 50, {255, 0, 0, 128}, {0, 0, 255, 255}),
                    {153, 0, 102, 160},
                    "premultiplied input alpha");
        expectPixel(render(transition, 50, {255, 0, 0, 0}, {0, 0, 255, 0}),
                    {0, 0, 0, 0},
                    "fully transparent inputs");
        Filter transform(profile, "placebo.color_transform");
        QVERIFY(transform.is_valid());
        transform.set("force_trc", "bt709");
        FramePtr reference(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {64, 64, 64, 255})),
                           mlt_frame_close);
        QVERIFY(reference);
        mlt_filter_process(publisher->get_filter(), reference.get());
        mlt_filter_process(transform.get_filter(), reference.get());
        RenderedImage encodedReference = readImage(reference.get());
        QCOMPARE(encodedReference.error, 0);
        QCOMPARE(encodedReference.format, mlt_image_rgba);
        QVERIFY(!encodedReference.testImage);
        QCOMPARE(encodedReference.pixels.size(), 16);
        const int encoded = static_cast<uint8_t>(encodedReference.pixels[0]);
        QVERIFY(encoded > 100 && encoded < 180);
        expectGray(render(transition, 50, {0, 0, 0, 255}, {255, 255, 255, 255}, "bt709"),
                   {encoded, encoded, encoded, encoded},
                   "native BT.709 transfer");

        transform.set("force_trc", "linear");
        FramePtr encodedSource(imageFrame(2,
                                          2,
                                          mlt_image_rgba,
                                          solidPixels(2, 2, {128, 128, 128, 255}),
                                          "bt709"),
                               mlt_frame_close);
        FramePtr bottom(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {0, 0, 0, 255})),
                        mlt_frame_close);
        FramePtr
            top(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {128, 128, 128, 255}), "bt709"),
                mlt_frame_close);
        QVERIFY(encodedSource && bottom && top);
        mlt_filter_process(publisher->get_filter(), encodedSource.get());
        mlt_filter_process(transform.get_filter(), encodedSource.get());
        RenderedImage linearReference = readImage(encodedSource.get());
        QCOMPARE(linearReference.error, 0);
        QCOMPARE(linearReference.format, mlt_image_rgba);
        QVERIFY(!linearReference.testImage);
        QCOMPARE(linearReference.pixels.size(), 16);
        const int mixed = qRound(static_cast<uint8_t>(linearReference.pixels[0]) * 0.25);
        mlt_frame_set_position(bottom.get(), 50);
        mlt_transition_process(transition.get_transition(), bottom.get(), top.get());
        expectGray(readImage(bottom.get()), {mixed, mixed, mixed, mixed}, "mixed input transfers");
    }

    void DimensionsAndRgba64()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        FramePtr a(imageFrame(4, 2, mlt_image_rgba, solidPixels(4, 2, {0, 0, 0, 255})),
                   mlt_frame_close);
        FramePtr b(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 255, 255, 255})),
                   mlt_frame_close);
        QVERIFY(a && b);
        mlt_frame_set_position(a.get(), 49);
        mlt_transition_process(transition.get_transition(), a.get(), b.get());
        RenderedImage resized = readImage(a.get());
        QCOMPARE(resized.error, 0);
        QCOMPARE(resized.format, mlt_image_rgba);
        QCOMPARE(resized.width, 4);
        QCOMPARE(resized.height, 2);
        QVERIFY(!resized.testImage);
        QCOMPARE(resized.pixels.size(), 32);
        const auto *pixels = reinterpret_cast<const uint8_t *>(resized.pixels.constData());
        QVERIFY(pixels[0] >= 252);
        QVERIFY(pixels[12] <= 3);

        transition.set("resource", static_cast<const char *>(nullptr));
        const std::array<uint16_t, 4> black = {0, 0, 0, 65535};
        const std::array<uint16_t, 4> white = {65535, 65535, 65535, 65535};
        FramePtr a16(imageFrame(1,
                                1,
                                mlt_image_rgba64,
                                QByteArray(reinterpret_cast<const char *>(black.data()),
                                           sizeof(black))),
                     mlt_frame_close);
        FramePtr b16(imageFrame(1,
                                1,
                                mlt_image_rgba64,
                                QByteArray(reinterpret_cast<const char *>(white.data()),
                                           sizeof(white))),
                     mlt_frame_close);
        QVERIFY(a16 && b16);
        mlt_frame_set_position(a16.get(), 50);
        mlt_transition_process(transition.get_transition(), a16.get(), b16.get());
        RenderedImage result = readImage(a16.get(), mlt_image_rgba64);
        QCOMPARE(result.error, 0);
        QCOMPARE(result.format, mlt_image_rgba64);
        QCOMPARE(result.width, 1);
        QCOMPARE(result.height, 1);
        QVERIFY(!result.testImage);
        QCOMPARE(result.pixels.size(), 8);
        std::array<uint16_t, 4> values;
        memcpy(values.data(), result.pixels.constData(), sizeof(values));
        for (int channel = 0; channel < 3; channel++)
            QVERIFY(qAbs(int(values[channel]) - 16384) <= 64);
        QVERIFY(qAbs(int(values[3]) - 65535) <= 64);

        const std::array<uint16_t, 4> partialA = {1000, 12345, 40000, 40000};
        const std::array<uint16_t, 4> partialB = {65000, 5000, 35000, 32000};
        FramePtr partialBottom(imageFrame(1,
                                          1,
                                          mlt_image_rgba64,
                                          QByteArray(reinterpret_cast<const char *>(partialA.data()),
                                                     sizeof(partialA))),
                               mlt_frame_close);
        FramePtr partialTop(imageFrame(1,
                                       1,
                                       mlt_image_rgba64,
                                       QByteArray(reinterpret_cast<const char *>(partialB.data()),
                                                  sizeof(partialB))),
                            mlt_frame_close);
        QVERIFY(partialBottom && partialTop);
        mlt_frame_set_position(partialBottom.get(), 50);
        mlt_transition_process(transition.get_transition(), partialBottom.get(), partialTop.get());
        RenderedImage partial = readImage(partialBottom.get(), mlt_image_rgba64);
        QCOMPARE(partial.error, 0);
        QCOMPARE(partial.format, mlt_image_rgba64);
        QCOMPARE(partial.width, 1);
        QCOMPARE(partial.height, 1);
        QVERIFY(!partial.testImage);
        QCOMPARE(partial.pixels.size(), 8);
        memcpy(values.data(), partial.pixels.constData(), sizeof(values));
        const double alpha = 0.75 * partialA[3] + 0.25 * partialB[3];
        for (int channel = 0; channel < 3; channel++) {
            double expected = (0.75 * partialA[channel] * partialA[3]
                               + 0.25 * partialB[channel] * partialB[3])
                              / alpha;
            QVERIFY(qAbs(int(values[channel]) - qRound(expected)) <= 64);
        }
        QVERIFY(qAbs(int(values[3]) - qRound(alpha)) <= 64);
    }

    void ResourceReplacementAndFailure()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        expectGray(render(transition, 49), {255, 0, 0, 0}, "constructor resource");
        transition.set("resource", "test_luma_wipe:reverse");
        expectGray(render(transition, 49), {0, 0, 0, 255}, "replaced resource");
        transition.set("resource", "test_luma_wipe:missing");
        expectGray(render(transition, 50), {64, 64, 64, 64}, "unavailable resource");
        transition.set("resource", "test_luma_wipe:no_frame");
        expectGray(render(transition, 50), {64, 64, 64, 64}, "unavailable wipe frame");
        transition.set("resource", "test_luma_wipe:gradient");
        expectGray(render(transition, 49), {255, 0, 0, 0}, "recovered resource");
        transition.set("resource", "");
        expectGray(render(transition, 50), {64, 64, 64, 64}, "empty resource");
        transition.set("resource", "test_luma_wipe:gradient");
        expectGray(render(transition, 49), {255, 0, 0, 0}, "restored resource");
        transition.set("resource", static_cast<const char *>(nullptr));
        expectGray(render(transition, 50), {64, 64, 64, 64}, "cleared resource");
    }

    void LoaderWipeResource()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "color:black");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        expectGray(render(transition, 49), {255, 255, 255, 255}, "black loader wipe");
        transition.set("resource", "color:white");
        expectGray(render(transition, 49), {0, 0, 0, 0}, "white loader wipe");
    }

    void InvalidSoftness()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        transition.set("softness", -0.5);
        RenderedImage result = render(transition, 50);
        QCOMPARE(result.error, 0);
        QVERIFY(result.testImage);
    }

    void ProducerPropertiesAndLooping()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        expectGray(render(transition, 50), {255, 0, 0, 0}, "default producer");
        transition.set("producer.bias", 64);
        expectGray(render(transition, 50), {0, 0, 0, 0}, "producer property");
        transition.set("producer.bias", 0);
        transition.set("resource", "test_luma_wipe:video");
        expectGray(render(transition, 50), {255, 255, 255, 255}, "loop frame 0");
        expectGray(render(transition, 51), {0, 0, 0, 0}, "loop frame 1");
        expectGray(render(transition, 52), {255, 255, 255, 255}, "loop frame 0 again");
        transition.set_in_and_out(11, 110);
        expectGray(render(transition, 61), {255, 255, 255, 255}, "relative seek frame 0");
        expectGray(render(transition, 62), {0, 0, 0, 0}, "relative seek frame 1");
    }

    void QueuedFramesSurviveReplacement()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        auto transition = std::make_unique<Transition>(profile,
                                                       "placebo.luma_mix",
                                                       "test_luma_wipe:gradient");
        QVERIFY(transition->is_valid());
        transition->set_in_and_out(0, 99);
        FramePtr a1(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {0, 0, 0, 255})),
                    mlt_frame_close);
        FramePtr b1(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 255, 255, 255})),
                    mlt_frame_close);
        FramePtr a2(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {0, 0, 0, 255})),
                    mlt_frame_close);
        FramePtr b2(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 255, 255, 255})),
                    mlt_frame_close);
        QVERIFY(a1 && b1 && a2 && b2);
        mlt_frame_set_position(a1.get(), 49);
        mlt_transition_process(transition->get_transition(), a1.get(), b1.get());
        transition->set("resource", "test_luma_wipe:reverse");
        transition->set("invert", 1);
        mlt_frame_set_position(a2.get(), 49);
        mlt_transition_process(transition->get_transition(), a2.get(), b2.get());
        transition.reset();
        expectGray(readImage(a2.get()), {255, 255, 0, 0}, "new queued wipe");
        expectGray(readImage(a1.get()), {255, 0, 0, 0}, "old queued wipe");
    }

    void ParallelFramesShareWipeProducer()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition transition(profile, "placebo.luma_mix", "test_luma_wipe:gradient");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 99);
        std::array<RenderedImage, 8> images;
        std::vector<std::thread> threads;
        for (int i = 0; i < int(images.size()); i++)
            threads.emplace_back([this, &transition, &images, i] {
                images[i] = render(transition, i % 2 ? 51 : 49);
            });
        for (auto &thread : threads)
            thread.join();
        for (int i = 0; i < int(images.size()); i++)
            expectGray(images[i],
                       i % 2 ? std::array<int, 4>{255, 255, 0, 0} : std::array<int, 4>{255, 0, 0, 0},
                       "parallel wipe frames");
    }

    void ChainedTransitionsStayOnGpu()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        Transition first(profile, "placebo.luma_mix");
        Transition second(profile, "placebo.luma_mix");
        QVERIFY(first.is_valid() && second.is_valid());
        first.set_in_and_out(0, 99);
        second.set_in_and_out(0, 99);
        FramePtr a(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {0, 0, 0, 255})),
                   mlt_frame_close);
        FramePtr b(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 255, 255, 255})),
                   mlt_frame_close);
        FramePtr c(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 0, 0, 255})),
                   mlt_frame_close);
        QVERIFY(a && b && c);
        bool observed = false;
        mlt_properties_set_data(MLT_FRAME_PROPERTIES(a.get()),
                                "_test_gpu_observed",
                                &observed,
                                0,
                                nullptr,
                                nullptr);
        mlt_frame_set_position(a.get(), 50);
        mlt_transition_process(first.get_transition(), a.get(), b.get());
        mlt_frame_push_get_image(a.get(), observeGpuImage);
        mlt_transition_process(second.get_transition(), a.get(), c.get());
        RenderedImage result = readImage(a.get());
        QVERIFY(observed);
        expectPixel(result, {112, 48, 48, 255}, "GPU transition chain");
    }

    void VulkanRoundTrip()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
            QSKIP("The published placebo backend is not Vulkan");
        Transition first(profile, "placebo.luma_mix");
        Transition second(profile, "placebo.luma_mix");
        QVERIFY(first.is_valid() && second.is_valid());
        first.set_in_and_out(0, 99);
        second.set_in_and_out(0, 99);
        FramePtr a(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {0, 0, 0, 255})),
                   mlt_frame_close);
        FramePtr b(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 255, 255, 255})),
                   mlt_frame_close);
        FramePtr c(imageFrame(2, 2, mlt_image_rgba, solidPixels(2, 2, {255, 0, 0, 255})),
                   mlt_frame_close);
        QVERIFY(a && b && c);
        mlt_frame_set_position(a.get(), 50);
        mlt_transition_process(first.get_transition(), a.get(), b.get());
        RenderedImage vulkan = readImage(a.get(), mlt_image_vulkan);
        QCOMPARE(vulkan.error, 0);
        QCOMPARE(vulkan.format, mlt_image_vulkan);
        QVERIFY(!vulkan.testImage);

        mlt_frame_set_position(a.get(), 0);
        mlt_transition_process(second.get_transition(), a.get(), c.get());
        expectGray(readImage(a.get()), {64, 64, 64, 64}, "Vulkan round trip");
    }

    void VulkanConsumer()
    {
        if (!gpuAvailable)
            QSKIP("No placebo GPU backend is available");
        if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
            QSKIP("The published placebo backend is not Vulkan");

        Producer bottom(profile, "loader", "color:black");
        Producer top(profile, "loader", "color:white");
        QVERIFY(bottom.is_valid() && top.is_valid());
        bottom.set("length", 4);
        top.set("length", 4);
        bottom.set_in_and_out(0, 3);
        top.set_in_and_out(0, 3);
        Tractor tractor(profile);
        QVERIFY(tractor.is_valid());
        QCOMPARE(tractor.set_track(bottom, 0), 0);
        QCOMPARE(tractor.set_track(top, 1), 0);
        Transition transition(profile, "placebo.luma_mix", "color:black");
        QVERIFY(transition.is_valid());
        transition.set_in_and_out(0, 3);
        tractor.plant_transition(transition, 0, 1);
        tractor.refresh();
        tractor.set_in_and_out(0, 3);

        ConsumerCapture capture;
        Consumer consumer(profile, "null");
        QVERIFY(consumer.is_valid());
        consumer.set("mlt_image_format", "vulkan");
        consumer.set("real_time", -1);
        consumer.set("audio_off", 1);
        std::unique_ptr<Event> event(
            consumer.listen("consumer-frame-show", &capture, (mlt_listener) consumerFrameShown));
        QVERIFY(event && event->is_valid());
        QCOMPARE(consumer.connect(tractor), 0);
        QCOMPARE(consumer.start(), 0);
        for (int i = 0; i < 500 && !consumer.is_stopped() && capture.frames.load() < 4; i++)
            QTest::qSleep(10);
        consumer.stop();
        QVERIFY(capture.frames.load() > 0);
        QVERIFY2(capture.transitionedVulkanFrames.load() > 0,
                 qPrintable(QString("frames=%1, Vulkan=%2, transitioned=%3")
                                .arg(capture.frames.load())
                                .arg(capture.vulkanFrames.load())
                                .arg(capture.transitionedFrames.load())));
    }
};

QTEST_APPLESS_MAIN(TestPlaceboLumaMix)

#include "test_placebo_luma_mix.moc"
