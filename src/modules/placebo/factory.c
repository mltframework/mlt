/*
 * factory.c -- module registration for libplacebo filters
 * Copyright (C) 2025-2026 D-Ogi
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

#include "mltplacebo_export.h"
#include <framework/mlt.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

extern mlt_filter filter_placebo_render_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg);
extern mlt_filter filter_placebo_shader_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg);
extern mlt_filter filter_placebo_scale_init(mlt_profile profile,
                                            mlt_service_type type,
                                            const char *id,
                                            char *arg);
extern mlt_filter filter_placebo_deinterlace_init(mlt_profile profile,
                                                  mlt_service_type type,
                                                  const char *id,
                                                  char *arg);
extern mlt_link link_placebo_deinterlace_init(mlt_profile profile,
                                              mlt_service_type type,
                                              const char *id,
                                              char *arg);
extern mlt_filter filter_placebo_color_transform_init(mlt_profile profile,
                                                      mlt_service_type type,
                                                      const char *id,
                                                      char *arg);
extern mlt_filter filter_placebo_blend_mode_init(mlt_profile profile,
                                                 mlt_service_type type,
                                                 const char *id,
                                                 char *arg);
extern mlt_transition transition_placebo_blend_init(mlt_profile profile,
                                                    mlt_service_type type,
                                                    const char *id,
                                                    char *arg);
extern mlt_transition transition_placebo_luma_mix_init(mlt_profile profile,
                                                       mlt_service_type type,
                                                       const char *id,
                                                       char *arg);
#ifdef PLACEBO_SDL
extern mlt_consumer consumer_placebo_sdl_init(mlt_profile profile,
                                              mlt_service_type type,
                                              const char *id,
                                              char *arg);
#endif

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/placebo/%s", mlt_environment("MLT_DATA"), (char *) data);
    return mlt_properties_parse_yaml(file);
}

static mlt_properties shader_metadata(mlt_service_type type, const char *id, void *data)
{
    mlt_properties result = mlt_properties_parse_yaml(data);
    if (result)
        mlt_properties_set(result, "identifier", id);
    else
        mlt_log_error(NULL, "[placebo] Failed to parse shader metadata: %s\n", (char *) data);
    return result;
}

static mlt_filter shader_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_properties filters = mlt_repository_filters(mlt_factory_repository());
    mlt_properties entry = mlt_properties_get_data(filters, id, NULL);
    char *resource = mlt_properties_get(entry, "_shader_resource");
    if (!resource) {
        mlt_log_error(NULL, "[placebo] No shader resource registered for %s\n", id);
        return NULL;
    }
    return filter_placebo_shader_init(profile, type, id, arg && *arg ? arg : resource);
}

static void register_shaders(mlt_repository repository, const char *directory, int depth)
{
    if (depth > 32) {
        mlt_log_error(NULL, "[placebo] Shader directory nesting too deep: %s\n", directory);
        return;
    }
    mlt_properties files = mlt_properties_new();
    if (!files) {
        mlt_log_error(NULL, "[placebo] Failed to allocate shader directory listing\n");
        return;
    }
    mlt_properties_dir_list(files, directory, "*", 1);
    for (int i = 0; i < mlt_properties_count(files); i++) {
        const char *path = mlt_properties_get_value(files, i);
        struct stat st;
        if (mlt_stat(path, &st)) {
            mlt_log_error(NULL, "[placebo] Failed to stat shader path: %s\n", path);
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            register_shaders(repository, path, depth + 1);
            continue;
        }
        const char *basename = strrchr(path, '/');
        basename = basename ? basename + 1 : path;
        size_t length = strlen(basename);
        if (!S_ISREG(st.st_mode) || length <= 4 || strcmp(basename + length - 4, ".yml"))
            continue;

        char id[PATH_MAX];
        char resource[PATH_MAX];
        int n = snprintf(id, sizeof(id), "placebo.%.*s", (int) length - 4, basename);
        int r = snprintf(resource, sizeof(resource), "%.*s.hook", (int) strlen(path) - 4, path);
        if (n < 0 || n >= sizeof(id) || r < 0 || r >= sizeof(resource)) {
            mlt_log_error(NULL, "[placebo] Shader path too long: %s\n", path);
            continue;
        }
        if (mlt_stat(resource, &st) || !S_ISREG(st.st_mode)) {
            mlt_log_error(NULL, "[placebo] Missing shader file for metadata: %s\n", path);
            continue;
        }
        mlt_properties filters = mlt_repository_filters(repository);
        if (mlt_properties_exists(filters, id)) {
            mlt_log_error(NULL, "[placebo] Duplicate shader registration: %s (%s)\n", id, path);
            continue;
        }
        MLT_REGISTER(mlt_service_filter_type, id, shader_init);
        mlt_properties entry = mlt_properties_get_data(filters, id, NULL);
        mlt_properties_set(entry, "_shader_resource", resource);
        mlt_properties_set(entry, "_shader_yaml", path);
        MLT_REGISTER_METADATA(mlt_service_filter_type,
                              id,
                              shader_metadata,
                              mlt_properties_get(entry, "_shader_yaml"));
    }
    mlt_properties_close(files);
}

MLTPLACEBO_EXPORT MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_filter_type, "placebo.render", filter_placebo_render_init);
    MLT_REGISTER(mlt_service_filter_type, "placebo.shader", filter_placebo_shader_init);
    MLT_REGISTER(mlt_service_filter_type, "placebo.scale", filter_placebo_scale_init);
    MLT_REGISTER(mlt_service_filter_type, "placebo.deinterlace", filter_placebo_deinterlace_init);
    MLT_REGISTER(mlt_service_link_type, "placebo.deinterlace", link_placebo_deinterlace_init);
    MLT_REGISTER(mlt_service_filter_type,
                 "placebo.color_transform",
                 filter_placebo_color_transform_init);
    MLT_REGISTER(mlt_service_link_type, "placebo.scale", mlt_link_filter_init);
    MLT_REGISTER(mlt_service_filter_type, "placebo.blend_mode", filter_placebo_blend_mode_init);
    MLT_REGISTER(mlt_service_transition_type, "placebo.blend", transition_placebo_blend_init);
    MLT_REGISTER(mlt_service_transition_type, "placebo.luma_mix", transition_placebo_luma_mix_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.render",
                          metadata,
                          "filter_placebo_render.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.shader",
                          metadata,
                          "filter_placebo_shader.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.scale",
                          metadata,
                          "filter_placebo_scale.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.deinterlace",
                          metadata,
                          "filter_placebo_deinterlace.yml");
    MLT_REGISTER_METADATA(mlt_service_link_type,
                          "placebo.deinterlace",
                          metadata,
                          "link_placebo_deinterlace.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.color_transform",
                          metadata,
                          "filter_placebo_color_transform.yml");
    MLT_REGISTER_METADATA(mlt_service_link_type, "placebo.scale", mlt_link_filter_metadata, NULL);
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "placebo.blend_mode",
                          metadata,
                          "filter_placebo_blend_mode.yml");
    MLT_REGISTER_METADATA(mlt_service_transition_type,
                          "placebo.blend",
                          metadata,
                          "transition_placebo_blend.yml");
    MLT_REGISTER_METADATA(mlt_service_transition_type,
                          "placebo.luma_mix",
                          metadata,
                          "transition_placebo_luma_mix.yml");
#ifdef PLACEBO_SDL
    MLT_REGISTER(mlt_service_consumer_type, "placebo.sdl", consumer_placebo_sdl_init);
    MLT_REGISTER_METADATA(mlt_service_consumer_type,
                          "placebo.sdl",
                          metadata,
                          "consumer_placebo_sdl.yml");
#endif
    char directory[PATH_MAX];
    int n
        = snprintf(directory, sizeof(directory), "%s/placebo/shaders", mlt_environment("MLT_DATA"));
    if (n < 0 || n >= sizeof(directory)) {
        mlt_log_error(NULL, "[placebo] Shader data directory path too long\n");
        return;
    }
    register_shaders(repository, directory, 0);
}
