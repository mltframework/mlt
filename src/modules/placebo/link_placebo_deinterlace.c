/*
 * link_placebo_deinterlace.c -- temporal libplacebo deinterlace chain link
 * Copyright (C) 2026 Meltytech, LLC
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

#include "deinterlace.h"

#include "gpu_context.h"

#include <framework/mlt_chain.h>
#include <framework/mlt_link.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>

static int link_get_image(mlt_frame frame,
                          uint8_t **image,
                          mlt_image_format *format,
                          int *width,
                          int *height,
                          int writable)
{
    mlt_link link = mlt_frame_pop_service(frame);
    mlt_properties unique = mlt_frame_get_unique_properties(frame, MLT_LINK_SERVICE(link));
    placebo_deinterlace_frames references = {
        .previous = unique ? mlt_properties_get_data(unique, "_previous", NULL) : NULL,
        .next = unique ? mlt_properties_get_data(unique, "_next", NULL) : NULL,
    };
    return placebo_deinterlace_get_image(MLT_LINK_SERVICE(link),
                                         frame,
                                         mlt_deinterlacer_yadif,
                                         &references,
                                         image,
                                         format,
                                         width,
                                         height,
                                         writable);
}

static void link_configure(mlt_link link, mlt_profile profile)
{
    (void) profile;
    mlt_service_set_profile(MLT_LINK_SERVICE(link),
                            mlt_service_profile(MLT_PRODUCER_SERVICE(link->next)));
}

static void get_reference(
    mlt_link link, mlt_properties unique, mlt_position position, int index, const char *key)
{
    mlt_frame reference = NULL;
    mlt_producer_seek(link->next, position);
    if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(link->next), &reference, index)) {
        mlt_log_warning(MLT_LINK_SERVICE(link),
                        "Failed to obtain frame " MLT_POSITION_FMT "\n",
                        position);
        mlt_frame_close(reference);
        return;
    }
    if (reference
        && mlt_properties_set_data(unique,
                                   key,
                                   reference,
                                   0,
                                   (mlt_destructor) mlt_frame_close,
                                   NULL)) {
        mlt_log_warning(MLT_LINK_SERVICE(link),
                        "Failed to retain frame " MLT_POSITION_FMT "\n",
                        position);
        mlt_frame_close(reference);
    }
}

static int link_get_frame(mlt_link link, mlt_frame_ptr frame, int index)
{
    mlt_position position = mlt_producer_position(MLT_LINK_PRODUCER(link));
    mlt_producer_seek(link->next, position);
    int error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(link->next), frame, index);
    if (error || !*frame)
        return error ? error : 1;

    mlt_position next_position = mlt_producer_position(link->next);
    mlt_chain chain = mlt_properties_get_data(MLT_LINK_PROPERTIES(link), "chain", NULL);
    int has_next = 1;
    if (chain) {
        mlt_producer chain_producer = MLT_CHAIN_PRODUCER(chain);
        mlt_position length = mlt_producer_get_length(chain_producer);
        double chain_fps = mlt_producer_get_fps(chain_producer);
        double link_fps = mlt_producer_get_fps(MLT_LINK_PRODUCER(link));
        if (length > 0 && chain_fps > 0 && link_fps > 0)
            has_next = (position + 1.0) * chain_fps < length * link_fps;
    }

    mlt_properties unique = mlt_frame_unique_properties(*frame, MLT_LINK_SERVICE(link));
    if (!unique) {
        mlt_log_error(MLT_LINK_SERVICE(link), "Failed to allocate reference properties\n");
        return 1;
    }
    mlt_properties properties = MLT_FRAME_PROPERTIES(*frame);
    mlt_producer original = mlt_frame_get_original_producer(*frame);
    mlt_properties source = original ? MLT_PRODUCER_PROPERTIES(original) : NULL;
    int progressive = mlt_properties_exists(properties, "progressive")
                          ? mlt_properties_get_int(properties, "progressive")
                          : source
                                && (mlt_properties_get_int(source, "meta.media.progressive")
                                    || mlt_properties_get_int(source, "progressive"));
    if (!progressive && position > 0)
        get_reference(link, unique, position - 1, index, "_previous");
    if (!progressive && has_next)
        get_reference(link, unique, position + 1, index, "_next");
    mlt_producer_seek(link->next, next_position);

    mlt_frame_push_service(*frame, link);
    mlt_frame_push_get_image(*frame, link_get_image);
    mlt_producer_prepare_next(MLT_LINK_PRODUCER(link));
    return 0;
}

mlt_link link_placebo_deinterlace_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg)
{
    (void) profile;
    (void) type;
    (void) id;
    (void) arg;
    if (!placebo_context_is_published())
        return NULL;
    mlt_link link = mlt_link_init();
    if (!link)
        return NULL;
    if (placebo_deinterlace_setup(MLT_LINK_SERVICE(link))) {
        mlt_link_close(link);
        return NULL;
    }
    link->configure = link_configure;
    link->get_frame = link_get_frame;
    return link;
}
