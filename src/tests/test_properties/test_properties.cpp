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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <QString>
#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

extern "C" {
#define __DARWIN__
#include <framework/mlt_property.h>
#include <framework/mlt_animation.h>
}

class TestProperties: public QObject
{
    Q_OBJECT

public:
    TestProperties() {}

private Q_SLOTS:
    void InstantiationIsAReference()
    {
        Properties p;
        QCOMPARE(p.ref_count(), 1);
    }

    void CopyAddsReference()
    {
        Properties p;
        Properties q = p;
        QCOMPARE(p.ref_count(), 2);
    }

    void DestructionRemovesReference()
    {
        Properties p;
        Properties* q = new Properties(p);
        QCOMPARE(p.ref_count(), 2);
        delete q;
        QCOMPARE(p.ref_count(), 1);
    }

    void SetAndGetString()
    {
        Properties p;
        p.set("key", "value");
        QVERIFY(p.get("key"));
        QVERIFY(QString(p.get("key")) != QString(""));
        QCOMPARE(p.get("key"), "value");
    }

    void SetAndGetInt()
    {
        Properties p;
        int i = 1;
        p.set("key", i);
        QCOMPARE(p.get_int("key"), i);
    }

    void SetAndGetDouble()
    {
        Properties p;
        double d = 1.0;
        p.set("key", d);
        QCOMPARE(p.get_double("key"), d);
    }

    void SetAndGetInt64()
    {
        Properties p;
        int64_t i = 1LL << 32;
        p.set("key", i);
        QCOMPARE(p.get_int64("key"), i);
    }

    void SetAndGetData()
    {
        Properties p;
        const char *value = "value";
        char* const s = strdup(value);
        p.set("key", s, strlen(s), free);
        int size = 0;
        QCOMPARE((char*) p.get_data("key", size), value);
        QCOMPARE(size, int(strlen(value)));
    }

    void IntFromString()
    {
        Properties p;
        const char *s = "-1";
        int i = -1;
        p.set("key", i);
        QCOMPARE(p.get("key"), s);
        p.set("key", s);
        QCOMPARE(p.get_int("key"), i);
    }

    void Int64FromString()
    {
        Properties p;
        const char *s = "-1";
        int64_t i = -1;
        p.set("key", i);
        QCOMPARE(p.get("key"), s);
        p.set("key", s);
        QCOMPARE(p.get_int64("key"), i);
    }

    void DoubleFromString()
    {
        Properties p;
        const char *s = "-1.234567";
        double d = -1.234567;
        p.set("key", d);
        QCOMPARE(p.get("key"), s);
        p.set("key", s);
        QCOMPARE(p.get_double("key"), d);
    }

    void SetNullRemovesProperty()
    {
        Properties p;
        const char *s = NULL;
        p.set("key", "value");
        p.set("key", s);
        QCOMPARE(p.get("key"), s);
    }

    void SetAndGetHexColor()
    {
        Properties p;
        const char *hexColorString = "0xaabbccdd";
        int hexColorInt = 0xaabbccdd;
        p.set("key", hexColorString);
        QCOMPARE(p.get_int("key"), hexColorInt);
    }

    void SetAndGetCssColor()
    {
        Properties p;
        const char *cssColorString = "#aabbcc";
        int cssColorInt = 0xaabbccff;
        p.set("key", cssColorString);
        QCOMPARE(p.get_int("key"), cssColorInt);

        const char *cssColorAlphaString = "#00aabbcc";
        int cssColorAlphaInt = 0xaabbcc00;
        p.set("key", cssColorAlphaString);
        QCOMPARE(p.get_int("key"), cssColorAlphaInt);
    }

    void SetAndGetTimeCode()
    {
        Profile profile;
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "11:22:33:04";
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), 1023829);
        p.set("key", 1023829);
        QCOMPARE(p.get_time("key", mlt_time_smpte), timeString);
    }

    void SetAndGetTimeClock()
    {
        Profile profile;
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "11:22:33.400";
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), 1023835);
        p.set("key", 1023835);
        QCOMPARE(p.get_time("key", mlt_time_clock), timeString);
    }

    void SetSimpleMathExpression()
    {
        Properties p;
        p.set("key", "@16.0/9.0 *2 +3 -1");
        QCOMPARE(p.get_int("key"), 5);
        QCOMPARE(p.get_double("key"), 16.0/9.0 *2 +3 -1);
    }

    void PassOneProperty()
    {
        Properties p[2];
        const char *s = "value";
        p[0].set("key", s);
        QCOMPARE(p[1].get("key"), (void*) 0);
        p[1].pass_property(p[0], "key");
        QCOMPARE(p[1].get("key"), s);
    }

    void PassMultipleByPrefix()
    {
        Properties p[2];
        const char *s = "value";
        p[0].set("key.one", s);
        p[0].set("key.two", s);
        QCOMPARE(p[1].get("key.one"), (void*) 0);
        QCOMPARE(p[1].get("key.two"), (void*) 0);
        p[1].pass_values(p[0], "key.");
        QCOMPARE(p[1].get("one"), s);
        QCOMPARE(p[1].get("two"), s);
    }

    void PassMultipleByList()
    {
        Properties p[2];
        const char *s = "value";
        p[0].set("key.one", s);
        p[0].set("key.two", s);
        QCOMPARE(p[1].get("key.one"), (void*) 0);
        QCOMPARE(p[1].get("key.two"), (void*) 0);
        p[1].pass_list(p[0], "key.one key.two");
        QCOMPARE(p[1].get("key.one"), s);
        QCOMPARE(p[1].get("key.two"), s);
    }

    void MirrorProperties()
    {
        Properties p[2];
        p[0].mirror(p[1]);
        p[0].set("key", "value");
        QCOMPARE(p[1].get("key"), "value");
    }

    void InheritProperties()
    {
        Properties p[2];
        p[0].set("key", "value");
        QVERIFY(p[1].get("key") == 0);
        p[1].inherit(p[0]);
        QCOMPARE(p[1].get("key"), "value");
    }

    void ParseString()
    {
        Properties p;
        QCOMPARE(p.get("key"), (void*) 0);
        p.parse("key=value");
        QCOMPARE(p.get("key"), "value");
        p.parse("key=\"new value\"");
        QCOMPARE(p.get("key"), "new value");
    }

    void RenameProperty()
    {
        Properties p;
        p.set("key", "value");
        QVERIFY(p.get("new key") == 0);
        p.rename("key", "new key");
        QCOMPARE(p.get("new key"), "value");
    }

    void SequenceDetected()
    {
        Properties p;
        p.set("1", 1);
        p.set("2", 2);
        p.set("3", 3);
        QVERIFY(p.is_sequence());
        p.set("four", 4);
        QVERIFY(!p.is_sequence());
    }

    void SerializesToYamlTiny()
    {
        Properties p[2];
        p[0].set("key1", "value1");
        p[0].set("key2", "value2");
        p[1].set("1", "value3");
        p[1].set("2", "value4");
        p[0].set("seq", p[1].get_properties(), 0);
        QCOMPARE(p[0].serialise_yaml(),
                "---\n"
                "key1: value1\n"
                "key2: value2\n"
                "seq:\n"
                "  - value3\n"
                "  - value4\n"
                "...\n");
    }

    void RadixRespondsToLocale()
    {
        Properties p;
        p.set_lcnumeric("en_US");
        p.set("key", "0.125");
        QCOMPARE(p.get_double("key"), double(1) / double(8));
        p.set_lcnumeric("de_DE");
        p.set("key", "0,125");
        QCOMPARE(p.get_double("key"), double(1) / double(8));
    }

    void DoubleAnimation()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=1; 60=60; 100=0", 100, fps, locale);
        mlt_animation_remove(a, 60);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=1;100=0");
        if (a_serialized) free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 1.0);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 1.0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 0.5);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 0.0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 0.0);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void IntAnimation()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=100; 60=60; 100=0", 100, fps, locale);
        mlt_animation_remove(a, 60);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=100;100=0");
        if (a_serialized) free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 100);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 100);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 50);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 0);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void AnimationWithTimeValueKeyframes()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, ":2.0=1; :4.0=0", 100, fps, locale);
        char *a_serialized = mlt_animation_serialize(a);
        // Time serializes to frame units :-\.
        QCOMPARE(a_serialized, "50=1;100=0");
        if (a_serialized) free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 1.0);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 1.0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 0.5);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 0.0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        QCOMPARE(mlt_property_get_double(item.property, fps, locale), 0.0);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void DiscreteIntAnimation()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50|=100; 60|=60; 100|=0", 100, fps, locale);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50|=100;60|=60;100|=0");
        if (a_serialized) free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 100);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 100);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 55);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 100);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 60);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 60);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 60);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 0);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void StringAnimation()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=hello world; 60=\"good night\"; 100=bar", 100, fps, locale);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=hello world;60=\"good night\";100=bar");
        if (a_serialized) free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        QCOMPARE(mlt_property_get_string(item.property), "hello world");
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        QCOMPARE(mlt_property_get_string(item.property), "hello world");
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        QCOMPARE(mlt_property_get_string(item.property), "\"good night\"");
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        QCOMPARE(mlt_property_get_string(item.property), "bar");
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        QCOMPARE(mlt_property_get_string(item.property), "bar");
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void test_property_get_double_pos()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=100; 20=200");
        QCOMPARE(mlt_property_get_double(p, fps, locale), 10.0);
        QCOMPARE(mlt_property_get_double_pos(p, fps, locale, 0, 100), 100.0);
        QCOMPARE(mlt_property_get_double_pos(p, fps, locale, 15, 100), 150.0);
        QCOMPARE(mlt_property_get_double_pos(p, fps, locale, 20, 100), 200.0);

        mlt_property_set_string(p, "1.5");
        QCOMPARE(mlt_property_get_double(p, fps, locale), 1.5);
        QCOMPARE(mlt_property_get_double_pos(p, fps, locale, 10, 100), 1.5);

        mlt_property_close(p);
    }

    void test_property_get_int_pos()
    {
        locale_t locale;
#if defined(__linux__) || defined(__DARWIN__)
        locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
#endif
        double fps = 25.0;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=100; 20=200");
        QCOMPARE(mlt_property_get_int(p, fps, locale), 10);
        QCOMPARE(mlt_property_get_int_pos(p, fps, locale, 0, 100), 100);
        QCOMPARE(mlt_property_get_int_pos(p, fps, locale, 15, 100), 150);
        QCOMPARE(mlt_property_get_int_pos(p, fps, locale, 20, 100), 200);

        mlt_property_set_string(p, "1.5");
        QCOMPARE(mlt_property_get_int(p, fps, locale), 1);
        QCOMPARE(mlt_property_get_int_pos(p, fps, locale, 10, 100), 1);

        mlt_property_close(p);
    }
};

QTEST_APPLESS_MAIN(TestProperties)

#include "test_properties.moc"
