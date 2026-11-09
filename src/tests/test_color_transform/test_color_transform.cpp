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

#include <mlt++/Mlt.h>
#include <QtTest>

using namespace Mlt;

namespace {

struct Pixel
{
    int r;
    int g;
    int b;
    int a;
};

} // namespace

class TestColorTransform : public QObject
{
    Q_OBJECT
    Profile profile;

public:
    TestColorTransform()
    {
        Factory::init();
        profile.set_width(8);
        profile.set_height(8);
        profile.set_progressive(1);
        profile.set_colorspace(709);
        profile.set_sample_aspect(1, 1);
        profile.set_display_aspect(1, 1);
        profile.set_explicit(1);
    }

private:
    bool available()
    {
        Transition probe(profile, "placebo.blend");
        return probe.is_valid();
    }

    Pixel sample(const char *color, bool linear, bool roundTrip)
    {
        Pixel out = {-1, -1, -1, -1};
        Producer producer(profile, "loader", color);
        if (!producer.is_valid())
            return out;
        Frame *frame = producer.get_frame();
        if (!frame)
            return out;
        if (linear)
            frame->set("consumer.mlt_color_trc", "linear");

        Filter encode(profile, "color_transform");
        if (roundTrip) {
            if (!encode.is_valid()) {
                delete frame;
                return out;
            }
            encode.set("force_trc", "bt709");
            encode.process(*frame);
        }

        mlt_image_format format = mlt_image_rgba;
        int width = 0;
        int height = 0;
        uint8_t *image = frame->get_image(format, width, height, 0);
        if (image && format == mlt_image_rgba && width > 0 && height > 0) {
            out.r = image[0];
            out.g = image[1];
            out.b = image[2];
            out.a = image[3];
        }
        delete frame;
        return out;
    }

    void expect(Pixel got, Pixel want, int tolerance, const char *label)
    {
        const bool close = qAbs(got.r - want.r) <= tolerance && qAbs(got.g - want.g) <= tolerance
                           && qAbs(got.b - want.b) <= tolerance
                           && qAbs(got.a - want.a) <= tolerance;
        QVERIFY2(close,
                 qPrintable(QString("%1 got (%2,%3,%4,%5) want (%6,%7,%8,%9) ±%10")
                                .arg(label)
                                .arg(got.r)
                                .arg(got.g)
                                .arg(got.b)
                                .arg(got.a)
                                .arg(want.r)
                                .arg(want.g)
                                .arg(want.b)
                                .arg(want.a)
                                .arg(tolerance)));
    }

private Q_SLOTS:
    void AlphaSurvivesLinear()
    {
        if (!available())
            QSKIP("libplacebo is unavailable");
        const int alphas[] = {0, 32, 128, 255};
        for (int alpha : alphas) {
            const QString color = QString("color:0xff0000%1").arg(alpha, 2, 16, QChar('0'));
            const Pixel got = sample(qPrintable(color), true, false);
            QVERIFY2(qAbs(got.a - alpha) <= 1,
                     qPrintable(QString("alpha %1 became %2 (rgb %3,%4,%5)")
                                    .arg(alpha)
                                    .arg(got.a)
                                    .arg(got.r)
                                    .arg(got.g)
                                    .arg(got.b)));
            if (alpha == 0) {
                QCOMPARE(got.r, 0);
                QCOMPARE(got.g, 0);
                QCOMPARE(got.b, 0);
            } else {
                QVERIFY2(got.r >= 250 && got.g <= 2 && got.b <= 2,
                         qPrintable(QString("red coverage %1 linearized to (%2,%3,%4,%5)")
                                        .arg(alpha)
                                        .arg(got.r)
                                        .arg(got.g)
                                        .arg(got.b)
                                        .arg(got.a)));
            }
        }
    }

    void EndpointsStayPut()
    {
        if (!available())
            QSKIP("libplacebo is unavailable");
        expect(sample("color:0x000000ff", true, false), {0, 0, 0, 255}, 1, "black");
        expect(sample("color:0xffffffff", true, false), {255, 255, 255, 255}, 1, "white");
    }

    void RampIncreases()
    {
        if (!available())
            QSKIP("libplacebo is unavailable");
        const int encoded[] = {0, 64, 128, 192, 255};
        int previous = -1;
        for (int value : encoded) {
            const QString color = QString("color:0x%1%1%1ff").arg(value, 2, 16, QChar('0'));
            const Pixel got = sample(qPrintable(color), true, false);
            QVERIFY2(got.r >= 0, qPrintable(QString("no pixel for %1").arg(value)));
            QVERIFY2(got.r > previous,
                     qPrintable(QString("linear %1 -> %2 did not increase past %3")
                                    .arg(value)
                                    .arg(got.r)
                                    .arg(previous)));
            QVERIFY(qAbs(got.r - got.g) <= 1);
            QVERIFY(qAbs(got.r - got.b) <= 1);
            QCOMPARE(got.a, 255);
            previous = got.r;
        }
    }

    void MidGrayDarkensAndRoundTrips()
    {
        if (!available())
            QSKIP("libplacebo is unavailable");
        const Pixel linear = sample("color:0x808080ff", true, false);
        QVERIFY2(linear.r > 20 && linear.r < 100,
                 qPrintable(QString("bt709 128 linearized to %1,%2,%3")
                                .arg(linear.r)
                                .arg(linear.g)
                                .arg(linear.b)));
        expect(sample("color:0x808080ff", true, true), {128, 128, 128, 255}, 3, "round trip");
    }
};

QTEST_APPLESS_MAIN(TestColorTransform)

#include "test_color_transform.moc"
