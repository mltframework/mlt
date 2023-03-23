/*
 * filter_cairoblend_mode.c
 * Copyright (C) 2019 Meltytech, LLC
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

#include "frei0r_helper.h"
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties_set(MLT_FRAME_PROPERTIES(frame),
                       CAIROBLEND_MODE_PROPERTY,
                       mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "mode"));
    return frame;
}

mlt_filter filter_cairoblend_mode_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "mode", arg ? arg : "normal");
    }
    return filter;
}
