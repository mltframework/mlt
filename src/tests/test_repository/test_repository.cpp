/*
 * Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
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
        Repository* r = Factory::init();
        Properties* producers = r->producers();
        QVERIFY(producers->is_valid());
        if (producers->is_valid())
            QVERIFY(producers->count() > 0);
        delete producers;
    }

    void ThereAreConsumers()
    {
        Repository* r = Factory::init();
        Properties* consumers = r->consumers();
        QVERIFY(consumers->is_valid());
        if (consumers->is_valid())
            QVERIFY(consumers->count() > 0);
        delete consumers;
    }
};

QTEST_APPLESS_MAIN(TestRepository)

#include "test_repository.moc"
