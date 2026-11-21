/*
 * filter_placebo_deinterlace.c -- GPU deinterlacing via libplacebo
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

#include <framework/mlt_filter.h>

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);
    return placebo_deinterlace_get_image(MLT_FILTER_SERVICE(filter),
                                         frame,
                                         mlt_deinterlacer_bob,
                                         NULL,
                                         image,
                                         format,
                                         width,
                                         height,
                                         writable);
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_placebo_deinterlace_init(mlt_profile profile,
                                           mlt_service_type type,
                                           const char *id,
                                           char *arg)
{
    (void) profile;
    (void) type;
    (void) id;
    (void) arg;
    mlt_filter filter = mlt_filter_new();
    if (!filter)
        return NULL;
    if (!placebo_context_is_published() || placebo_deinterlace_setup(MLT_FILTER_SERVICE(filter))) {
        mlt_filter_close(filter);
        return NULL;
    }
    filter->process = filter_process;
    return filter;
}
