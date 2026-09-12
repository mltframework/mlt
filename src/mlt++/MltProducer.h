/**
 * MltProducer.h - MLT Wrapper
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

#ifndef MLTPP_PRODUCER_H
#define MLTPP_PRODUCER_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltService.h"

namespace Mlt {
class Service;
class Filter;
class Profile;
class Frame;

/** \brief C++ wrapper for ::mlt_producer — origin of audio/video data.
 *
 * A producer generates frames of audio and video data. It may read from a
 * file, network stream, or synthesize content. Producers support in/out
 * points, playback speed, and can be "cut" (sub-range) instances that share
 * the underlying resource.
 *
 * \extends Service
 * \see mlt_producer_s
 */
class MLTPP_DECLSPEC Producer : public Service
{
private:
    mlt_producer instance;
    Producer *parent_;

public:
    Producer();
    /** Construct and instantiate producer \p id from the repository. */
    Producer(Profile &profile, const char *id, const char *service = NULL);
    Producer(mlt_profile profile, const char *id, const char *service = NULL);
    Producer(Service &producer);
    /** Wrap an existing ::mlt_producer handle. */
    Producer(mlt_producer producer);
    Producer(Producer &producer);
    Producer(const Producer &producer);
    Producer(Producer *producer);
    virtual ~Producer();
    Producer &operator=(const Producer &producer);
    virtual mlt_producer get_producer();
    /** Return the parent producer (for cut producers, the shared root). */
    Producer &parent();
    mlt_producer get_parent();
    mlt_service get_service() override;
    /** Seek to \p position (relative to in-point). */
    int seek(int position);
    /** Seek to a timecode string (e.g. "00:01:23.04"). */
    int seek(const char *time);
    /** Return the current position relative to in-point. */
    int position();
    /** Return the current absolute frame number. */
    int frame();
    /** Return the current timecode string. */
    char *frame_time(mlt_time_format = mlt_time_smpte_df);
    /** Set the playback speed (1.0 = normal, 0.0 = paused, -1.0 = reverse). */
    int set_speed(double speed);
    /** Pause playback (sets speed to 0). */
    int pause();
    /** Return the current playback speed. */
    double get_speed();
    /** Return the frames-per-second rate for this producer's profile. */
    double get_fps();
    /** Set the active in/out range in frame numbers. */
    int set_in_and_out(int in, int out);
    /** Return the in-point frame number. */
    int get_in();
    /** Return the out-point frame number. */
    int get_out();
    /** Return the total length of the underlying resource in frames. */
    int get_length();
    char *get_length_time(mlt_time_format = mlt_time_smpte_df);
    /** Return the active play duration (out - in + 1). */
    int get_playtime();
    /** Create a cut (sub-range) producer sharing this producer's resource. Caller owns result. */
    Producer *cut(int in = 0, int out = -1);
    /** Return true if this producer is a cut. */
    bool is_cut();
    /** Return true if this is a blank/gap clip. */
    bool is_blank();
    bool same_clip(Producer &that);
    bool runs_into(Producer &that);
    void optimise();
    int clear();
    int64_t get_creation_time();
    void set_creation_time(int64_t creation_time);
    bool probe();
    /** Copy this producer's serializable \c meta. properties onto \p frame and
     * copy serializable \c set. properties with the prefix removed. */
    void pass_frame_properties(Frame &frame);
};
} // namespace Mlt

#endif
