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

namespace PlaceboDeinterlaceTest {

std::mutex logMutex;
QByteArray loggedWarnings;
QByteArray loggedVerbose;
std::atomic<int> upstreamConversions{0};
std::atomic<int> referenceConversions{0};
std::atomic<int> sourceFrames{0};

static void collectLog(void *, int level, const char *format, va_list args)
{
    if (level > MLT_LOG_VERBOSE)
        return;
    char message[1024];
    vsnprintf(message, sizeof(message), format, args);
    std::lock_guard<std::mutex> lock(logMutex);
    if (level > MLT_LOG_WARNING) {
        loggedVerbose += message;
        return;
    }
    loggedWarnings += message;
    fprintf(stderr, "%s", message);
}

QByteArray pixelsFor(int even, int odd, int alpha)
{
    QByteArray pixels(imageWidth * imageHeight * 4, '\0');
    for (int y = 0; y < imageHeight; y++) {
        for (int x = 0; x < imageWidth; x++) {
            int offset = 4 * (y * imageWidth + x);
            int value = y % 2 ? odd : even;
            pixels[offset] = char(value);
            pixels[offset + 1] = char(value);
            pixels[offset + 2] = char(value);
            pixels[offset + 3] = char(alpha);
        }
    }
    return pixels;
}

QByteArray temporalPixelsFor(int even, int odd)
{
    QByteArray pixels = pixelsFor(even, even);
    for (int x = 0; x < imageWidth; x++) {
        int offset = 4 * (3 * imageWidth + x);
        pixels[offset] = char(odd);
        pixels[offset + 1] = char(odd);
        pixels[offset + 2] = char(odd);
    }
    return pixels;
}

QByteArray temporalRgbPixelsFor(int even, int odd)
{
    QByteArray rgba = temporalPixelsFor(even, odd);
    QByteArray rgb(imageWidth * imageHeight * 3, '\0');
    for (int i = 0; i < imageWidth * imageHeight; i++)
        memcpy(rgb.data() + 3 * i, rgba.constData() + 4 * i, 3);
    return rgb;
}

QByteArray highDepthPixels(mlt_image_format format)
{
    int size = mlt_image_format_size(format, imageWidth, imageHeight, nullptr);
    QByteArray pixels(size, '\0');
    mlt_image_s image{};
    mlt_image_set_values(&image, pixels.data(), format, imageWidth, imageHeight);
    int depth16 = format == mlt_image_yuv422p16;
    for (int y = 0; y < imageHeight; y++) {
        auto *row = reinterpret_cast<uint16_t *>(image.planes[0] + y * image.strides[0]);
        for (int x = 0; x < imageWidth; x++)
            row[x] = depth16 ? (y % 2 ? 52000 : 32000) : (y % 2 ? 800 : 493);
    }
    int chromaWidth = format == mlt_image_yuv444p10 ? imageWidth : imageWidth / 2;
    int chromaHeight = format == mlt_image_yuv420p10 ? imageHeight / 2 : imageHeight;
    for (int plane = 1; plane < 3; plane++) {
        for (int y = 0; y < chromaHeight; y++) {
            auto *row = reinterpret_cast<uint16_t *>(image.planes[plane] + y * image.strides[plane]);
            for (int x = 0; x < chromaWidth; x++)
                row[x] = depth16 ? 32768 : 512;
        }
    }
    return pixels;
}

static QByteArray temporalHighDepthPixels(int odd)
{
    QByteArray pixels = highDepthPixels(mlt_image_yuv422p16);
    auto *luma = reinterpret_cast<uint16_t *>(pixels.data());
    for (int y = 0; y < imageHeight; y++)
        for (int x = 0; x < imageWidth; x++)
            luma[y * imageWidth + x] = y == 3 ? odd : 16000;
    return pixels;
}

QByteArray rgba64Pixels()
{
    QByteArray pixels(imageWidth * imageHeight * 8, '\0');
    for (int y = 0; y < imageHeight; y++) {
        for (int x = 0; x < imageWidth; x++) {
            uint16_t color[] = {uint16_t(y % 2 ? 52000 : 12345),
                                uint16_t(y % 2 ? 42000 : 23456),
                                uint16_t(y % 2 ? 32000 : 34567),
                                uint16_t(y % 2 ? 22000 : 11111)};
            memcpy(pixels.data() + 8 * (y * imageWidth + x), color, sizeof(color));
        }
    }
    return pixels;
}

FramePtr imageFrame(const QByteArray &pixels,
                    int position,
                    bool progressive,
                    bool topFieldFirst,
                    mlt_image_format format)
{
    FramePtr frame(mlt_frame_init(nullptr), mlt_frame_close);
    if (!frame)
        return frame;
    auto *data = static_cast<uint8_t *>(mlt_pool_alloc(pixels.size()));
    if (!data)
        return FramePtr(nullptr, mlt_frame_close);
    memcpy(data, pixels.constData(), pixels.size());
    if (mlt_frame_set_image(frame.get(), data, pixels.size(), mlt_pool_release)) {
        mlt_pool_release(data);
        return FramePtr(nullptr, mlt_frame_close);
    }
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame.get());
    mlt_frame_set_position(frame.get(), position);
    mlt_properties_set_int(properties, "format", format);
    mlt_properties_set_int(properties, "width", imageWidth);
    mlt_properties_set_int(properties, "height", imageHeight);
    mlt_properties_set_int(properties, "progressive", progressive);
    mlt_properties_set_int(properties, "top_field_first", topFieldFirst);
    mlt_properties_set_int(properties, "colorspace", 709);
    mlt_properties_set(properties, "color_trc", "linear");
    return frame;
}

Result readImage(mlt_frame frame, mlt_image_format requested)
{
    Result result;
    result.format = requested;
    uint8_t *image = nullptr;
    result.error
        = mlt_frame_get_image(frame, &image, &result.format, &result.width, &result.height, 0);
    result.progressive = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "progressive");
    result.testImage = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "test_image");
    if (!result.error && image && result.format != mlt_image_private
        && result.format != mlt_image_vulkan && result.format != mlt_image_movit
        && result.format != mlt_image_opengl_texture && result.format != mlt_image_none
        && result.format != mlt_image_invalid) {
        int size = mlt_image_format_size(result.format, result.width, result.height, nullptr);
        result.pixels = QByteArray(reinterpret_cast<const char *>(image), size);
    }
    return result;
}

int redAt(const Result &result, int x, int y)
{
    return static_cast<unsigned char>(result.pixels[4 * (y * result.width + x)]);
}

uint16_t component16(const Result &result, int x, int y, int component)
{
    uint16_t value = 0;
    memcpy(&value, result.pixels.constData() + 2 * (4 * (y * result.width + x) + component), 2);
    return value;
}

static void storedKey(char *key, size_t size, mlt_position position)
{
    snprintf(key, size, "_test_frame.%d", int(position));
}

static int storedGetFrame(mlt_producer producer, mlt_frame_ptr frame, int)
{
    char key[32];
    storedKey(key, sizeof(key), mlt_producer_position(producer));
    auto stored = static_cast<mlt_frame>(
        mlt_properties_get_data(MLT_PRODUCER_PROPERTIES(producer), key, nullptr));
    mlt_producer_prepare_next(producer);
    if (!stored) {
        *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
        return 1;
    }
    mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(stored));
    *frame = stored;
    return 0;
}

static void storeFrame(mlt_producer producer, mlt_position position, mlt_frame frame)
{
    char key[32];
    storedKey(key, sizeof(key), position);
    mlt_properties_set_data(MLT_PRODUCER_PROPERTIES(producer),
                            key,
                            frame,
                            0,
                            reinterpret_cast<mlt_destructor>(mlt_frame_close),
                            nullptr);
}

static int sourceGetImage(
    mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int)
{
    auto producer = static_cast<mlt_producer>(mlt_frame_pop_service(frame));
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    mlt_properties producerProperties = MLT_PRODUCER_PROPERTIES(producer);
    int position = mlt_properties_get_int(props, "_test_position");
    int odd = 120 + 10 * position;
    bool highDepth = mlt_properties_get_int(producerProperties, "_test_high_depth");
    QByteArray pixels = highDepth ? temporalHighDepthPixels(40000 + 1000 * position)
                                  : temporalPixelsFor(40, odd);
    auto *data = static_cast<uint8_t *>(mlt_pool_alloc(pixels.size()));
    if (!data)
        return 1;
    memcpy(data, pixels.constData(), pixels.size());
    if (mlt_frame_set_image(frame, data, pixels.size(), mlt_pool_release)) {
        mlt_pool_release(data);
        return 1;
    }
    mlt_properties_set_int(props,
                           "progressive",
                           mlt_properties_get_int(producerProperties, "_test_progressive"));
    mlt_properties_set_int(props,
                           "top_field_first",
                           mlt_properties_get_int(producerProperties, "_test_tff"));
    mlt_properties_set_int(props, "full_range", highDepth);
    *image = data;
    *format = highDepth ? mlt_image_yuv422p16 : mlt_image_rgba;
    *width = imageWidth;
    *height = imageHeight;
    return 0;
}

static int sourceGetFrame(mlt_producer producer, mlt_frame_ptr frame, int)
{
    sourceFrames++;
    mlt_position position = mlt_producer_position(producer);
    mlt_properties producerProperties = MLT_PRODUCER_PROPERTIES(producer);
    if (mlt_properties_exists(producerProperties, "_test_fail_position")
        && mlt_properties_get_position(producerProperties, "_test_fail_position") == position) {
        *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
        return 1;
    }
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
    if (!*frame)
        return 1;
    mlt_frame_set_position(*frame, position);
    mlt_properties props = MLT_FRAME_PROPERTIES(*frame);
    mlt_properties_set_position(props, "_test_position", position);
    mlt_properties_set_int(props, "width", imageWidth);
    mlt_properties_set_int(props, "height", imageHeight);
    mlt_properties_set_int(props, "meta.media.width", imageWidth);
    mlt_properties_set_int(props, "meta.media.height", imageHeight);
    mlt_properties_set_int(props, "colorspace", 709);
    mlt_properties_set(props, "color_trc", "linear");
    mlt_frame_set_aspect_ratio(*frame, 1.0);
    if (!mlt_properties_get_int(producerProperties, "_test_scan_in_image"))
        mlt_properties_set_int(props,
                               "progressive",
                               mlt_properties_get_int(producerProperties, "_test_progressive"));
    mlt_frame_push_service(*frame, producer);
    mlt_frame_push_get_image(*frame, sourceGetImage);
    mlt_producer_prepare_next(producer);
    return 0;
}

static void *sourceInit(mlt_profile profile, mlt_service_type, const char *, const void *)
{
    mlt_producer producer = mlt_producer_new(profile);
    if (!producer)
        return nullptr;
    mlt_properties props = MLT_PRODUCER_PROPERTIES(producer);
    mlt_properties_set_int(props, "_test_tff", 1);
    mlt_properties_set_int(props, "_test_progressive", 0);
    mlt_properties_set_int(props, "meta.media.progressive", 0);
    mlt_properties_set_position(props, "length", 8);
    mlt_producer_set_in_and_out(producer, 0, 7);
    producer->get_frame = sourceGetFrame;
    return producer;
}

int upstreamConvert(mlt_frame frame,
                    uint8_t **image,
                    mlt_image_format *format,
                    mlt_image_format requested)
{
    if (*format != mlt_image_rgba || requested != mlt_image_rgba64)
        return 1;
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int pixels = mlt_properties_get_int(props, "width") * mlt_properties_get_int(props, "height");
    auto *output = static_cast<uint16_t *>(mlt_pool_alloc(pixels * 8));
    if (!output)
        return 1;
    for (int i = 0; i < pixels * 4; i++)
        output[i] = uint16_t((*image)[i]) * 257;
    if (mlt_frame_set_image(frame,
                            reinterpret_cast<uint8_t *>(output),
                            pixels * 8,
                            mlt_pool_release)) {
        mlt_pool_release(output);
        return 1;
    }
    *image = reinterpret_cast<uint8_t *>(output);
    *format = mlt_image_rgba64;
    upstreamConversions++;
    return 0;
}

int referenceConvert(mlt_frame frame,
                     uint8_t **image,
                     mlt_image_format *format,
                     mlt_image_format requested)
{
    if (*format != mlt_image_rgb || requested != mlt_image_rgba)
        return 1;
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int pixels = mlt_properties_get_int(props, "width") * mlt_properties_get_int(props, "height");
    auto *output = static_cast<uint8_t *>(mlt_pool_alloc(pixels * 4));
    if (!output)
        return 1;
    for (int i = 0; i < pixels; i++) {
        memcpy(output + 4 * i, *image + 3 * i, 3);
        output[4 * i + 3] = 255;
    }
    if (mlt_frame_set_image(frame, output, pixels * 4, mlt_pool_release)) {
        mlt_pool_release(output);
        return 1;
    }
    *image = output;
    *format = mlt_image_rgba;
    referenceConversions++;
    return 0;
}

int failingConvert(mlt_frame, uint8_t **, mlt_image_format *, mlt_image_format)
{
    return 1;
}

void clearLogs()
{
    std::lock_guard<std::mutex> lock(logMutex);
    loggedWarnings.clear();
    loggedVerbose.clear();
}

bool loggedWarning(const char *message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    return loggedWarnings.contains(message);
}

void requestDeinterlace(mlt_properties properties, const char *method)
{
    mlt_properties_set_int(properties, "consumer.progressive", 1);
    mlt_properties_set(properties, "consumer.deinterlacer", method);
}

void requestDeinterlace(mlt_frame frame, const char *method)
{
    requestDeinterlace(MLT_FRAME_PROPERTIES(frame), method);
}

bool hasFilter(mlt_producer producer, const char *name)
{
    mlt_service service = MLT_PRODUCER_SERVICE(producer);
    for (int i = 0; i < mlt_service_filter_count(service); i++) {
        mlt_filter filter = mlt_service_filter(service, i);
        const char *id = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "mlt_service");
        if (id && !strcmp(id, name))
            return true;
    }
    return false;
}

bool hasLink(mlt_chain chain, const char *name)
{
    for (int i = 0; i < mlt_chain_link_count(chain); i++) {
        mlt_link link = mlt_chain_link(chain, i);
        const char *id = mlt_properties_get(MLT_LINK_PROPERTIES(link), "mlt_service");
        if (id && !strcmp(id, name))
            return true;
    }
    return false;
}

FramePtr sourceFrame(mlt_producer producer, int position, const char *method)
{
    mlt_producer_seek(producer, position);
    mlt_frame frame = nullptr;
    if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &frame, 0)) {
        mlt_frame_close(frame);
        return FramePtr(nullptr, mlt_frame_close);
    }
    requestDeinterlace(frame, method);
    return FramePtr(frame, mlt_frame_close);
}

} // namespace PlaceboDeinterlaceTest

FilterPtr TestPlaceboDeinterlace::newFilter()
{
    return FilterPtr(mlt_factory_filter(profile.get_profile(), "placebo.deinterlace", nullptr),
                     mlt_filter_close);
}

bool TestPlaceboDeinterlace::linkReferences(mlt_frame frame, FramePtr previous, FramePtr next)
{
    mlt_link link = mlt_factory_link("placebo.deinterlace", nullptr);
    mlt_producer store = mlt_producer_new(profile.get_profile());
    if (!link || !store) {
        mlt_link_close(link);
        mlt_producer_close(store);
        return false;
    }
    store->get_frame = storedGetFrame;
    mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(frame));
    storeFrame(store, storedPosition, frame);
    storeFrame(store, storedPosition - 1, previous.release());
    storeFrame(store, storedPosition + 1, next.release());
    mlt_link_connect_next(link, store, profile.get_profile());
    // Frames refer to the store as their producer, so it lives with the link.
    mlt_properties_set_data(MLT_LINK_PROPERTIES(link),
                            "_test_store",
                            store,
                            0,
                            reinterpret_cast<mlt_destructor>(mlt_producer_close),
                            nullptr);
    mlt_producer_seek(MLT_LINK_PRODUCER(link), storedPosition);
    mlt_frame linked = nullptr;
    int error = mlt_service_get_frame(MLT_LINK_SERVICE(link), &linked, 0);
    for (int offset : {-1, 0, 1})
        storeFrame(store, storedPosition + offset, nullptr);
    if (error || linked != frame) {
        mlt_frame_close(linked);
        mlt_link_close(link);
        return false;
    }
    mlt_frame_close(linked);
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                            "_test_link",
                            link,
                            0,
                            reinterpret_cast<mlt_destructor>(mlt_link_close),
                            nullptr);
    return true;
}

Result TestPlaceboDeinterlace::render(
    const char *method, int even, int odd, bool topFieldFirst, bool addReferences)
{
    auto filter = newFilter();
    auto frame = imageFrame(addReferences ? temporalPixelsFor(even, odd) : pixelsFor(even, odd),
                            3,
                            false,
                            topFieldFirst);
    if (!filter || !frame)
        return {};
    if (addReferences) {
        if (!linkReferences(frame.get(),
                            imageFrame(temporalPixelsFor(even, odd - 20), 2, false, topFieldFirst),
                            imageFrame(temporalPixelsFor(even, odd + 20), 4, false, topFieldFirst)))
            return {};
    } else {
        mlt_filter_process(filter.get(), frame.get());
    }
    requestDeinterlace(frame.get(), method);
    return readImage(frame.get());
}

void TestPlaceboDeinterlace::initTestCase()
{
    Factory::init();
    profile.set_width(imageWidth);
    profile.set_height(imageHeight);
    profile.set_progressive(1);
    profile.set_frame_rate(25, 1);
    profile.set_sample_aspect(1, 1);
    profile.set_display_aspect(1, 1);
    profile.set_explicit(1);
    mlt_repository_register(mlt_factory_repository(),
                            mlt_service_producer_type,
                            "test_deinterlace_source",
                            sourceInit);

    QVERIFY(!newFilter());
    mlt_link unpublishedLink = mlt_factory_link("placebo.deinterlace", nullptr);
    QVERIFY(!unpublishedLink);
    ProducerPtr fallback(mlt_factory_producer(profile.get_profile(),
                                              "loader",
                                              "test_deinterlace_source:rows"),
                         mlt_producer_close);
    QVERIFY(fallback);
    QVERIFY(!hasFilter(fallback.get(), "placebo.deinterlace"));
    FilterPtr cpuFilter(mlt_factory_filter(profile.get_profile(), "deinterlace", nullptr),
                        mlt_filter_close);
    if (!cpuFilter)
        cpuFilter.reset(mlt_factory_filter(profile.get_profile(), "avdeinterlace", nullptr));
    if (cpuFilter)
        QVERIFY(hasFilter(fallback.get(), "deinterlace")
                || hasFilter(fallback.get(), "avdeinterlace"));

    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(source && chain);
    mlt_chain_set_source(chain.get(), source.get());
    mlt_chain_attach_normalizers(chain.get());
    QVERIFY(!hasLink(chain.get(), "placebo.deinterlace"));
    mlt_link cpuLink = mlt_factory_link("avdeinterlace", nullptr);
    if (!cpuLink)
        cpuLink = mlt_factory_link("deinterlace", nullptr);
    if (cpuLink) {
        QVERIFY(hasLink(chain.get(), "avdeinterlace") || hasLink(chain.get(), "deinterlace"));
        mlt_link_close(cpuLink);
    }

    publisher.reset(mlt_factory_filter(profile.get_profile(), "placebo.render", nullptr));
    QVERIFY(publisher);
    gpuAvailable = bool(newFilter());
    mlt_log_set_callback(collectLog);
}

void TestPlaceboDeinterlace::init()
{
    if (!gpuAvailable)
        QSKIP("No published placebo GPU");
    clearLogs();
}

void TestPlaceboDeinterlace::RegistrationAndNormalizers()
{
    auto repository = mlt_factory_repository();
    QVERIFY(mlt_properties_exists(mlt_repository_filters(repository), "placebo.deinterlace"));
    QVERIFY(mlt_properties_exists(mlt_repository_links(repository), "placebo.deinterlace"));
    mlt_properties filterMetadata = mlt_repository_metadata(repository,
                                                            mlt_service_filter_type,
                                                            "placebo.deinterlace");
    mlt_properties linkMetadata = mlt_repository_metadata(repository,
                                                          mlt_service_link_type,
                                                          "placebo.deinterlace");
    QVERIFY(filterMetadata && linkMetadata);
    QCOMPARE(QByteArray(mlt_properties_get(filterMetadata, "type")), QByteArray("filter"));
    QCOMPARE(QByteArray(mlt_properties_get(linkMetadata, "type")), QByteArray("link"));
    mlt_properties_close(filterMetadata);
    mlt_properties_close(linkMetadata);

    ProducerPtr loader(mlt_factory_producer(profile.get_profile(),
                                            "loader",
                                            "test_deinterlace_source:rows"),
                       mlt_producer_close);
    QVERIFY(loader);
    QVERIFY(hasFilter(loader.get(), "placebo.deinterlace"));
    QVERIFY(!hasFilter(loader.get(), "deinterlace"));
    QVERIFY(!hasFilter(loader.get(), "avdeinterlace"));

    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(source && chain);
    mlt_chain_set_source(chain.get(), source.get());
    mlt_chain_attach_normalizers(chain.get());
    QVERIFY(hasLink(chain.get(), "placebo.deinterlace"));
    QVERIFY(!hasLink(chain.get(), "avdeinterlace"));
}

void TestPlaceboDeinterlace::MethodSubstitution_data()
{
    QTest::addColumn<bool>("link");
    QTest::addColumn<QByteArray>("method");
    QTest::addColumn<QByteArray>("used");
    // The filter has no frame references, so every method but weave and none is bob.
    for (const char *method : {"",
                               "bob",
                               "yadif",
                               "yadif-nospatial",
                               "bwdif",
                               "onefield",
                               "linearblend",
                               "greedy",
                               "estdif",
                               "not-a-method"})
        QTest::newRow(qPrintable(QString("filter %1").arg(*method ? method : "default")))
            << false << QByteArray(method) << QByteArray("bob");
    QTest::newRow("link default") << true << QByteArray() << QByteArray("yadif");
    QTest::newRow("link onefield") << true << QByteArray("onefield") << QByteArray("bob");
    for (const char *method : {"linearblend", "greedy", "estdif", "not-a-method"})
        QTest::newRow(qPrintable(QString("link %1").arg(method)))
            << true << QByteArray(method) << QByteArray("yadif");
}

void TestPlaceboDeinterlace::MethodSubstitution()
{
    QFETCH(bool, link);
    QFETCH(QByteArray, method);
    QFETCH(QByteArray, used);
    const char *name = method.isEmpty() ? nullptr : method.constData();
    Result expected = render(used.constData(), 40, 200, true, link);
    Result actual = render(name, 40, 200, true, link);
    QVERIFY_DEINTERLACED(actual);
    QCOMPARE(actual.pixels, expected.pixels);
    QByteArray warning = "Deinterlacer " + method + " is unavailable; using GPU " + used;
    QCOMPARE(loggedWarning(warning.constData()), name && method != used);
}

QTEST_APPLESS_MAIN(TestPlaceboDeinterlace)
