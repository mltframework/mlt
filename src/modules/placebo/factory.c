/*
 * factory.c -- module registration for libplacebo filters
 * Copyright (C) 2025 D-Ogi
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

#include "mltplacebo_export.h"
#include <framework/mlt.h>
#include <limits.h>
#include <string.h>

extern mlt_filter filter_placebo_render_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg);
extern mlt_filter filter_placebo_shader_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg);

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/placebo/%s", mlt_environment("MLT_DATA"), (char *) data);
    return mlt_properties_parse_yaml(file);
}

MLTPLACEBO_EXPORT MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_filter_type, "placebo.render", filter_placebo_render_init);
    MLT_REGISTER(mlt_service_filter_type, "placebo.shader", filter_placebo_shader_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.render",
                          metadata,
                          "filter_placebo_render.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.shader",
                          metadata,
                          "filter_placebo_shader.yml");
}
