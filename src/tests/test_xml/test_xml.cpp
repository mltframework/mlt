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

#include <cstring>

#include <mlt++/Mlt.h>
using namespace Mlt;

#include <QString>
#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestXml : public QObject
{
    Q_OBJECT

public:
    TestXml() { Factory::init(); }

private:
    static Filter *findFilter(Producer &producer, const char *id)
    {
        for (int i = 0; i < producer.filter_count(); i++) {
            Filter *filter = producer.filter(i);
            if (filter && QString::fromUtf8(filter->get("mlt_service")) == id)
                return filter;
            delete filter;
        }
        return nullptr;
    }

private Q_SLOTS:

    void VulkanConsumerPreflight_data()
    {
        QTest::addColumn<QString>("consumer");
        QTest::addColumn<bool>("afterSource");
        QTest::addColumn<bool>("expected");
        QTest::newRow("attribute")
            << QStringLiteral("<consumer mlt_service='null' mlt_image_format='vulkan'/>") << false
            << true;
        QTest::newRow("consumer-after-source")
            << QStringLiteral("<consumer mlt_service='null' mlt_image_format='vulkan'/>") << true
            << true;
        QTest::newRow("multi-consumer")
            << QStringLiteral("<consumer mlt_service='null'/><consumer "
                              "mlt_service='null' mlt_image_format='vulkan'/>")
            << true << true;
        QTest::newRow("software-format")
            << QStringLiteral("<consumer mlt_service='null' mlt_image_format='yuv420p' "
                              "vcodec='hevc_vulkan'/>")
            << false << false;
        QTest::newRow("hardware-decode-only")
            << QStringLiteral("<consumer mlt_service='null'><property name='hwaccel'>vulkan"
                              "</property></consumer>")
            << false << false;
    }

    void VulkanConsumerPreflight()
    {
        QFETCH(QString, consumer);
        QFETCH(bool, afterSource);
        QFETCH(bool, expected);
        struct PreflightState
        {
            int renderRequests = 0;
            int requestsBeforeSource = -1;
        } state;
        mlt_properties events = mlt_factory_event_object();
        const auto disconnect = qScopeGuard([&]() { mlt_events_disconnect(events, &state); });
        mlt_events_listen(events,
                          &state,
                          "filter-create-request",
                          [](mlt_properties, void *opaque, mlt_event_data event) {
                              auto &state = *static_cast<PreflightState *>(opaque);
                              auto data = static_cast<mlt_factory_event_data *>(
                                  mlt_event_data_to_object(event));
                              if (!strcmp(data->name, "placebo.render")) {
                                  ++state.renderRequests;
                                  *static_cast<mlt_filter *>(data->service) = mlt_filter_new();
                              }
                          });
        mlt_events_listen(events,
                          &state,
                          "producer-create-request",
                          [](mlt_properties, void *opaque, mlt_event_data event) {
                              auto &state = *static_cast<PreflightState *>(opaque);
                              auto data = static_cast<mlt_factory_event_data *>(
                                  mlt_event_data_to_object(event));
                              if (!strcmp(data->name, "colour"))
                                  state.requestsBeforeSource = state.renderRequests;
                          });
        const QString source = "<producer mlt_service='colour'><property name='resource'>red"
                               "</property></producer>";
        const QByteArray xml
            = ("<mlt>" + (afterSource ? source + consumer : consumer + source) + "</mlt>").toUtf8();
        Profile profile;
        Producer producer(profile, "xml-string", xml.constData());
        QVERIFY(producer.is_valid());
        QCOMPARE(state.renderRequests, expected ? 1 : 0);
        QCOMPARE(state.requestsBeforeSource, expected ? 1 : 0);
    }

    void NestedPropertiesRoundTrip()
    {
        // Create a producer
        Profile profile;
        Producer producer(profile, "noise");

        // Nest properties two deep
        Properties child1;
        producer.set("child1", child1);
        Properties child2;
        child2.set("test_param", "C2");
        child1.set("child2", child2);

        // Convert to XML
        Consumer c(profile, "xml", "string");
        c.connect(producer);
        c.start();

        Producer retProducer(profile, "xml-string", c.get("string"));
        QCOMPARE(retProducer.get("mlt_service"), "noise");

        Properties *pchild1 = retProducer.get_props("child1");
        QVERIFY(pchild1 != nullptr);
        QVERIFY(pchild1->is_valid());

        Properties *pchild2 = pchild1->get_props("child2");
        QVERIFY(pchild2 != nullptr);
        QVERIFY(pchild2->is_valid());
        QCOMPARE(pchild2->get("test_param"), "C2");

        delete pchild1;
        delete pchild2;
    }

    void InvalidProducerLookupDoesNotCrash()
    {
        Profile profile;
        const char *xml = R"(<?xml version="1.0" encoding="utf-8"?>
<mlt>
  <producer mlt_service="missing-producer"/>
</mlt>
)";

        Producer producer(profile, "xml-string", xml);

        QVERIFY(producer.is_valid());
        QVERIFY(producer.get_producer() != nullptr);
    }

    void InvalidFilterTransitionAndLinkLookupsDoNotCrash()
    {
        Profile profile;
        const char *xml = R"(<?xml version="1.0" encoding="utf-8"?>
<mlt>
  <producer id="track0" mlt_service="colour:red"/>
  <chain id="track1">
    <property name="resource">colour:blue</property>
    <link mlt_service="missing-link"/>
  </chain>
  <playlist>
    <entry>
      <multitrack>
        <track producer="track0"/>
        <track producer="track1"/>
      </multitrack>
      <filter mlt_service="missing-filter"/>
      <transition a_track="0" b_track="1" mlt_service="missing-transition"/>
    </entry>
  </playlist>
</mlt>
)";

        Producer producer(profile, "xml-string", xml);

        QVERIFY(producer.is_valid());
        QVERIFY(producer.get_producer() != nullptr);
    }

    void PlaceboShaderResourceIsQualified()
    {
        if (!mlt_properties_exists(mlt_repository_filters(mlt_factory_repository()),
                                   "placebo.shader"))
            QSKIP("Placebo module is unavailable");
        Profile profile;
        const char *xml = R"(<mlt root="/shader-test">
  <producer mlt_service="colour">
    <property name="resource">red</property>
    <filter mlt_service="placebo.shader">
      <property name="resource">blur.hook</property>
      <property name="param.sigma">0=1;10=5</property>
    </filter>
  </producer>
</mlt>)";
        Producer producer(profile, "xml-string", xml);
        QVERIFY(producer.is_valid());
        Filter *filter = findFilter(producer, "placebo.shader");
        QVERIFY(filter);
        QVERIFY(filter->is_valid());
        QCOMPARE(QString::fromUtf8(filter->get("resource")), QString("/shader-test/blur.hook"));
        QCOMPARE(QString::fromUtf8(filter->get("param.sigma")), QString("0=1;10=5"));
        delete filter;
    }

    void PlaceboNamedShaderRoundTrip()
    {
        if (!mlt_properties_exists(mlt_repository_filters(mlt_factory_repository()),
                                   "placebo.shader"))
            QSKIP("Placebo module is unavailable");
        Profile profile;
        Producer producer(profile, "colour", "red");
        Filter filter(profile, "placebo.unsharp_mask");
        QVERIFY(filter.is_valid());
        QString resource = QString::fromUtf8(filter.get("resource"));
        filter.set("param.amount", "0=0.5;10=2");
        producer.attach(filter);
        Consumer consumer(profile, "xml", "string");
        consumer.connect(producer);
        consumer.start();

        Producer restored(profile, "xml-string", consumer.get("string"));
        QVERIFY(restored.is_valid());
        Filter *restoredFilter = findFilter(restored, "placebo.unsharp_mask");
        QVERIFY(restoredFilter);
        QVERIFY(restoredFilter->is_valid());
        QCOMPARE(QString::fromUtf8(restoredFilter->get("mlt_service")),
                 QString("placebo.unsharp_mask"));
        QCOMPARE(QString::fromUtf8(restoredFilter->get("resource")), resource);
        QCOMPARE(QString::fromUtf8(restoredFilter->get("param.amount")), QString("0=0.5;10=2"));
        delete restoredFilter;
    }
};

QTEST_APPLESS_MAIN(TestXml)

#include "test_xml.moc"
