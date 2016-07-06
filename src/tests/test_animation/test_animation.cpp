/*
 * Copyright (C) 2015 Dan Dennedy <dan@dennedy.org>
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

	void EmptyAnimationIsZeroKeyCount()
	{
		Properties p;
		p.set("foo", "");
		// Cause the string to be interpreted as animated value.
		p.anim_get_int("foo", 0);
		Animation a = p.get_animation("foo");
		QVERIFY(a.is_valid());
		QCOMPARE(a.key_count(), 0);
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
};

QTEST_APPLESS_MAIN(TestAnimation)

#include "test_animation.moc"
