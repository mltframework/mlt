/*
 * mlt_animation.h -- provides the property animation API
 * Copyright (C) 2004-2013 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MLT_ANIMATION_H
#define MLT_ANIMATION_H

#include "mlt_types.h"
#include "mlt_property.h"

typedef enum {
	mlt_keyframe_discrete,
	mlt_keyframe_linear
} mlt_keyframe_type;

struct mlt_animation_item_s
{
	int is_key; /**< = whether this is a key frame or an interpolated item */
	int frame; /**< The actual frame this corresponds to */
	mlt_property property;
	mlt_keyframe_type keyframe_type;
};
typedef struct mlt_animation_item_s *mlt_animation_item;

struct mlt_animation_s;
typedef struct mlt_animation_s *mlt_animation;

/* Create a new animation object. */
extern mlt_animation mlt_animation_new( );
/* Parse the geometry specification for a given duration and range */
extern int mlt_animation_parse(mlt_animation self, const char *data, int length, double fps, locale_t locale );
/* Conditionally refresh the animation if it's modified */
extern int mlt_animation_refresh( mlt_animation self, const char *data, int length );
/* Get and set the length */
extern int mlt_animation_get_length( mlt_animation self );
extern void mlt_animation_set_length( mlt_animation self, int length );
/* Parse an item - doesn't affect the animation itself but uses current information for evaluation */
/* (item->frame should be specified if not included in the data itself) */
extern int mlt_animation_parse_item( mlt_animation self, mlt_animation_item item, const char *data );
/* Fetch an animation item for an absolute position */
extern int mlt_animation_get_item( mlt_animation self, mlt_animation_item item, int position );
/* Specify an animation item at an absolute position */
extern int mlt_animation_insert( mlt_animation self, mlt_animation_item item );
/* Remove the key at the specified position */
extern int mlt_animation_remove( mlt_animation self, int position );
/* Typically, re-interpolate after a series of insertions or removals. */
extern void mlt_animation_interpolate( mlt_animation self );
/* Get the key at the position or the next following */
extern int mlt_animation_next_key( mlt_animation self, mlt_animation_item item, int position );
extern int mlt_animation_prev_key( mlt_animation self, mlt_animation_item item, int position );
/* Serialize the current animation. */
extern char *mlt_animation_serialize_cut( mlt_animation self, int in, int out );
extern char *mlt_animation_serialize( mlt_animation self );
/* Close and destrory the animation. */
extern void mlt_animation_close( mlt_animation self );

#endif

