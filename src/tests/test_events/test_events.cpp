/*
 * Copyright (C) 2019-2021 Meltytech, LLC
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

#include <QtTest>
#include <QString>

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestEvents : public QObject
{
    Q_OBJECT

private:
    mlt_properties m_properties;
    
public:
    TestEvents()
        : m_properties(nullptr)
    {
        Factory::init();
    }
    
    
private:

    void checkOwner(mlt_properties owner)
    {
        QVERIFY(owner == m_properties);
    }

    static void onPropertyChanged(mlt_properties owner, TestEvents* self, mlt_event_data data)
    {
        QVERIFY(self != nullptr);
        QCOMPARE(Mlt::EventData(data).to_string(), "foo");
        self->checkOwner(owner);
    }

private Q_SLOTS:
    
    void ListenToPropertyChanged()
    {
        Profile profile;
        Producer producer(profile, "noise");
        QVERIFY(producer.is_valid());
        Event* event = producer.listen("property-changed", this, (mlt_listener) onPropertyChanged);
        QVERIFY(event != nullptr);
        QVERIFY(event->is_valid());
        m_properties = producer.get_properties();
        producer.set("foo", 1);
        delete event;
    }
   
    void BlockEvent()
    {
        Profile profile;
        Producer producer(profile, "noise");
        m_properties = nullptr;
        Event* event = producer.listen("property-changed", nullptr, (mlt_listener) onPropertyChanged);
        QVERIFY(event != nullptr);
        QVERIFY(event->is_valid());
        event->block();
        producer.set("bar", 1);
        delete event;
    }
  
    void UnblockEvent()
    {
        Profile profile;
        Producer producer(profile, "noise");
        m_properties = producer.get_properties();
        Event* event = producer.listen("property-changed", this, (mlt_listener) onPropertyChanged);
        QVERIFY(event != nullptr);
        QVERIFY(event->is_valid());
        event->block();
        producer.set("bar", 1);
        event->unblock();
        producer.set("foo", 1);
        delete event;
    }
  
};

QTEST_APPLESS_MAIN(TestEvents)

#include "test_events.moc"
