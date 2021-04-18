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

class TestProducer : public QObject
{
	Q_OBJECT

public:
	TestProducer()
	{
		Factory::init();
	}

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
		Mlt::Service* cutService = producer.cut(0, 1);
		QCOMPARE(cutService->type(), mlt_service_producer_type);

		// Promoted to a producer and the identity is correct
		Mlt::Producer cutProducer(*cutService);
		QCOMPARE(cutProducer.type(), mlt_service_producer_type);

		delete cutService;
	}
};

QTEST_APPLESS_MAIN(TestProducer)

#include "test_producer.moc"
