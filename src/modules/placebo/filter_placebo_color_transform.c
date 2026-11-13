/*
 * filter_placebo_color_transform.c -- GPU transfer conversion
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
#include "render_cache.h"
#include "tex_pool.h"
#include "vulkan_frame.h"

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_image.h>
#include <framework/mlt_log.h>

#include <libplacebo/config.h>
#include <libplacebo/renderer.h>

#include <string.h>

/* color_transform factories this and runs it ahead of its CPU converter.
   CPU images stay on that converter. An mlt_image_vulkan texture is the usual
   input: this filter is the first one after avformat, so the decoded picture
   is still the producer's VkImage and not yet a placebo texture. */

static int subtype_is_foreign(const char *subtype)
{
    return subtype && *subtype && strcmp(subtype, "placebo") && strcmp(subtype, "vulkan");
}

static int request_is_gpu(mlt_frame frame, mlt_image_format format)
{
    const char *subtype;

    if (format == mlt_image_vulkan)
        return 1;
    if (format != mlt_image_private)
        return 0;
    if (placebo_frame_wants_tex(frame, format))
        return 1;
    subtype = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), MLT_PLACEBO_IMAGE_PRIVATE);
    return subtype && !strcmp(subtype, "placebo");
}

/* avformat keeps the decoded picture in VRAM only for mlt_image_vulkan.
   Ask for that unless another module already claimed mlt_image_private. */
static void request_vulkan_frame(mlt_frame frame, mlt_image_format *format)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    const char *subtype = mlt_properties_get(properties, MLT_PLACEBO_IMAGE_PRIVATE);

    if (subtype_is_foreign(subtype))
        return;
    placebo_request_gpu_image(frame, format);
}

static int target_trc(mlt_filter filter, mlt_frame frame, mlt_color_trc *out)
{
    const char *name = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "force_trc");
    if (!name)
        name = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "consumer.mlt_color_trc");
    if (!name)
        return 0;
    mlt_color_trc trc = mlt_image_color_trc_id(name);
    if (trc == mlt_color_trc_none || trc == mlt_color_trc_invalid)
        return 0;
    *out = trc;
    return 1;
}

static mlt_color_trc source_trc(mlt_frame frame)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    const char *name = mlt_properties_get(properties, "color_trc");
    if (name)
        return mlt_image_color_trc_id(name);

    mlt_colorspace colorspace = mlt_image_colorspace_id(
        mlt_properties_get(properties, "colorspace"));
    if (colorspace == mlt_colorspace_invalid || colorspace == mlt_colorspace_unspecified) {
        colorspace = mlt_image_default_colorspace(mlt_image_yuv422,
                                                  mlt_properties_get_int(properties, "height"));
    }
    mlt_color_trc trc = mlt_image_default_trc(colorspace);
    mlt_properties_set_int(properties, "color_trc", trc);
    return trc;
}

static void log_trc(mlt_filter filter, mlt_color_trc in_trc, mlt_color_trc out_trc)
{
    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "color trc: %s -> %s\n",
                  mlt_image_color_trc_name(in_trc),
                  mlt_image_color_trc_name(out_trc));
    if (in_trc == out_trc)
        return;
    if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_logged_trc"))
        return;
    mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_logged_trc", 1);
    mlt_log_info(MLT_FILTER_SERVICE(filter),
                 "color trc: %s -> %s\n",
                 mlt_image_color_trc_name(in_trc),
                 mlt_image_color_trc_name(out_trc));
}

static int transfers_match(struct pl_color_space src, enum pl_color_transfer transfer)
{
    struct pl_color_space dst = src;
    dst.transfer = transfer;
    pl_color_space_infer_map(&src, &dst);
    return src.transfer == dst.transfer;
}

/* Pixel requests have to land in a texture download_rgba can read.
   rgba64 is 16-bit unorm. Everything else is rgba8. A private request keeps
   half-float when the output transfer is linear. */
static pl_fmt output_tex_format(mlt_filter filter,
                                pl_gpu gpu,
                                mlt_color_trc out_trc,
                                mlt_image_format requested)
{
    if ((requested == mlt_image_private || requested == mlt_image_vulkan)
        && out_trc == mlt_color_trc_linear) {
        pl_fmt linear = placebo_linear_format(gpu);
        if (linear)
            return linear;
        if (!mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_logged_rgba8")) {
            mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_logged_rgba8", 1);
            mlt_log_warning(MLT_FILTER_SERVICE(filter),
                            "No half-float texture; linear light stays in rgba8\n");
        }
    }
    if (requested == mlt_image_rgba64) {
        pl_fmt rgba16 = placebo_rgba16_format(gpu);
        if (rgba16)
            return rgba16;
        if (!mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_logged_rgba16")) {
            mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_logged_rgba16", 1);
            mlt_log_warning(MLT_FILTER_SERVICE(filter),
                            "No rgba16 texture; rgba64 is encoded as rgba8\n");
        }
    }
    return placebo_rgba8_format(gpu);
}

static int tex_matches_request(pl_tex tex, pl_gpu gpu, mlt_image_format requested)
{
    pl_fmt want;

    if (requested == mlt_image_private)
        return 1;
    want = requested == mlt_image_rgba64 ? placebo_rgba16_format(gpu) : placebo_rgba8_format(gpu);
    return placebo_tex_is(tex, want);
}

static int convert_trc(mlt_filter filter,
                       mlt_frame frame,
                       uint8_t **image,
                       mlt_image_format *format,
                       int *width,
                       int *height,
                       mlt_color_trc out_trc,
                       mlt_image_format requested)
{
    pl_gpu gpu = placebo_gpu_get();
    if (!gpu || *width < 1 || *height < 1)
        return 1;

    int is_tex = placebo_frame_is_tex(frame, *format);
    int is_vulkan = placebo_frame_is_vulkan(frame, *format);
    if (!is_tex && !is_vulkan)
        return 0;
    if (is_tex && !placebo_image_get_tex(*image))
        return 1;

    int need_pixels = requested != mlt_image_private;
    enum pl_color_transfer dst_transfer = placebo_transfer_from_mlt(out_trc);
    if (dst_transfer == PL_COLOR_TRC_UNKNOWN) {
        mlt_log_info(MLT_FILTER_SERVICE(filter),
                     "No libplacebo curve for %s\n",
                     mlt_image_color_trc_name(out_trc));
        return need_pixels && is_vulkan ? 1 : 0;
    }

    mlt_color_trc in_trc = source_trc(frame);

    placebo_render_lock();

    struct pl_frame mapped = {0};
    int mapped_vulkan = 0;
    struct pl_color_space src_color = {0};
    if (is_vulkan) {
        if (!placebo_map_vulkan_image(gpu, *image, &mapped)) {
            mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to map Vulkan frame\n");
            placebo_render_unlock();
            return 1;
        }
        mapped_vulkan = 1;
        src_color = mapped.color;
    } else {
        src_color = placebo_color_space_for_trc(frame, placebo_transfer_from_mlt(in_trc));
    }

    pl_tex current = is_tex ? placebo_image_get_tex(*image) : NULL;
    int same = transfers_match(src_color, dst_transfer);
    int tex_usable = is_tex && tex_matches_request(current, gpu, requested);
    if (same && (tex_usable || (is_vulkan && !need_pixels))) {
        int image_matches = 0;
        if (is_vulkan) {
            mlt_vulkan_image *vulkan = (mlt_vulkan_image *) *image;
            image_matches = vulkan && vulkan->color_trc == (int) out_trc;
        } else {
            image_matches = in_trc == out_trc;
        }
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        placebo_render_unlock();
        /* No new image. bt709 and smpte170m share one libplacebo curve, so
           only write the property when the image already has this enum. */
        if (image_matches)
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "color_trc", out_trc);
        log_trc(filter, in_trc, out_trc);
        return 0;
    }

    pl_fmt dst_fmt = output_tex_format(filter, gpu, out_trc, requested);
    if (!dst_fmt) {
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        placebo_render_unlock();
        return 1;
    }

    pl_tex dst_tex = placebo_tex_acquire_target(gpu, *width, *height, dst_fmt);
    if (!dst_tex) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to create dest texture\n");
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        placebo_render_unlock();
        return 1;
    }

    struct pl_frame pl_src = {0};
    const struct pl_frame *src = &pl_src;
    if (mapped_vulkan) {
        src = &mapped;
    } else {
        pl_src = (struct pl_frame){
            .num_planes = 1,
            .planes = {{
                .texture = current,
                .components = 4,
                .component_mapping = {0, 1, 2, 3},
            }},
            .repr = pl_color_repr_rgb,
            .color = src_color,
        };
        /* Uploads are straight RGBA. Unknown alpha is independent on input
           and premultiplied on output, and the default background then
           composites the coverage onto opaque black. */
        pl_src.repr.alpha = PL_ALPHA_INDEPENDENT;
    }

    const enum pl_color_transfer output = placebo_frame_output_transfer(frame);
    struct pl_color_space dst_color = placebo_color_space_retarget(&src_color, dst_transfer, output);
    struct pl_frame pl_dst = {
        .num_planes = 1,
        .planes = {{
            .texture = dst_tex,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = pl_color_repr_rgb,
        .color = dst_color,
    };
    pl_dst.repr.alpha = PL_ALPHA_INDEPENDENT;

    struct pl_render_params params = pl_render_default_params;
    /* A YUV frame has no alpha. An RGBA frame, such as qtext, does. The
       default background composites that coverage onto black and leaves an
       opaque card, which hides the picture under a source-over blend. */
#if PL_API_VER >= 346
    params.background = PL_CLEAR_SKIP;
#else
    params.skip_target_clearing = true;
#endif
    params.background_transparency = 1.f;
    if (!placebo_render(PLACEBO_SITE_COLOR_TRANSFORM, src, &pl_dst, &params)) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "pl_render_image failed\n");
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        placebo_tex_recycle(gpu, &dst_tex);
        placebo_render_unlock();
        return 1;
    }
    if (mapped_vulkan)
        placebo_unmap_vulkan_image(gpu, &mapped);
    placebo_render_unlock();

    if (placebo_frame_publish_tex(frame, dst_tex, requested, image, format, out_trc))
        return 1;
    log_trc(filter, in_trc, out_trc);
    return 0;
}

/* Pull, then register the downloader.

   color_transform calls process() from get_image. On a tractor that frame is
   the output wrapper, and get_image_from_service copies the track's converters
   only when this frame's list is still empty (first list wins). Registering
   during process() fills the list beforehand, so the copy never happens and a
   yuv422 consumer is left holding RGBA. sdl2 uploads that as YUY2. */
static int pull_image(mlt_frame frame,
                      uint8_t **image,
                      mlt_image_format *format,
                      int *width,
                      int *height,
                      int writable)
{
    int error = mlt_frame_get_image(frame, image, format, width, height, writable);
    mlt_frame_prepend_convert_image(frame, placebo_convert_image);
    return error;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = mlt_frame_pop_service(frame);
    mlt_image_format requested = *format;
    const char *subtype = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), MLT_PLACEBO_IMAGE_PRIVATE);
    int consumer_mode = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "force_trc") != NULL;

    /* Another module's private image is not a Vulkan frame or a placebo texture. */
    if (requested == mlt_image_private && subtype_is_foreign(subtype))
        return pull_image(frame, image, format, width, height, writable);

    /* CPU frames stay on color_transform. force_trc is the consumer path,
       which has to encode before download. A GPU request is a placebo
       texture or a Vulkan frame from the producer. */
    if (!request_is_gpu(frame, requested) && !consumer_mode)
        return pull_image(frame, image, format, width, height, writable);

    mlt_color_trc out_trc = mlt_color_trc_none;
    if (!target_trc(filter, frame, &out_trc))
        return pull_image(frame, image, format, width, height, writable);

    request_vulkan_frame(frame, format);

    int error = pull_image(frame, image, format, width, height, writable);
    if (error || !*image)
        return error;
    if (!placebo_frame_is_tex(frame, *format) && !placebo_frame_is_vulkan(frame, *format))
        return 0;

    return convert_trc(filter, frame, image, format, width, height, out_trc, requested);
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_placebo_color_transform_init(mlt_profile profile,
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
    /* Do not create or publish the GPU. color_transform ignores NULL and
       keeps the CPU converter when no other placebo service has published. */
    if (!placebo_context_is_published()) {
        mlt_filter_close(filter);
        return NULL;
    }
    filter->process = filter_process;
    return filter;
}
