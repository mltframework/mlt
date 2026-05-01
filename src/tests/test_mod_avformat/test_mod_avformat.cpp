/*
 * Copyright (C) 2026 Julius Künzel <julius.kuenzel@kde.org>
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

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

class TestModAvformat : public QObject
{
    Q_OBJECT

public:
    TestModAvformat() { Factory::init(); }

    ~TestModAvformat() {}

private Q_SLOTS:
    void BasicAvformatProducer()
    {
        Profile profile;
        Producer producer(profile, "avformat:blue.mpg");
    }

    void BasicAvformatNovalidateProducer()
    {
        Profile profile;
        Producer producer(profile, "avformat-novalidate:blue.mpg");
    }

    void XmlRelativeRootIsRelativeToDocument()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QVERIFY(QDir(tempDir.path()).mkpath("subdir"));

        const QString xmlPath = tempDir.filePath("subdir/project.mlt");
        QFile xmlFile(xmlPath);
        QVERIFY(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text));
        xmlFile.write(R"(<?xml version="1.0"?>
<mlt root=".">
  <producer id="producer0">
    <property name="mlt_service">avformat-novalidate</property>
    <property name="resource">../media.mp4</property>
  </producer>
</mlt>
)");
        xmlFile.close();

        Profile profile;
        Producer producer(profile, "xml", xmlPath.toUtf8().constData());

        QVERIFY(producer.is_valid());
        QCOMPARE(QString::fromUtf8(producer.get("resource")),
                 QDir(tempDir.filePath("subdir")).filePath("./../media.mp4"));
    }
};

QTEST_APPLESS_MAIN(TestModAvformat)

#include "test_mod_avformat.moc"
