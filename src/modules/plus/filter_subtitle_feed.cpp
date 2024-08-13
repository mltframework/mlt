/*
 * filter_subtitle_feed.cpp
 * Copyright (C) 2024 Meltytech, LLC
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

#include <framework/mlt.h>

#include "subtitles/subtitles.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // for stat()
#include <sys/types.h> // for stat()

static void destroy_subtitles(void *p)
{
    delete static_cast<Subtitles::SubtitleVector *>(p);
}

static void property_changed(mlt_service owner, mlt_filter filter, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);
    if (name && !strcmp(name, "text")) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_int(properties, "_reset", 1);
    }
}

static void refresh_resource_subtitles(mlt_filter filter)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    const char *resource = mlt_properties_get(properties, "resource");

    struct stat file_info;
    if (mlt_stat(resource, &file_info)) {
        mlt_log_debug(filter, "File not found %s\n", resource);
        return;
    }

    int64_t mtime = (int64_t) file_info.st_mtime;
    if (mtime != mlt_properties_get_int64(properties, "_mtime")) {
        mlt_log_info(filter, "File has changed. Reopen: %s\n", resource);
        Subtitles::SubtitleVector *psubtitles = new Subtitles::SubtitleVector();
        if (!psubtitles) {
            mlt_log_error(filter, "Unable to allocate subtitles %s\n", resource);
            return;
        }
        *psubtitles = Subtitles::readFromSrtFile(resource);
        mlt_properties_set_data(properties,
                                "_subtitles",
                                psubtitles,
                                0,
                                (mlt_destructor) destroy_subtitles,
                                NULL);
        mlt_properties_set_int64(properties, "_mtime", mtime);
    }
}

static void refresh_text_subtitles(mlt_filter filter)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    if (mlt_properties_get_int(properties, "_reset")) {
        const char *text = mlt_properties_get(properties, "text");
        Subtitles::SubtitleVector *psubtitles = new Subtitles::SubtitleVector();
        if (!psubtitles) {
            mlt_log_error(filter, "Unable to allocate subtitles\n");
            return;
        }
        *psubtitles = Subtitles::readFromSrtString(text);
        mlt_properties_set_data(properties,
                                "_subtitles",
                                psubtitles,
                                0,
                                (mlt_destructor) destroy_subtitles,
                                NULL);
        mlt_properties_clear(properties, "_reset");
    }
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    if (mlt_properties_exists(properties, "resource")) {
        refresh_resource_subtitles(filter);
    } else if (mlt_properties_exists(properties, "text")) {
        refresh_text_subtitles(filter);
    }

    Subtitles::SubtitleVector *psubtitles = static_cast<Subtitles::SubtitleVector *>(
        mlt_properties_get_data(properties, "_subtitles", NULL));
    if (!psubtitles) {
        psubtitles = new Subtitles::SubtitleVector();
        if (!psubtitles) {
            mlt_log_error(filter, "Unable to allocate subtitles\n");
            return frame;
        }
        mlt_properties_set_data(properties,
                                "_subtitles",
                                psubtitles,
                                0,
                                (mlt_destructor) destroy_subtitles,
                                NULL);
    }
    const Subtitles::SubtitleVector &subtitles = *psubtitles;
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    int64_t frameMs = (int64_t) mlt_frame_get_position(frame) * 1000 * profile->frame_rate_den
                      / profile->frame_rate_num;
    int prevIndex = mlt_properties_get_int(properties, "_prevIndex");
    int marginMs = 999 * profile->frame_rate_den / profile->frame_rate_num;
    int index = Subtitles::indexForTime(subtitles, frameMs, prevIndex, marginMs);
    if (index > -1) {
        mlt_properties_set_int(properties, "_prevIndex", index);
    }
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties subtitle_properties = mlt_properties_get_properties(frame_properties,
                                                                       "subtitles");
    if (!subtitle_properties) {
        subtitle_properties = mlt_properties_new();
        mlt_properties_set_properties(frame_properties, "subtitles", subtitle_properties);
    }
    mlt_properties feed_properties = mlt_properties_new();
    mlt_properties_set(feed_properties, "lang", mlt_properties_get(properties, "lang"));
    if (index > -1) {
        mlt_position start = subtitles[index].start * profile->frame_rate_num
                             / profile->frame_rate_den / 1000;
        mlt_properties_set_position(feed_properties, "start", start);
        mlt_position end = subtitles[index].end * profile->frame_rate_num / profile->frame_rate_den
                           / 1000;
        mlt_properties_set_position(feed_properties, "end", end);
        mlt_properties_set(feed_properties, "text", subtitles[index].text.c_str());
    } else {
        mlt_properties_set_position(feed_properties, "start", -1);
        mlt_properties_set_position(feed_properties, "end", -1);
        mlt_properties_set(feed_properties, "text", "");
    }
    char *feed = mlt_properties_get(properties, "feed");
    mlt_properties_set_properties(subtitle_properties, feed, feed_properties);

    return frame;
}

extern "C" {

mlt_filter filter_subtitle_feed_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

        // Assign default values
        if (arg) {
            mlt_properties_set_string(properties, "resource", arg);
        }
        mlt_properties_set_string(properties, "feed", "0");
        mlt_properties_set_string(properties, "lang", "eng");
        mlt_properties_set_int(properties, "_reset", 1);

        filter->process = filter_process;
        mlt_events_listen(properties, filter, "property-changed", (mlt_listener) property_changed);
    } else {
        mlt_log_error(NULL, "[filter_subtitle_feed] Unable to allocate filter.\n");
    }
    return filter;
}
}
