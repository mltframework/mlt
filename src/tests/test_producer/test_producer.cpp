/*
 * Copyright (C) 2021-2026 Meltytech, LLC
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

#include <QString>
#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

#include <QString>
#include <QtTest>

#include <memory>
#include <mlt++/Mlt.h>
using namespace Mlt;

class TestProducer : public QObject
{
    Q_OBJECT

public:
    TestProducer() { Factory::init(); }

private Q_SLOTS:

    void DefaultConstructorIsInvalid()
    {
        Producer p;
        QVERIFY(!p.is_valid());
    }

    void CutIdentifiesAsProducer()
    {
        Profile profile;
        Producer producer(profile, "noise");
        QCOMPARE(producer.type(), mlt_service_producer_type);

        // Taken as a service, the identity is correct
        Mlt::Service *cutService = producer.cut(0, 1);
        QCOMPARE(cutService->type(), mlt_service_producer_type);

        // Promoted to a producer and the identity is correct
        Mlt::Producer cutProducer(*cutService);
        QCOMPARE(cutProducer.type(), mlt_service_producer_type);

        delete cutService;
    }

    void FractionalSpeedAdvances_data()
    {
        QTest::addColumn<double>("speed");
        QTest::addColumn<int>("start");
        QTest::addColumn<QList<int>>("positions");
        QTest::addColumn<bool>("previousNext");
        for (bool previousNext : {false, true}) {
            // Filters like the yadif deinterlacer make the service also fetch the previous and
            // next frames, which must not lose the fraction.
            const char *suffix = previousNext ? " with previous and next" : "";
            QTest::newRow(QByteArray("one and a half").append(suffix))
                << 1.5 << 0 << QList<int>{0, 1, 3, 4, 6} << previousNext;
            QTest::newRow(QByteArray("half").append(suffix))
                << 0.5 << 0 << QList<int>{0, 0, 1, 1, 2} << previousNext;
            QTest::newRow(QByteArray("double").append(suffix))
                << 2.0 << 0 << QList<int>{0, 2, 4, 6, 8} << previousNext;
            QTest::newRow(QByteArray("minus one and a half").append(suffix))
                << -1.5 << 10 << QList<int>{10, 8, 7, 5, 4} << previousNext;
        }
    }

    void FractionalSpeedAdvances()
    {
        QFETCH(double, speed);
        QFETCH(int, start);
        QFETCH(QList<int>, positions);
        QFETCH(bool, previousNext);
        Profile profile("dv_pal");
        Producer producer(profile, "noise");
        QVERIFY(producer.is_valid());
        producer.set("_need_previous_next", previousNext);
        producer.seek(start);
        producer.set_speed(speed);
        for (int expected : positions) {
            QCOMPARE(int(producer.position()), expected);
            std::unique_ptr<Frame> frame(producer.get_frame());
        }
    }
};

QTEST_APPLESS_MAIN(TestProducer)

#include "test_producer.moc"
