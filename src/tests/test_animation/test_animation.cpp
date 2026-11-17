/*
 * Copyright (C) 2015-2026 Meltytech, LLC
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

class TestAnimation : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void DefaultConstructorIsInvalid()
    {
        Animation a;
        QVERIFY(!a.is_valid());
    }

    void ConstructFromProperties()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation *a = p.get_anim("foo");
        QVERIFY(a);
        QVERIFY(a->is_valid());
        delete a;
        a = p.get_anim("bar");
        QVERIFY(a);
        QVERIFY(!a->is_valid());
    }

    void ConstructFromCType()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation a1(p.get_animation("foo"));
        QVERIFY(a1.is_valid());
        Animation a2(a1.get_animation());
        QVERIFY(a2.is_valid());
        QVERIFY(a1.get_animation() == a2.get_animation());
    }

    void CopyConstructor()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation a1(p.get_animation("foo"));
        QVERIFY(a1.is_valid());
        Animation a2(a1);
        QVERIFY(a2.is_valid());
        QVERIFY(a1.get_animation() == a2.get_animation());
    }

    void Assignment()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation a1 = p.get_animation("foo");
        QVERIFY(a1.is_valid());
        Animation a2;
        a2 = a1;
        QVERIFY(a2.is_valid());
        QVERIFY(a1.get_animation() == a2.get_animation());
    }

    void LengthIsCorrect()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.length(), 10);
    }

    void IsAKeyFrame()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QVERIFY(!a.is_key(5));
        QVERIFY(a.is_key(10));
    }

    void KeyFrameTypeIsDiscrete()
    {
        Properties p;
        p.anim_set("foo", "bar", 10);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.keyframe_type(0), mlt_keyframe_discrete);
        QCOMPARE(a.keyframe_type(10), mlt_keyframe_discrete);
        QCOMPARE(a.keyframe_type(11), mlt_keyframe_discrete);
    }

    void KeyFrameTypeIsLinear()
    {
        Properties p;
        p.anim_set("foo", 1, 10);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.keyframe_type(0), mlt_keyframe_linear);
        QCOMPARE(a.keyframe_type(10), mlt_keyframe_linear);
        QCOMPARE(a.keyframe_type(11), mlt_keyframe_linear);
    }

    void KeyFrameTypeIsSmooth()
    {
        Properties p;
        int pos = 10;
        p.anim_set("foo", 1, pos, pos, mlt_keyframe_smooth);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.keyframe_type(0), mlt_keyframe_smooth);
        QCOMPARE(a.keyframe_type(10), mlt_keyframe_smooth);
        QCOMPARE(a.keyframe_type(11), mlt_keyframe_smooth);
    }

    void GetItem()
    {
        Properties p;
        int pos = 10;
        p.anim_set("foo", 1, pos, pos, mlt_keyframe_smooth);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        bool is_key = true;
        mlt_keyframe_type type = mlt_keyframe_linear;
        int error = a.get_item(10, is_key, type);
        QVERIFY(!error);
        QVERIFY(is_key);
        QCOMPARE(type, mlt_keyframe_smooth);
        error = a.get_item(1, is_key, type);
        QVERIFY(!error);
        QVERIFY(!is_key);
        QCOMPARE(type, mlt_keyframe_smooth);
    }

    void AnimationFromStringProperty()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        Animation a = p.get_animation("foo");
        QVERIFY(!a.is_valid());
        // Cause the string to be interpreted as animated value.
        p.anim_get("foo", 0);
        a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.length(), 100);
    }

    void SetLength()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.length(), 100);
        a.set_length(200);
        QCOMPARE(a.length(), 200);
        a.set_length(60);
        QCOMPARE(a.length(), 60);
        QCOMPARE(a.serialize_cut(), "50=100;60=60");
    }

    void RemoveMiddleKeyframe()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        int error = a.remove(60);
        QVERIFY(!error);
        QCOMPARE(a.serialize_cut(), "50=100;100=0");
        QCOMPARE(a.length(), 100);
        QCOMPARE(p.anim_get_int("foo", 50), 100);
        QCOMPARE(p.anim_get_int("foo", 75), 50);
        QCOMPARE(p.anim_get_int("foo", 100), 0);
    }

    void RemoveFirstKeyframe()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        int error = a.remove(0);
        QVERIFY(error);
        error = a.remove(50);
        QVERIFY(!error);
        QCOMPARE(a.serialize_cut(), "60=60;100=0");
        QCOMPARE(a.length(), 100);
        QCOMPARE(p.anim_get_int("foo", 50), 60);
        QCOMPARE(p.anim_get_int("foo", 80), 30);
        QCOMPARE(p.anim_get_int("foo", 100), 0);
    }

    void RemoveLastKeyframe()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        int error = a.remove(101);
        QVERIFY(error);
        error = a.remove(100);
        QVERIFY(!error);
        QCOMPARE(a.serialize_cut(), "50=100;60=60");
        QCOMPARE(a.length(), 60);
        QCOMPARE(p.anim_get_int("foo", 50), 100);
        QCOMPARE(p.anim_get_int("foo", 55), 80);
        QCOMPARE(p.anim_get_int("foo", 60), 60);
    }

    void EmptyAnimationIsInvalid()
    {
        Properties p;
        p.set("foo", "");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(!a.is_valid());
    }

    void NonEmptyAnimationKeyCount()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.key_count(), 3);
    }

    void RemoveKeyframeCount()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        a.remove(50);
        QCOMPARE(a.key_count(), 2);
    }

    void GetKeyFrame()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        int frame = -1;
        mlt_keyframe_type type = mlt_keyframe_smooth;
        int error = a.key_get(0, frame, type);
        QVERIFY(!error);
        QCOMPARE(frame, 50);
        QCOMPARE(type, mlt_keyframe_linear);
        QCOMPARE(a.key_count(), 3);
        QCOMPARE(a.key_get_frame(0), 50);
        QCOMPARE(a.key_get_frame(1), 60);
        QCOMPARE(a.key_get_frame(2), 100);
        QCOMPARE(a.key_get_frame(3), -1);
        QCOMPARE(a.keyframe_type(0), mlt_keyframe_linear);
        QCOMPARE(a.keyframe_type(1), mlt_keyframe_linear);
        QCOMPARE(a.keyframe_type(2), mlt_keyframe_linear);
    }

    void SerializesInTimeFormat()
    {
        Profile profile;
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.serialize_cut(mlt_time_clock), "00:00:02.000=100;00:00:02.400=60;00:00:04.000=0");
        QCOMPARE(a.serialize_cut(mlt_time_smpte_ndf),
                 "00:00:02:00=100;00:00:02:10=60;00:00:04:00=0");
    }

    void GetPropertyInTimeFormat()
    {
        Profile profile;
        Properties p;
        p.set("_profile", profile.get_profile(), 0);
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        for (int i = 0; i < p.count(); i++) {
            if (!qstrcmp(p.get_name(i), "foo")) {
                QCOMPARE(p.get(i, mlt_time_clock),
                         "00:00:02.000=100;00:00:02.400=60;00:00:04.000=0");
                QCOMPARE(p.get(i, mlt_time_smpte_ndf),
                         "00:00:02:00=100;00:00:02:10=60;00:00:04:00=0");
                break;
            }
        }
    }

    void AnimationClears()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        p.clear("foo");
        QCOMPARE(p.get_animation("foo"), mlt_animation(0));
    }

    void CanBeEscapedWithQuotes()
    {
        Properties p;
        p.set("foo", "\"50=100; 60=60; 100=0\"");
        // Quotes are retained when using the non-anim getter.
        QCOMPARE(p.get("foo"), "\"50=100; 60=60; 100=0\"");
        // Quotes are removed when using the anim getter.
        QCOMPARE(p.anim_get("foo", 0), "50=100; 60=60; 100=0");
        // Anim strings may contain delimiters and equal signs if quoted.
        p.set("foo", "50=100; 60=\"60; 100=0\";0=\"hello=world\"");
        QVERIFY(p.is_anim("foo"));
        QCOMPARE(p.anim_get("foo", 0), "hello=world");
        QCOMPARE(p.anim_get("foo", 50), "100");
        QCOMPARE(p.anim_get("foo", 60), "60; 100=0");
    }

    void UrlWithEqualSignIsNotAnimation()
    {
        Properties p;
        // A URL containing '=' in query parameters must not be treated as animation.
        const char *url = "https://example.com/video?token=abc123&quality=high";
        p.set("foo", url);
        QVERIFY(!p.is_anim("foo"));
        QCOMPARE(p.get("foo"), url);
        QCOMPARE(p.anim_get("foo", 0), url);
        // Verify the property is not animated.
        p.anim_get("foo", 0);
        QVERIFY(!p.get_animation("foo"));
    }

    void ParseUntilJunk()
    {
        Properties p;
        // "foo=bar" is junk and parsing will stop after it
        const char *str = "50=100;foo=bar;60=120";
        p.set("foo", str);
        QVERIFY(p.is_anim("foo"));
        QCOMPARE(p.get("foo"), str);
        QCOMPARE(p.anim_get("foo", 50), "100");
        QCOMPARE(p.anim_get("foo", 60), "100"); // NOT 120
        // Verify the property is animated and only has one key.
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.key_count(), 1);
    }

    void ShiftFramesPositive()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        a.shift_frames(60);
        QCOMPARE(a.key_get_frame(0), 110);
        QCOMPARE(a.key_get_frame(1), 120);
        QCOMPARE(a.key_get_frame(2), 160);
        QCOMPARE(a.key_get_frame(3), -1);
    }

    void ShiftFramesNegative()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        a.shift_frames(-60);
        QCOMPARE(a.key_get_frame(0), -10);
        QCOMPARE(a.key_get_frame(1), 0);
        QCOMPARE(a.key_get_frame(2), 40);
        QCOMPARE(a.key_get_frame(3), -1);
    }

    void NextKey()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());

        int key;
        bool ret;

        ret = a.next_key(0, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 50);

        ret = a.next_key(59, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 60);

        ret = a.next_key(60, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 60);

        ret = a.next_key(61, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 100);

        ret = a.next_key(100, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 100);

        key = 7; // random value
        ret = a.next_key(101, key);
        QCOMPARE(ret, true); // error - No next key
        QCOMPARE(key, 7);    // Not modified on error
    }

    void PreviousKey()
    {
        Properties p;
        p.set("foo", "50=100; 60=60; 100=0");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());

        int key;
        bool ret;

        key = 7; // random value
        ret = a.previous_key(0, key);
        QCOMPARE(ret, true); // error - no previous key
        QCOMPARE(key, 7);    // Not modified on error

        ret = a.previous_key(59, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 50);

        ret = a.previous_key(60, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 60);

        ret = a.previous_key(61, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 60);

        ret = a.previous_key(100, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 100);

        ret = a.previous_key(101, key);
        QCOMPARE(ret, false);
        QCOMPARE(key, 100);
    }

    void PreviousKeyOnEmptyAnimation()
    {
        mlt_animation animation = mlt_animation_new();
        QVERIFY(animation);

        struct mlt_animation_item_s item = {};
        item.frame = 7;
        QCOMPARE(mlt_animation_prev_key(animation, &item, 0), 1);
        QCOMPARE(item.frame, 7);

        mlt_animation_close(animation);
    }

    void PreviousKeyAfterRemovingLastKeyframe()
    {
        Properties p;
        p.anim_set("foo", 100, 50);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        QCOMPARE(a.remove(50), 0);
        QCOMPARE(a.key_count(), 0);

        int key = 7;
        QCOMPARE(a.previous_key(50, key), true);
        QCOMPARE(key, 7);
    }

    void LinearInterpolationOneKey()
    {
        Properties p;
        p.set("foo", "50=10");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 10 should all be 10
        for (int i = 0; i <= 50; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 10);
        }
    }

    void SmoothInterpolationOneKey()
    {
        Properties p;
        p.set("foo", "50~=10");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 10 should all be 10
        for (int i = 0; i <= 50; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 10);
        }
    }

    void LinearInterpolationTwoKey()
    {
        Properties p;
        double prev;
        p.set("foo", "10=50; 20=100");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 10 should all be 50
        for (int i = 0; i <= 10; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 50);
        }
        // Values from 10 to 20 should step by 5
        prev = 50 - 5;
        for (int i = 10; i <= 20; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, prev + 5);
            prev = current;
        }
        // Values after 20 should all be 100
        for (int i = 20; i <= 30; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 100);
        }
    }

    void SmoothInterpolationTwoKey()
    {
        Properties p;
        double prev;
        p.set("foo", "10~=50; 20~=100");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 10 should all be 50
        for (int i = 0; i <= 10; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 50);
        }
        // Values from 10 to 20 should increase but not exceed 100
        prev = 49;
        for (int i = 10; i <= 20; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current <= 100);
            prev = current;
        }
        // Values after 20 should all be 100
        for (int i = 20; i <= 30; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 100);
        }
    }

    void LinearInterpolationThreeKey()
    {
        Properties p;
        double prev;
        p.set("foo", "10=50; 20=100; 30=50");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 10 should all be 50
        for (int i = 0; i <= 10; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 50);
        }
        // Values from 10 to 20 should step up by 5
        prev = 50 - 5;
        for (int i = 10; i <= 20; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, prev + 5);
            prev = current;
        }
        // Values from 20 to 30 should step down by 5
        prev = 100 + 5;
        for (int i = 20; i <= 30; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, prev - 5);
            prev = current;
        }
        // Values after 30 should all be 50
        for (int i = 30; i <= 40; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 50);
        }
    }

    void SmoothInterpolationThreeKey()
    {
        Properties p;
        double prev;
        p.set("foo", "10~=50; 20~=100; 30~=50");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 10 should all be 50
        for (int i = 0; i <= 10; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 50);
        }
        // Values from 10 to 20 should increase but not exceed 100
        prev = 49;
        for (int i = 10; i <= 20; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current <= 100);
            prev = current;
        }
        // Test two arbitrary intermediate points
        QCOMPARE(p.anim_get_double("foo", 13), 64.475);
        QCOMPARE(p.anim_get_double("foo", 18), 95.6);
        // Values from 20 to 30 should decrease but not exceed 50
        prev = 101;
        for (int i = 20; i <= 30; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current < prev);
            QVERIFY(current >= 50);
            prev = current;
        }
        // Test two arbitrary intermediate points
        QCOMPARE(p.anim_get_double("foo", 23), 90.775);
        QCOMPARE(p.anim_get_double("foo", 28), 58.4);
        // Values after 30 should all be 50
        for (int i = 30; i <= 40; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 50);
        }
    }

    void LinearDoesNotReverse()
    {
        Properties p;
        double prev;
        // This sequence of keyframes has abrupt changes. For some interpolation algorithms, this
        // can result in values changing direction along the interpolation path (cusp or
        // overshoot).
        // The purpose of this test is to ensure that values do not reverse direction (except as
        // expected at keyframes if the user specified a direction change).
        p.set("foo",
              "50=0; 60=100; 100=110; 150=200; 200=110; 240=100; 260=50; 300=200; 301=10; 350=11");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 50 should all be 0
        for (int i = 0; i <= 50; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 0);
        }
        // Values from 50 to 150 should only go up
        prev = 0;
        for (int i = 50; i <= 150; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current >= prev);
            prev = current;
        }
        // Values from 150 to 260 should only go down
        prev = 201;
        for (int i = 150; i <= 260; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current < prev);
            prev = current;
        }
        // Values from 260 to 300 should only go up
        prev = 49;
        for (int i = 260; i <= 300; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 200.01);
            prev = current;
        }
        // Values above 301 to 350 should go up from 10 to 11
        prev = 9.99;
        for (int i = 301; i <= 350; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 11.01);
            prev = current;
        }
        // Values above 350 should be 11
        for (int i = 350; i <= 360; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 11);
        }
    }

    void SmoothLooseCanReverse()
    {
        Properties p;
        p.set("foo",
              "50~=0; 60~=100; 100~=110; 150~=200; 200~=110; 240~=100; 260~=50; 300~=200; 301~=10; "
              "350~=11");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 50 should all be 0
        for (int i = 0; i <= 50; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 0);
        }
        // Values from 50 to 150 will cusp
        for (int i = 50; i <= 150; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current >= 0);
            QVERIFY(current < 200.01);
        }
        // Values from 150 to 260 will cusp
        for (int i = 150; i <= 260; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current < 200.01);
            QVERIFY(current > 47);
        }
        // Values from 260 to 300 will overshoot
        for (int i = 260; i <= 300; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > 49.99);
            QVERIFY(current < 205.01);
        }
        // Values above 301 to 350 will overshoot below 10
        for (int i = 301; i <= 350; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > -3.8);
            QVERIFY(current < 11.01);
        }
        // Values above 350 should be 11
        for (int i = 350; i <= 360; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 11);
        }
    }

    void SmoothNaturalDoesNotReverse()
    {
        Properties p;
        double prev;
        // This sequence of keyframes has abrupt changes. For some interpolation algorithms, this
        // can result in values changing direction along the interpolation path (cusp or
        // overshoot).
        // The purpose of this test is to ensure that values do not reverse direction (except as
        // expected at keyframes if the user specified a direction change).
        p.set("foo",
              "50$=0; 60$=100; 100$=110; 150$=200; 200$=110; 240$=100; 260$=50; 300$=200; 301$=10; "
              "350$=11");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 50 should all be 0
        for (int i = 0; i <= 50; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 0);
        }
        // Values from 50 to 150 should only go up
        prev = -0.01;
        for (int i = 50; i <= 150; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 200.01);
            prev = current;
        }
        // Values from 150 to 260 should only go down
        prev = 200.01;
        for (int i = 150; i <= 260; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current < prev);
            prev = current;
        }
        // Values from 260 to 300 should only go up
        prev = 49.99;
        for (int i = 260; i <= 300; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 200.01);
            prev = current;
        }
        // Values above 301 to 350 should go up from 10 to 11
        prev = 9.99;
        for (int i = 301; i <= 350; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 11.01);
            prev = current;
        }
        // Values above 350 should be 11
        for (int i = 350; i <= 360; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 11);
        }
    }

    void SmoothTightDoesNotReverse()
    {
        Properties p;
        double prev;
        // This sequence of keyframes has abrupt changes. For some interpolation algorithms, this
        // can result in values changing direction along the interpolation path (cusp or
        // overshoot).
        // The purpose of this test is to ensure that values do not reverse direction (except as
        // expected at keyframes if the user specified a direction change).
        p.set("foo",
              "50-=0; 60-=100; 100-=110; 150-=200; 200-=110; 240-=100; 260-=50; 300-=200; 301-=10; "
              "350-=11");
        // Cause the string to be interpreted as animated value.
        p.anim_get_int("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.is_valid());
        // Values from 0 to 50 should all be 0
        for (int i = 0; i <= 50; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 0);
        }
        // Values from 50 to 150 should only go up
        prev = -0.01;
        for (int i = 50; i <= 150; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 200.01);
            prev = current;
        }
        // Values from 150 to 260 should only go down
        prev = 200.01;
        for (int i = 150; i <= 260; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current < prev);
            prev = current;
        }
        // Values from 260 to 300 should only go up
        prev = 49.99;
        for (int i = 260; i <= 300; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 200.01);
            prev = current;
        }
        // Values above 301 to 350 should go up from 10 to 11
        prev = 9.99;
        for (int i = 301; i <= 350; i++) {
            double current = p.anim_get_double("foo", i);
            QVERIFY(current > prev);
            QVERIFY(current < 11.01);
            prev = current;
        }
        // Values above 350 should be 11
        for (int i = 350; i <= 360; i++) {
            double current = p.anim_get_double("foo", i);
            QCOMPARE(current, 11);
        }
    }

    void EaseIn()
    {
        Properties p;
        p.set("sinu", "0a=0; 100a=100;");
        p.set("quad", "0d=0; 100d=100;");
        p.set("cube", "0g=0; 100g=100;");
        p.set("quar", "0j=0; 100j=100;");
        p.set("quin", "0m=0; 100m=100;");
        p.set("expo", "0p=0; 100p=100;");
        p.set("circ", "0s=0; 100s=100;");
        p.set("back", "0v=0; 100v=100;");
        p.set("elas", "0y=0; 100y=100;");
        p.set("boun", "0B=0; 100B=100;");
        p.anim_get_int("sinu", 0);
        p.anim_get_int("quad", 0);
        p.anim_get_int("cube", 0);
        p.anim_get_int("quar", 0);
        p.anim_get_int("quin", 0);
        p.anim_get_int("expo", 0);
        p.anim_get_int("circ", 0);
        p.anim_get_int("back", 0);
        p.anim_get_int("elas", 0);
        p.anim_get_int("boun", 0);

        // fprintf(stderr, "sinu\tquad\tcube\tquar\tquin\texpo\tcirc\tback\telas\tboun\n");

        for (int i = 0; i <= 100; i++) {
            double sinu = p.anim_get_double("sinu", i);
            double quad = p.anim_get_double("quad", i);
            double cube = p.anim_get_double("cube", i);
            double quar = p.anim_get_double("quar", i);
            double quin = p.anim_get_double("quin", i);
            double expo = p.anim_get_double("expo", i);
            double circ = p.anim_get_double("circ", i);
            double back = p.anim_get_double("back", i);
            double elas = p.anim_get_double("elas", i);
            double boun = p.anim_get_double("boun", i);

            // fprintf(stderr,"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", sinu, quad, cube, quar, quin, expo, circ, back, elas, boun);

            QVERIFY(sinu >= 0.0);
            QVERIFY(sinu <= 100.0);
            QVERIFY(quad >= 0.0);
            QVERIFY(quad <= 100.0);
            QVERIFY(quad <= sinu);
            QVERIFY(cube >= 0.0);
            QVERIFY(cube <= 100.0);
            QVERIFY(cube <= quad);
            QVERIFY(quar >= 0.0);
            QVERIFY(quar <= 100.0);
            QVERIFY(quar <= cube);
            QVERIFY(quin >= 0.0);
            QVERIFY(quin <= 100.0);
            QVERIFY(quin <= quar);
            QVERIFY(expo >= 0.0);
            QVERIFY(expo <= 100.0);
            QVERIFY(circ >= 0.0);
            QVERIFY(circ <= 100.0);
            QVERIFY(back >= -37.88);
            QVERIFY(back <= 100.0);
            QVERIFY(elas >= -36.39);
            QVERIFY(elas <= 100.0);
            QVERIFY(boun >= -0.00000001);
            QVERIFY(boun <= 100.0);
        }
    }

    void EaseOut()
    {
        Properties p;
        p.set("sinu", "0b=0; 100b=100;");
        p.set("quad", "0e=0; 100e=100;");
        p.set("cube", "0h=0; 100h=100;");
        p.set("quar", "0k=0; 100k=100;");
        p.set("quin", "0n=0; 100n=100;");
        p.set("expo", "0q=0; 100q=100;");
        p.set("circ", "0t=0; 100t=100;");
        p.set("back", "0w=0; 100w=100;");
        p.set("elas", "0z=0; 100z=100;");
        p.set("boun", "0C=0; 100C=100;");
        p.anim_get_int("sinu", 0);
        p.anim_get_int("quad", 0);
        p.anim_get_int("cube", 0);
        p.anim_get_int("quar", 0);
        p.anim_get_int("quin", 0);
        p.anim_get_int("expo", 0);
        p.anim_get_int("circ", 0);
        p.anim_get_int("back", 0);
        p.anim_get_int("elas", 0);
        p.anim_get_int("boun", 0);

        // fprintf(stderr, "sinu\tquad\tcube\tquar\tquin\texpo\tcirc\tback\telas\tboun\n");

        for (int i = 0; i <= 100; i++) {
            double sinu = p.anim_get_double("sinu", i);
            double quad = p.anim_get_double("quad", i);
            double cube = p.anim_get_double("cube", i);
            double quar = p.anim_get_double("quar", i);
            double quin = p.anim_get_double("quin", i);
            double expo = p.anim_get_double("expo", i);
            double circ = p.anim_get_double("circ", i);
            double back = p.anim_get_double("back", i);
            double elas = p.anim_get_double("elas", i);
            double boun = p.anim_get_double("boun", i);

            // fprintf(stderr,"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", sinu, quad, cube, quar, quin, expo, circ, back, elas, boun);

            QVERIFY(sinu >= 0.0);
            QVERIFY(sinu <= 100.0);
            QVERIFY(quad >= 0.0);
            QVERIFY(quad <= 100.0);
            QVERIFY(quad >= sinu);
            QVERIFY(cube >= 0.0);
            QVERIFY(cube <= 100.0);
            QVERIFY(cube >= quad);
            QVERIFY(quar >= 0.0);
            QVERIFY(quar <= 100.0);
            QVERIFY(quar >= cube);
            QVERIFY(quin >= 0.0);
            QVERIFY(quin <= 100.0);
            QVERIFY(quin >= quar);
            QVERIFY(expo >= 0.0);
            QVERIFY(expo <= 100.0);
            QVERIFY(circ >= 0.0);
            QVERIFY(circ <= 100.0);
            QVERIFY(back >= 0.0);
            QVERIFY(back <= 137.9);
            QVERIFY(elas >= 0.0);
            QVERIFY(elas <= 136.4);
            QVERIFY(boun >= 0.0);
            QVERIFY(boun <= 100.1);
        }
    }

    void EaseInOut()
    {
        Properties p;
        p.set("sinu", "0c=0; 100c=100;");
        p.set("quad", "0f=0; 100f=100;");
        p.set("cube", "0i=0; 100i=100;");
        p.set("quar", "0l=0; 100l=100;");
        p.set("quin", "0o=0; 100o=100;");
        p.set("expo", "0r=0; 100r=100;");
        p.set("circ", "0u=0; 100u=100;");
        p.set("back", "0x=0; 100x=100;");
        p.set("elas", "0A=0; 100A=100;");
        p.set("boun", "0D=0; 100D=100;");
        p.anim_get_int("sinu", 0);
        p.anim_get_int("quad", 0);
        p.anim_get_int("cube", 0);
        p.anim_get_int("quar", 0);
        p.anim_get_int("quin", 0);
        p.anim_get_int("expo", 0);
        p.anim_get_int("circ", 0);
        p.anim_get_int("back", 0);
        p.anim_get_int("elas", 0);
        p.anim_get_int("boun", 0);

        // fprintf(stderr, "sinu\tquad\tcube\tquar\tquin\texpo\tcirc\tback\telas\tboun\n");

        for (int i = 0; i <= 100; i++) {
            double sinu = p.anim_get_double("sinu", i);
            double quad = p.anim_get_double("quad", i);
            double cube = p.anim_get_double("cube", i);
            double quar = p.anim_get_double("quar", i);
            double quin = p.anim_get_double("quin", i);
            double expo = p.anim_get_double("expo", i);
            double circ = p.anim_get_double("circ", i);
            double back = p.anim_get_double("back", i);
            double elas = p.anim_get_double("elas", i);
            double boun = p.anim_get_double("boun", i);

            // fprintf(stderr,"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", sinu, quad, cube, quar, quin, expo, circ, back, elas, boun);

            if (i < 50) {
                QVERIFY(quad <= sinu);
                QVERIFY(cube <= quad);
                QVERIFY(quar <= cube);
                QVERIFY(quin <= quar);
            } else {
                QVERIFY(quad >= sinu);
                QVERIFY(cube >= quad);
                QVERIFY(quar >= cube);
                QVERIFY(quin >= quar);
            }
            QVERIFY(sinu >= 0.0);
            QVERIFY(sinu <= 100.0);
            QVERIFY(quad >= 0.0);
            QVERIFY(quad <= 100.0);
            QVERIFY(cube >= 0.0);
            QVERIFY(cube <= 100.0);
            QVERIFY(quar >= 0.0);
            QVERIFY(quar <= 100.0);
            QVERIFY(quin >= 0.0);
            QVERIFY(quin <= 100.0);
            QVERIFY(expo >= 0.0);
            QVERIFY(expo <= 100.0);
            QVERIFY(circ >= 0.0);
            QVERIFY(circ <= 100.0);
            QVERIFY(back >= -19.0);
            QVERIFY(back <= 119.0);
            QVERIFY(elas >= -18.2);
            QVERIFY(elas <= 118.2);
            QVERIFY(boun >= -0.01);
            QVERIFY(boun <= 100.1);
        }
    }

    void LoopRepeatsCycleWithoutRepeatingFrame()
    {
        Properties p;
        p.set("foo", "0=10; 30=50; 60=10; loop");
        // The last keyframe's position shows the first keyframe's value.
        QCOMPARE(p.anim_get_double("foo", 60), 10.0);
        // The frames on either side of the loop point differ from it.
        QVERIFY(p.anim_get_double("foo", 59) > 10.0);
        QVERIFY(p.anim_get_double("foo", 61) > 10.0);
        for (int i = 0; i < 60; i++) {
            double expected = p.anim_get_double("foo", i);
            QCOMPARE(p.anim_get_double("foo", i + 60), expected);
            QCOMPARE(p.anim_get_double("foo", i + 600), expected);
        }
    }

    void LoopSuffixIsParsed()
    {
        Properties p;
        p.set("plain", "0=10;60=10");
        p.set("loop", "0=10;60=10;loop");
        p.set("spaces", "0=10; 60=10 ;  loop ; ");
        p.set("middle", "0=10;loop;60=20");
        p.set("value", "0=foo;60=loop");
        QVERIFY(p.is_anim("loop"));
        p.anim_get_double("plain", 0);
        p.anim_get_double("loop", 0);
        p.anim_get_double("spaces", 0);
        p.anim_get_double("middle", 0);
        p.anim_get("value", 0);
        QVERIFY(!Animation(p.get_animation("plain")).get_loop());
        QVERIFY(Animation(p.get_animation("loop")).get_loop());
        QVERIFY(Animation(p.get_animation("spaces")).get_loop());
        QCOMPARE(Animation(p.get_animation("spaces")).key_count(), 2);
        // "loop" is only recognized as the last item. Elsewhere it ends
        // parsing like any other unexpected item.
        Animation middle = p.get_animation("middle");
        QVERIFY(!middle.get_loop());
        QCOMPARE(middle.key_count(), 1);
        // A keyframe value of "loop" is not the suffix.
        Animation value = p.get_animation("value");
        QVERIFY(!value.get_loop());
        QCOMPARE(value.key_count(), 2);
        QCOMPARE(QString(p.anim_get("value", 60)), QString("loop"));
    }

    void LoopIsResetWhenStringChanges()
    {
        Properties p;
        p.set("foo", "0=10;30=50;60=10;loop");
        QCOMPARE(p.anim_get_double("foo", 75), 30.0);
        p.set("foo", "0=10;30=50;60=10");
        QCOMPARE(p.anim_get_double("foo", 75), 10.0);
        QVERIFY(!Animation(p.get_animation("foo")).get_loop());
    }

    void LoopSerializesSuffix()
    {
        Properties p;
        p.set("foo", "0=10;30=50;60=10;loop");
        p.anim_get_double("foo", 0);
        QCOMPARE(QString(p.get("foo")), QString("0=10;30=50;60=10;loop"));
        // Editing keyframes keeps the loop.
        p.anim_set("foo", 20, 15);
        QCOMPARE(QString(p.get("foo")), QString("0=10;15=20;30=50;60=10;loop"));
        Animation a = p.get_animation("foo");
        char *cut = a.serialize_cut(mlt_time_frames, 0, 90);
        QCOMPARE(QString(cut), QString("0=10;15=20;30=50;60=10;loop"));
        free(cut);
        // The serialized string parses back to a looping animation.
        Properties q;
        q.set("foo", p.get("foo"));
        q.anim_get_double("foo", 0);
        QVERIFY(Animation(q.get_animation("foo")).get_loop());
        QCOMPARE(q.anim_get_double("foo", 75), 20.0);
    }

    void LoopToggleKeepsKeyframes()
    {
        Properties p;
        p.set("foo", "0=10;30=50;60=10");
        p.anim_get_double("foo", 0);
        Animation a = p.get_animation("foo");
        a.set_loop(true);
        QVERIFY(a.get_loop());
        QCOMPARE(p.anim_get_double("foo", 75), 30.0);
        QCOMPARE(QString(p.get("foo")), QString("0=10;30=50;60=10;loop"));
        a.set_loop(false);
        QVERIFY(!a.get_loop());
        QCOMPARE(p.anim_get_double("foo", 75), 10.0);
        QCOMPARE(QString(p.get("foo")), QString("0=10;30=50;60=10"));
        QCOMPARE(a.key_count(), 3);
    }

    void LoopReportsOnlyAuthoredKeyframes()
    {
        Properties p;
        p.set("foo", "0=10; 30=50; 60|=10; loop");
        p.anim_get_double("foo", 0);
        Animation a = p.get_animation("foo");
        QVERIFY(a.get_loop());
        QVERIFY(a.is_key(0));
        QVERIFY(a.is_key(30));
        QVERIFY(a.is_key(60));
        QCOMPARE(a.keyframe_type(60), mlt_keyframe_linear);
        QVERIFY(!a.is_key(90));
        QVERIFY(!a.is_key(120));
        QCOMPARE(a.keyframe_type(90), mlt_keyframe_linear);
    }

    void LoopHoldsBeforeFirstKeyframe()
    {
        Properties p;
        p.set("foo", "10=0; 20=100; 30=0; loop");
        for (int i = 0; i <= 10; i++)
            QCOMPARE(p.anim_get_double("foo", i), 0.0);
        QCOMPARE(p.anim_get_double("foo", 30), 0.0);
        QCOMPARE(p.anim_get_double("foo", 35), 50.0);
        QCOMPARE(p.anim_get_double("foo", 40), 100.0);
        QCOMPARE(p.anim_get_double("foo", 50), 0.0);
    }

    void LoopNeedsTwoKeyframes()
    {
        Properties p;
        p.set("foo", "10=5; loop");
        QCOMPARE(p.anim_get_double("foo", 0), 5.0);
        QCOMPARE(p.anim_get_double("foo", 100), 5.0);
    }

    void LoopSmoothJoinsLikeInteriorKeyframe()
    {
        // The loop point should interpolate exactly like the same keyframes
        // written out for two cycles without looping.
        const char *types[] = {"~", "$", "-"};
        for (const char *t : types) {
            Properties p;
            p.set("loop",
                  QStringLiteral("0%1=0; 10%1=100; 50%1=0; loop").arg(t).toUtf8().constData());
            p.set("unrolled",
                  QStringLiteral("0%1=0; 10%1=100; 50%1=0; 60%1=100; 100%1=0")
                      .arg(t)
                      .toUtf8()
                      .constData());
            for (int i = 40; i < 60; i++)
                QCOMPARE(p.anim_get_double("loop", i), p.anim_get_double("unrolled", i));
        }
    }

    void LoopUsesAuthoredLastKeyframeValue()
    {
        Properties p;
        p.set("rotation", "0=0;60|=360;loop");
        QCOMPARE(p.anim_get_double("rotation", 1), 6.0);
        QCOMPARE(p.anim_get_double("rotation", 59), 354.0);
        QCOMPARE(p.anim_get_double("rotation", 60), 0.0);
        QCOMPARE(p.anim_get_double("rotation", 61), 6.0);
        QCOMPARE(p.anim_get_double("rotation", 119), 354.0);
        Animation a = p.get_animation("rotation");
        struct mlt_animation_item_s item;
        item.property = mlt_property_init();
        QCOMPARE(mlt_animation_key_get(a.get_animation(), &item, 1), 0);
        QCOMPARE(mlt_property_get_double(item.property, 0, NULL), 360.0);
        QCOMPARE(item.keyframe_type, mlt_keyframe_discrete);
        mlt_property_close(item.property);
        QCOMPARE(QString(p.get("rotation")), QString("0=0;60|=360;loop"));
        p.anim_set("rotation", 300.0, 60, 0, mlt_keyframe_discrete);
        QCOMPARE(p.anim_get_double("rotation", 59), 295.0);
        QCOMPARE(p.anim_get_double("rotation", 60), 0.0);
        QCOMPARE(QString(p.get("rotation")), QString("0=0;60|=300;loop"));
        a.set_loop(false);
        QCOMPARE(p.anim_get_double("rotation", 60), 300.0);
    }

    void LoopSmoothDifferentEndpointsUseEndNeighbors()
    {
        Properties p;
        p.set("loop", "0~=0;10~=100;50~=360;loop");
        p.set("plain", "0~=0;10~=100;50~=360");
        for (int i = 0; i < 50; i++)
            QCOMPARE(p.anim_get_double("loop", i), p.anim_get_double("plain", i));
        QCOMPARE(p.anim_get_double("loop", 50), 0.0);
    }

    void LoopDifferentRectAndColorEndpoints()
    {
        Properties p;
        p.set("rect", "0=0 0 10 10;10=100 0 10 10;loop");
        p.set("color", "0=#000000ff;10=#ffffffff;loop");
        QCOMPARE(p.anim_get_rect("rect", 9).x, 90.0);
        QCOMPARE(p.anim_get_rect("rect", 10).x, 0.0);
        QCOMPARE(p.anim_get_rect("rect", 19).x, 90.0);
        mlt_color before = p.anim_get_color("color", 9);
        mlt_color after = p.anim_get_color("color", 10);
        QCOMPARE(before.r, static_cast<uint8_t>(229));
        QCOMPARE(after.r, static_cast<uint8_t>(0));
    }

    void LoopRectAndColor()
    {
        Properties p;
        p.set("rect", "0=0 0 10 10; 30=30 0 10 10; 60=0 0 10 10; loop");
        p.set("color", "0=#ff000000; 30=#ff00ff00; 60=#ff000000; loop");
        mlt_rect r1 = p.anim_get_rect("rect", 15);
        mlt_rect r2 = p.anim_get_rect("rect", 75);
        QCOMPARE(r2.x, r1.x);
        QCOMPARE(p.anim_get_rect("rect", 60).x, 0.0);
        mlt_color c1 = p.anim_get_color("color", 15);
        mlt_color c2 = p.anim_get_color("color", 75);
        QCOMPARE(c2.g, c1.g);
        QCOMPARE(p.anim_get_color("color", 60).g, (uint8_t) 0);
    }
};

QTEST_APPLESS_MAIN(TestAnimation)

#include "test_animation.moc"
