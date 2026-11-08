/*
 * Copyright (C) 2013-2026 Meltytech, LLC
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

class TestRepository : public QObject
{
    Q_OBJECT

public:
    TestRepository() {}

private Q_SLOTS:
    void ThereAreProducers()
    {
        Repository *r = Factory::init();
        Properties *producers = r->producers();
        QVERIFY(producers->is_valid());
        if (producers->is_valid())
            QVERIFY(producers->count() > 0);
        delete producers;
    }

    void ThereAreConsumers()
    {
        Repository *r = Factory::init();
        Properties *consumers = r->consumers();
        QVERIFY(consumers->is_valid());
        if (consumers->is_valid())
            QVERIFY(consumers->count() > 0);
        delete consumers;
    }

    void PlaceboShaders_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("parameterCount");
        QTest::newRow("gaussian blur") << QString("gaussian_blur") << 2;
        QTest::newRow("unsharp mask") << QString("unsharp_mask") << 4;
    }

    void PlaceboShaders()
    {
        QFETCH(QString, name);
        QFETCH(int, parameterCount);
        Repository *repository = Factory::init();
        mlt_properties filters = mlt_repository_filters(mlt_factory_repository());
        if (!mlt_properties_exists(filters, "placebo.shader"))
            QSKIP("Placebo module is unavailable");

        QByteArray id = ("placebo." + name).toUtf8();
        QVERIFY(mlt_properties_exists(filters, id.constData()));
        Properties *metadata = repository->metadata(mlt_service_filter_type, id.constData());
        QVERIFY(metadata);
        QVERIFY(metadata->is_valid());
        QCOMPARE(QString::fromUtf8(metadata->get("identifier")), QString::fromUtf8(id));
        mlt_properties parameters = static_cast<mlt_properties>(metadata->get_data("parameters"));
        QVERIFY(parameters);
        QCOMPARE(mlt_properties_count(parameters), parameterCount);
        for (int i = 0; i < mlt_properties_count(parameters); i++) {
            mlt_properties parameter = static_cast<mlt_properties>(
                mlt_properties_get_data_at(parameters, i, nullptr));
            QVERIFY(parameter);
            QVERIFY(
                QString::fromUtf8(mlt_properties_get(parameter, "identifier")).startsWith("param."));
            QCOMPARE(QString::fromUtf8(mlt_properties_get(parameter, "animation")), QString("yes"));
        }
        delete metadata;

        Profile profile;
        Filter filter(profile, id.constData());
        QVERIFY(filter.is_valid());
        QString resource = QString::fromUtf8(filter.get("resource"));
        QVERIFY(QFileInfo::exists(resource));
        QVERIFY(resource.endsWith("/" + name + ".hook"));
        QVERIFY(!filter.property_exists("shader_path"));

        Filter custom(profile, "placebo.shader", resource.toUtf8().constData());
        QVERIFY(custom.is_valid());
        QCOMPARE(QString::fromUtf8(custom.get("resource")), resource);
        Filter override(profile, id.constData(), resource.toUtf8().constData());
        QVERIFY(override.is_valid());
        QCOMPARE(QString::fromUtf8(override.get("resource")), resource);
    }
};

QTEST_APPLESS_MAIN(TestRepository)

#include "test_repository.moc"
