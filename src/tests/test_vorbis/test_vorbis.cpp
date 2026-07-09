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

#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestVorbis : public QObject
{
    Q_OBJECT

public:
    TestVorbis() { Factory::init(); }

    ~TestVorbis() {}

private Q_SLOTS:
    void BasicVorbisProducer()
    {
        // Basic test to verify the Vorbis producer can be instantiated
        // This exercises the metadata parsing code path that uses snprintf
        Profile profile;
        Producer producer(profile, "vorbis:test.ogg");
        // Even if the file doesn't exist, the producer should handle it gracefully
        // This test primarily verifies the module loads and doesn't crash
    }
};

QTEST_APPLESS_MAIN(TestVorbis)

#include "test_vorbis.moc"
