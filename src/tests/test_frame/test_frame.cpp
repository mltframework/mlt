/*
 * Copyright (C) 2015-2026 Meltytech, LLC
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

#include <framework/mlt.h>
#include <mlt++/Mlt.h>
#include <QtTest>
using namespace Mlt;

// --- helpers for convert_image tests ---

static int g_convert_calls = 0;
static int g_convert_b_calls = 0;

// A converter that always succeeds (returns 0).
static int convert_always(mlt_frame, uint8_t **, mlt_image_format *format, mlt_image_format output)
{
    ++g_convert_calls;
    *format = output;
    return 0;
}

// A converter that always fails (returns 1), letting the next one try.
static int convert_fail(mlt_frame frame,
                        uint8_t **image,
                        mlt_image_format *format,
                        mlt_image_format output)
{
    ++g_convert_calls;
    return mlt_frame_next_convert_image(frame, image, format, output);
}

// A second succeed converter used to verify fallback ordering.
static int convert_b(mlt_frame, uint8_t **, mlt_image_format *format, mlt_image_format output)
{
    ++g_convert_b_calls;
    *format = output;
    return 0;
}

class TestFrame : public QObject
{
    Q_OBJECT

public:
    TestFrame() {}

private Q_SLOTS:
    void FrameConstructorAddsReference()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        QCOMPARE(mlt_properties_ref_count(MLT_FRAME_PROPERTIES(frame)), 1);
        Frame f(frame);
        QCOMPARE(f.ref_count(), 2);
        mlt_frame_close(frame);
    }

    void CopyConstructorAddsReference()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        QCOMPARE(mlt_properties_ref_count(MLT_FRAME_PROPERTIES(frame)), 1);
        Frame f1(frame);
        QCOMPARE(f1.ref_count(), 2);
        Frame f2(f1);
        QCOMPARE(f1.ref_count(), 3);
        mlt_frame_close(frame);
    }

    void ConstCopyConstructorAddsReference()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        QCOMPARE(mlt_properties_ref_count(MLT_FRAME_PROPERTIES(frame)), 1);
        Frame f1(frame);
        QCOMPARE(f1.ref_count(), 2);
        const Frame &cf1 = f1; // Force const to avoid non-const constructor.
        Frame f2(cf1);
        QCOMPARE(f1.ref_count(), 3);
        QCOMPARE(f2.ref_count(), 3);
        mlt_frame_close(frame);
    }

    void DefaultConstructorIsNotValid()
    {
        Frame f1;
        QCOMPARE(f1.is_valid(), false);
        QCOMPARE(f1.ref_count(), 0);
    }

    void OperatorEqualsAddsReference()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        QCOMPARE(mlt_properties_ref_count(MLT_FRAME_PROPERTIES(frame)), 1);
        Frame f1(frame);
        QCOMPARE(f1.ref_count(), 2);
        Frame f2;
        f2 = f1;
        QCOMPARE(f1.ref_count(), 3);
        QCOMPARE(f2.ref_count(), 3);
        mlt_frame_close(frame);
    }

    void DestructionRemovesReference()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        QCOMPARE(mlt_properties_ref_count(MLT_FRAME_PROPERTIES(frame)), 1);
        Frame f1(frame);
        QCOMPARE(f1.ref_count(), 2);
        Frame *f2 = new Frame(f1);
        QCOMPARE(f2->ref_count(), 3);
        delete f2;
        QCOMPARE(f1.ref_count(), 2);
        mlt_frame_close(frame);
    }

    // --- convert_image dispatch tests ---

    void ConvertImageShimSetOnInit()
    {
        // mlt_frame_init must install the shim so the field is never NULL.
        mlt_frame frame = mlt_frame_init(NULL);
        QVERIFY(frame->convert_image == mlt_frame_convert_image);
        mlt_frame_close(frame);
    }

    void HasConvertImageFalseWhenEmpty()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        QCOMPARE(mlt_frame_has_convert_image(frame), 0);
        mlt_frame_close(frame);
    }

    void HasConvertImageTrueAfterAppend()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(frame, convert_always);
        QCOMPARE(mlt_frame_has_convert_image(frame), 1);
        mlt_frame_close(frame);
    }

    void AppendConvertImageDeduplicate()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(frame, convert_always);
        mlt_frame_append_convert_image(frame, convert_always); // duplicate — must be ignored
        // Invoke the dispatcher: a single converter should be called exactly once.
        g_convert_calls = 0;
        mlt_image_format fmt = mlt_image_rgb;
        mlt_frame_convert_image(frame, NULL, &fmt, mlt_image_rgba);
        QCOMPARE(g_convert_calls, 1);
        mlt_frame_close(frame);
    }

    void HasConvertImageTrueAfterPrepend()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        mlt_frame_prepend_convert_image(frame, convert_always);
        QCOMPARE(mlt_frame_has_convert_image(frame), 1);
        mlt_frame_close(frame);
    }

    void PrependConvertImageDeduplicate()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        mlt_frame_prepend_convert_image(frame, convert_always);
        mlt_frame_prepend_convert_image(frame, convert_always); // duplicate — must be ignored
        // Invoke the dispatcher: a single converter should be called exactly once.
        g_convert_calls = 0;
        mlt_image_format fmt = mlt_image_rgb;
        mlt_frame_convert_image(frame, NULL, &fmt, mlt_image_rgba);
        QCOMPARE(g_convert_calls, 1);
        mlt_frame_close(frame);
    }

    void ConvertImageFirstSuccessWins()
    {
        // fail → b: only b should run and succeed; fail tries next via mlt_frame_next_convert_image.
        mlt_frame frame = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(frame, convert_fail);
        mlt_frame_append_convert_image(frame, convert_b);
        g_convert_calls = 0;
        g_convert_b_calls = 0;
        mlt_image_format fmt = mlt_image_rgb;
        int error = mlt_frame_convert_image(frame, NULL, &fmt, mlt_image_rgba);
        QCOMPARE(error, 0);
        QCOMPARE(g_convert_calls, 1);   // convert_fail was called once
        QCOMPARE(g_convert_b_calls, 1); // convert_b succeeded
        QCOMPARE(fmt, mlt_image_rgba);
        mlt_frame_close(frame);
    }

    void ConvertImageAllFailReturnsError()
    {
        mlt_frame frame = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(frame, convert_fail);
        g_convert_calls = 0;
        mlt_image_format fmt = mlt_image_rgb;
        int error = mlt_frame_convert_image(frame, NULL, &fmt, mlt_image_rgba);
        QCOMPARE(error, 1); // no converter succeeded
        mlt_frame_close(frame);
    }

    void CopyConvertImagePropagatesToClone()
    {
        mlt_frame src = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(src, convert_always);
        mlt_frame dst = mlt_frame_clone(src, 0);
        QCOMPARE(mlt_frame_has_convert_image(dst), 1);
        g_convert_calls = 0;
        mlt_image_format fmt = mlt_image_rgb;
        int error = mlt_frame_convert_image(dst, NULL, &fmt, mlt_image_rgba);
        QCOMPARE(error, 0);
        QCOMPARE(g_convert_calls, 1);
        mlt_frame_close(dst);
        mlt_frame_close(src);
    }

    void CopyConvertImageDoesNotCopyTwice()
    {
        // mlt_frame_copy_convert_image() must not overwrite an existing list on dst.
        mlt_frame src = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(src, convert_always);
        mlt_frame dst = mlt_frame_init(NULL);
        mlt_frame_append_convert_image(dst, convert_b);
        mlt_frame_copy_convert_image(dst, src);
        // dst already had a list — its own converter (convert_b) must still be there.
        g_convert_calls = 0;
        g_convert_b_calls = 0;
        mlt_image_format fmt = mlt_image_rgb;
        mlt_frame_convert_image(dst, NULL, &fmt, mlt_image_rgba);
        QCOMPARE(g_convert_b_calls, 1); // dst's original converter ran
        QCOMPARE(g_convert_calls, 0);   // src's converter was NOT copied over
        mlt_frame_close(dst);
        mlt_frame_close(src);
    }
};

QTEST_APPLESS_MAIN(TestFrame)

#include "test_frame.moc"
