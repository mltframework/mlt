/**
 * MLT C++ wrapper
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MLTPP_ANIMATION_H
#define MLTPP_ANIMATION_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt {

/** \brief C++ wrapper for ::mlt_animation — keyframe animation data.
 *
 * An animation holds a set of keyframes that define how a property value
 * changes over time. Obtain one from Properties via the anim_get / anim_set
 * family of methods.
 *
 * \see mlt_animation_s
 */
class MLTPP_DECLSPEC Animation
{
private:
    mlt_animation instance;

public:
    Animation();
    /** Wrap an existing ::mlt_animation handle (does not take ownership). */
    Animation(mlt_animation animation);
    Animation(const Animation &);
    ~Animation();

    /** Return true if the underlying handle is non-null. */
    bool is_valid() const;
    mlt_animation get_animation() const;
    Animation &operator=(const Animation &);

    /** Return the animation length in frames. */
    int length();
    /** Get the interpolated value and keyframe type at \p position. */
    int get_item(int position, bool &is_key, mlt_keyframe_type &);
    /** Return true if \p position is exactly a keyframe. */
    bool is_key(int position);
    /** Return the interpolation type of the keyframe at \p position. */
    mlt_keyframe_type keyframe_type(int position);
    /** Return the next keyframe position at or after \p position. */
    int next_key(int position);
    bool next_key(int position, int &key);
    /** Return the previous keyframe position at or before \p position. */
    int previous_key(int position);
    bool previous_key(int position, int &key);
    /** Return the total number of keyframes. */
    int key_count();
    /** Get the frame and type of the keyframe at \p index. */
    int key_get(int index, int &frame, mlt_keyframe_type &);
    /** Return the frame number of the keyframe at \p index. */
    int key_get_frame(int index);
    /** Return the interpolation type of the keyframe at \p index. */
    mlt_keyframe_type key_get_type(int index);
    /** Set the interpolation type of the keyframe at \p index. */
    int key_set_type(int index, mlt_keyframe_type type);
    /** Move the keyframe at \p index to \p frame. */
    int key_set_frame(int index, int frame);
    /** Shift all keyframe positions by \p shift frames. */
    void shift_frames(int shift);
    /** Set a new total length, scaling keyframe positions proportionally. */
    void set_length(int length);
    /** Remove the keyframe at \p position. */
    int remove(int position);
    /** Recalculate intermediate values between keyframes. */
    void interpolate();
    /** Serialize the animation as a string, optionally clipped to [\p in, \p out].
     *  Caller must \c free() the returned string. */
    char *serialize_cut(int in = -1, int out = -1);
    /** \overload */
    char *serialize_cut(mlt_time_format format, int in = -1, int out = -1);
    /** Set whether positions after the last keyframe repeat the cycle from the first keyframe.
     *  \see mlt_animation_set_loop */
    void set_loop(bool loop);
    /** Return true if the animation loops. */
    bool get_loop();
};
} // namespace Mlt

#endif
