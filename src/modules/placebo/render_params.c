/*
 * render_params.c -- libplacebo render parameters shared by the filters
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

#include "render_params.h"

#include <libplacebo/config.h>
#include <libplacebo/filters.h>

#include <string.h>

struct pl_render_params placebo_render_params(mlt_frame frame)
{
    struct pl_render_params params = pl_render_default_params;
    const char *rescale = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "consumer.rescale");

    if (!rescale || !strcmp(rescale, "bilinear") || !strcmp(rescale, "fast_bilinear")) {
        params.upscaler = &pl_filter_bilinear;
        params.downscaler = &pl_filter_bilinear;
    } else if (!strcmp(rescale, "nearest") || !strcmp(rescale, "neighbor")) {
        params.upscaler = &pl_filter_nearest;
        params.downscaler = &pl_filter_nearest;
    }
    return params;
}

void placebo_render_params_skip_background(struct pl_render_params *params)
{
#if PL_API_VER >= 346
    params->background = PL_CLEAR_SKIP;
#else
    params->skip_target_clearing = true;
#endif
}

void placebo_render_params_preserve_alpha(struct pl_render_params *params)
{
    placebo_render_params_skip_background(params);
    params->background_transparency = 1.f;
}
