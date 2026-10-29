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

#include "conceal.h"

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
#include <stdio.h>
#include <string.h>
#include <QtTest>

using namespace Mlt;

namespace {

struct AlphaCapture
{
    int did;
    int exists;
    int value;
};

struct YuvSource
{
    int width;
    int height;
    int fill;
    int perRow;
    AlphaCapture *capture;
};

struct CopiedImage
{
    int width;
    int height;
    int hasAlpha;
    uint8_t pixels[16 * 16 * 2];
    uint8_t alpha[16 * 16];
};

int yuv422FromSpec(mlt_frame frame,
                   uint8_t **image,
                   mlt_image_format *format,
                   int *width,
                   int *height,
                   int writable)
{
    Q_UNUSED(writable)
    YuvSource *spec = (YuvSource *) mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame),
                                                            "yuv_source",
                                                            NULL);
    if (!spec || spec->width < 1 || spec->height < 1)
        return 1;

    const int size = spec->width * spec->height * 2;
    uint8_t *buffer = (uint8_t *) mlt_pool_alloc(size);
    if (!buffer)
        return 1;
    for (int y = 0; y < spec->height; y++) {
        const int value = spec->perRow ? spec->fill + y * 10 : spec->fill;
        memset(buffer + y * spec->width * 2, value, spec->width * 2);
    }
    mlt_frame_set_image(frame, buffer, size, mlt_pool_release);
    *image = buffer;
    *format = mlt_image_yuv422;
    *width = spec->width;
    *height = spec->height;
    if (spec->capture) {
        mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
        spec->capture->did = 1;
        spec->capture->exists = mlt_properties_exists(properties, "resize_alpha");
        spec->capture->value = mlt_properties_get_int(properties, "resize_alpha");
    }
    return 0;
}

void attachSource(mlt_frame frame, YuvSource *spec)
{
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame), "yuv_source", spec, 0, NULL, NULL);
    mlt_frame_push_get_image(frame, yuv422FromSpec);
}

int yAt(const CopiedImage *image, int x, int y)
{
    return image->pixels[(y * image->width + x) * 2];
}

int alphaAt(const CopiedImage *image, int x, int y)
{
    return image->alpha[y * image->width + x];
}

// composite_line_yuv at mix 0.5 with a missing alpha mask. Weight is 0.5 * 65536.
int halfMix(int dest, int src)
{
    const int mix = 32768;
    return (src * mix + dest * ((1 << 16) - mix)) >> 16;
}

void pass(const char *name)
{
    printf("PASS %s\n", name);
    fflush(stdout);
}

mlt_frame makePicture(int marker, mlt_position key)
{
    mlt_frame frame = mlt_frame_init(NULL);
    const int size = 8 * 8 * 2;
    uint8_t *image = (uint8_t *) mlt_pool_alloc(size);
    memset(image, marker, size);
    mlt_frame_set_image(frame, image, size, mlt_pool_release);
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties_set_int(properties, "format", mlt_image_yuv422);
    mlt_properties_set_int(properties, "width", 8);
    mlt_properties_set_int(properties, "height", 8);
    mlt_properties_set_position(properties, "original_position", key);
    return frame;
}

int pictureMarker(mlt_frame frame)
{
    int size = 0;
    uint8_t *image = (uint8_t *) mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame),
                                                         "image",
                                                         &size);
    if (!image || size < 1)
        return -1;
    return image[0];
}

} // namespace

class TestDissolve : public QObject
{
    Q_OBJECT

public:
    // Factory::close faults while unloading unrelated modules on this Windows
    // build. Sibling tests such as test_filter leave that cleanup to process exit.
    TestDissolve() { Factory::init(); }

private:
    CopiedImage fetch(mlt_frame frame, int width, int height)
    {
        CopiedImage copy;
        memset(&copy, 0, sizeof(copy));
        mlt_image_format format = mlt_image_yuv422;
        uint8_t *image = NULL;
        int error = mlt_frame_get_image(frame, &image, &format, &width, &height, 1);
        if (error || !image || width < 1 || height < 1 || width > 16 || height > 16)
            return copy;
        copy.width = width;
        copy.height = height;
        memcpy(copy.pixels, image, width * height * 2);
        uint8_t *alpha = mlt_frame_get_alpha(frame);
        if (alpha) {
            copy.hasAlpha = 1;
            memcpy(copy.alpha, alpha, width * height);
        }
        return copy;
    }

    CopiedImage runResize(int setAlpha, int alphaValue)
    {
        Profile profile;
        mlt_frame frame = mlt_frame_init(NULL);
        YuvSource spec = {8, 8, 80, 0, NULL};
        attachSource(frame, &spec);
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "distort", 1);
        if (setAlpha)
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "resize_alpha", alphaValue);

        mlt_filter filter = mlt_factory_filter(profile.get_profile(), "resize", NULL);
        if (!filter) {
            mlt_frame_close(frame);
            return CopiedImage();
        }
        mlt_filter_process(filter, frame);
        CopiedImage image = fetch(frame, 16, 16);
        mlt_filter_close(filter);
        mlt_frame_close(frame);
        return image;
    }

    CopiedImage runLuma(
        YuvSource *dest, YuvSource *src, int requestWidth, int requestHeight, int wipe)
    {
        Profile profile;
        mlt_transition transition = mlt_factory_transition(profile.get_profile(), "luma", NULL);
        CopiedImage image;
        memset(&image, 0, sizeof(image));
        if (!transition)
            return image;

        mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);
        if (mlt_properties_get(properties, "resource"))
            mlt_properties_clear(properties, "resource");
        mlt_properties_set_double(properties, "fixed", 0.5);
        mlt_properties_set_int(properties, "fix_background_alpha", 1);
        mlt_properties_set_int(properties, "alpha_over", 0);
        mlt_properties_set_int(properties, "progressive", 1);
        mlt_properties_set_int(properties, "threads", 1);
        mlt_properties_set_double(properties, "softness", 0);
        mlt_properties_set_int(properties, "in", 0);
        mlt_properties_set_int(properties, "out", 10);
        if (wipe) {
            const int count = requestWidth * requestHeight;
            uint16_t *bitmap = (uint16_t *) mlt_pool_alloc(count * sizeof(uint16_t));
            memset(bitmap, 0, count * sizeof(uint16_t));
            mlt_properties_set_int(properties, "width", requestWidth);
            mlt_properties_set_int(properties, "height", requestHeight);
            mlt_properties_set_data(properties,
                                    "bitmap",
                                    bitmap,
                                    count * (int) sizeof(uint16_t),
                                    mlt_pool_release,
                                    NULL);
        }

        mlt_frame a = mlt_frame_init(NULL);
        mlt_frame b = mlt_frame_init(NULL);
        attachSource(a, dest);
        attachSource(b, src);
        mlt_transition_process(transition, a, b);
        image = fetch(a, requestWidth, requestHeight);
        mlt_frame_close(b);
        mlt_frame_close(a);
        mlt_transition_close(transition);
        return image;
    }

private Q_SLOTS:
    void resizeAlphaMask()
    {
        CopiedImage missing = runResize(0, 0);
        QCOMPARE(missing.width, 16);
        QCOMPARE(missing.height, 16);
        QCOMPARE(missing.hasAlpha, 0);
        pass("resizeAlphaMask missing");

        CopiedImage transparent = runResize(1, 0);
        QCOMPARE(transparent.width, 16);
        QCOMPARE(transparent.height, 16);
        QCOMPARE(transparent.hasAlpha, 1);
        // filter_resize centers an 8x8 picture in 16x16, so (4,4) is picture and (0,0) is padding.
        QCOMPARE(alphaAt(&transparent, 4, 4), 255);
        QCOMPARE(alphaAt(&transparent, 0, 0), 0);
        QCOMPARE(alphaAt(&transparent, 15, 15), 0);
        pass("resizeAlphaMask explicit0");

        CopiedImage opaque = runResize(1, 255);
        QCOMPARE(opaque.width, 16);
        QCOMPARE(opaque.height, 16);
        QCOMPARE(opaque.hasAlpha, 0);
        pass("resizeAlphaMask explicit255");
    }

    void resizeAlphaRoundTrip()
    {
        Profile profile;

        // Consumer-hint copy: a missing outer property clears a pre-existing inner 0.
        {
            mlt_frame outer = mlt_frame_init(NULL);
            mlt_frame inner = mlt_frame_init(NULL);
            AlphaCapture capture = {0, -1, -1};
            YuvSource spec = {8, 8, 40, 0, &capture};
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(inner), "resize_alpha", 0);
            attachSource(inner, &spec);
            QCOMPARE(mlt_frame_prepend_image_from_service(outer, inner), 0);
            CopiedImage image = fetch(outer, 8, 8);
            QCOMPARE(image.width, 8);
            QCOMPARE(capture.did, 1);
            QCOMPARE(capture.exists, 0);
            QCOMPARE(mlt_properties_exists(MLT_FRAME_PROPERTIES(outer), "resize_alpha"), 0);
            QCOMPARE(mlt_properties_exists(MLT_FRAME_PROPERTIES(inner), "resize_alpha"), 0);
            mlt_frame_close(outer);
            mlt_frame_close(inner);
        }
        pass("resizeAlphaRoundTrip hint-missing");

        // Consumer-hint copy: an explicit 0 stays an explicit 0.
        {
            mlt_frame outer = mlt_frame_init(NULL);
            mlt_frame inner = mlt_frame_init(NULL);
            AlphaCapture capture = {0, -1, -1};
            YuvSource spec = {8, 8, 40, 0, &capture};
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(outer), "resize_alpha", 0);
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(inner), "resize_alpha", 255);
            attachSource(inner, &spec);
            QCOMPARE(mlt_frame_prepend_image_from_service(outer, inner), 0);
            CopiedImage image = fetch(outer, 8, 8);
            QCOMPARE(image.width, 8);
            QCOMPARE(capture.did, 1);
            QCOMPARE(capture.exists, 1);
            QCOMPARE(capture.value, 0);
            QCOMPARE(mlt_properties_get_int(MLT_FRAME_PROPERTIES(inner), "resize_alpha"), 0);
            QCOMPARE(mlt_properties_exists(MLT_FRAME_PROPERTIES(inner), "resize_alpha"), 1);
            mlt_frame_close(outer);
            mlt_frame_close(inner);
        }
        pass("resizeAlphaRoundTrip hint-explicit0");

        // Composite overwrites resize_alpha with 255 while unaligned, then restores it.
        mlt_transition composite = mlt_factory_transition(profile.get_profile(), "composite", NULL);
        QVERIFY(composite);
        mlt_properties properties = MLT_TRANSITION_PROPERTIES(composite);
        mlt_properties_set(properties, "geometry", "0/0:100%x100%:100");
        mlt_properties_set_int(properties, "aligned", 0);
        mlt_properties_set_int(properties, "progressive", 1);
        mlt_properties_set_int(properties, "in", 0);
        mlt_properties_set_int(properties, "out", 10);

        {
            mlt_frame a = mlt_frame_init(NULL);
            mlt_frame b = mlt_frame_init(NULL);
            AlphaCapture capture = {0, -1, -1};
            YuvSource dest = {16, 16, 40, 0, NULL};
            YuvSource src = {16, 16, 80, 0, &capture};
            attachSource(a, &dest);
            attachSource(b, &src);
            mlt_transition_process(composite, a, b);
            CopiedImage image = fetch(a, 16, 16);
            QCOMPARE(image.width, 16);
            QCOMPARE(capture.did, 1);
            QCOMPARE(capture.exists, 1);
            QCOMPARE(capture.value, 255);
            QCOMPARE(mlt_properties_exists(MLT_FRAME_PROPERTIES(b), "resize_alpha"), 0);
            mlt_frame_close(b);
            mlt_frame_close(a);
        }
        pass("resizeAlphaRoundTrip composite-missing");

        {
            mlt_frame a = mlt_frame_init(NULL);
            mlt_frame b = mlt_frame_init(NULL);
            AlphaCapture capture = {0, -1, -1};
            YuvSource dest = {16, 16, 40, 0, NULL};
            YuvSource src = {16, 16, 80, 0, &capture};
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(b), "resize_alpha", 0);
            attachSource(a, &dest);
            attachSource(b, &src);
            mlt_transition_process(composite, a, b);
            CopiedImage image = fetch(a, 16, 16);
            QCOMPARE(image.width, 16);
            QCOMPARE(capture.did, 1);
            QCOMPARE(capture.exists, 1);
            QCOMPARE(capture.value, 255);
            QCOMPARE(mlt_properties_exists(MLT_FRAME_PROPERTIES(b), "resize_alpha"), 1);
            QCOMPARE(mlt_properties_get_int(MLT_FRAME_PROPERTIES(b), "resize_alpha"), 0);
            mlt_frame_close(b);
            mlt_frame_close(a);
        }
        pass("resizeAlphaRoundTrip composite-explicit0");
        mlt_transition_close(composite);
    }

    void lumaEqualSize()
    {
        YuvSource dest = {16, 16, 40, 0, NULL};
        YuvSource src = {16, 16, 200, 0, NULL};

        CopiedImage dissolve = runLuma(&dest, &src, 16, 16, 0);
        QCOMPARE(dissolve.width, 16);
        QCOMPARE(dissolve.height, 16);
        QCOMPARE(dissolve.hasAlpha, 0);
        QCOMPARE(yAt(&dissolve, 0, 0), halfMix(40, 200));
        QCOMPARE(yAt(&dissolve, 15, 15), halfMix(40, 200));
        pass("lumaEqualSize dissolve");

        CopiedImage wipe = runLuma(&dest, &src, 16, 16, 1);
        QCOMPARE(wipe.width, 16);
        QCOMPARE(wipe.height, 16);
        QCOMPARE(wipe.hasAlpha, 0);
        QCOMPARE(yAt(&wipe, 0, 0), 200);
        QCOMPARE(yAt(&wipe, 15, 15), 200);
        pass("lumaEqualSize wipe");
    }

    void lumaMismatch()
    {
        // Dest 16x8 constant 40. Src 8x4, each row a distinct value. Overlap is 8x4.
        YuvSource dest = {16, 8, 40, 0, NULL};
        YuvSource src = {8, 4, 100, 1, NULL};

        CopiedImage dissolve = runLuma(&dest, &src, 16, 8, 0);
        QCOMPARE(dissolve.width, 16);
        QCOMPARE(dissolve.height, 8);
        for (int y = 0; y < 4; y++) {
            const int srcValue = 100 + y * 10;
            QCOMPARE(yAt(&dissolve, 0, y), halfMix(40, srcValue));
            QCOMPARE(yAt(&dissolve, 7, y), halfMix(40, srcValue));
            QCOMPARE(yAt(&dissolve, 8, y), 40);
            QCOMPARE(yAt(&dissolve, 15, y), 40);
        }
        for (int y = 4; y < 8; y++) {
            QCOMPARE(yAt(&dissolve, 0, y), 40);
            QCOMPARE(yAt(&dissolve, 15, y), 40);
        }
        pass("lumaMismatch dissolve");

        CopiedImage wipe = runLuma(&dest, &src, 16, 8, 1);
        QCOMPARE(wipe.width, 16);
        QCOMPARE(wipe.height, 8);
        for (int y = 0; y < 4; y++) {
            const int srcValue = 100 + y * 10;
            QCOMPARE(yAt(&wipe, 0, y), srcValue);
            QCOMPARE(yAt(&wipe, 7, y), srcValue);
            QCOMPARE(yAt(&wipe, 8, y), 40);
            QCOMPARE(yAt(&wipe, 15, y), 40);
        }
        for (int y = 4; y < 8; y++) {
            QCOMPARE(yAt(&wipe, 0, y), 40);
            QCOMPARE(yAt(&wipe, 15, y), 40);
        }
        pass("lumaMismatch wipe");
    }

    void concealForward()
    {
        avformat_conceal state;
        avformat_conceal_init(&state);
        QCOMPARE(state.owned, (mlt_frame) NULL);
        QCOMPARE(state.source_position, 0);

        mlt_cache cache = mlt_cache_init();
        QVERIFY(cache);
        mlt_cache_set_size(cache, 4);
        mlt_frame frame = makePicture(0x11, 5);
        mlt_cache_put_frame_image(cache, frame);
        avformat_conceal_record(&state, frame, cache, 10, 5);

        QCOMPARE(state.owned, (mlt_frame) NULL);
        QCOMPARE((int) state.cache_position, 5);
        mlt_frame fetched = avformat_conceal_fetch(&state, cache);
        QVERIFY(fetched);
        QCOMPARE(pictureMarker(fetched), 0x11);
        QCOMPARE(state.owned, (mlt_frame) NULL);
        pass("concealForward");

        mlt_frame_close(fetched);
        mlt_frame_close(frame);
        mlt_cache_close(cache);
        avformat_conceal_close(&state);
    }

    void concealDetach()
    {
        avformat_conceal state;
        avformat_conceal_init(&state);
        mlt_cache cache = mlt_cache_init();
        QVERIFY(cache);
        mlt_cache_set_size(cache, 2);

        mlt_frame good = makePicture(0x21, 3);
        mlt_cache_put_frame_image(cache, good);
        avformat_conceal_record(&state, good, cache, 10, 3);
        QCOMPARE(state.owned, (mlt_frame) NULL);

        avformat_conceal_detach(&state, cache);
        QVERIFY(state.owned);
        QCOMPARE(pictureMarker(state.owned), 0x21);

        mlt_frame otherA = makePicture(0x22, 4);
        mlt_frame otherB = makePicture(0x23, 5);
        mlt_cache_put_frame_image(cache, otherA);
        mlt_cache_put_frame_image(cache, otherB);
        mlt_frame dropped = mlt_cache_get_frame(cache, 3);
        QCOMPARE(dropped, (mlt_frame) NULL);

        mlt_frame fetched = avformat_conceal_fetch(&state, cache);
        QVERIFY(fetched);
        QCOMPARE(pictureMarker(fetched), 0x21);
        pass("concealDetach");

        mlt_frame_close(fetched);
        mlt_frame_close(otherB);
        mlt_frame_close(otherA);
        mlt_frame_close(good);
        mlt_cache_close(cache);
        avformat_conceal_close(&state);
    }

    void concealBackward()
    {
        avformat_conceal state;
        avformat_conceal_init(&state);
        mlt_cache cache = mlt_cache_init();
        QVERIFY(cache);
        mlt_cache_set_size(cache, 4);

        mlt_frame newer = makePicture(0x31, 6);
        mlt_cache_put_frame_image(cache, newer);
        avformat_conceal_record(&state, newer, cache, 10, 6);
        QCOMPARE(state.owned, (mlt_frame) NULL);

        mlt_frame older = makePicture(0x32, 7);
        mlt_cache_put_frame_image(cache, older);
        avformat_conceal_record(&state, older, cache, 4, 7);

        QCOMPARE(state.owned, (mlt_frame) NULL);
        QCOMPARE((int) state.cache_position, 6);
        QCOMPARE(state.source_position, 10);
        mlt_frame fetched = avformat_conceal_fetch(&state, cache);
        QVERIFY(fetched);
        QCOMPARE(pictureMarker(fetched), 0x31);
        pass("concealBackward");

        mlt_frame_close(fetched);
        mlt_frame_close(older);
        mlt_frame_close(newer);
        mlt_cache_close(cache);
        avformat_conceal_close(&state);
    }
};

QTEST_APPLESS_MAIN(TestDissolve)

#include "test_dissolve.moc"
