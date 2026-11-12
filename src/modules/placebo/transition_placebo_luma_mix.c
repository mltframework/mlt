/*
 * transition_placebo_luma_mix.c -- GPU dissolve and wipe
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

#include "color_space.h"
#include "convert.h"
#include "gpu_context.h"
#include "tex_pool.h"
#include "transition_common.h"
#include "vulkan_frame.h"

#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_transition.h>

#include <libplacebo/dispatch.h>
#include <libplacebo/gpu.h>
#include <libplacebo/shaders/custom.h>

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct luma_cache
{
    pthread_mutex_t mutex;
    char *resource;
    mlt_producer producer;
};

struct luma_frame
{
    mlt_transition transition;
    mlt_frame wipe;
    float progress;
    float width;
    int invert;
    int error;
};

static const char *k_mix_header = "vec4 mix_sources(vec4 a, vec4 b, float amount) {          \n"
                                  "    float alpha = clamp(mix(a.a, b.a, amount), 0.0, 1.0);\n"
                                  "    vec3 rgb = mix(a.rgb * a.a, b.rgb * b.a, amount);    \n"
                                  "    return vec4(alpha > 0.0 ? rgb / alpha : vec3(0.0), alpha);\n"
                                  "}                                                       \n";

static const char *k_dissolve_body
    = "color = mix_sources(texture(a_tex, uv), texture(b_tex, uv), progress);\n";

static const char *k_wipe_body
    = "vec4 sample_mask = texture(mask_tex, uv);                       \n"
      "float red = sample_mask.r * sample_mask.a;                       \n"
      "float mask = invert != 0 ? red : 1.0 - red;                      \n"
      "float amount = clamp((mask - 1.0) * width + progress * (width + 1.0), 0.0, 1.0);\n"
      "color = mix_sources(texture(a_tex, uv), texture(b_tex, uv), amount);\n";

static void close_cache(void *data)
{
    struct luma_cache *cache = data;
    if (!cache)
        return;
    if (cache->producer)
        mlt_producer_close(cache->producer);
    free(cache->resource);
    pthread_mutex_destroy(&cache->mutex);
    free(cache);
}

static void close_frame_state(void *data)
{
    struct luma_frame *state = data;
    if (!state)
        return;
    if (state->wipe)
        mlt_frame_close(state->wipe);
    if (state->transition)
        mlt_transition_close(state->transition);
    free(state);
}

static int get_wipe(mlt_transition transition, mlt_frame a_frame, mlt_frame *wipe)
{
    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);
    mlt_service service = MLT_TRANSITION_SERVICE(transition);
    struct luma_cache *cache = mlt_properties_get_data(properties, "_luma_cache", NULL);
    const char *resource;
    int error = 0;

    *wipe = NULL;
    if (!cache) {
        mlt_log_error(service, "Missing wipe producer cache\n");
        return 1;
    }

    pthread_mutex_lock(&cache->mutex);
    resource = mlt_properties_get(properties, "resource");
    if (!resource || !*resource) {
        if (cache->producer)
            mlt_producer_close(cache->producer);
        cache->producer = NULL;
        free(cache->resource);
        cache->resource = NULL;
    } else {
        if (!cache->producer || !cache->resource || strcmp(resource, cache->resource)) {
            char *name = strdup(resource);
            if (!name) {
                mlt_log_error(service, "Failed to copy wipe resource\n");
                error = 1;
                goto done;
            }
            mlt_producer producer = mlt_factory_producer(mlt_service_profile(service), NULL, name);
            if (cache->producer)
                mlt_producer_close(cache->producer);
            free(cache->resource);
            cache->resource = name;
            cache->producer = producer;
            if (producer) {
                mlt_properties_set(MLT_PRODUCER_PROPERTIES(producer), "eof", "loop");
            } else {
                mlt_log_warning(service, "Cannot load wipe resource '%s'; using dissolve\n", name);
            }
        }
        if (cache->producer) {
            mlt_properties producer_props = MLT_PRODUCER_PROPERTIES(cache->producer);
            mlt_position position = mlt_transition_get_position(transition, a_frame);
            mlt_properties_pass(producer_props, properties, "producer.");
            if (mlt_producer_seek(cache->producer, position)
                || mlt_service_get_frame(MLT_PRODUCER_SERVICE(cache->producer), wipe, 0)
                || !*wipe) {
                mlt_log_warning(service, "Cannot obtain wipe frame; using dissolve\n");
                if (*wipe)
                    mlt_frame_close(*wipe);
                *wipe = NULL;
            } else {
                mlt_properties_set_int(MLT_FRAME_PROPERTIES(*wipe), "distort", 1);
            }
        }
    }

done:
    pthread_mutex_unlock(&cache->mutex);
    return error;
}

static int mix_textures(pl_gpu gpu,
                        pl_dispatch dispatch,
                        pl_tex a,
                        pl_tex b,
                        pl_tex mask,
                        pl_tex target,
                        float progress,
                        float width,
                        int invert)
{
    pl_fmt uv_fmt = pl_find_vertex_fmt(gpu, PL_FMT_FLOAT, 2);
    struct pl_shader_desc descs[3] = {0};
    struct pl_shader_var vars[3] = {0};
    struct pl_shader_va uv = {0};
    struct pl_custom_shader custom = {0};
    pl_shader shader;

    if (!uv_fmt || !dispatch)
        return 1;

    const char *names[] = {"a_tex", "b_tex", "mask_tex"};
    pl_tex textures[] = {a, b, mask};
    int count = mask ? 3 : 2;
    for (int i = 0; i < count; i++) {
        descs[i].desc.name = names[i];
        descs[i].desc.type = PL_DESC_SAMPLED_TEX;
        descs[i].binding.object = textures[i];
        descs[i].binding.address_mode = PL_TEX_ADDRESS_CLAMP;
        descs[i].binding.sample_mode = PL_TEX_SAMPLE_NEAREST;
    }

    uv.attr.name = "uv";
    uv.attr.fmt = uv_fmt;
    for (int i = 0; i < 4; i++)
        uv.data[i] = placebo_transition_uv[i];

    vars[0].var = pl_var_float("progress");
    vars[0].data = &progress;
    vars[0].dynamic = true;
    vars[1].var = pl_var_float("width");
    vars[1].data = &width;
    vars[1].dynamic = true;
    vars[2].var = pl_var_int("invert");
    vars[2].data = &invert;
    vars[2].dynamic = true;

    custom.header = k_mix_header;
    custom.body = mask ? k_wipe_body : k_dissolve_body;
    custom.description = "placebo.luma_mix";
    custom.input = PL_SHADER_SIG_NONE;
    custom.output = PL_SHADER_SIG_COLOR;
    custom.descriptors = descs;
    custom.num_descriptors = count;
    custom.variables = vars;
    custom.num_variables = mask ? 3 : 1;
    custom.vertex_attribs = &uv;
    custom.num_vertex_attribs = 1;

    shader = pl_dispatch_begin(dispatch);
    if (!shader)
        return 1;
    if (!pl_shader_custom(shader, &custom)) {
        pl_dispatch_abort(dispatch, &shader);
        return 1;
    }
    return pl_dispatch_finish(dispatch, pl_dispatch_params(.shader = &shader, .target = target))
               ? 0
               : 1;
}

static int get_image(mlt_frame a_frame,
                     uint8_t **image,
                     mlt_image_format *format,
                     int *width,
                     int *height,
                     int writable)
{
    struct luma_frame *state = mlt_frame_pop_service(a_frame);
    mlt_frame b_frame = mlt_frame_pop_frame(a_frame);
    mlt_service service = MLT_TRANSITION_SERVICE(state->transition);
    pl_gpu gpu = placebo_gpu_get();
    pl_dispatch dispatch = placebo_dispatch_get();
    mlt_image_format requested = *format;
    mlt_image_format a_format = requested;
    mlt_image_format b_format = requested;
    mlt_image_format mask_format = requested;
    uint8_t *a_image = NULL;
    uint8_t *b_image = NULL;
    uint8_t *mask_image = NULL;
    int out_w = *width;
    int out_h = *height;
    int b_w;
    int b_h;
    int mask_w;
    int mask_h;
    int a_owned = 0;
    int b_owned = 0;
    int mask_owned = 0;
    pl_tex a_tex = NULL;
    pl_tex b_tex = NULL;
    pl_tex mask_tex = NULL;
    pl_tex mixed = NULL;
    pl_tex result = NULL;
    struct pl_color_space output_color = pl_color_space_srgb;
    struct pl_color_space working_color;
    pl_fmt output_fmt = NULL;
    pl_fmt working_fmt;
    mlt_color_trc trc;

    if (state->error)
        return 1;
    if (!gpu || !dispatch) {
        mlt_log_error(service, "GPU not available for luma transition\n");
        return 1;
    }
    if (placebo_frame_get_gpu_image(a_frame, &a_image, &a_format, &out_w, &out_h, writable)) {
        mlt_log_error(service, "Failed to obtain lower image\n");
        return 1;
    }
    if (out_w < 1 || out_h < 1) {
        mlt_log_error(service, "Invalid image size: %dx%d\n", out_w, out_h);
        return 1;
    }
    b_w = out_w;
    b_h = out_h;
    if (placebo_frame_get_gpu_image(b_frame, &b_image, &b_format, &b_w, &b_h, writable)) {
        mlt_log_error(service, "Failed to obtain upper image\n");
        return 1;
    }
    if (state->wipe) {
        mask_w = out_w;
        mask_h = out_h;
        if (placebo_frame_get_gpu_image(state->wipe,
                                        &mask_image,
                                        &mask_format,
                                        &mask_w,
                                        &mask_h,
                                        writable)) {
            mlt_log_error(service, "Failed to obtain wipe image\n");
            return 1;
        }
    }

    placebo_output_format(gpu, a_frame, requested, &output_color, &output_fmt);
    working_fmt = placebo_linear_format(gpu);
    if (!output_fmt || !working_fmt
        || (requested == mlt_image_rgba64 && !placebo_rgba16_format(gpu))) {
        mlt_log_error(service, "GPU does not support the required RGBA formats\n");
        return 1;
    }
    working_color = output_color;
    working_color.transfer = PL_COLOR_TRC_LINEAR;
    trc = placebo_frame_color_trc(a_frame, a_image, a_format);

    placebo_render_lock();
    if (placebo_transition_normalize(gpu,
                                     a_frame,
                                     a_image,
                                     a_format,
                                     requested,
                                     out_w,
                                     out_h,
                                     &working_color,
                                     working_fmt,
                                     &a_tex,
                                     &a_owned)
        || placebo_transition_normalize(gpu,
                                        b_frame,
                                        b_image,
                                        b_format,
                                        requested,
                                        out_w,
                                        out_h,
                                        &working_color,
                                        working_fmt,
                                        &b_tex,
                                        &b_owned)
        || (state->wipe
            && placebo_transition_normalize(gpu,
                                            state->wipe,
                                            mask_image,
                                            mask_format,
                                            requested,
                                            out_w,
                                            out_h,
                                            &working_color,
                                            working_fmt,
                                            &mask_tex,
                                            &mask_owned))) {
        mlt_log_error(service, "Failed to normalize luma transition inputs\n");
        goto fail_locked;
    }

    mixed = placebo_tex_acquire_work(gpu,
                                     out_w,
                                     out_h,
                                     working_fmt,
                                     PLACEBO_TEX_STORABLE | PLACEBO_TEX_HOST_READABLE);
    if (!mixed
        || mix_textures(gpu,
                        dispatch,
                        a_tex,
                        b_tex,
                        mask_tex,
                        mixed,
                        state->progress,
                        state->width,
                        state->invert)) {
        mlt_log_error(service, "Luma transition shader failed\n");
        goto fail_locked;
    }

    if (output_fmt == working_fmt && pl_color_space_equal(&output_color, &working_color)) {
        result = mixed;
        mixed = NULL;
    } else {
        result = placebo_tex_acquire_work(gpu,
                                          out_w,
                                          out_h,
                                          output_fmt,
                                          PLACEBO_TEX_STORABLE | PLACEBO_TEX_HOST_READABLE);
        if (!result) {
            mlt_log_error(service, "Failed to create luma output texture\n");
            goto fail_locked;
        }
        struct pl_frame src = placebo_transition_texture_frame(mixed, &working_color);
        struct pl_frame dst = placebo_transition_texture_frame(result, &output_color);
        if (placebo_transition_render(PLACEBO_SITE_TRANSITION, &src, &dst)) {
            mlt_log_error(service, "Failed to encode luma output\n");
            goto fail_locked;
        }
    }

    if (a_owned)
        placebo_tex_recycle(gpu, &a_tex);
    if (b_owned)
        placebo_tex_recycle(gpu, &b_tex);
    if (mask_owned)
        placebo_tex_recycle(gpu, &mask_tex);
    if (mixed)
        placebo_tex_recycle(gpu, &mixed);
    placebo_render_unlock();

    *width = out_w;
    *height = out_h;
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(a_frame), "width", out_w);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(a_frame), "height", out_h);
    if (placebo_frame_publish_tex(a_frame, result, requested, image, format, trc)) {
        mlt_log_error(service, "Failed to publish luma output\n");
        return 1;
    }
    return 0;

fail_locked:
    if (a_owned)
        placebo_tex_recycle(gpu, &a_tex);
    if (b_owned)
        placebo_tex_recycle(gpu, &b_tex);
    if (mask_owned)
        placebo_tex_recycle(gpu, &mask_tex);
    if (mixed)
        placebo_tex_recycle(gpu, &mixed);
    if (result)
        placebo_tex_recycle(gpu, &result);
    placebo_render_unlock();
    return 1;
}

static int failed_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    (void) frame;
    (void) format;
    (void) width;
    (void) height;
    (void) writable;
    *image = NULL;
    return 1;
}

static mlt_frame process(mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame)
{
    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);
    mlt_service service = MLT_TRANSITION_SERVICE(transition);
    struct luma_frame *state = calloc(1, sizeof(*state));
    char name[64];

    if (!state) {
        mlt_log_error(service, "Failed to allocate luma frame state\n");
        mlt_frame_push_get_image(a_frame, failed_get_image);
        return a_frame;
    }
    state->transition = transition;
    mlt_properties_inc_ref(properties);
    state->invert = mlt_properties_get_int(properties, "invert") != 0;
    double progress = mlt_transition_get_progress(transition, a_frame);
    double eased = state->invert ? 1.0 - (1.0 - progress) * (1.0 - progress) : progress * progress;
    state->progress = (float) (mlt_properties_get_int(properties, "reverse") ? 1.0 - eased : eased);
    state->error = get_wipe(transition, a_frame, &state->wipe);
    if (state->wipe) {
        mlt_position position = mlt_transition_get_position(transition, a_frame);
        mlt_position length = mlt_transition_get_length(transition);
        double softness = mlt_properties_anim_get_double(properties, "softness", position, length);
        double width = 1.0 / (softness + 0.0001);
        if (!isfinite(width) || width < 0.0) {
            mlt_log_error(service, "Invalid luma softness: %g\n", softness);
            state->error = 1;
        } else {
            state->width = (float) width;
        }
    }

    int n = snprintf(name, sizeof(name), "_placebo_luma_mix.%p", (void *) state);
    if (n < 0 || n >= (int) sizeof(name)
        || mlt_properties_set_data(MLT_FRAME_PROPERTIES(a_frame),
                                   name,
                                   state,
                                   0,
                                   close_frame_state,
                                   NULL)) {
        mlt_log_error(service, "Failed to retain luma frame state\n");
        close_frame_state(state);
        mlt_frame_push_get_image(a_frame, failed_get_image);
        return a_frame;
    }

    mlt_frame_prepend_convert_image(a_frame, placebo_convert_image);
    mlt_frame_prepend_convert_image(b_frame, placebo_convert_image);
    if (state->wipe)
        mlt_frame_prepend_convert_image(state->wipe, placebo_convert_image);

    if (mlt_frame_push_frame(a_frame, b_frame))
        goto fail;
    if (mlt_frame_push_service(a_frame, state)) {
        mlt_frame_pop_frame(a_frame);
        goto fail;
    }
    if (mlt_frame_push_get_image(a_frame, get_image)) {
        mlt_frame_pop_service(a_frame);
        mlt_frame_pop_frame(a_frame);
        goto fail;
    }
    return a_frame;

fail:
    mlt_log_error(service, "Failed to stack luma transition\n");
    mlt_frame_push_get_image(a_frame, failed_get_image);
    return a_frame;
}

mlt_transition transition_placebo_luma_mix_init(mlt_profile profile,
                                                mlt_service_type type,
                                                const char *id,
                                                char *arg)
{
    mlt_transition transition = mlt_transition_new();
    struct luma_cache *cache;
    int error;

    (void) profile;
    (void) type;
    (void) id;
    if (!transition)
        return NULL;

    cache = calloc(1, sizeof(*cache));
    if (!cache) {
        mlt_log_error(MLT_TRANSITION_SERVICE(transition), "Failed to allocate wipe cache\n");
        mlt_transition_close(transition);
        return NULL;
    }
    error = pthread_mutex_init(&cache->mutex, NULL);
    if (error) {
        mlt_log_error(MLT_TRANSITION_SERVICE(transition),
                      "Failed to initialize wipe cache mutex: %s\n",
                      strerror(error));
        free(cache);
        mlt_transition_close(transition);
        return NULL;
    }

    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);
    if (mlt_properties_set_data(properties, "_luma_cache", cache, 0, close_cache, NULL)) {
        mlt_log_error(MLT_TRANSITION_SERVICE(transition), "Failed to retain wipe cache\n");
        close_cache(cache);
        mlt_transition_close(transition);
        return NULL;
    }
    mlt_properties_set(properties, "resource", arg);
    mlt_properties_set_int(properties, "_transition_type", 1);
    transition->process = process;
    if (placebo_gpu_get())
        placebo_vulkan_publish();
    return transition;
}
