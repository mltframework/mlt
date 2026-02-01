/*
 * filter_placebo_shader.c -- custom .hook shader loader via libplacebo
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

#include "gpu_context.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <libplacebo/gpu.h>
#include <libplacebo/renderer.h>
#include <libplacebo/shaders/custom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
/* Use _stat on Windows to avoid deprecation warnings (S5) */
#include <sys/stat.h>
#define stat_func _stat
#define stat_struct _stat
#else
#include <sys/stat.h>
#define stat_func stat
#define stat_struct stat
#endif

/* Private data stored on the filter */
typedef struct
{
    const struct pl_hook *hooks[1]; /* parsed shader hook (single) */
    int num_hooks;
    char *loaded_path;   /* path that was loaded */
    time_t loaded_mtime; /* mtime when last loaded */
    char *loaded_text;   /* inline text that was loaded */
} shader_private;

/* C3 fix: single cleanup function used both by filter_close and mlt_properties destructor */
static void shader_private_destroy(void *ptr)
{
    shader_private *priv = (shader_private *) ptr;
    if (!priv)
        return;
    if (priv->hooks[0])
        pl_mpv_user_shader_destroy(&priv->hooks[0]);
    priv->num_hooks = 0;
    free(priv->loaded_path);
    priv->loaded_path = NULL;
    free(priv->loaded_text);
    priv->loaded_text = NULL;
    free(priv);
}

/* Read entire file into malloc'd string; caller must free. */
static char *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len <= 0 || len > 10 * 1024 * 1024) { /* W7 fix: reject files > 10 MB */
        fclose(f);
        return NULL;
    }

    char *buf = malloc(len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if ((long) fread(buf, 1, len, f) != len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[len] = '\0';
    fclose(f);

    if (out_len)
        *out_len = (size_t) len;
    return buf;
}

static time_t file_mtime(const char *path)
{
    struct stat_struct st;
    if (stat_func(path, &st) == 0)
        return st.st_mtime;
    return 0;
}

/* Load/reload shader if needed. Returns 1 if hooks are available. */
static int ensure_shader(mlt_filter filter, shader_private *priv, pl_gpu gpu)
{
    mlt_properties props = MLT_FILTER_PROPERTIES(filter);
    const char *shader_path = mlt_properties_get(props, "shader_path");
    const char *shader_text = mlt_properties_get(props, "shader_text");

    /* Check if we need to reload from file */
    if (shader_path && *shader_path) {
        time_t mtime = file_mtime(shader_path);
        int need_reload = 0;

        if (!priv->loaded_path || strcmp(priv->loaded_path, shader_path) != 0)
            need_reload = 1;
        else if (mtime != priv->loaded_mtime)
            need_reload = 1;

        if (need_reload) {
            /* Free old shader */
            if (priv->hooks[0])
                pl_mpv_user_shader_destroy(&priv->hooks[0]);
            priv->num_hooks = 0;
            free(priv->loaded_path);
            priv->loaded_path = NULL;

            /* Read and parse */
            size_t len = 0;
            char *text = read_file(shader_path, &len);
            if (!text) {
                mlt_log_error(MLT_FILTER_SERVICE(filter),
                              "Failed to read shader file: %s\n",
                              shader_path);
                return 0;
            }

            priv->hooks[0] = pl_mpv_user_shader_parse(gpu, text, len);
            free(text);

            if (!priv->hooks[0]) {
                mlt_log_error(MLT_FILTER_SERVICE(filter),
                              "Failed to parse shader: %s\n",
                              shader_path);
                return 0;
            }

            priv->num_hooks = 1;
            priv->loaded_path = strdup(shader_path);
            priv->loaded_mtime = mtime;
            mlt_log_info(MLT_FILTER_SERVICE(filter), "Loaded shader: %s\n", shader_path);
        }
        return priv->num_hooks > 0;
    }

    /* Check inline shader_text */
    if (shader_text && *shader_text) {
        if (!priv->loaded_text || strcmp(priv->loaded_text, shader_text) != 0) {
            /* Free old shader */
            if (priv->hooks[0])
                pl_mpv_user_shader_destroy(&priv->hooks[0]);
            priv->num_hooks = 0;
            free(priv->loaded_text);
            priv->loaded_text = NULL;

            priv->hooks[0] = pl_mpv_user_shader_parse(gpu, shader_text, strlen(shader_text));
            if (!priv->hooks[0]) {
                mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to parse inline shader_text\n");
                return 0;
            }

            priv->num_hooks = 1;
            priv->loaded_text = strdup(shader_text);
            mlt_log_info(MLT_FILTER_SERVICE(filter),
                         "Loaded inline shader (%zu bytes)\n",
                         strlen(shader_text));
        }
        return priv->num_hooks > 0;
    }

    return 0; /* no shader configured */
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_properties filter_props = MLT_FILTER_PROPERTIES(filter);

    pl_gpu gpu = placebo_gpu_get();
    pl_renderer renderer = placebo_renderer_get();
    if (!gpu || !renderer) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "GPU not available, passing frame through\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    /* W2 fix: verify RGBA8 format is supported */
    pl_fmt rgba_fmt = pl_find_named_fmt(gpu, "rgba8");
    if (!rgba_fmt) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "GPU does not support rgba8 format\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    /* Get or create private data (C3 fix: single destructor path) */
    shader_private *priv = mlt_properties_get_data(filter_props, "_shader_priv", NULL);
    if (!priv) {
        priv = calloc(1, sizeof(shader_private));
        if (!priv)
            return mlt_frame_get_image(frame, image, format, width, height, writable);
        mlt_properties_set_data(filter_props, "_shader_priv", priv, 0, shader_private_destroy, NULL);
    }

    /* Ensure shader is loaded */
    if (!ensure_shader(filter, priv, gpu)) {
        /* No shader available -- pass through */
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    /* Request RGBA from upstream */
    *format = mlt_image_rgba;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);
    if (error || !*image)
        return error;

    int w = *width;
    int h = *height;
    size_t stride = w * 4;

    /* W4 fix: hold render lock for all GPU operations */
    placebo_render_lock();

    /* Create source texture and upload */
    pl_tex src_tex = pl_tex_create(gpu,
                                   pl_tex_params(.w = w,
                                                 .h = h,
                                                 .format = rgba_fmt,
                                                 .sampleable = true,
                                                 .host_writable = true, ));
    if (!src_tex) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to create source texture\n");
        placebo_render_unlock();
        return 0;
    }

    /* C4 fix: check upload return */
    if (!pl_tex_upload(gpu,
                       pl_tex_transfer_params(.tex = src_tex,
                                              .row_pitch = stride,
                                              .ptr = *image, ))) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "GPU texture upload failed\n");
        pl_tex_destroy(gpu, &src_tex);
        placebo_render_unlock();
        return 0;
    }

    /* Create destination texture */
    pl_tex dst_tex = pl_tex_create(gpu,
                                   pl_tex_params(.w = w,
                                                 .h = h,
                                                 .format = rgba_fmt,
                                                 .renderable = true,
                                                 .host_readable = true, ));
    if (!dst_tex) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to create dest texture\n");
        pl_tex_destroy(gpu, &src_tex);
        placebo_render_unlock();
        return 0;
    }

    /* Build pl_frames */
    struct pl_frame pl_src = {
        .num_planes = 1,
        .planes = {{
            .texture = src_tex,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = pl_color_space_srgb,
    };

    struct pl_frame pl_dst = {
        .num_planes = 1,
        .planes = {{
            .texture = dst_tex,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = pl_color_space_srgb,
    };

    /* Render with shader hooks */
    struct pl_render_params params = pl_render_default_params;
    params.hooks = priv->hooks;
    params.num_hooks = priv->num_hooks;

    if (!pl_render_image(renderer, &pl_src, &pl_dst, &params)) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "pl_render_image with shader hook failed\n");
    }

    /* C4 fix: check download return */
    if (!pl_tex_download(gpu,
                         pl_tex_transfer_params(.tex = dst_tex,
                                                .row_pitch = stride,
                                                .ptr = *image, ))) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "GPU texture download failed\n");
    }

    /* Cleanup */
    pl_tex_destroy(gpu, &src_tex);
    pl_tex_destroy(gpu, &dst_tex);

    placebo_render_unlock();

    return 0;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_placebo_shader_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        /* C3 fix: no filter->close needed -- cleanup via mlt_properties destructor */
        mlt_properties props = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set(props, "shader_path", arg ? arg : "");
        mlt_properties_set(props, "shader_text", "");
    }
    return filter;
}
