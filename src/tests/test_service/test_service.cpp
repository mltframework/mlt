/*
 * Copyright (C) 2019 Meltytech, LLC
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
#include <mlt++/Mlt.h>
using namespace Mlt;

class TestService : public QObject
{
    Q_OBJECT
    
public:
    TestService()
    {
        repo = Factory::init();
    }

    ~TestService()
    {
        Factory::close();
    }
    
private Q_SLOTS:
    void CanIdentifyServicesFromFactory()
    {
        Profile profile;
        Producer producer(profile, "color");
        QCOMPARE(mlt_service_identify(producer.get_service()), mlt_service_producer_type);
        Filter filter(profile, "resize");
        QCOMPARE(mlt_service_identify(filter.get_service()), mlt_service_filter_type);
        Transition transition(profile, "mix");
        QCOMPARE(mlt_service_identify(transition.get_service()), mlt_service_transition_type);
        Consumer consumer(profile, "null");
        QCOMPARE(mlt_service_identify(consumer.get_service()), mlt_service_consumer_type);
    }
    
    void CanIdentifyServicesFromAPI()
    {
        mlt_profile profile = mlt_profile_init(NULL);

        mlt_playlist playlist = mlt_playlist_new(profile);
        QCOMPARE(mlt_service_identify(MLT_PLAYLIST_SERVICE(playlist)), mlt_service_playlist_type);
        mlt_tractor tractor = mlt_tractor_new();
        QCOMPARE(mlt_service_identify(MLT_TRACTOR_SERVICE(tractor)), mlt_service_tractor_type);
        QCOMPARE(mlt_service_identify(MLT_MULTITRACK_SERVICE(mlt_tractor_multitrack(tractor))), mlt_service_multitrack_type);

        mlt_producer producer = mlt_producer_new(profile);
        QCOMPARE(mlt_service_identify(MLT_PRODUCER_SERVICE(producer)), mlt_service_producer_type);
        mlt_filter filter = mlt_filter_new();
        QCOMPARE(mlt_service_identify(MLT_FILTER_SERVICE(filter)), mlt_service_filter_type);
        mlt_transition transition = mlt_transition_new();
        QCOMPARE(mlt_service_identify(MLT_TRANSITION_SERVICE(transition)), mlt_service_transition_type);
        mlt_consumer consumer = mlt_consumer_new(profile);
        QCOMPARE(mlt_service_identify(MLT_CONSUMER_SERVICE(consumer)), mlt_service_consumer_type);
    }

private:
    Repository* repo;
};

QTEST_APPLESS_MAIN(TestService)

#include "test_service.moc"
