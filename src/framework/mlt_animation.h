/**
 * \file mlt_animation.h
 * \brief Property Animation class declaration
 * \see mlt_animation_s
 *
 * Copyright (C) 2004-2026 Meltytech, LLC
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

#include "mlt_export.h"
#include "mlt_property.h"
#include "mlt_types.h"

/** \brief Animation class
 *
 * Once an animation has been constructed using mlt_properties_s, this interface
 * provides a to query and manipulate the animation except for values. One must
 * use mlt_properties_s still to get, set, and change values.
 *
 * \envvar \em MLT_ANIMATION_TIME_FORMAT the time value string format to use,
 * defaults to mlt_time_frames. Use the numeric value of mlt_time_format as
 * the value of this variable.
 *
 * \par Looping
 * An animation can loop instead of holding its last keyframe's value. End the
 * animation string with a "loop" item (for example, "0=10;30=50;60=10;loop"),
 * or call mlt_animation_set_loop(), which adds or removes that item when the
 * animation is serialized.
 * The first and last keyframes define the cycle, and its period is the
 * distance between them. The final segment interpolates toward the authored
 * last keyframe value, but its position evaluates to the first value. For
 * example, "0=0;60=360;loop" evaluates to 354 at frame 59, 0 at frame 60,
 * and 6 at frame 61. The stored endpoint is available via
 * mlt_animation_key_get(). Matching endpoints use cross-boundary neighbors
 * for smooth interpolation; different endpoints use ordinary end neighbors.
 * Positions before the first keyframe hold its value.
 * The "loop" item must be last. Versions of MLT without looping stop parsing
 * at it, keep the keyframes before it, and hold the last keyframe's value.
 */

/** \brief An animation item that represents a keyframe-property combination. */

struct mlt_animation_item_s
{
    int is_key;            /**< a boolean of whether this is a key frame or an interpolated item */
    int frame;             /**< the frame number for this instance of the property */
    mlt_property property; /**< the property for this point in time */
    mlt_keyframe_type keyframe_type; /**< the method of interpolation for this key frame */
};
typedef struct mlt_animation_item_s *mlt_animation_item; /**< pointer to an animation item */

MLT_EXPORT mlt_animation mlt_animation_new();
MLT_EXPORT int mlt_animation_parse(
    mlt_animation self, const char *data, int length, double fps, mlt_locale_t locale);
MLT_EXPORT int mlt_animation_refresh(mlt_animation self, const char *data, int length);
MLT_EXPORT int mlt_animation_get_length(mlt_animation self);
MLT_EXPORT void mlt_animation_set_length(mlt_animation self, int length);
MLT_EXPORT int mlt_animation_parse_item(mlt_animation self,
                                        mlt_animation_item item,
                                        const char *data);
MLT_EXPORT int mlt_animation_get_item(mlt_animation self, mlt_animation_item item, int position);
MLT_EXPORT int mlt_animation_insert(mlt_animation self, mlt_animation_item item);
MLT_EXPORT int mlt_animation_remove(mlt_animation self, int position);
MLT_EXPORT void mlt_animation_interpolate(mlt_animation self);
MLT_EXPORT int mlt_animation_next_key(mlt_animation self, mlt_animation_item item, int position);
MLT_EXPORT int mlt_animation_prev_key(mlt_animation self, mlt_animation_item item, int position);
MLT_EXPORT char *mlt_animation_serialize_cut_tf(mlt_animation self,
                                                int in,
                                                int out,
                                                mlt_time_format);
MLT_EXPORT char *mlt_animation_serialize_cut(mlt_animation self, int in, int out);
MLT_EXPORT char *mlt_animation_serialize_tf(mlt_animation self, mlt_time_format);
MLT_EXPORT char *mlt_animation_serialize(mlt_animation self);
MLT_EXPORT int mlt_animation_key_count(mlt_animation self);
MLT_EXPORT int mlt_animation_key_get(mlt_animation self, mlt_animation_item item, int index);
MLT_EXPORT void mlt_animation_close(mlt_animation self);
MLT_EXPORT int mlt_animation_key_set_type(mlt_animation self, int index, mlt_keyframe_type type);
MLT_EXPORT int mlt_animation_key_set_frame(mlt_animation self, int index, int frame);
MLT_EXPORT void mlt_animation_shift_frames(mlt_animation self, int shift);
MLT_EXPORT const char *mlt_animation_get_string(mlt_animation self);
MLT_EXPORT void mlt_animation_set_loop(mlt_animation self, int loop);
MLT_EXPORT int mlt_animation_get_loop(mlt_animation self);

#endif
