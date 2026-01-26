/*
 * filter_subtitle.cpp -- subtitle overlay filter
 * Copyright (C) 2024-2026 Meltytech, LLC
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
    const char *text = NULL;
    if (mlt_properties_exists(properties, "resource") || mlt_properties_exists(properties, "text")) {
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
            text = subtitles[index].text.c_str();
            mlt_properties_set_int(properties, "_prevIndex", index);
        } else {
            return frame;
        }
    } else if (mlt_properties_exists(properties, "feed")) {
        mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
        mlt_properties subtitle_properties = mlt_properties_get_properties(frame_properties,
                                                                           "subtitles");
        if (!subtitle_properties) {
            mlt_log_info(filter, "No feed subtitles found\n");
            return frame;
        }
        char *feed = mlt_properties_get(properties, "feed");
        mlt_properties feed_properties = mlt_properties_get_properties(subtitle_properties, feed);
        if (!feed_properties) {
            mlt_log_info(filter, "Feed %s not found\n", feed);
            return frame;
        }
        text = mlt_properties_get(feed_properties, "text");
    }

    if (!text || !strcmp(text, "")) {
        // Do not draw empty text
        return frame;
    }

    mlt_filter text_filter = (mlt_filter) mlt_properties_get_data(properties, "_t", NULL);
    if (!text_filter) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Text filter not found\n");
        return frame;
    }

    // We have some text and a text filter. Let's apply them to the frame.
    mlt_properties text_filter_properties
        = mlt_frame_unique_properties(frame, MLT_FILTER_SERVICE(text_filter));
    mlt_properties_set_string(text_filter_properties, "argument", text);
    mlt_properties_pass_list(text_filter_properties,
                             properties,
                             "geometry family size weight style fgcolour bgcolour olcolour pad "
                             "halign valign outline underline strikethrough opacity");
    mlt_filter_set_in_and_out(text_filter, mlt_filter_get_in(filter), mlt_filter_get_out(filter));
    return mlt_filter_process(text_filter, frame);
}

extern "C" {

mlt_filter filter_subtitle_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    mlt_filter text_filter = mlt_factory_filter(profile, "qtext", NULL);
    if (!text_filter)
        text_filter = mlt_factory_filter(profile, "text", NULL);
    if (!text_filter) {
        mlt_log_error(NULL, "[filter_subtitle] Unable to create text filter.\n");
        return NULL;
    }

    mlt_filter filter = mlt_filter_new();

    if (filter) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

        // Assign default values
        if (arg && strcmp(arg, "")) {
            mlt_properties_set_string(properties, "resource", arg);
        }
        mlt_properties_set_string(properties, "geometry", "20%/80%:60%x20%:100");
        mlt_properties_set_string(properties, "family", "Sans");
        mlt_properties_set_string(properties, "size", "48");
        mlt_properties_set_string(properties, "weight", "400");
        mlt_properties_set_string(properties, "style", "normal");
        mlt_properties_set_string(properties, "fgcolour", "0x000000ff");
        mlt_properties_set_string(properties, "bgcolour", "0x00000020");
        mlt_properties_set_string(properties, "olcolour", "0x00000000");
        mlt_properties_set_string(properties, "pad", "0");
        mlt_properties_set_string(properties, "halign", "left");
        mlt_properties_set_string(properties, "valign", "top");
        mlt_properties_set_string(properties, "outline", "0");
        mlt_properties_set_string(properties, "underline", "0");
        mlt_properties_set_string(properties, "strikethrough", "0");
        mlt_properties_set_string(properties, "opacity", "1.0");
        mlt_properties_set_int(properties, "_filter_private", 1);

        // Register the text filter for reuse/destruction
        mlt_properties_set_data(properties,
                                "_t",
                                text_filter,
                                0,
                                (mlt_destructor) mlt_filter_close,
                                NULL);

        filter->process = filter_process;
        mlt_events_listen(properties, filter, "property-changed", (mlt_listener) property_changed);
    } else {
        mlt_log_error(NULL, "[filter_subtitle] Unable to allocate filter.\n");
        mlt_filter_close(text_filter);
    }
    return filter;
}
}
