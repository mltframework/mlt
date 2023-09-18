/*
 * Copyright (C) 2013-2018 Dan Dennedy <dan@dennedy.org>
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
#include <QTemporaryFile>
#include <QtTest>

#include <mlt++/Mlt.h>
using namespace Mlt;

extern "C" {
#define __APPLE__
#include <framework/mlt_animation.h>
#include <framework/mlt_property.h>
}
#include <cfloat>

static const bool kRunLongTests = true;

class TestProperties : public QObject
{
    Q_OBJECT
    mlt_locale_t locale;

public:
    TestProperties()
    {
#if !defined(_WIN32) && (defined(__GLIBC__) || defined(__APPLE__))
        locale = newlocale(LC_NUMERIC_MASK, "POSIX", NULL);
#else
        locale = 0;
#endif
        Factory::init();
    }

    ~TestProperties()
    {
#if !defined(_WIN32) && (defined(__GLIBC__) || defined(__APPLE__))
        freelocale(locale);
#endif
    }

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
        Properties *q = new Properties(p);
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
        char *const s = strdup(value);
        p.set("key", s, strlen(s), free);
        int size = 0;
        QCOMPARE((char *) p.get_data("key", size), value);
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
        const char *s = "-1.23456";
        double d = -1.23456;
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
        QCOMPARE(p.get_time("key", mlt_time_smpte_df), timeString);
    }

    void SetAndGetTimeCodeNtscFps()
    {
        Profile profile("atsc_720p_2997");
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "00:00:17;09";
        // Looking just add seconds:
        //   510 f / 29.97 fps = 17.017017017 s
        //   29.97 fps * 17 s  = 509.49 f
        //   round(509.49) = 509 f, which it not equal to 510, the original amount.
        //   However, ceiling(509.49) = 510 f
        // Now, adding on the frames part:
        //   509.49 f + 9 f = 518.49 f
        //   ceiling(518.49) = 519 f
        const int frames = 519;
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), frames);
        p.set("key", frames);
        QCOMPARE(p.get_time("key", mlt_time_smpte_df), timeString);
    }

    void SetAndGetTimeCode5994Fps()
    {
        Profile profile("atsc_720p_5994");
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "00:01:00;04";
        int frames = 3600;
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), frames);
        p.set("key", frames);
        QCOMPARE(p.get_time("key", mlt_time_smpte_df), timeString);
        timeString = "00:10:00;01";
        frames = 36001 - 9 * 4;
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), frames);
        p.set("key", frames);
        QCOMPARE(p.get_time("key", mlt_time_smpte_df), timeString);
    }

    void SetAndGetTimeCodeNonIntFps()
    {
        Profile profile("atsc_720p_2398");
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "11:22:33:04";
        // 11 * 3600 + 22 * 60 + 33 = 40953 s
        // floor(23.98 fps * 40953 s) = 981890 f
        // 981890 f + 4 f = 981894 f
        int frames = 981894;
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), frames);
        p.set("key", frames);
        // floor(981894 f / (24000/1001 * 3600)) = 11 h
        // 981894 f - floor(11 * 3600 * 24000/1001) = 32444 f
        // floor(32444 f / (24000/1001 * 60)) = 22 m
        // 981894 f - floor((11 * 3600 + 22 * 60) * 24000/1001) = 796 f
        // floor(796 f / (24000/1001)) = 33 s
        // 981894 f - ceil((11 * 3600 + 22 * 60 + 33) * 24000/1001) = 4f
        QCOMPARE(p.get_time("key", mlt_time_smpte_df), timeString);
        QCOMPARE(p.get_time("key", mlt_time_clock), "11:22:33.200");

        // Case that is known to have floating point error
        frames = 2877;
        p.set("key", frames);
        QCOMPARE(p.get_time("key", mlt_time_smpte_df), "00:02:00:00");
        QCOMPARE(p.get_time("key", mlt_time_clock), "00:02:00.000");

        if (kRunLongTests)
            for (int i = 0; i < 9999999; ++i) {
                p.set("key", i);
                //            QWARN(p.get_time("key", mlt_time_smpte_df));
                p.set("test", p.get_time("key", mlt_time_smpte_df));
                QCOMPARE(p.get_int("test"), i);
            }
    }

    void SetAndGetTimeCodeNonDropFrame()
    {
        Profile profile("dv_ntsc");
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "11:22:33:04";
        const int frames = 1228594;
        p.set("key", frames);
        QCOMPARE(p.get_time("key", mlt_time_smpte_ndf), timeString);
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

    void SetAndGetTimeClockNtscFps()
    {
        Profile profile("dv_ntsc");
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        const char *timeString = "11:22:33.400";
        const int frames = 1227374;
        p.set("key", timeString);
        QCOMPARE(p.get_int("key"), frames);
        p.set("key", frames);
        QCOMPARE(p.get_time("key", mlt_time_clock), timeString);

        if (kRunLongTests)
            for (int i = 0; i < 9999999; ++i) {
                p.set("key", i);
                //            QWARN(p.get_time("key", mlt_time_clock));
                p.set("test", p.get_time("key", mlt_time_clock));
                QCOMPARE(p.get_int("test"), i);
            }
    }

    void SetAndGetTimeClockNonIntFps()
    {
        Profile profile("atsc_720p_2398");
        Properties p;
        p.set("_profile", profile.get_profile(), 0);

        if (kRunLongTests)
            for (int i = 0; i < 9999999; ++i) {
                p.set("key", i);
                //            QWARN(p.get_time("key", mlt_time_clock));
                p.set("test", p.get_time("key", mlt_time_clock));
                QCOMPARE(p.get_int("test"), i);
            }
    }

    void SetSimpleMathExpression()
    {
        Properties p;
        p.set("key", "@16.0/9.0 *2 +3 -1");
        QCOMPARE(p.get_int("key"), 5);
        QCOMPARE(p.get_double("key"), 16.0 / 9.0 * 2 + 3 - 1);
    }

    void SetMathExpressionWithProperty()
    {
        Properties p;
        p.set("width", 100);
        p.set("key", "@16.0/9.0 *width");
        QCOMPARE(p.get_int("key"), 177);
    }

    void PassOneProperty()
    {
        Properties p[2];
        const char *s = "value";
        p[0].set("key", s);
        QCOMPARE(p[1].get("key"), (void *) 0);
        p[1].pass_property(p[0], "key");
        QCOMPARE(p[1].get("key"), s);
    }

    void PassMultipleByPrefix()
    {
        Properties p[2];
        const char *s = "value";
        p[0].set("key.one", s);
        p[0].set("key.two", s);
        QCOMPARE(p[1].get("key.one"), (void *) 0);
        QCOMPARE(p[1].get("key.two"), (void *) 0);
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
        QCOMPARE(p[1].get("key.one"), (void *) 0);
        QCOMPARE(p[1].get("key.two"), (void *) 0);
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
        QCOMPARE(p.get("key"), (void *) 0);
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
        Properties p[3];
        p[0].set("key1", "value1");
        p[0].set("key:2", "value[2]");
        p[1].set("1", "value3");
        p[1].set("2", "value:4");
        p[2].set("1", "value5");
        p[2].set("2", "\"value6\"");
        p[0].set("seq1", p[1].get_properties(), 0);
        p[0].set("seq'2", p[2].get_properties(), 0);
        char *serializedYaml = p[0].serialise_yaml();
        QCOMPARE(serializedYaml,
                 "---\n"
                 "key1: value1\n"
                 "\"key:2\": \"value[2]\"\n"
                 "seq1:\n"
                 "  - value3\n"
                 "  - \"value:4\"\n"
                 "\"seq'2\":\n"
                 "  - value5\n"
                 "  - '\"value6\"'\n"
                 "...\n");
        free(serializedYaml);
    }

    void ParsesYamlTiny()
    {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            tempFile.write("---\n"
                           "key1: value1\n"
                           "\"key:2\":\"value[2]\"\n"
                           "seq1:\n"
                           "  - value3\n"
                           "  - \"value:4\"\n"
                           "\"seq'2\":\n"
                           "  - value5\n"
                           "  - \"value:6\"\n"
                           "...\n");
            tempFile.close();
        }
        QScopedPointer<Properties> p(
            Properties::parse_yaml(tempFile.fileName().toUtf8().constData()));
        QVERIFY(!p.isNull());
        QVERIFY(p->is_valid());
        QCOMPARE(p->get("key1"), "value1");
        QCOMPARE(p->get("key:2"), "value[2]");
    }

    void RadixRespondsToLocale()
    {
        Properties p;
        p.set_lcnumeric("en_US.UTF-8");
        p.set("key", "0.125");
        QCOMPARE(p.get_double("key"), double(1) / double(8));
#if !defined(_WIN32)
        p.set_lcnumeric("de_DE.UTF-8");
        p.set("key", "0,125");
        QCOMPARE(p.get_double("key"), double(1) / double(8));
#endif
    }

    void AnimationInsert()
    {
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        item.is_key = 1;
        item.keyframe_type = mlt_keyframe_discrete;
        item.property = mlt_property_init();

        item.frame = 0;
        mlt_property_set_string(item.property, "0");
        mlt_animation_insert(a, &item);

        item.frame = 1;
        mlt_property_set_string(item.property, "1");
        mlt_animation_insert(a, &item);

        item.frame = 2;
        mlt_property_set_string(item.property, "2");
        mlt_animation_insert(a, &item);

        QCOMPARE(mlt_animation_get_length(a), 2);

        char *a_serialized = mlt_animation_serialize(a);
        mlt_animation_parse(a, a_serialized, 0, fps, locale);
        QCOMPARE(a_serialized, "0|=0;1|=1;2|=2");
        if (a_serialized)
            free(a_serialized);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void DoubleAnimation()
    {
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=1; 60=60; 100=0", 100, fps, locale);
        mlt_animation_remove(a, 60);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=1;100=0");
        if (a_serialized)
            free(a_serialized);
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
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=100; 60=60; 100=0", 100, fps, locale);
        mlt_animation_remove(a, 60);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=100;100=0");
        if (a_serialized)
            free(a_serialized);
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
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, ":2.0=1; :4.0=0", 100, fps, locale);
        char *a_serialized = mlt_animation_serialize(a);
        // Time serializes to frame units :-\.
        QCOMPARE(a_serialized, "50=1;100=0");
        if (a_serialized)
            free(a_serialized);
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
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50|=100; 60|=60; 100|=0", 100, fps, locale);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50|=100;60|=60;100|=0");
        if (a_serialized)
            free(a_serialized);
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
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=hello world; 60=\"good night\"; 100=bar", 100, fps, locale);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=hello world;60=good night;100=bar");
        if (a_serialized)
            free(a_serialized);

        mlt_animation_parse(a, "50=hello world; 60=\"good; night\"; 100=bar", 100, fps, locale);
        a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=hello world;60=\"good; night\";100=bar");
        if (a_serialized)
            free(a_serialized);

        mlt_animation_parse(a, "50=hello world; 60=\"\"good night\"\"; 100=bar", 100, fps, locale);
        a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=hello world;60=\"good night\";100=bar");
        if (a_serialized)
            free(a_serialized);

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

    void test_property_anim_get_double()
    {
        double fps = 25.0;
        int len = 0;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=100; 20=200");
        QCOMPARE(mlt_property_get_double(p, fps, locale), 10.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 0, len), 100.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 15, len), 150.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 20, len), 200.0);

        mlt_property_set_string(p, "1.5");
        QCOMPARE(mlt_property_get_double(p, fps, locale), 1.5);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 10, 100), 1.5);

        mlt_property_close(p);
    }

    void test_property_anim_get_int()
    {
        double fps = 25.0;
        int len = 100;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=100; 20=200");
        QCOMPARE(mlt_property_get_int(p, fps, locale), 10);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 0, len), 100);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 15, len), 150);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 20, len), 200);

        mlt_property_set_string(p, "1.5");
        QCOMPARE(mlt_property_get_int(p, fps, locale), 1);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 10, 100), 1);

        mlt_property_close(p);
    }

    void test_property_anim_get_color()
    {
        double fps = 25.0;
        int len = 100;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=#ffff00ff;20=green");

        QCOMPARE(mlt_property_get_int(p, fps, locale), 10);

        mlt_color color = mlt_property_anim_get_color(p, fps, locale, 0, len);
        QCOMPARE(color.r, 255);
        QCOMPARE(color.g, 0);
        QCOMPARE(color.b, 255);
        QCOMPARE(color.a, 255);

        color = mlt_property_anim_get_color(p, fps, locale, 15, len);
        QCOMPARE(color.r, 127);
        QCOMPARE(color.g, 127);
        QCOMPARE(color.b, 127);
        QCOMPARE(color.a, 255);

        color = mlt_property_anim_get_color(p, fps, locale, 20, len);
        QCOMPARE(color.r, 0);
        QCOMPARE(color.g, 255);
        QCOMPARE(color.b, 0);
        QCOMPARE(color.a, 255);

        mlt_property_close(p);
    }

    void SmoothIntAnimation()
    {
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a,
                            "0=80;10~=80; 20~=30; 30~=40; 40~=28; 50=90; 60=0; 70=60; 80=20",
                            100,
                            fps,
                            locale);
        item.property = mlt_property_init();
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "0=80;10~=80;20~=30;30~=40;40~=28;50=90;60=0;70=60;80=20");
        if (a_serialized)
            free(a_serialized);

        mlt_animation_get_item(a, &item, 10);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 80);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 50);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 90);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 55);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 45);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 60);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 0);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 40);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 20);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 110);
        QCOMPARE(mlt_property_get_int(item.property, fps, locale), 20);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void test_property_anim_set_double()
    {
        double fps = 25.0;
        int len = 100;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=100; 20=200");
        mlt_property_anim_set_double(p, 1.5, fps, locale, 30, len, mlt_keyframe_linear);
        QCOMPARE(mlt_property_get_double(p, fps, locale), 10.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 0, len), 100.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 15, len), 150.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 20, len), 200.0);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 25, len), 100.75);
        QCOMPARE(mlt_property_anim_get_double(p, fps, locale, 30, len), 1.5);
        mlt_property_close(p);
    }

    void test_property_anim_set_int()
    {
        double fps = 25.0;
        int len = 0;
        mlt_property p = mlt_property_init();
        mlt_property_set_string(p, "10=100; 20=200");
        mlt_property_anim_set_int(p, 300, fps, locale, 30, len, mlt_keyframe_linear);
        QCOMPARE(mlt_property_get_int(p, fps, locale), 10);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 0, len), 100);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 15, len), 150);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 20, len), 200);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 25, len), 250);
        QCOMPARE(mlt_property_anim_get_int(p, fps, locale, 30, len), 300);
        mlt_property_close(p);
    }

    void PercentAsRatio()
    {
        Properties p;
        p.set("foo", "12.3%");
        QCOMPARE(p.get_double("foo"), 0.123);
        p.set("foo", "456 %");
        QCOMPARE(p.get_double("foo"), 456.0);
    }

    void PropertiesAnimInt()
    {
        Properties p;
        p.set_lcnumeric("POSIX");

        // Construct animation from scratch
        p.anim_set("foo", 0, 0);
        p.anim_set("foo", 100, 50, -1, mlt_keyframe_smooth);
        QCOMPARE(p.anim_get_int("foo", 0), 0);
        QCOMPARE(p.anim_get_int("foo", 25), 50);
        QCOMPARE(p.anim_get_int("foo", 50), 100);
        QCOMPARE(p.get("foo"), "0=0;50~=100");

        // Animation from string value
        p.set("foo", "10=100;20=200");
        QCOMPARE(p.anim_get_int("foo", 0), 100);
        QCOMPARE(p.anim_get_int("foo", 15), 150);
        QCOMPARE(p.anim_get_int("foo", 20), 200);

        // Animation from string using time clock values
        // Need to set a profile so fps can be used to convert time to frames.
        Profile profile("dv_pal");
        p.set("_profile", profile.get_profile(), 0);
        p.set("foo", ":0.0=100; :2.0=200");
        QCOMPARE(p.anim_get_int("foo", 0), 100);
        QCOMPARE(p.anim_get_int("foo", 25), 150);
        QCOMPARE(p.anim_get_int("foo", 50), 200);
    }

    void PropertiesAnimDouble()
    {
        Properties p;
        p.set_lcnumeric("POSIX");

        // Construct animation from scratch
        p.anim_set("foo", 0.0, 0);
        p.anim_set("foo", 100.0, 50, -1, mlt_keyframe_smooth);
        QCOMPARE(p.anim_get_double("foo", 0), 0.0);
        QCOMPARE(p.anim_get_double("foo", 25), 50.0);
        QCOMPARE(p.anim_get_double("foo", 50), 100.0);
        QCOMPARE(p.get("foo"), "0=0;50~=100");

        // Animation from string value
        p.set("foo", "10=100.2;20=200.8");
        QCOMPARE(p.anim_get_double("foo", 0), 100.2);
        QCOMPARE(p.anim_get_double("foo", 15), 150.5);
        QCOMPARE(p.anim_get_double("foo", 20), 200.8);

        // Animation from string using time clock values
        // Need to set a profile so fps can be used to convert time to frames.
        Profile profile("dv_pal");
        p.set("_profile", profile.get_profile(), 0);
        p.set("foo", ":0.0=100; :2.0=200");
        QCOMPARE(p.anim_get_double("foo", 0), 100.0);
        QCOMPARE(p.anim_get_double("foo", 25), 150.0);
        QCOMPARE(p.anim_get_double("foo", 50), 200.0);

        // Test a non-animation string.
        p.set("bar", "0.5");
        QCOMPARE(p.anim_get_double("bar", 25), 0.5);
    }

    void PropertiesStringAnimation()
    {
        Properties p;
        p.anim_set("key", "foo", 10);
        p.anim_set("key", "bar", 30);
        QCOMPARE(p.get("key"), "10|=foo;30|=bar");
        p.set("key", "0=; 10=foo bar; 30=hello world");
        QCOMPARE(p.anim_get("key", 1), "");
        QCOMPARE(p.anim_get("key", 15), "foo bar");
        QCOMPARE(p.anim_get("key", 45), "hello world");
    }

    void PropertyRefreshOnAnimationChange()
    {
        // Create an animation property from string and see that it works.
        // Get the animation and modify the first position.
        // Ensure that change affects other get() functions

        {
            Properties p;
            p.set("foo", "10=100; 20=200");
            QCOMPARE(p.get_double("foo"), 10.0);
            // Call anim_get_double() to create the animation
            QCOMPARE(p.anim_get_double("foo", 15, 20), 150.0);
            Mlt::Animation animation = p.get_animation("foo");
            animation.key_set_frame(0, 15);
            QCOMPARE(p.get_double("foo"), 15.0);
        }

        {
            Properties p;
            p.set("foo", "10=100;20=200");
            QCOMPARE(p.anim_get_double("foo", 15, 20), 150.0);
            Mlt::Animation animation = p.get_animation("foo");
            animation.key_set_frame(0, 15);
            QCOMPARE(p.anim_get_double("foo", 15, 0), 100.0);
        }

        {
            Properties p;
            p.set("foo", "10=100; 20=200");
            QCOMPARE(p.get_int("foo"), 10);
            // Call anim_get_int() to create the animation
            QCOMPARE(p.anim_get_int("foo", 15, 20), 150);
            Mlt::Animation animation = p.get_animation("foo");
            animation.key_set_frame(0, 15);
            QCOMPARE(p.get_int("foo"), 15);
        }

        {
            Properties p;
            p.set("foo", "10=100; 20=200");
            QCOMPARE(p.anim_get_int("foo", 15, 20), 150);
            Mlt::Animation animation = p.get_animation("foo");
            animation.key_set_frame(0, 15);
            QCOMPARE(p.anim_get_int("foo", 15, 0), 100);
        }

        {
            Properties p;
            p.set("foo", "10=100;20=200");
            // Call anim_get_int() to create the animation
            QCOMPARE(p.anim_get_int("foo", 15, 20), 150);
            Mlt::Animation animation = p.get_animation("foo");
            QCOMPARE(p.get("foo"), "10=100;20=200");
            animation.key_set_frame(0, 15);
            QCOMPARE(p.get("foo"), "15=100;20=200");
        }
    }

    void test_mlt_rect()
    {
        mlt_property p = mlt_property_init();
        mlt_rect r = {1, 2, 3, 4, 5};

        mlt_property_set_rect(p, r);
        QCOMPARE(mlt_property_get_string(p), "1 2 3 4 5");
        r.o = DBL_MIN;
        mlt_property_set_rect(p, r);
        QCOMPARE(mlt_property_get_string(p), "1 2 3 4");
        r.w = DBL_MIN;
        r.h = DBL_MIN;
        mlt_property_set_rect(p, r);
        QCOMPARE(mlt_property_get_string(p), "1 2");

        mlt_property_set_string(p, "1.1/2.2:3.3x4.4:5.5");
        r = mlt_property_get_rect(p, locale);
        QCOMPARE(r.x, 1.1);
        QCOMPARE(r.y, 2.2);
        QCOMPARE(r.w, 3.3);
        QCOMPARE(r.h, 4.4);
        QCOMPARE(r.o, 5.5);

        mlt_property_set_string(p, "1.1 2.2");
        r = mlt_property_get_rect(p, locale);
        QCOMPARE(r.x, 1.1);
        QCOMPARE(r.y, 2.2);
        QCOMPARE(r.w, DBL_MIN);

        mlt_property_set_int64(p, UINT_MAX);
        r = mlt_property_get_rect(p, locale);
        QCOMPARE(r.x, double(UINT_MAX));

        mlt_property_close(p);
    }

    void SetAndGetRect()
    {
        Properties p;
        mlt_rect r;
        r.x = 1.1;
        r.y = 2.2;
        r.w = 3.3;
        r.h = 4.4;
        r.o = 5.5;
        p.set("key", r);
        mlt_rect q = p.get_rect("key");
        QCOMPARE(q.x, 1.1);
        QCOMPARE(q.y, 2.2);
        QCOMPARE(q.w, 3.3);
        QCOMPARE(q.h, 4.4);
        QCOMPARE(q.o, 5.5);
        p.set("key", 10, 20, 30, 40);
        q = p.get_rect("key");
        QCOMPARE(q.x, 10.0);
        QCOMPARE(q.y, 20.0);
        QCOMPARE(q.w, 30.0);
        QCOMPARE(q.h, 40.0);
    }

    void RectFromString()
    {
        Properties p;
        p.set_lcnumeric("POSIX");
        const char *s = "1.1 2.2 3.3 4.4 5.5";
        mlt_rect r = {1.1, 2.2, 3.3, 4.4, 5.5};
        p.set("key", r);
        QCOMPARE(p.get("key"), s);
        p.set("key", s);
        r = p.get_rect("key");
        QCOMPARE(r.x, 1.1);
        QCOMPARE(r.y, 2.2);
        QCOMPARE(r.w, 3.3);
        QCOMPARE(r.h, 4.4);
        QCOMPARE(r.o, 5.5);
    }

    void RectAnimation()
    {
        mlt_rect r1 = {0, 0, 200, 200, 0};
        mlt_rect r2 = {100, 100, 400, 400, 1.0};
        Properties p;
        p.set_lcnumeric("POSIX");

        // Construct animation from scratch
        p.anim_set("key", r1, 0);
        p.anim_set("key", r2, 50);
        QCOMPARE(p.anim_get_rect("key", 0).x, 0.0);
        QCOMPARE(p.anim_get_rect("key", 25).x, 50.0);
        QCOMPARE(p.anim_get_rect("key", 25).y, 50.0);
        QCOMPARE(p.anim_get_rect("key", 25).w, 300.0);
        QCOMPARE(p.anim_get_rect("key", 25).h, 300.0);
        QCOMPARE(p.anim_get_rect("key", 25).o, 0.5);
        QCOMPARE(p.anim_get_rect("key", 50).x, 100.0);
        QCOMPARE(p.get("key"), "0=0 0 200 200 0;50=100 100 400 400 1");

        // Animation from string value
        QCOMPARE(p.anim_get_rect("key", 0).x, 0.0);
        QCOMPARE(p.anim_get_rect("key", 0).y, 0.0);
        QCOMPARE(p.anim_get_rect("key", 0).w, 200.0);
        QCOMPARE(p.anim_get_rect("key", 0).h, 200.0);
        QCOMPARE(p.anim_get_rect("key", 0).o, 0.0);
        QCOMPARE(p.anim_get_rect("key", 50).x, 100.0);
        QCOMPARE(p.anim_get_rect("key", 50).y, 100.0);
        QCOMPARE(p.anim_get_rect("key", 50).w, 400.0);
        QCOMPARE(p.anim_get_rect("key", 50).h, 400.0);
        QCOMPARE(p.anim_get_rect("key", 50).o, 1.0);
        QCOMPARE(p.anim_get_rect("key", 15).x, 30.0);
        QCOMPARE(p.anim_get_rect("key", 15).y, 30.0);
        QCOMPARE(p.anim_get_rect("key", 15).w, 260.0);
        QCOMPARE(p.anim_get_rect("key", 15).h, 260.0);
        QCOMPARE(p.anim_get_rect("key", 15).o, 0.3);

        // Smooth animation
        p.set("key", "0~=0/0:200x200:0; 50=100/100:400x400:1");
        QCOMPARE(p.anim_get_rect("key", 0).x, 0.0);
        QCOMPARE(p.anim_get_rect("key", 0).y, 0.0);
        QCOMPARE(p.anim_get_rect("key", 0).w, 200.0);
        QCOMPARE(p.anim_get_rect("key", 0).h, 200.0);
        QCOMPARE(p.anim_get_rect("key", 0).o, 0.0);
        QCOMPARE(p.anim_get_rect("key", 50).x, 100.0);
        QCOMPARE(p.anim_get_rect("key", 50).y, 100.0);
        QCOMPARE(p.anim_get_rect("key", 50).w, 400.0);
        QCOMPARE(p.anim_get_rect("key", 50).h, 400.0);
        QCOMPARE(p.anim_get_rect("key", 50).o, 1.0);
        QCOMPARE(p.anim_get_rect("key", 15).x, 21.6);
        QCOMPARE(p.anim_get_rect("key", 15).y, 21.6);
        QCOMPARE(p.anim_get_rect("key", 15).w, 243.2);
        QCOMPARE(p.anim_get_rect("key", 15).h, 243.2);
        QCOMPARE(p.anim_get_rect("key", 15).o, 0.216);

        // Using percentages
        p.set("key", "0=0 0; 50=100% 200%");
        QCOMPARE(p.anim_get_rect("key", 25).x, 0.5);
        QCOMPARE(p.anim_get_rect("key", 25).y, 1.0);
    }

    void ColorFromInt()
    {
        Properties p;
        p.set_lcnumeric("POSIX");
        p.set("key", (int) 0xaabbccdd);
        mlt_color color = p.get_color("key");
        QCOMPARE(color.r, quint8(0xaa));
        QCOMPARE(color.g, quint8(0xbb));
        QCOMPARE(color.b, quint8(0xcc));
        QCOMPARE(color.a, quint8(0xdd));
        p.set("key", color);
        QCOMPARE(p.get_int("key"), int(0xaabbccdd));
    }

    void ColorFromString()
    {
        Properties p;
        p.set_lcnumeric("POSIX");
        p.set("key", "red");
        mlt_color color = p.get_color("key");
        QCOMPARE(color.r, quint8(0xff));
        QCOMPARE(color.g, quint8(0x00));
        QCOMPARE(color.b, quint8(0x00));
        QCOMPARE(color.a, quint8(0xff));
        //pattern #AARRGGBB
        p.set("key", "#deadd00d");
        color = p.get_color("key");
        QCOMPARE(color.r, quint8(0xad));
        QCOMPARE(color.g, quint8(0xd0));
        QCOMPARE(color.b, quint8(0x0d));
        QCOMPARE(color.a, quint8(0xde));
        //pattern #0xRRGGBBAA
        p.set("key", "0xadd00dde");
        color = p.get_color("key");
        QCOMPARE(color.r, quint8(0xad));
        QCOMPARE(color.g, quint8(0xd0));
        QCOMPARE(color.b, quint8(0x0d));
        QCOMPARE(color.a, quint8(0xde));
    }

    void StringFromColor()
    {
        mlt_color color;
        color.r = quint8(0xad);
        color.g = quint8(0xd0);
        color.b = quint8(0x0d);
        color.a = quint8(0xde);
        mlt_property p = mlt_property_init();
        mlt_property_set_color(p, color);
        QCOMPARE(mlt_property_get_string(p), "#deadd00d");
        mlt_property_close(p);

        Properties pies;

        pies.set("key", color);
        QCOMPARE(pies.get("key"), "#deadd00d");
    }

    void ColorAnimationCpp()
    {
        mlt_color c1 = {0xff, 0xef, 0xab, 0x00};
        mlt_color c2 = {0x00, 0xff, 0xab, 0xdf};
        Properties p;
        p.set_lcnumeric("POSIX");

        // Construct animation from scratch
        p.anim_set("key", c1, 0);
        p.anim_set("key", c2, 50);
        QCOMPARE(p.anim_get_color("key", 0).r, 255);
        QCOMPARE(p.anim_get_color("key", 25).g, 247);
        QCOMPARE(p.anim_get_color("key", 25).b, 171);
        QCOMPARE(p.anim_get_color("key", 25).a, 111);
        QCOMPARE(p.get("key"), "0=#00ffefab;50=#df00ffab");

        // Animation from string value
        QCOMPARE(p.anim_get_color("key", 0).r, 255);
        QCOMPARE(p.anim_get_color("key", 0).g, 239);
        QCOMPARE(p.anim_get_color("key", 0).b, 171);
        QCOMPARE(p.anim_get_color("key", 0).a, 0);
        QCOMPARE(p.anim_get_color("key", 50).r, 0);
        QCOMPARE(p.anim_get_color("key", 50).g, 255);
        QCOMPARE(p.anim_get_color("key", 50).b, 171);
        QCOMPARE(p.anim_get_color("key", 50).a, 223);
        QCOMPARE(p.anim_get_color("key", 15).r, 178);
        QCOMPARE(p.anim_get_color("key", 15).g, 243);
        QCOMPARE(p.anim_get_color("key", 15).b, 171);
        QCOMPARE(p.anim_get_color("key", 15).a, 66);
    }

    void SmoothColorAnimationCpp()
    {
        Properties p;
        p.set_lcnumeric("POSIX");

        // Smooth animation
        p.set("key", "0~=#00ffefab; 50~=#df00ffab; 100~=#df00ffab; 150~=#df00ffab");
        QCOMPARE(p.anim_get_color("key", 0).r, 255);
        QCOMPARE(p.anim_get_color("key", 0).g, 239);
        QCOMPARE(p.anim_get_color("key", 0).b, 171);
        QCOMPARE(p.anim_get_color("key", 0).a, 0);
        QCOMPARE(p.anim_get_color("key", 25).r, 127);
        QCOMPARE(p.anim_get_color("key", 25).g, 247);
        QCOMPARE(p.anim_get_color("key", 25).b, 171);
        QCOMPARE(p.anim_get_color("key", 25).a, 111);
        QCOMPARE(p.anim_get_color("key", 50).r, 0);
        QCOMPARE(p.anim_get_color("key", 50).g, 255);
        QCOMPARE(p.anim_get_color("key", 50).b, 171);
        QCOMPARE(p.anim_get_color("key", 50).a, 223);
        QCOMPARE(p.anim_get_color("key", 60).a, 223);
        QCOMPARE(p.anim_get_color("key", 70).a, 223);
        QCOMPARE(p.anim_get_color("key", 75).r, 0);
        QCOMPARE(p.anim_get_color("key", 75).g, 255);
        QCOMPARE(p.anim_get_color("key", 75).b, 171);
        QCOMPARE(p.anim_get_color("key", 75).a, 223);
        QCOMPARE(p.anim_get_color("key", 100).r, 0);
        QCOMPARE(p.anim_get_color("key", 100).g, 255);
        QCOMPARE(p.anim_get_color("key", 100).b, 171);
        QCOMPARE(p.anim_get_color("key", 100).a, 223);
    }

    void ColorAnimationCssString()
    {
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=0xff00ffff; 60=0xaa00ffff; 100=0x00ff00ff", 100, fps, locale);
        mlt_animation_remove(a, 60);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=0xff00ffff;100=0x00ff00ff");
        if (a_serialized)
            free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        mlt_color color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 255);
        QCOMPARE(color.g, 0);
        QCOMPARE(color.b, 255);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 255);
        QCOMPARE(color.g, 0);
        QCOMPARE(color.b, 255);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 127);
        QCOMPARE(color.g, 127);
        QCOMPARE(color.b, 127);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 0);
        QCOMPARE(color.g, 255);
        QCOMPARE(color.b, 0);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 0);
        QCOMPARE(color.g, 255);
        QCOMPARE(color.b, 0);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void ColorAnimationHexString()
    {
        double fps = 25.0;
        mlt_animation a = mlt_animation_new();
        struct mlt_animation_item_s item;

        mlt_animation_parse(a, "50=#ffff00ff; 60=#ffaa00ff; 100=#ff00ff00", 100, fps, locale);
        mlt_animation_remove(a, 60);
        char *a_serialized = mlt_animation_serialize(a);
        QCOMPARE(a_serialized, "50=#ffff00ff;100=#ff00ff00");
        if (a_serialized)
            free(a_serialized);
        item.property = mlt_property_init();

        mlt_animation_get_item(a, &item, 10);
        mlt_color color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 255);
        QCOMPARE(color.g, 0);
        QCOMPARE(color.b, 255);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 50);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 255);
        QCOMPARE(color.g, 0);
        QCOMPARE(color.b, 255);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 75);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 127);
        QCOMPARE(color.g, 127);
        QCOMPARE(color.b, 127);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 0);

        mlt_animation_get_item(a, &item, 100);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 0);
        QCOMPARE(color.g, 255);
        QCOMPARE(color.b, 0);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 1);

        mlt_animation_get_item(a, &item, 110);
        color = mlt_property_get_color(item.property, fps, locale);
        QCOMPARE(color.r, 0);
        QCOMPARE(color.g, 255);
        QCOMPARE(color.b, 0);
        QCOMPARE(color.a, 255);
        QCOMPARE(item.is_key, 0);

        mlt_property_close(item.property);
        mlt_animation_close(a);
    }

    void SetIntAndGetAnim()
    {
        Properties p;
        p.set_lcnumeric("POSIX");
        p.set("key", 123);
        QCOMPARE(p.anim_get_int("key", 10, 50), 123);
        p.set("key", "123");
        QCOMPARE(p.anim_get_int("key", 10, 50), 123);
    }

    void SetDoubleAndGetAnim()
    {
        Properties p;
        p.set_lcnumeric("POSIX");
        p.set("key", 123.0);
        QCOMPARE(p.anim_get_double("key", 10, 50), 123.0);
        p.set("key", "123");
        QCOMPARE(p.anim_get_double("key", 10, 50), 123.0);
    }

    void AnimNegativeTimevalue()
    {
        Properties p;
        Profile profile("dv_pal");
        p.set("_profile", profile.get_profile(), 0);
        p.set_lcnumeric("POSIX");
        p.set("key", "0=100; -1=200");
        QCOMPARE(p.anim_get_int("key", 75, 100), 175);
        p.set("key", "0=100; -1:=200");
        QCOMPARE(p.anim_get_int("key", 75, 125), 175);
    }

    void NestedProperties()
    {
        Properties parent;

        Properties child1;
        child1.set("c1A", "A");
        child1.set("c1B", "B");
        parent.set("c1", child1);
        QCOMPARE(child1.ref_count(), 2);

        Properties child2;
        child2.set("c2C", "C");
        child2.set("c2D", "D");
        parent.set("c2", child2);
        QCOMPARE(child2.ref_count(), 2);

        QCOMPARE(parent.count(), 2);

        Properties *pChild1 = parent.get_props("c1");
        QCOMPARE(pChild1->get("c1B"), "B");
        delete pChild1;

        Properties *pChild2 = parent.get_props_at(1);
        QCOMPARE(pChild1->get("c2D"), "D");
        delete pChild2;
    }

    void PropertyClears()
    {
        Properties p;
        p.set("key", 1);
        QCOMPARE(p.get_int("key"), 1);
        QCOMPARE(p.get("key"), "1");
        p.clear("key");
        QCOMPARE(p.get("key"), (char *) 0);
        QCOMPARE(p.get_data("key"), (void *) 0);
        QCOMPARE(p.get_animation("key"), mlt_animation(0));
        QCOMPARE(p.get_int("key"), 0);
    }

    void PropertyExists()
    {
        Properties p;
        // Never set should return false
        QCOMPARE(p.property_exists("key"), false);
        // Set should return true
        p.set("key", 1);
        QCOMPARE(p.property_exists("key"), true);
        // Cleared should return false
        p.clear("key");
        QCOMPARE(p.property_exists("key"), false);
    }

    void AnimationExists()
    {
        Properties p;
        // Never set should return false
        QCOMPARE(p.property_exists("key"), false);
        // Get an animation but don't set anything - should return false
        Mlt::Animation animation = p.get_animation("key");
        QCOMPARE(p.property_exists("key"), false);
        // Set animation should return true
        p.anim_set("key", 1, 0);
        QCOMPARE(p.property_exists("key"), true);
        // Cleared should return false
        p.clear("key");
        QCOMPARE(p.property_exists("key"), false);
    }

    void SetString()
    {
        Properties p;
        p.set_string("foo", "123.4");
        QCOMPARE(p.get("foo"), "123.4");
        QCOMPARE(p.get_int("foo"), 123);
        QCOMPARE(p.get_double("foo"), 123.4);
    }
};

QTEST_APPLESS_MAIN(TestProperties)

#include "test_properties.moc"
