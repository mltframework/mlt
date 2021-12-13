/*
 * Copyright (C) 2015-2021 Dan Dennedy <dan@dennedy.org>
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
		Animation* a = p.get_anim("foo");
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
		QCOMPARE(a.serialize_cut(mlt_time_smpte_ndf), "00:00:02:00=100;00:00:02:10=60;00:00:04:00=0");
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
				QCOMPARE(p.get(i, mlt_time_clock), "00:00:02.000=100;00:00:02.400=60;00:00:04.000=0");
				QCOMPARE(p.get(i, mlt_time_smpte_ndf), "00:00:02:00=100;00:00:02:10=60;00:00:04:00=0");
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
		p.set("foo", "50=100; 60=\"60; 100=0\";\"hello=world\"");
		QCOMPARE(p.anim_get("foo", 0), "hello=world");
		QCOMPARE(p.anim_get("foo", 50), "100");
		QCOMPARE(p.anim_get("foo", 60), "60; 100=0");
	}

	void ShiftFramesPositive()
	{
		Properties p;
		p.set("foo", "50=100; 60=60; 100=0");
		// Cause the string to be interpreted as animated value.
		p.anim_get_int("foo", 0);
		Animation a = p.get_animation("foo");
		QVERIFY(a.is_valid());
		a.shift_frames( 60 );
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
		a.shift_frames( -60 );
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
		QCOMPARE(key, 7); // Not modified on error
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
		QCOMPARE(key, 7); // Not modified on error

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
};

QTEST_APPLESS_MAIN(TestAnimation)

#include "test_animation.moc"
