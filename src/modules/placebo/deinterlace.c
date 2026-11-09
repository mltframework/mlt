/*
 * deinterlace.c -- shared libplacebo deinterlacing for filters and links
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

#include "color_space.h"
#include "convert.h"
#include "gpu_context.h"
#include "transition_common.h"
#include "vulkan_frame.h"

#include <framework/mlt_image.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>

#include <libplacebo/config.h>
#include <libplacebo/dispatch.h>
#include <libplacebo/renderer.h>
#include <libplacebo/shaders/deinterlacing.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    pthread_mutex_t mutex;
    mlt_deinterlacer requested;
    mlt_deinterlacer used;
    char *unknown;
} deinterlace_context;

typedef struct
{
    mlt_deinterlacer used;
    enum pl_deinterlace_algorithm algo;
    bool skip_spatial_check;
} deinterlace_method;

typedef struct
{
    mlt_frame frame;
    uint8_t *image;
    mlt_image_format format;
    struct pl_frame mapped;
    int is_vulkan;
    int is_mapped;
    int acquired;
} gpu_image;

typedef struct
{
    int level;
    const char *message;
} deinterlace_fallback;

static const deinterlace_fallback no_references = {MLT_LOG_VERBOSE, "no adjacent frames"};
static const deinterlace_fallback incompatible_references = {MLT_LOG_WARNING,
                                                             "incompatible frame references"};
static const deinterlace_fallback aliased_references = {MLT_LOG_VERBOSE,
                                                        "adjacent frames share images"};
static const deinterlace_fallback mismatched_references
    = {MLT_LOG_VERBOSE, "adjacent frames differ in source, scan type, or field order"};
static const deinterlace_fallback failed_references = {MLT_LOG_WARNING,
                                                       "unable to obtain adjacent frames"};

static void close_context(void *data)
{
    deinterlace_context *context = data;
    if (!context)
        return;
    pthread_mutex_destroy(&context->mutex);
    free(context->unknown);
    free(context);
}

int placebo_deinterlace_setup(mlt_service service)
{
    deinterlace_context *context = calloc(1, sizeof(*context));
    if (!context) {
        mlt_log_error(service, "Failed to allocate deinterlacing state\n");
        return 1;
    }
    context->requested = context->used = mlt_deinterlacer_invalid;
    if (pthread_mutex_init(&context->mutex, NULL)) {
        mlt_log_error(service, "Failed to initialize deinterlacing mutex\n");
        free(context);
        return 1;
    }
    if (mlt_properties_set_data(MLT_SERVICE_PROPERTIES(service),
                                "_placebo_deinterlace",
                                context,
                                0,
                                close_context,
                                NULL)) {
        mlt_log_error(service, "Failed to store deinterlacing state\n");
        close_context(context);
        return 1;
    }
    return 0;
}

/* Logs the method only when it changes, so this is quiet per frame. */
static void log_method(mlt_service service,
                       mlt_deinterlacer requested,
                       const char *name,
                       mlt_deinterlacer used,
                       const deinterlace_fallback *fallback)
{
    deinterlace_context *context = mlt_properties_get_data(MLT_SERVICE_PROPERTIES(service),
                                                           "_placebo_deinterlace",
                                                           NULL);
    if (!context)
        return;
    pthread_mutex_lock(&context->mutex);
    int unknown = requested == mlt_deinterlacer_invalid;
    int changed = context->requested != requested || context->used != used
                  || (unknown && (!context->unknown || strcmp(context->unknown, name)));
    if (changed) {
        free(context->unknown);
        context->unknown = unknown ? strdup(name) : NULL;
        context->requested = requested;
        context->used = used;
    }
    int allocation_failed = changed && unknown && !context->unknown;
    pthread_mutex_unlock(&context->mutex);
    if (!changed)
        return;
    if (allocation_failed)
        mlt_log_error(service, "Failed to retain unknown deinterlacer name\n");
    const char *requested_name = unknown ? name : mlt_deinterlacer_name(requested);
    const char *used_name = mlt_deinterlacer_name(used);
    if (fallback)
        mlt_log(service,
                fallback->level,
                "Deinterlacing %s with GPU %s: %s\n",
                requested_name,
                used_name,
                fallback->message);
    else if (requested != used)
        mlt_log_warning(service,
                        "Deinterlacer %s is unavailable; using GPU %s\n",
                        requested_name,
                        used_name);
    else
        mlt_log_verbose(service, "Deinterlacing with GPU %s\n", used_name);
}

static deinterlace_method select_method(mlt_deinterlacer requested, mlt_deinterlacer default_method)
{
    if (requested == mlt_deinterlacer_none || requested == mlt_deinterlacer_weave)
        return (deinterlace_method){.used = requested};
    if (requested == mlt_deinterlacer_bob || requested == mlt_deinterlacer_onefield
        || default_method == mlt_deinterlacer_bob)
        return (deinterlace_method){.used = mlt_deinterlacer_bob, .algo = PL_DEINTERLACE_BOB};
#if PL_API_VER >= 353
    if (requested == mlt_deinterlacer_bwdif)
        return (deinterlace_method){.used = mlt_deinterlacer_bwdif, .algo = PL_DEINTERLACE_BWDIF};
#endif
    int nospatial = requested == mlt_deinterlacer_yadif_nospatial;
    return (deinterlace_method){.used = nospatial ? mlt_deinterlacer_yadif_nospatial
                                                  : mlt_deinterlacer_yadif,
                                .algo = PL_DEINTERLACE_YADIF,
                                .skip_spatial_check = nospatial};
}

static int image_is_gpu(mlt_frame frame, mlt_image_format format)
{
    return placebo_frame_is_tex(frame, format) || placebo_frame_is_vulkan(frame, format);
}

static int upload_image(mlt_service service,
                        mlt_frame frame,
                        uint8_t **image,
                        mlt_image_format *format)
{
    if (image_is_gpu(frame, *format))
        return 0;
    if (*format == mlt_image_private || *format == mlt_image_movit
        || *format == mlt_image_opengl_texture) {
        mlt_log_error(service,
                      "Cannot deinterlace foreign image format %s\n",
                      mlt_image_format_name(*format));
        return 1;
    }
    placebo_frame_set_requested_tex(frame, 1);
    int error = mlt_frame_convert_image(frame, image, format, mlt_image_private);
    placebo_frame_set_requested_tex(frame, 0);
    if (error || !*image || !image_is_gpu(frame, *format)) {
        mlt_log_error(service, "Failed to upload deinterlacing input\n");
        return 1;
    }
    return 0;
}

static int gpu_images_alias(const gpu_image *a, const gpu_image *b)
{
    if (!a->image || !b->image)
        return 0;
    if (a->image == b->image)
        return 1;
    if (a->format != mlt_image_vulkan || b->format != mlt_image_vulkan)
        return 0;
    const mlt_vulkan_image *va = (const mlt_vulkan_image *) a->image;
    const mlt_vulkan_image *vb = (const mlt_vulkan_image *) b->image;
    for (int i = 0; i < va->nb_planes && i < MLT_VULKAN_MAX_PLANES; i++)
        for (int j = 0; j < vb->nb_planes && j < MLT_VULKAN_MAX_PLANES; j++)
            if (va->planes[i].image && va->planes[i].image == vb->planes[j].image)
                return 1;
    return 0;
}

static int references_alias(const gpu_image *current,
                            const gpu_image *previous,
                            const gpu_image *next)
{
    return gpu_images_alias(current, previous) || gpu_images_alias(current, next)
           || gpu_images_alias(previous, next);
}

static gpu_image cached_image(mlt_frame frame)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    return (gpu_image){.frame = frame,
                       .image = mlt_properties_get_data(properties, "image", NULL),
                       .format = mlt_properties_get_int(properties, "format")};
}

static int is_adjacent(mlt_frame current, mlt_frame reference, mlt_position offset)
{
    mlt_producer original = mlt_frame_get_original_producer(current);
    mlt_producer ref_original = mlt_frame_get_original_producer(reference);
    if (original && ref_original
        && mlt_producer_cut_parent(original) != mlt_producer_cut_parent(ref_original))
        return 0;
    mlt_properties props = MLT_FRAME_PROPERTIES(current);
    mlt_properties ref_props = MLT_FRAME_PROPERTIES(reference);
    return !mlt_properties_exists(props, "original_position")
           || !mlt_properties_exists(ref_props, "original_position")
           || mlt_frame_original_position(reference)
                  == mlt_frame_original_position(current) + offset;
}

/* References fetched upstream of converter links borrow the current frame's
   converters through a proxy owned by this image request. */
static mlt_frame reference_proxy(mlt_service service,
                                 mlt_frame current,
                                 mlt_frame reference,
                                 mlt_color_trc trc)
{
    mlt_frame proxy = mlt_frame_init(service);
    if (!proxy)
        return NULL;
    mlt_properties proxy_props = MLT_FRAME_PROPERTIES(proxy);
    mlt_frame_copy_convert_image(proxy, current);
    mlt_frame_copy_convert_image(proxy, reference);
    if (mlt_frame_prepend_image_from_service(proxy, reference)) {
        mlt_frame_close(proxy);
        return NULL;
    }
    mlt_properties_pass_list(proxy_props,
                             MLT_FRAME_PROPERTIES(current),
                             "consumer.rescale consumer.color_trc");
    if (trc != mlt_color_trc_none && trc != mlt_color_trc_invalid
        && trc != mlt_color_trc_unspecified)
        mlt_properties_set_int(proxy_props, "consumer.mlt_color_trc", trc);
    return proxy;
}

static const deinterlace_fallback *fetch_reference(mlt_service service,
                                                   const gpu_image *current,
                                                   gpu_image *reference,
                                                   mlt_position offset,
                                                   int width,
                                                   int height,
                                                   mlt_color_trc trc,
                                                   mlt_frame *owned_proxy)
{
    mlt_frame source = reference->frame;
    if (!is_adjacent(current->frame, source, offset))
        return &mismatched_references;
    mlt_frame proxy = reference_proxy(service, current->frame, source, trc);
    if (!proxy)
        return &failed_references;
    *owned_proxy = proxy;
    mlt_properties ref_props = MLT_FRAME_PROPERTIES(source);
    reference->frame = proxy;
    // Keep hardware references on the GPU; defer software conversion until
    // shared source buffers have been checked.
    reference->format = current->format == mlt_image_vulkan ? mlt_image_vulkan : mlt_image_none;
    placebo_frame_set_requested_tex(proxy, 1);
    placebo_frame_set_requested_tex(source, 1);
    int error
        = mlt_frame_get_image(proxy, &reference->image, &reference->format, &width, &height, 0);
    placebo_frame_set_requested_tex(source, 0);
    placebo_frame_set_requested_tex(proxy, 0);
    if (error || !reference->image
        || mlt_properties_get_int(MLT_FRAME_PROPERTIES(proxy), "test_image")
        || mlt_properties_get_int(ref_props, "test_image"))
        return &failed_references;
    if (mlt_properties_get_int(ref_props, "progressive")
        || mlt_properties_get_int(ref_props, "top_field_first")
               != mlt_properties_get_int(MLT_FRAME_PROPERTIES(current->frame), "top_field_first"))
        return &mismatched_references;
    return NULL;
}

static const deinterlace_fallback *get_references(mlt_service service,
                                                  const gpu_image *current,
                                                  gpu_image *previous,
                                                  gpu_image *next,
                                                  int width,
                                                  int height,
                                                  mlt_color_trc trc,
                                                  mlt_frame proxies[2])
{
    const deinterlace_fallback *fallback
        = fetch_reference(service, current, previous, -1, width, height, trc, &proxies[0]);
    if (!fallback)
        fallback = fetch_reference(service, current, next, 1, width, height, trc, &proxies[1]);
    if (fallback)
        return fallback;
    // Lazy producers can reveal shared buffers only after image acquisition.
    if (references_alias(current, previous, next))
        return &aliased_references;
    mlt_frame_prepend_convert_image(previous->frame, placebo_convert_image);
    mlt_frame_prepend_convert_image(next->frame, placebo_convert_image);
    if (upload_image(service, previous->frame, &previous->image, &previous->format)
        || upload_image(service, next->frame, &next->image, &next->format))
        return &failed_references;
    return NULL;
}

static int map_image(pl_gpu gpu, gpu_image *image)
{
    image->is_vulkan = placebo_frame_is_vulkan(image->frame, image->format);
    if (image->is_vulkan) {
        if (!placebo_map_vulkan_image(gpu, image->image, &image->mapped))
            return 1;
        image->is_mapped = 1;
    } else {
        pl_tex tex = placebo_image_get_tex(image->image);
        if (!tex)
            return 1;
        struct pl_color_space color = pl_color_space_srgb;
        pl_fmt ignored = NULL;
        placebo_output_format(gpu, image->frame, image->format, &color, &ignored);
        image->mapped = (struct pl_frame){
            .num_planes = 1,
            .planes = {{
                .texture = tex,
                .components = 4,
                .component_mapping = {0, 1, 2, 3},
            }},
            .repr = pl_color_repr_rgb,
            .color = color,
        };
        image->mapped.repr.alpha = PL_ALPHA_INDEPENDENT;
    }
    return 0;
}

static void unmap_image(pl_gpu gpu, gpu_image *image)
{
    if (image->acquired && image->mapped.release) {
        image->mapped.release(gpu, &image->mapped);
        image->acquired = 0;
    }
    if (image->is_mapped) {
        placebo_unmap_vulkan_image(gpu, &image->mapped);
        image->is_mapped = 0;
    }
}

static int acquire_image(pl_gpu gpu, gpu_image *image)
{
    if (image->mapped.acquire) {
        if (!image->mapped.acquire(gpu, &image->mapped))
            return 1;
        image->acquired = 1;
    }
    return 0;
}

static int compatible_images(const struct pl_frame *current, const struct pl_frame *reference)
{
    if (current->num_planes != reference->num_planes || current->repr.sys != reference->repr.sys
        || current->repr.levels != reference->repr.levels
        || current->repr.alpha != reference->repr.alpha
        || current->repr.bits.color_depth != reference->repr.bits.color_depth
        || current->repr.bits.sample_depth != reference->repr.bits.sample_depth
        || current->repr.bits.bit_shift != reference->repr.bits.bit_shift
        || current->color.transfer != reference->color.transfer
        || current->color.primaries != reference->color.primaries
        || current->crop.x0 != reference->crop.x0 || current->crop.y0 != reference->crop.y0
        || current->crop.x1 != reference->crop.x1 || current->crop.y1 != reference->crop.y1)
        return 0;
    for (int i = 0; i < current->num_planes; i++) {
        const struct pl_plane *a = &current->planes[i];
        const struct pl_plane *b = &reference->planes[i];
        if (!a->texture || !b->texture || a->components != b->components
            || a->texture->params.w != b->texture->params.w
            || a->texture->params.h != b->texture->params.h
            || a->texture->params.format != b->texture->params.format || a->shift_x != b->shift_x
            || a->shift_y != b->shift_y
            || memcmp(a->component_mapping, b->component_mapping, sizeof(a->component_mapping)))
            return 0;
    }
    return 1;
}

static pl_fmt plane_format(pl_gpu gpu, pl_tex source)
{
    pl_fmt fmt = source ? source->params.format : NULL;
    if (!fmt)
        return NULL;
    if (fmt->type == PL_FMT_FLOAT) {
        if (fmt->component_depth[0] > 16)
            return pl_find_named_fmt(gpu, "rgba32f");
        return placebo_linear_format(gpu);
    }
    if (fmt->type == PL_FMT_UNORM)
        return fmt->component_depth[0] > 8 ? placebo_rgba16_format(gpu) : placebo_rgba8_format(gpu);
    return NULL;
}

static int map_inputs(mlt_service service,
                      pl_gpu gpu,
                      gpu_image *current,
                      gpu_image *previous,
                      gpu_image *next,
                      struct pl_deinterlace_params *params,
                      const deinterlace_fallback **fallback)
{
    if (map_image(gpu, current)) {
        mlt_log_error(service, "Failed to map deinterlacing input\n");
        return 1;
    }
    int temporal = pl_deinterlace_needs_refs(params->algo);
    if (temporal
        && (references_alias(current, previous, next)
            || current->is_vulkan != (previous->format == mlt_image_vulkan)
            || current->is_vulkan != (next->format == mlt_image_vulkan) || map_image(gpu, previous)
            || map_image(gpu, next) || !compatible_images(&current->mapped, &previous->mapped)
            || !compatible_images(&current->mapped, &next->mapped))) {
        *fallback = &incompatible_references;
        unmap_image(gpu, previous);
        unmap_image(gpu, next);
        params->algo = PL_DEINTERLACE_BOB;
        temporal = 0;
    }

    if (acquire_image(gpu, current)
        || (temporal && (acquire_image(gpu, previous) || acquire_image(gpu, next)))) {
        mlt_log_error(service, "Failed to acquire GPU deinterlacing input\n");
        return 1;
    }
    return 0;
}

static int dispatch_planes(mlt_service service,
                           pl_gpu gpu,
                           pl_dispatch dispatch,
                           const gpu_image *current,
                           const gpu_image *previous,
                           const gpu_image *next,
                           const struct pl_deinterlace_params *params,
                           enum pl_field field,
                           mlt_image_format requested,
                           pl_tex planes[PL_MAX_PLANES])
{
    int temporal = pl_deinterlace_needs_refs(params->algo);
    pl_dispatch_reset_frame(dispatch);
    for (int i = 0; i < current->mapped.num_planes; i++) {
        const struct pl_plane *plane = &current->mapped.planes[i];
        pl_fmt fmt = plane_format(gpu, plane->texture);
        if (!current->is_vulkan && requested == mlt_image_rgba64)
            fmt = placebo_rgba16_format(gpu);
        if (!fmt || !(fmt->caps & PL_FMT_CAP_SAMPLEABLE) || !(fmt->caps & PL_FMT_CAP_RENDERABLE)) {
            mlt_log_error(service, "No suitable GPU format for deinterlacing plane %d\n", i);
            return 1;
        }
        planes[i] = pl_tex_create(gpu,
                                  pl_tex_params(.w = plane->texture->params.w,
                                                .h = plane->texture->params.h,
                                                .format = fmt,
                                                .sampleable = true,
                                                .renderable = true,
                                                .storable = !!(fmt->caps & PL_FMT_CAP_STORABLE),
                                                .host_readable = !!(fmt->caps
                                                                    & PL_FMT_CAP_HOST_READABLE), ));
        if (!planes[i]) {
            mlt_log_error(service, "Failed to allocate deinterlacing plane %d\n", i);
            return 1;
        }
        struct pl_deinterlace_source source = {
            .cur = {.top = plane->texture},
            .prev = {.top = temporal ? previous->mapped.planes[i].texture : NULL},
            .next = {.top = temporal ? next->mapped.planes[i].texture : NULL},
            .field = field,
            .first_field = field,
            .component_mask = (1 << plane->components) - 1,
        };
        pl_shader shader = pl_dispatch_begin(dispatch);
        if (!shader) {
            mlt_log_error(service, "Failed to allocate deinterlacing shader\n");
            return 1;
        }
        pl_shader_deinterlace(shader, &source, params);
        if (!pl_dispatch_finish(dispatch,
                                pl_dispatch_params(.shader = &shader, .target = planes[i]))) {
            mlt_log_error(service, "Failed to run deinterlacing shader for plane %d\n", i);
            return 1;
        }
    }
    return 0;
}

static int compose_output(mlt_service service,
                          pl_gpu gpu,
                          pl_renderer renderer,
                          const gpu_image *current,
                          pl_tex planes[PL_MAX_PLANES],
                          mlt_image_format requested,
                          int width,
                          int height,
                          pl_tex *output)
{
    if (!current->is_vulkan) {
        *output = planes[0];
        planes[0] = NULL;
        return 0;
    }
    struct pl_color_space color;
    pl_fmt output_format = NULL;
    placebo_output_format(gpu, current->frame, requested, &color, &output_format);
    color = current->mapped.color;
    pl_tex source = current->mapped.planes[0].texture;
    if (source && source->params.format
        && (source->params.format->type == PL_FMT_FLOAT
            || source->params.format->component_depth[0] > 8))
        output_format = plane_format(gpu, source);
    if (!output_format) {
        mlt_log_error(service, "No suitable GPU output format\n");
        return 1;
    }
    struct pl_frame src = current->mapped;
    src.acquire = NULL;
    src.release = NULL;
    src.user_data = NULL;
    src.field = PL_FIELD_NONE;
    src.first_field = PL_FIELD_NONE;
    src.prev = NULL;
    src.next = NULL;
    if (planes)
        for (int i = 0; i < src.num_planes; i++)
            src.planes[i].texture = planes[i];
    *output = pl_tex_create(gpu,
                            pl_tex_params(.w = width,
                                          .h = height,
                                          .format = output_format,
                                          .sampleable = true,
                                          .renderable = true,
                                          .host_readable = true, ));
    if (!*output) {
        mlt_log_error(service, "Failed to allocate deinterlaced output\n");
        return 1;
    }
    struct pl_frame dst = placebo_transition_texture_frame(*output, &color);
    if (placebo_transition_render(renderer, &src, &dst)) {
        mlt_log_error(service, "Failed to render deinterlaced output\n");
        return 1;
    }
    return 0;
}

static int render_deinterlace(mlt_service service,
                              gpu_image *current,
                              gpu_image *previous,
                              gpu_image *next,
                              struct pl_deinterlace_params params,
                              const deinterlace_fallback **fallback,
                              enum pl_field field,
                              mlt_image_format requested,
                              int width,
                              int height,
                              pl_tex *result)
{
    pl_gpu gpu = placebo_gpu_get();
    pl_dispatch dispatch = placebo_dispatch_get();
    pl_renderer renderer = placebo_renderer_get();
    if (!gpu || !dispatch || !renderer) {
        mlt_log_error(service, "GPU deinterlacing is unavailable\n");
        return 1;
    }
    pl_tex planes[PL_MAX_PLANES] = {0};
    pl_tex output = NULL;
    placebo_render_lock();
    int error = map_inputs(service, gpu, current, previous, next, &params, fallback)
                || (field != PL_FIELD_NONE
                    && dispatch_planes(service,
                                       gpu,
                                       dispatch,
                                       current,
                                       previous,
                                       next,
                                       &params,
                                       field,
                                       requested,
                                       planes))
                || compose_output(service,
                                  gpu,
                                  renderer,
                                  current,
                                  field == PL_FIELD_NONE ? NULL : planes,
                                  requested,
                                  width,
                                  height,
                                  &output);
    unmap_image(gpu, next);
    unmap_image(gpu, previous);
    unmap_image(gpu, current);
    for (int i = 0; i < PL_MAX_PLANES; i++)
        pl_tex_destroy(gpu, &planes[i]);
    if (error)
        pl_tex_destroy(gpu, &output);
    else
        *result = output;
    placebo_render_unlock();
    return error;
}

static int passthrough(mlt_service service,
                       mlt_frame frame,
                       mlt_deinterlacer requested,
                       const char *name,
                       mlt_deinterlacer used,
                       uint8_t **image,
                       mlt_image_format *format,
                       int *width,
                       int *height,
                       int writable)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    int error = mlt_frame_get_image(frame, image, format, width, height, writable);
    mlt_frame_prepend_convert_image(frame, placebo_convert_image);
    if (!error && *image && used == mlt_deinterlacer_weave
        && mlt_properties_get_int(properties, "consumer.progressive")
        && !mlt_properties_get_int(properties, "test_image")
        && !mlt_properties_get_int(properties, "progressive")) {
        log_method(service, requested, name, used, NULL);
        mlt_properties_set_int(properties, "progressive", 1);
    }
    return error;
}

int placebo_deinterlace_get_image(mlt_service service,
                                  mlt_frame frame,
                                  mlt_deinterlacer default_method,
                                  const placebo_deinterlace_frames *references,
                                  uint8_t **image,
                                  mlt_image_format *format,
                                  int *width,
                                  int *height,
                                  int writable)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    const char *name = mlt_properties_get(properties, "consumer.deinterlacer");
    mlt_deinterlacer requested = name && *name ? mlt_deinterlacer_id(name) : default_method;
    deinterlace_method method = select_method(requested, default_method);
    if (!mlt_properties_get_int(properties, "consumer.progressive")
        || mlt_properties_get_int(properties, "test_image") || method.used == mlt_deinterlacer_none
        || method.used == mlt_deinterlacer_weave)
        return passthrough(
            service, frame, requested, name, method.used, image, format, width, height, writable);

    mlt_image_format requested_format = *format;
    placebo_image_source source = {.image = NULL};
    int error
        = placebo_frame_get_gpu_image_source(frame, image, format, width, height, writable, &source);
    mlt_frame_prepend_convert_image(frame, placebo_convert_image);
    if (error || !*image) {
        mlt_log_error(service, "Failed to obtain deinterlacing input\n");
        placebo_image_source_release(&source);
        return 1;
    }
    int progressive = mlt_properties_get_int(properties, "progressive");
    if (mlt_properties_get_int(properties, "test_image")
        || (progressive
            && (!placebo_frame_is_vulkan(frame, *format) || requested_format == mlt_image_vulkan
                || requested_format == mlt_image_private || requested_format == mlt_image_none))) {
        placebo_image_source_release(&source);
        return 0;
    }
    if (*width < 1 || *height < (progressive ? 1 : 2)) {
        mlt_log_error(service, "Invalid deinterlacing dimensions %dx%d\n", *width, *height);
        placebo_image_source_release(&source);
        return 1;
    }
    gpu_image previous = {0};
    gpu_image next = {0};
    gpu_image original = {.frame = frame,
                          .image = source.image ? source.image : *image,
                          .format = source.image ? source.format : *format};
    const deinterlace_fallback *fallback = NULL;
    if (!progressive && pl_deinterlace_needs_refs(method.algo)) {
        if (!references || !references->previous || !references->next) {
            fallback = &no_references;
        } else {
            previous = cached_image(references->previous);
            next = cached_image(references->next);
            // Compare source buffers even if the current image was automatically uploaded.
            if (previous.frame == frame || next.frame == frame || previous.frame == next.frame
                || references_alias(&original, &previous, &next))
                fallback = &aliased_references;
        }
    }
    gpu_image current = {.frame = frame, .image = *image, .format = *format};
    mlt_frame proxies[2] = {NULL, NULL};
    mlt_color_trc trc = placebo_frame_color_trc(frame, *image, *format);
    if (!progressive && pl_deinterlace_needs_refs(method.algo) && !fallback)
        fallback
            = get_references(service, &original, &previous, &next, *width, *height, trc, proxies);
    if (upload_image(service, frame, image, format)) {
        error = 1;
        goto done;
    }
    current.image = *image;
    current.format = *format;
    trc = placebo_frame_color_trc(frame, *image, *format);
    struct pl_deinterlace_params params = {
        .algo = progressive || fallback ? PL_DEINTERLACE_BOB : method.algo,
        .skip_spatial_check = method.skip_spatial_check,
    };

    enum pl_field field = mlt_properties_get_int(properties, "top_field_first") ? PL_FIELD_TOP
                                                                                : PL_FIELD_BOTTOM;
    if (progressive)
        field = PL_FIELD_NONE;
    pl_tex result = NULL;
    error = render_deinterlace(service,
                               &current,
                               &previous,
                               &next,
                               params,
                               &fallback,
                               field,
                               requested_format,
                               *width,
                               *height,
                               &result);
    if (error)
        goto done;
    if (!progressive)
        log_method(service, requested, name, fallback ? mlt_deinterlacer_bob : method.used, fallback);
    if (placebo_frame_publish_tex(frame, result, requested_format, image, format, trc)) {
        mlt_log_error(service, "Failed to publish deinterlaced image\n");
        error = 1;
        goto done;
    }
    mlt_properties_set_int(properties, "progressive", 1);
done:
    mlt_frame_close(proxies[1]);
    mlt_frame_close(proxies[0]);
    placebo_image_source_release(&source);
    return error;
}
