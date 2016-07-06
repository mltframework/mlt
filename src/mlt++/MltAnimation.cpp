/**
 * MltAnimation.cpp - MLT Wrapper
 * Copyright (C) 2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <stdlib.h>
#include "MltAnimation.h"
using namespace Mlt;


Animation::Animation()
	: instance( 0 )
{
}

Animation::Animation( mlt_animation animation )
	: instance( animation )
{
}

Animation::Animation( const Animation &animation )
	: instance( animation.instance )
{
}

Animation::~Animation()
{
	// Do not call mlt_animation_close() because mlt_animation is not reference-
	// counted, and typically a mlt_properties owns it.
	instance = 0;
}

bool Animation::is_valid() const
{
	return instance != 0;
}

mlt_animation Animation::get_animation() const
{
	return instance;
}

Animation &Animation::operator=( const Animation &animation )
{
	if ( this != &animation )
	{
		instance = animation.instance;
	}
	return *this;
}

int Animation::length()
{
	return mlt_animation_get_length( instance );
}

int Animation::get_item( int position, bool& is_key, mlt_keyframe_type& type )
{
	struct mlt_animation_item_s item;
	item.property = NULL;
	int error = mlt_animation_get_item( instance, &item, position );
	if ( !error )
	{
		is_key = item.is_key;
		type = item.keyframe_type;
	}
	return error;
}

bool Animation::is_key( int position )
{
	struct mlt_animation_item_s item;
	item.is_key = 0;
	item.property = NULL;
	mlt_animation_get_item( instance, &item, position );
	return item.is_key;
}

mlt_keyframe_type Animation::keyframe_type( int position )
{
	struct mlt_animation_item_s item;
	item.property = NULL;
	int error = mlt_animation_get_item( instance, &item, position );
	if ( !error )
		return item.keyframe_type;
	else
		return (mlt_keyframe_type) -1;
}

int Animation::next_key( int position )
{
	struct mlt_animation_item_s item;
	item.property = NULL;
	int error = mlt_animation_next_key( instance, &item, position );
	if ( !error )
		return item.frame;
	else
		return error;
}

int Animation::previous_key( int position )
{
	struct mlt_animation_item_s item;
	item.property = NULL;
	int error = mlt_animation_prev_key( instance, &item, position );
	if ( !error )
		return item.frame;
	else
		return error;
}

int Animation::key_count()
{
	return mlt_animation_key_count( instance );
}

int Animation::key_get( int index, int& frame, mlt_keyframe_type& type )
{
	struct mlt_animation_item_s item;
	item.property = NULL;
	int error = mlt_animation_key_get( instance, &item, index );
	if ( !error )
	{
		frame = item.frame;
		type = item.keyframe_type;
	}
	return error;
}

int Animation::key_get_frame( int index )
{
	struct mlt_animation_item_s item;
	item.is_key = 0;
	item.property = NULL;
	int error = mlt_animation_key_get( instance, &item, index );
	if ( !error )
		return item.frame;
	else
		return -1;
}

mlt_keyframe_type Animation::key_get_type( int index )
{
	struct mlt_animation_item_s item;
	item.property = NULL;
	int error = mlt_animation_key_get( instance, &item, index );
	if ( !error )
		return item.keyframe_type;
	else
		return (mlt_keyframe_type) -1;
}

void Animation::set_length( int length )
{
	return mlt_animation_set_length( instance, length );
}

int Animation::remove( int position )
{
	return mlt_animation_remove( instance, position );
}

void Animation::interpolate()
{
	mlt_animation_interpolate( instance );
}

char *Animation::serialize_cut( int in, int out )
{
	return mlt_animation_serialize_cut( instance, in, out );
}
