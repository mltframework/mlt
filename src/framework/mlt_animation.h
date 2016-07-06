/**
 * \file mlt_animation.h
 * \brief Property Animation class declaration
 * \see mlt_animation_s
 *
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#ifndef MLT_ANIMATION_H
#define MLT_ANIMATION_H

#include "mlt_types.h"
#include "mlt_property.h"

/** \brief An animation item that represents a keyframe-property combination. */

struct mlt_animation_item_s
{
	int is_key;                      /**< a boolean of whether this is a key frame or an interpolated item */
	int frame;                       /**< the frame number for this instance of the property */
	mlt_property property;           /**< the property for this point in time */
	mlt_keyframe_type keyframe_type; /**< the method of interpolation for this key frame */
};
typedef struct mlt_animation_item_s *mlt_animation_item; /**< pointer to an animation item */

extern mlt_animation mlt_animation_new( );
extern int mlt_animation_parse(mlt_animation self, const char *data, int length, double fps, locale_t locale );
extern int mlt_animation_refresh( mlt_animation self, const char *data, int length );
extern int mlt_animation_get_length( mlt_animation self );
extern void mlt_animation_set_length( mlt_animation self, int length );
extern int mlt_animation_parse_item( mlt_animation self, mlt_animation_item item, const char *data );
extern int mlt_animation_get_item( mlt_animation self, mlt_animation_item item, int position );
extern int mlt_animation_insert( mlt_animation self, mlt_animation_item item );
extern int mlt_animation_remove( mlt_animation self, int position );
extern void mlt_animation_interpolate( mlt_animation self );
extern int mlt_animation_next_key( mlt_animation self, mlt_animation_item item, int position );
extern int mlt_animation_prev_key( mlt_animation self, mlt_animation_item item, int position );
extern char *mlt_animation_serialize_cut( mlt_animation self, int in, int out );
extern char *mlt_animation_serialize( mlt_animation self );
extern int mlt_animation_key_count( mlt_animation self );
extern int mlt_animation_key_get( mlt_animation self, mlt_animation_item item, int index );
extern void mlt_animation_close( mlt_animation self );

#endif

