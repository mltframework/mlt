/*
 * Copyright (C) 2025 Julius Künzel <julius.kuenzel@kde.org>
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

#include <modules/qt/gps_parser.h>

extern int64_t datetimeXMLstring_to_mseconds(const char *text, char *format);

class TestModQt : public QObject
{
    Q_OBJECT

public:
    TestModQt() {}

    ~TestModQt() {}

private Q_SLOTS:
    void DatetimeXMLstring_to_mseconds_ValidConversion()
    {
        int64_t gps_proc_t = 0;

        // ISO 8601 extended date with timezone info Z
        const char *text = "2020-07-11T09:03:23.000Z";
        gps_proc_t = datetimeXMLstring_to_mseconds(text);
        QCOMPARE(gps_proc_t, 1594458203000);

        // ISO 8601 extended date with timezone offset
        text = "2021-02-27T12:10:00+00:00";
        gps_proc_t = datetimeXMLstring_to_mseconds(text);
        QCOMPARE(gps_proc_t, 1614427800000);

        // ISO 8601 extended date with ms and timezone offset
        text = "2020-07-11T09:03:23.000+00:00";
        gps_proc_t = datetimeXMLstring_to_mseconds(text);
        QCOMPARE(gps_proc_t, 1594458203000);

        // ISO 8601 extended date without timezone info
        text = "2020-07-11T09:03:23.000";
        gps_proc_t = datetimeXMLstring_to_mseconds(text);
        QCOMPARE(gps_proc_t, 1594458203000);
    }

    void DatetimeXMLstring_to_mseconds_FormatString()
    {
        int64_t gps_proc_t = 0;

        // Unknown input, with format string as hint
        const char *text = "12:10:00 27-02-2021";
        gps_proc_t = datetimeXMLstring_to_mseconds(text, (char *) "HH:mm:ss dd-MM-yyyy");
        QCOMPARE(gps_proc_t, 1614427800000);

        // Unknown input, without format string
        text = "12:10:00 27-02-2021";
        gps_proc_t = datetimeXMLstring_to_mseconds(text);
        QCOMPARE(gps_proc_t, 0);

        // Valid input, but mismatching format string
        text = "2021-02-27T12:10:00+00:00";
        gps_proc_t = datetimeXMLstring_to_mseconds(text, (char *) "HH:mm:ss dd-MM-yyyy");
        QCOMPARE(gps_proc_t, 0);
    }

    void DatetimeXMLstring_to_mseconds_InvalidInput()
    {
        int64_t gps_proc_t = 0;

        // Invalid input, without format string
        const char *text = "invalid-date-format";
        gps_proc_t = datetimeXMLstring_to_mseconds(text);
        QCOMPARE(gps_proc_t, 0);

        // Invalid input, with format string
        gps_proc_t = datetimeXMLstring_to_mseconds(text, (char *) "%Y-%m-%dT%H:%M:%S");
        QCOMPARE(gps_proc_t, 0);
    }
};

QTEST_APPLESS_MAIN(TestModQt)

#include "test_mod_qt_gps.moc"
