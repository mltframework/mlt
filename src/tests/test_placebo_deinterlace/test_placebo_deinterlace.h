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

#ifndef TEST_PLACEBO_DEINTERLACE_H
#define TEST_PLACEBO_DEINTERLACE_H

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
#include <QtTest>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace PlaceboDeinterlaceTest {

constexpr int imageWidth = 8;
constexpr int imageHeight = 8;
using FramePtr = std::unique_ptr<mlt_frame_s, decltype(&mlt_frame_close)>;
using FilterPtr = std::unique_ptr<mlt_filter_s, decltype(&mlt_filter_close)>;
using ProducerPtr = std::unique_ptr<mlt_producer_s, decltype(&mlt_producer_close)>;
using ChainPtr = std::unique_ptr<mlt_chain_s, decltype(&mlt_chain_close)>;

constexpr int storedPosition = 3;

struct Result
{
    int error = 1;
    mlt_image_format format = mlt_image_none;
    int width = imageWidth;
    int height = imageHeight;
    int progressive = 0;
    int testImage = 0;
    QByteArray pixels;
};

extern std::mutex logMutex;
extern QByteArray loggedWarnings;
extern QByteArray loggedVerbose;
extern std::atomic<int> upstreamConversions;
extern std::atomic<int> referenceConversions;
extern std::atomic<int> sourceFrames;

QByteArray pixelsFor(int even, int odd, int alpha = 255);

QByteArray temporalPixelsFor(int even, int odd);

QByteArray temporalRgbPixelsFor(int even, int odd);

QByteArray highDepthPixels(mlt_image_format format);

QByteArray rgba64Pixels();

FramePtr imageFrame(const QByteArray &pixels,
                    int position,
                    bool progressive = false,
                    bool topFieldFirst = true,
                    mlt_image_format format = mlt_image_rgba);

Result readImage(mlt_frame frame, mlt_image_format requested = mlt_image_rgba);

int redAt(const Result &result, int x, int y);

uint16_t component16(const Result &result, int x, int y, int component);

int upstreamConvert(mlt_frame frame,
                    uint8_t **image,
                    mlt_image_format *format,
                    mlt_image_format requested);

int referenceConvert(mlt_frame frame,
                     uint8_t **image,
                     mlt_image_format *format,
                     mlt_image_format requested);

int failingConvert(mlt_frame, uint8_t **, mlt_image_format *, mlt_image_format);

void clearLogs();

bool loggedWarning(const char *message);

void requestDeinterlace(mlt_properties properties, const char *method);

void requestDeinterlace(mlt_frame frame, const char *method);

bool hasFilter(mlt_producer producer, const char *name);

bool hasLink(mlt_chain chain, const char *name);

FramePtr sourceFrame(mlt_producer producer, int position, const char *method);

} // namespace PlaceboDeinterlaceTest

// A deinterlaced image was produced on the expected path.
#define QVERIFY_DEINTERLACED(result) \
    do { \
        QCOMPARE((result).error, 0); \
        QCOMPARE((result).testImage, 0); \
        QCOMPARE((result).progressive, 1); \
        QVERIFY(!(result).pixels.isEmpty()); \
    } while (0)

class TestPlaceboDeinterlace : public QObject
{
    Q_OBJECT
    using FilterPtr = PlaceboDeinterlaceTest::FilterPtr;
    using FramePtr = PlaceboDeinterlaceTest::FramePtr;
    using Result = PlaceboDeinterlaceTest::Result;
    Mlt::Profile profile;
    FilterPtr publisher{nullptr, mlt_filter_close};
    bool gpuAvailable = false;

    FilterPtr newFilter();
    bool linkReferences(mlt_frame frame, FramePtr previous, FramePtr next);
    Result render(
        const char *method, int even, int odd, bool topFieldFirst, bool addReferences = false);

private Q_SLOTS:
    void initTestCase();
    void init();
    void RegistrationAndNormalizers();
    void LoaderSourceInChainUsesLink();
    void BypassAndWeave();
    void BobFieldOrder();
    void TemporalMethodsAndFallback();
    void IncompatibleReferencesUseBob();
    void AliasedSourceBuffersUseBob_data();
    void AliasedSourceBuffersUseBob();
    void SharedReferenceFrames();
    void MethodSubstitution_data();
    void MethodSubstitution();
    void LogsMethodChanges();
    void LoaderBobAndChainTemporalReferences();
    void UnknownMethodChanges();
    void ChangingScanTypeAndCuts();
    void ReferencePullsAndMethodChanges_data();
    void ReferencePullsAndMethodChanges();
    void PreserveUpstreamConverters();
    void ReferenceConverters();
    void HighBitDepth();
    void ChainHighBitDepthTemporalReferences();
    void PrivateOutputAndAlpha();
    void Rgba64AndTransfer();
    void VulkanRoundTrip();
    void VulkanHighDepthAndTransfer();
    void ProgressiveVulkanOutput();
    void VulkanPlanarYuv();
    void ConcurrentFrames();
    void ConcurrentChainFrames();
};

#endif
