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

void TestPlaceboDeinterlace::LoaderSourceInChainUsesLink()
{
    ProducerPtr normalized(mlt_factory_producer(profile.get_profile(),
                                                "loader",
                                                "test_deinterlace_source:rows"),
                           mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(normalized && chain);
    mlt_chain_set_source(chain.get(), normalized.get());
    mlt_chain_attach_normalizers(chain.get());
    QVERIFY(!hasFilter(normalized.get(), "placebo.deinterlace"));
    QVERIFY(hasLink(chain.get(), "placebo.deinterlace"));
    auto warmup = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 2, "none");
    QVERIFY(warmup);
    sourceFrames = 0;
    auto chainFrame = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 3, "yadif");
    QVERIFY(chainFrame);
    int normalizedRequests = sourceFrames.load();

    ProducerPtr raw(mlt_factory_producer(profile.get_profile(), "test_deinterlace_source", "rows"),
                    mlt_producer_close);
    ChainPtr rawChain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(raw && rawChain);
    mlt_chain_set_source(rawChain.get(), raw.get());
    mlt_chain_attach_normalizers(rawChain.get());
    auto rawWarmup = sourceFrame(MLT_CHAIN_PRODUCER(rawChain.get()), 2, "none");
    QVERIFY(rawWarmup);
    sourceFrames = 0;
    auto rawFrame = sourceFrame(MLT_CHAIN_PRODUCER(rawChain.get()), 3, "yadif");
    QVERIFY(rawFrame);
    QCOMPARE(normalizedRequests, sourceFrames.load());

    Result chainResult = readImage(chainFrame.get());
    QCOMPARE(chainResult.testImage, 0);
    QVERIFY(!chainResult.pixels.isEmpty());
    QVERIFY2(redAt(chainResult, 3, 3) > 80, loggedVerbose.constData());
}

void TestPlaceboDeinterlace::TemporalMethodsAndFallback()
{
    Result bob = render("bob", 40, 200, true);
    Result noReferences = render("yadif", 40, 200, true);
    Result yadif = render("yadif", 40, 200, true, true);
    Result defaultMethod = render(nullptr, 40, 200, true, true);
    Result nospatial = render("yadif-nospatial", 40, 200, true, true);
    Result bwdif = render("bwdif", 40, 200, true, true);
    for (const Result *result : {&bob, &noReferences, &yadif, &defaultMethod, &nospatial, &bwdif}) {
        QVERIFY_DEINTERLACED(*result);
    }
    QCOMPARE(noReferences.pixels, bob.pixels);
    QCOMPARE(defaultMethod.pixels, yadif.pixels);
    QVERIFY2(redAt(yadif, 3, 3) > redAt(bob, 3, 3) + 60,
             qPrintable(QString("yadif=%1 bob=%2").arg(redAt(yadif, 3, 3)).arg(redAt(bob, 3, 3))));
    QVERIFY2(redAt(bwdif, 3, 3) > redAt(bob, 3, 3) + 20,
             qPrintable(QString("bwdif=%1 bob=%2").arg(redAt(bwdif, 3, 3)).arg(redAt(bob, 3, 3))));

    auto flatFields = [this](const char *method) {
        auto frame = imageFrame(pixelsFor(40, 200), 3);
        if (!frame
            || !linkReferences(frame.get(),
                               imageFrame(pixelsFor(40, 180), 2),
                               imageFrame(pixelsFor(40, 220), 4)))
            return Result{};
        requestDeinterlace(frame.get(), method);
        return readImage(frame.get());
    };
    Result spatial = flatFields("yadif");
    Result noSpatialCheck = flatFields("yadif-nospatial");
    QVERIFY(!spatial.pixels.isEmpty() && !noSpatialCheck.pixels.isEmpty());
    QVERIFY(redAt(noSpatialCheck, 3, 3) > redAt(spatial, 3, 3) + 60);
}

void TestPlaceboDeinterlace::IncompatibleReferencesUseBob()
{
    for (int scenario : {0, 1, 2, 3, 4, 5, 6}) {
        auto frame = imageFrame(temporalPixelsFor(40, 200), 3);
        auto previous
            = imageFrame(temporalPixelsFor(40, 180), scenario == 0 ? 1 : 2, false, scenario != 1);
        auto next = imageFrame(temporalPixelsFor(40, 220), 4, scenario == 2);
        ProducerPtr source(mlt_producer_new(profile.get_profile()), mlt_producer_close);
        ProducerPtr otherSource(mlt_producer_new(profile.get_profile()), mlt_producer_close);
        QVERIFY(frame && previous && next);
        if (scenario == 3) {
            QVERIFY(source && otherSource);
            mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame.get()),
                                    "_producer",
                                    source.get(),
                                    0,
                                    nullptr,
                                    nullptr);
            mlt_properties_set_data(MLT_FRAME_PROPERTIES(previous.get()),
                                    "_producer",
                                    otherSource.get(),
                                    0,
                                    nullptr,
                                    nullptr);
        } else if (scenario == 4) {
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame.get()),
                                   "color_primaries",
                                   mlt_color_pri_bt709);
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(previous.get()),
                                   "color_primaries",
                                   mlt_color_pri_bt2020);
        } else if (scenario == 5) {
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(previous.get()), "width", imageWidth / 2);
        } else if (scenario == 6) {
            mlt_properties_set(MLT_FRAME_PROPERTIES(previous.get()), "color_trc", "bt709");
        }
        QVERIFY(linkReferences(frame.get(), std::move(previous), std::move(next)));
        mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
        requestDeinterlace(props, "yadif");
        Result result = readImage(frame.get());
        QCOMPARE(result.testImage, 0);
        QCOMPARE(result.progressive, 1);
        QVERIFY(!result.pixels.isEmpty());
        QVERIFY2(qAbs(redAt(result, 3, 3) - 40) <= 1,
                 qPrintable(QString("scenario %1: %2").arg(scenario).arg(redAt(result, 3, 3))));
    }

    auto frame = imageFrame(temporalPixelsFor(40, 200), 3);
    auto next = imageFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(frame && next);
    mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(frame.get()));
    FramePtr self(frame.get(), mlt_frame_close);
    QVERIFY(linkReferences(frame.get(), std::move(self), std::move(next)));
    mlt_properties props = MLT_FRAME_PROPERTIES(frame.get());
    requestDeinterlace(props, "yadif");
    Result result = readImage(frame.get());
    auto link = static_cast<mlt_link>(mlt_properties_get_data(props, "_test_link", nullptr));
    QVERIFY(link);
    mlt_properties unique = mlt_frame_get_unique_properties(frame.get(), MLT_LINK_SERVICE(link));
    QVERIFY(unique);
    QCOMPARE(mlt_properties_get_data(unique, "_previous", nullptr), (void *) frame.get());
    // Break the self-reference cycle.
    mlt_properties_clear(unique, "_previous");
    QCOMPARE(result.testImage, 0);
    QVERIFY(!result.pixels.isEmpty());
    QVERIFY(qAbs(redAt(result, 3, 3) - 40) <= 1);
}

void TestPlaceboDeinterlace::AliasedSourceBuffersUseBob_data()
{
    QTest::addColumn<bool>("lazy");
    QTest::addColumn<int>("pair");
    QTest::addColumn<int>("requested");
    QTest::addColumn<int>("input");
    const char *pairs[] = {"previous-next", "current-previous", "current-next"};
    for (bool lazy : {false, true})
        for (int pair = 0; pair < 3; pair++)
            for (auto input : {mlt_image_rgba, mlt_image_rgb})
                for (auto format : {mlt_image_rgba, mlt_image_private, mlt_image_vulkan}) {
                    QByteArray name = QByteArray(lazy ? "lazy-" : "cached-") + pairs[pair] + "-"
                                      + mlt_image_format_name(input) + "-"
                                      + mlt_image_format_name(format);
                    QTest::newRow(name.constData()) << lazy << pair << int(format) << int(input);
                }
}

void TestPlaceboDeinterlace::AliasedSourceBuffersUseBob()
{
    QFETCH(bool, lazy);
    QFETCH(int, pair);
    QFETCH(int, requested);
    QFETCH(int, input);
    if (requested == mlt_image_vulkan
        && !mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
        QSKIP("The published placebo backend is not Vulkan");
    auto sharedGetImage = +[](mlt_frame frame,
                              uint8_t **image,
                              mlt_image_format *format,
                              int *width,
                              int *height,
                              int) {
        *image = static_cast<uint8_t *>(
            mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), "_test_shared_image", nullptr));
        *format = mlt_image_format(mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "format"));
        *width = imageWidth;
        *height = imageHeight;
        return mlt_frame_set_image(frame, *image, 0, nullptr);
    };
    auto makeFrame = [input](int odd, int position) {
        return imageFrame(input == mlt_image_rgb ? temporalRgbPixelsFor(40, odd)
                                                 : temporalPixelsFor(40, odd),
                          position,
                          false,
                          true,
                          mlt_image_format(input));
    };
    auto current = makeFrame(200, 3);
    auto shared = makeFrame(200, 2);
    auto previous = makeFrame(180, 2);
    auto next = makeFrame(220, 4);
    QVERIFY(current && shared && previous && next);
    // Install the real GPU converter without changing the interlaced pixels.
    auto converter = newFilter();
    QVERIFY(converter);
    requestDeinterlace(current.get(), "none");
    mlt_filter_process(converter.get(), current.get());
    QCOMPARE(readImage(current.get(), mlt_image_format(input)).error, 0);
    mlt_frame_append_convert_image(current.get(), referenceConvert);
    auto *image = static_cast<uint8_t *>(
        mlt_properties_get_data(MLT_FRAME_PROPERTIES(shared.get()), "image", nullptr));
    QVERIFY(image);
    for (auto reference :
         {pair == 0 ? previous.get() : current.get(), pair == 1 ? previous.get() : next.get()}) {
        QCOMPARE(mlt_frame_set_image(reference, lazy ? nullptr : image, 0, nullptr), 0);
        if (lazy) {
            mlt_properties_set_data(MLT_FRAME_PROPERTIES(reference),
                                    "_test_shared_image",
                                    image,
                                    0,
                                    nullptr,
                                    nullptr);
            mlt_frame_push_get_image(reference, sharedGetImage);
        }
    }
    mlt_frame_push_get_image(
        current.get(),
        +[](mlt_frame frame,
            uint8_t **image,
            mlt_image_format *format,
            int *width,
            int *height,
            int writable) {
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "_test_upstream_format", *format);
            return mlt_frame_get_image(frame, image, format, width, height, writable);
        });
    QVERIFY(linkReferences(current.get(), std::move(previous), std::move(next)));
    requestDeinterlace(current.get(), "yadif");
    Result result = readImage(current.get(), mlt_image_format(requested));
    int upstream = mlt_properties_get_int(MLT_FRAME_PROPERTIES(current.get()),
                                          "_test_upstream_format");
    QVERIFY(upstream == mlt_image_private || upstream == mlt_image_vulkan);
    if (requested != mlt_image_rgba) {
        QCOMPARE(result.error, 0);
        QCOMPARE(int(result.format), requested);
        mlt_filter_process(publisher.get(), current.get());
        result = readImage(current.get());
    }
    QVERIFY_DEINTERLACED(result);
    QVERIFY(qAbs(redAt(result, 3, 3) - 40) <= 1);
    std::lock_guard<std::mutex> lock(logMutex);
    QVERIFY(loggedVerbose.contains("adjacent frames share images"));
}

void TestPlaceboDeinterlace::SharedReferenceFrames()
{
    struct Interleave
    {
        mlt_frame current;
        mlt_frame reference;
        Result result;
        bool sourceWasUnmodified = false;
    };
    auto pullSecond = +[](mlt_frame frame,
                          uint8_t **image,
                          mlt_image_format *format,
                          int *width,
                          int *height,
                          int writable) {
        auto *state = static_cast<Interleave *>(
            mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), "_test_interleave", nullptr));
        state->sourceWasUnmodified = !mlt_properties_get_data(MLT_FRAME_PROPERTIES(state->reference),
                                                              "_placebo_deinterlace_proxy",
                                                              nullptr);
        if (state->sourceWasUnmodified)
            state->result = readImage(state->current);
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    };
    ProducerPtr producer(mlt_producer_new(profile.get_profile()), mlt_producer_close);
    auto first = imageFrame(temporalPixelsFor(40, 200), 3);
    auto second = imageFrame(temporalPixelsFor(40, 200), 3);
    auto previous = imageFrame(temporalPixelsFor(40, 180), 2);
    auto firstNext = imageFrame(temporalPixelsFor(40, 220), 4);
    auto secondNext = imageFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(producer && first && second && previous && firstNext && secondNext);
    for (auto frame : {first.get(), second.get(), previous.get(), firstNext.get(), secondNext.get()})
        mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                                "_producer",
                                producer.get(),
                                0,
                                nullptr,
                                nullptr);
    Interleave state{second.get(), previous.get(), {}, false};
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(firstNext.get()),
                            "_test_interleave",
                            &state,
                            0,
                            nullptr,
                            nullptr);
    mlt_frame_push_get_image(firstNext.get(), pullSecond);
    mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(previous.get()));
    FramePtr shared(previous.get(), mlt_frame_close);
    QVERIFY(linkReferences(first.get(), std::move(shared), std::move(firstNext)));
    QVERIFY(linkReferences(second.get(), std::move(previous), std::move(secondNext)));
    requestDeinterlace(first.get(), "yadif");
    requestDeinterlace(second.get(), "yadif");
    Result result = readImage(first.get());
    QVERIFY(state.sourceWasUnmodified);
    QVERIFY_DEINTERLACED(result);
    QVERIFY_DEINTERLACED(state.result);
    QCOMPARE(result.pixels, state.result.pixels);
    QVERIFY(redAt(result, 3, 3) > 100);
}

void TestPlaceboDeinterlace::LoaderBobAndChainTemporalReferences()
{
    ProducerPtr loader(mlt_factory_producer(profile.get_profile(),
                                            "loader",
                                            "test_deinterlace_source:rows"),
                       mlt_producer_close);
    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(loader && source && chain);
    mlt_chain_set_source(chain.get(), source.get());
    mlt_chain_attach_normalizers(chain.get());
    for (int position : {2, 3, 2, 7, 0, 2}) {
        // The loader filter must not fetch adjacent frames.
        sourceFrames = 0;
        auto loaderFrame = sourceFrame(loader.get(), position, "yadif");
        QVERIFY(loaderFrame);
        QCOMPARE(sourceFrames.load(), 1);
        auto chainFrame = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), position, "yadif");
        QVERIFY(chainFrame);
        Result loaderResult = readImage(loaderFrame.get());
        Result chainResult = readImage(chainFrame.get());
        QVERIFY_DEINTERLACED(loaderResult);
        QVERIFY_DEINTERLACED(chainResult);
        QCOMPARE(loaderResult.width, imageWidth);
        QCOMPARE(chainResult.width, imageWidth);
        QVERIFY2(qAbs(redAt(loaderResult, 3, 3) - 40) <= 1,
                 qPrintable(
                     QString("position %1: loader=%2").arg(position).arg(redAt(loaderResult, 3, 3))));
        if (position == 0 || position == 7) {
            QVERIFY(qAbs(redAt(chainResult, 3, 3) - 40) <= 3);
        } else {
            QVERIFY2(redAt(chainResult, 3, 3) > 80,
                     qPrintable(
                         QString("position %1: %2").arg(position).arg(redAt(chainResult, 3, 3))));
        }
    }
    QVERIFY(loggedWarning("Deinterlacer yadif is unavailable; using GPU bob"));
}

void TestPlaceboDeinterlace::ChangingScanTypeAndCuts()
{
    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(source && chain);
    mlt_properties props = MLT_PRODUCER_PROPERTIES(source.get());
    mlt_properties_set_int(props, "_test_progressive", 1);
    mlt_properties_set_int(props, "meta.media.progressive", 1);
    mlt_chain_set_source(chain.get(), source.get());
    mlt_chain_attach_normalizers(chain.get());
    auto progressive = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 2, "yadif");
    QVERIFY(progressive);
    Result result = readImage(progressive.get());
    QCOMPARE(result.testImage, 0);
    QCOMPARE(result.progressive, 1);
    QVERIFY(qAbs(redAt(result, 3, 3) - 140) <= 3);

    mlt_properties_set_int(props, "_test_progressive", 0);
    auto interlaced = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 3, "yadif");
    QVERIFY(interlaced);
    result = readImage(interlaced.get());
    QCOMPARE(result.testImage, 0);
    QVERIFY(redAt(result, 3, 3) > 80);

    auto bob = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 4, "bob");
    QVERIFY(bob);
    result = readImage(bob.get());
    QCOMPARE(result.progressive, 1);
    QVERIFY(qAbs(redAt(result, 3, 3) - 40) <= 3);

    auto weave = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 5, "weave");
    QVERIFY(weave);
    result = readImage(weave.get());
    QCOMPARE(result.progressive, 1);
    QVERIFY(qAbs(redAt(result, 3, 3) - 170) <= 3);

    ProducerPtr cut(mlt_producer_cut(MLT_CHAIN_PRODUCER(chain.get()), 3, 5), mlt_producer_close);
    QVERIFY(cut);
    for (int position : {0, 1, 0}) {
        auto frame = sourceFrame(cut.get(), position, "yadif");
        QVERIFY(frame);
        result = readImage(frame.get());
        QCOMPARE(result.testImage, 0);
        QCOMPARE(result.progressive, 1);
        QVERIFY(redAt(result, 3, 3) > 80);
    }
}

void TestPlaceboDeinterlace::ReferencePullsAndMethodChanges_data()
{
    QTest::addColumn<bool>("scanInImage");
    QTest::newRow("frame scan type") << false;
    QTest::newRow("producer scan type") << true;
}

void TestPlaceboDeinterlace::ReferencePullsAndMethodChanges()
{
    QFETCH(bool, scanInImage);
    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    std::unique_ptr<mlt_link_s, decltype(&mlt_link_close)>
        link(mlt_factory_link("placebo.deinterlace", nullptr), mlt_link_close);
    QVERIFY(source && link);
    mlt_link_connect_next(link.get(), source.get(), profile.get_profile());
    auto props = MLT_PRODUCER_PROPERTIES(source.get());
    mlt_properties_set_int(props, "_test_scan_in_image", scanInImage);
    mlt_properties_set_int(props, "_test_progressive", 1);
    mlt_properties_set_int(props, "meta.media.progressive", 1);
    sourceFrames = 0;
    auto frame = sourceFrame(MLT_LINK_PRODUCER(link.get()), 3, "yadif");
    QVERIFY(frame);
    QCOMPARE(sourceFrames.load(), 1);
    QCOMPARE(readImage(frame.get()).progressive, 1);

    mlt_properties_set_int(props, "_test_progressive", 0);
    mlt_properties_set_int(props, "meta.media.progressive", 0);
    for (const char *method : {"bob", "none", "weave", "yadif", "bwdif", "yadif"}) {
        sourceFrames = 0;
        frame = sourceFrame(MLT_LINK_PRODUCER(link.get()), 3, method);
        QVERIFY(frame);
        QCOMPARE(sourceFrames.load(), 3);
        Result result = readImage(frame.get());
        QCOMPARE(result.error, 0);
        QVERIFY(!result.pixels.isEmpty());
        if (!strcmp(method, "yadif") || !strcmp(method, "bwdif"))
            QVERIFY(redAt(result, 3, 3) > 80);
    }
}

void TestPlaceboDeinterlace::ReferenceConverters()
{
    // The current frame's converters must convert references, which are fetched
    // upstream of a chain's converter links; a reference's own failing
    // converters may be tried first.
    for (bool referencesHaveConverters : {false, true}) {
        auto frame = imageFrame(temporalPixelsFor(40, 200), 3);
        auto previous = imageFrame(temporalRgbPixelsFor(40, 180), 2, false, true, mlt_image_rgb);
        auto next = imageFrame(temporalRgbPixelsFor(40, 220), 4, false, true, mlt_image_rgb);
        QVERIFY(frame && previous && next);
        if (referencesHaveConverters) {
            mlt_frame_append_convert_image(previous.get(), failingConvert);
            mlt_frame_append_convert_image(next.get(), failingConvert);
        }
        mlt_frame_append_convert_image(frame.get(), referenceConvert);
        referenceConversions = 0;
        QVERIFY(linkReferences(frame.get(), std::move(previous), std::move(next)));
        requestDeinterlace(frame.get(), "yadif");
        Result result = readImage(frame.get());
        QCOMPARE(result.error, 0);
        QCOMPARE(result.testImage, 0);
        QCOMPARE(result.progressive, 1);
        QCOMPARE(referenceConversions.load(), 2);
        QVERIFY(!result.pixels.isEmpty());
        QVERIFY2(redAt(result, 3, 3) > 100,
                 qPrintable(QString("temporal reference result: %1").arg(redAt(result, 3, 3))));
    }
}

void TestPlaceboDeinterlace::ChainHighBitDepthTemporalReferences()
{
    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(source && chain);
    mlt_properties_set_int(MLT_PRODUCER_PROPERTIES(source.get()), "_test_high_depth", 1);
    mlt_chain_set_source(chain.get(), source.get());
    mlt_chain_attach_normalizers(chain.get());
    if (!hasLink(chain.get(), "avcolor_space"))
        QSKIP("The chain's high-bit-depth converter is unavailable");
    auto bob = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 3, "bob");
    QVERIFY(bob);
    Result bobResult = readImage(bob.get(), mlt_image_rgba64);
    auto yadif = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), 3, "yadif");
    QVERIFY(yadif);
    Result yadifResult = readImage(yadif.get(), mlt_image_rgba64);
    for (const Result *result : {&bobResult, &yadifResult}) {
        QCOMPARE(result->error, 0);
        QCOMPARE(result->testImage, 0);
        QCOMPARE(result->format, mlt_image_rgba64);
        QCOMPARE(result->progressive, 1);
        QVERIFY(!result->pixels.isEmpty());
    }
    QVERIFY2(component16(yadifResult, 3, 3, 0) > component16(bobResult, 3, 3, 0) + 10000,
             qPrintable(QString("chain high-depth yadif=%1 bob=%2")
                            .arg(component16(yadifResult, 3, 3, 0))
                            .arg(component16(bobResult, 3, 3, 0))));
}

void TestPlaceboDeinterlace::VulkanRoundTrip()
{
    if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
        QSKIP("The published placebo backend is not Vulkan");
    auto vulkanFrame = [this](const QByteArray &pixels, int position) {
        auto frame = imageFrame(pixels, position);
        if (!frame)
            return frame;
        mlt_filter_process(publisher.get(), frame.get());
        Result result = readImage(frame.get(), mlt_image_vulkan);
        if (result.error || result.testImage || result.format != mlt_image_vulkan)
            return FramePtr(nullptr, mlt_frame_close);
        return frame;
    };
    auto current = vulkanFrame(temporalPixelsFor(40, 200), 3);
    auto previous = vulkanFrame(temporalPixelsFor(40, 180), 2);
    auto next = vulkanFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(current && previous && next);
    QVERIFY(linkReferences(current.get(), std::move(previous), std::move(next)));
    mlt_properties props = MLT_FRAME_PROPERTIES(current.get());
    requestDeinterlace(props, "yadif");
    Result output = readImage(current.get(), mlt_image_vulkan);
    QCOMPARE(output.error, 0);
    QCOMPARE(output.testImage, 0);
    QCOMPARE(output.format, mlt_image_vulkan);
    QCOMPARE(output.progressive, 1);
    mlt_filter_process(publisher.get(), current.get());
    Result pixels = readImage(current.get());
    QCOMPARE(pixels.error, 0);
    QCOMPARE(pixels.testImage, 0);
    QCOMPARE(pixels.format, mlt_image_rgba);
    QVERIFY(!pixels.pixels.isEmpty());
    QVERIFY2(redAt(pixels, 3, 3) > 100,
             qPrintable(QString("Vulkan temporal result: %1").arg(redAt(pixels, 3, 3))));

    current = vulkanFrame(temporalPixelsFor(40, 200), 3);
    previous = vulkanFrame(temporalPixelsFor(40, 180), 2);
    next = imageFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(current && previous && next);
    auto *shared = static_cast<uint8_t *>(
        mlt_properties_get_data(MLT_FRAME_PROPERTIES(previous.get()), "image", nullptr));
    QVERIFY(shared);
    QCOMPARE(mlt_frame_set_image(next.get(), shared, 0, nullptr), 0);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(next.get()), "format", mlt_image_vulkan);
    QVERIFY(linkReferences(current.get(), std::move(previous), std::move(next)));
    requestDeinterlace(current.get(), "yadif");
    output = readImage(current.get(), mlt_image_vulkan);
    QCOMPARE(output.format, mlt_image_vulkan);
    QCOMPARE(output.testImage, 0);
    mlt_filter_process(publisher.get(), current.get());
    pixels = readImage(current.get());
    QCOMPARE(pixels.format, mlt_image_rgba);
    QVERIFY(!pixels.pixels.isEmpty());
    QVERIFY(qAbs(redAt(pixels, 3, 3) - 40) <= 1);

    current = vulkanFrame(temporalPixelsFor(40, 200), 3);
    previous = vulkanFrame(temporalPixelsFor(40, 180), 2);
    next = vulkanFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(current && previous && next);
    auto *cropped = static_cast<mlt_vulkan_image *>(
        mlt_properties_get_data(MLT_FRAME_PROPERTIES(previous.get()), "image", nullptr));
    QVERIFY(cropped);
    cropped->crop_left = 1;
    QVERIFY(linkReferences(current.get(), std::move(previous), std::move(next)));
    requestDeinterlace(current.get(), "yadif");
    output = readImage(current.get(), mlt_image_vulkan);
    QCOMPARE(output.format, mlt_image_vulkan);
    QCOMPARE(output.testImage, 0);
    mlt_filter_process(publisher.get(), current.get());
    pixels = readImage(current.get());
    QCOMPARE(pixels.format, mlt_image_rgba);
    QVERIFY(!pixels.pixels.isEmpty());
    QVERIFY(qAbs(redAt(pixels, 3, 3) - 40) <= 1);
}

void TestPlaceboDeinterlace::VulkanPlanarYuv()
{
    if (!mlt_properties_get_data(mlt_global_properties(), "placebo.vulkan", nullptr))
        QSKIP("The published placebo backend is not Vulkan");
    FilterPtr available(mlt_factory_filter(profile.get_profile(), "avfilter.scale_vulkan", nullptr),
                        mlt_filter_close);
    if (!available)
        QSKIP("FFmpeg's Vulkan scaler is unavailable");

    auto planarFrame = [this](const QByteArray &pixels, int position) {
        auto frame = imageFrame(pixels, position);
        FilterPtr scale(mlt_factory_filter(profile.get_profile(), "avfilter.scale_vulkan", nullptr),
                        mlt_filter_close);
        if (!frame || !scale)
            return FramePtr(nullptr, mlt_frame_close);
        mlt_properties_set(MLT_FRAME_PROPERTIES(frame.get()), "color_trc", "bt709");
        mlt_properties_set(MLT_FILTER_PROPERTIES(scale.get()), "av.format", "yuv420p");
        mlt_properties_set(MLT_FILTER_PROPERTIES(scale.get()), "av.out_range", "full");
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(scale.get()), "av.w", imageWidth * 2);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(scale.get()), "av.h", imageHeight * 2);
        mlt_properties_set(MLT_FILTER_PROPERTIES(scale.get()), "av.scaler", "nearest");
        mlt_filter_process(publisher.get(), frame.get());
        mlt_filter_process(scale.get(), frame.get());
        Result result = readImage(frame.get(), mlt_image_vulkan);
        if (result.error || result.testImage || result.format != mlt_image_vulkan)
            return FramePtr(nullptr, mlt_frame_close);
        return frame;
    };
    auto current = planarFrame(temporalPixelsFor(40, 200), 3);
    auto previous = planarFrame(temporalPixelsFor(40, 180), 2);
    auto next = planarFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(current && previous && next);
    for (mlt_frame frame : {current.get(), previous.get(), next.get()}) {
        auto image = static_cast<mlt_vulkan_image *>(
            mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), "image", nullptr));
        QVERIFY(image);
        if (image->rgb || image->nb_planes < 2)
            QSKIP("The Vulkan scaler cannot produce planar YUV on this device");
    }
    QVERIFY(linkReferences(current.get(), std::move(previous), std::move(next)));
    mlt_properties props = MLT_FRAME_PROPERTIES(current.get());
    requestDeinterlace(props, "yadif");
    Result output = readImage(current.get(), mlt_image_vulkan);
    QCOMPARE(output.error, 0);
    QCOMPARE(output.testImage, 0);
    QCOMPARE(output.format, mlt_image_vulkan);
    QCOMPARE(output.progressive, 1);
    mlt_filter_process(publisher.get(), current.get());
    Result pixels = readImage(current.get());
    QCOMPARE(pixels.error, 0);
    QCOMPARE(pixels.testImage, 0);
    QCOMPARE(pixels.format, mlt_image_rgba);
    QVERIFY(!pixels.pixels.isEmpty());
    QCOMPARE(pixels.width, imageWidth * 2);
    QCOMPARE(pixels.height, imageHeight * 2);
    QVERIFY2(redAt(pixels, 6, 6) > 90,
             qPrintable(QString("planar Vulkan temporal result: %1").arg(redAt(pixels, 6, 6))));

    current = planarFrame(temporalPixelsFor(40, 200), 3);
    previous = planarFrame(temporalPixelsFor(40, 180), 2);
    next = planarFrame(temporalPixelsFor(40, 220), 4);
    QVERIFY(current && previous && next);
    auto *chroma = static_cast<mlt_vulkan_image *>(
        mlt_properties_get_data(MLT_FRAME_PROPERTIES(previous.get()), "image", nullptr));
    QVERIFY(chroma);
    chroma->chroma_location = chroma->chroma_location == 1 ? 2 : 1;
    QVERIFY(linkReferences(current.get(), std::move(previous), std::move(next)));
    requestDeinterlace(current.get(), "yadif");
    output = readImage(current.get(), mlt_image_vulkan);
    QCOMPARE(output.error, 0);
    QCOMPARE(output.testImage, 0);
    QCOMPARE(output.progressive, 1);
    QVERIFY(loggedWarning("Deinterlacing yadif with GPU bob: incompatible frame references"));
}

void TestPlaceboDeinterlace::ConcurrentChainFrames()
{
    ProducerPtr source(mlt_factory_producer(profile.get_profile(),
                                            "test_deinterlace_source",
                                            "rows"),
                       mlt_producer_close);
    ChainPtr chain(mlt_chain_init(profile.get_profile()), mlt_chain_close);
    QVERIFY(source && chain);
    mlt_chain_set_source(chain.get(), source.get());
    mlt_chain_attach_normalizers(chain.get());

    std::mutex producerMutex;
    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 6; i++) {
        threads.emplace_back([&, i]() {
            for (int n = 0; n < 3; n++) {
                int position = 1 + (i + n) % 6;
                FramePtr frame(nullptr, mlt_frame_close);
                {
                    std::lock_guard<std::mutex> lock(producerMutex);
                    frame = sourceFrame(MLT_CHAIN_PRODUCER(chain.get()), position, "yadif");
                }
                if (!frame) {
                    failures++;
                    continue;
                }
                Result result = readImage(frame.get());
                if (result.error || result.testImage || !result.progressive
                    || result.pixels.isEmpty() || redAt(result, 3, 3) <= 80)
                    failures++;
            }
        });
    }
    for (auto &thread : threads)
        thread.join();
    QCOMPARE(failures.load(), 0);
}
