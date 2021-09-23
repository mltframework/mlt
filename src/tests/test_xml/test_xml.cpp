/*
 * Copyright (C) 2021  Meltytech, LLC
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

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestXml : public QObject
{
	Q_OBJECT

public:
	TestXml()
	{
		Factory::init();
	}

private Q_SLOTS:

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

		Properties* pchild1 = retProducer.get_props("child1");
		QVERIFY(pchild1 != nullptr);
		QVERIFY(pchild1->is_valid());

		Properties* pchild2 = pchild1->get_props("child2");
		QVERIFY(pchild2 != nullptr);
		QVERIFY(pchild2->is_valid());
		QCOMPARE(pchild2->get("test_param"), "C2");

		delete pchild1;
		delete pchild2;
	}
};

QTEST_APPLESS_MAIN(TestXml)

#include "test_xml.moc"
