/*
 * filter_movit_overlay_mode.cpp
 * Copyright (C) 2026 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt.h>
#include <stdlib.h>

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties_set(MLT_FRAME_PROPERTIES(frame),
                       "movit.overlay.compositing",
                       mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "mode"));
    return frame;
}

extern "C" mlt_filter filter_movit_overlay_mode_init(mlt_profile profile,
                                                     mlt_service_type type,
                                                     const char *id,
                                                     char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "mode", arg ? arg : "0");
    }
    return filter;
}
