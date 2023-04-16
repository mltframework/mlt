/*
 * transition_luma.c -- a generic dissolve/wipe processor
 * Copyright (C) 2003-2023 Meltytech, LLC
 *
 * Adapted from Kino Plugin Timfx, which is
 * Copyright (C) 2002 Timothy M. Shead <tshead@k-3d.com>
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

#include <framework/mlt.h>
#include <framework/mlt_luma_map.h>

#include "transition_composite.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int is_opaque(uint8_t *alpha_channel, int width, int height)
{
    int n = width * height + 1;
    while (--n)
        if (*alpha_channel++ != 0xff)
            return 0;
    return 1;
}

static inline float calculate_mix(float weight, float alpha)
{
    return weight * alpha / 255.f;
}

static inline uint8_t sample_mix(uint8_t dest, uint8_t src, float mix)
{
    return src * mix + dest * (1.f - mix);
}

static void composite_line_yuv_float(
    uint8_t *dest, uint8_t *src, int width, uint8_t *alpha_b, uint8_t *alpha_a, float weight)
{
    register int j = 0;
    float mix_a, mix_b;

    for (; j < width; j++) {
        mix_a = calculate_mix(1.0f - weight, alpha_a ? *alpha_a : 255);
        mix_b = calculate_mix(weight, alpha_b ? *alpha_b : 255);
        if (alpha_a) {
            float mix2 = mix_b + mix_a - mix_b * mix_a;
            *alpha_a = 255 * mix2;
            if (mix2 != 0.f)
                mix_b /= mix2;
        }
        *dest = sample_mix(*dest, *src++, mix_b);
        dest++;
        *dest = sample_mix(*dest, *src++, mix_b);
        dest++;
        if (alpha_a)
            alpha_a++;
        if (alpha_b)
            alpha_b++;
    }
}

struct dissolve_slice_context
{
    uint8_t *dst_image;
    uint8_t *src_image;
    uint8_t *dst_alpha;
    uint8_t *src_alpha;
    int width;
    int height;
    float weight;
};

static int dissolve_slice(int id, int index, int count, void *context)
{
    struct dissolve_slice_context ctx = *((struct dissolve_slice_context *) context);
    int stride = ctx.width * 2;
    int slice_start, slice_height = mlt_slices_size_slice(count, index, ctx.height, &slice_start);
    int i;

    ctx.dst_image += slice_start * stride;
    ctx.src_image += slice_start * stride;
    if (ctx.dst_alpha)
        ctx.dst_alpha += slice_start * ctx.width;
    if (ctx.src_alpha)
        ctx.src_alpha += slice_start * ctx.width;

    for (i = 0; i < slice_height; i++) {
        composite_line_yuv_float(ctx.dst_image,
                                 ctx.src_image,
                                 ctx.width,
                                 ctx.src_alpha,
                                 ctx.dst_alpha,
                                 ctx.weight);
        ctx.dst_image += stride;
        ctx.src_image += stride;
        if (ctx.dst_alpha)
            ctx.dst_alpha += ctx.width;
        if (ctx.src_alpha)
            ctx.src_alpha += ctx.width;
    }
    return 0;
}

static inline int dissolve_yuv(mlt_frame frame,
                               mlt_frame that,
                               float weight,
                               int width,
                               int height,
                               int threads,
                               int alpha_over,
                               int fix_background_alpha)
{
    int ret = 0;
    int i = height + 1;
    int width_src = width, height_src = height;
    mlt_image_format format = (fix_background_alpha && frame->convert_image) ? mlt_image_rgba
                                                                             : mlt_image_yuv422;
    uint8_t *p_src, *p_dest;
    uint8_t *alpha_src;
    uint8_t *alpha_dst;
    int mix = weight * (1 << 16);

    if (mlt_properties_get(&frame->parent, "distort"))
        mlt_properties_set(&that->parent, "distort", mlt_properties_get(&frame->parent, "distort"));
    mlt_frame_get_image(frame, &p_dest, &format, &width, &height, 1);
    if (fix_background_alpha && frame->convert_image)
        frame->convert_image(frame, &p_dest, &format, mlt_image_yuv422);
    alpha_dst = mlt_frame_get_alpha(frame);
    if (fix_background_alpha && that->convert_image)
        format = mlt_image_rgba;
    mlt_frame_get_image(that, &p_src, &format, &width_src, &height_src, 0);
    if (that->convert_image)
        that->convert_image(that, &p_src, &format, mlt_image_yuv422);
    alpha_src = mlt_frame_get_alpha(that);
    int is_translucent = (alpha_dst && !is_opaque(alpha_dst, width, height))
                         || (alpha_src && !is_opaque(alpha_src, width_src, height_src));

    // Pick the lesser of two evils ;-)
    width_src = width_src > width ? width : width_src;
    height_src = height_src > height ? height : height_src;

    if (is_translucent && alpha_over) {
        struct dissolve_slice_context context = {.dst_image = p_dest,
                                                 .src_image = p_src,
                                                 .dst_alpha = alpha_dst,
                                                 .src_alpha = alpha_src,
                                                 .width = width_src,
                                                 .height = height_src,
                                                 .weight = weight};
        mlt_slices_run_normal(threads, dissolve_slice, &context);
    } else {
        while (--i) {
            composite_line_yuv(p_dest, p_src, width_src, alpha_src, alpha_dst, mix, NULL, 0, 0);
            p_src += width_src << 1;
            p_dest += width << 1;
            if (alpha_src)
                alpha_src += width_src;
            if (alpha_dst)
                alpha_dst += width;
        }
    }

    return ret;
}

/** A smoother, non-linear threshold determination function.
*/

static inline int32_t smoothstep(int32_t edge1, int32_t edge2, uint32_t a)
{
    if (a < edge1)
        return 0;

    if (a >= edge2)
        return 0x10000;

    a = ((a - edge1) << 16) / (edge2 - edge1);

    return (((a * a) >> 16) * ((3 << 16) - (2 * a))) >> 16;
}

static float smoothstep_float(float edge1, float edge2, float a)
{
    if (a < edge1)
        return 0.f;

    if (a >= edge2)
        return 1.f;

    a = (a - edge1) / (edge2 - edge1);

    return (a * a) * (3 - (2 * a));
}

/** powerful stuff

    \param field_order -1 = progressive, 0 = lower field first, 1 = top field first
*/
static void luma_composite(mlt_frame a_frame,
                           mlt_frame b_frame,
                           int luma_width,
                           int luma_height,
                           uint16_t *luma_bitmap,
                           float pos,
                           float frame_delta,
                           float softness,
                           int field_order,
                           int *width,
                           int *height,
                           int invert,
                           int fix_background_alpha)
{
    int width_src = *width, height_src = *height;
    int width_dest = *width, height_dest = *height;
    mlt_image_format format_src = (fix_background_alpha && a_frame->convert_image)
                                      ? mlt_image_rgba
                                      : mlt_image_yuv422;
    mlt_image_format format_dest = (fix_background_alpha && b_frame->convert_image)
                                       ? mlt_image_rgba
                                       : mlt_image_yuv422;
    uint8_t *p_src, *p_dest;
    uint8_t *alpha_src, *alpha_dest;
    int i, j;
    int stride_src;
    int stride_dest;

    if (mlt_properties_get(&a_frame->parent, "distort"))
        mlt_properties_set(&b_frame->parent,
                           "distort",
                           mlt_properties_get(&a_frame->parent, "distort"));
    mlt_frame_get_image(a_frame, &p_dest, &format_dest, &width_dest, &height_dest, 1);
    if (fix_background_alpha && a_frame->convert_image)
        a_frame->convert_image(a_frame, &p_dest, &format_dest, mlt_image_yuv422);
    alpha_dest = mlt_frame_get_alpha(a_frame);
    mlt_frame_get_image(b_frame, &p_src, &format_src, &width_src, &height_src, 0);
    if (fix_background_alpha && b_frame->convert_image)
        b_frame->convert_image(b_frame, &p_src, &format_src, mlt_image_yuv422);
    alpha_src = mlt_frame_get_alpha(b_frame);

    if (*width == 0 || *height == 0)
        return;

    int is_translucent = (alpha_dest && !is_opaque(alpha_dest, width_dest, height_dest))
                         || (alpha_src && !is_opaque(alpha_src, width_src, height_src));

    // Pick the lesser of two evils ;-)
    width_src = width_src > width_dest ? width_dest : width_src;
    height_src = height_src > height_dest ? height_dest : height_src;

    stride_src = width_src * 2;
    stride_dest = width_dest * 2;

    // Offset the position based on which field we're looking at ...
    float field_pos[2];
    field_pos[0] = (pos + ((field_order == 0 ? 1 : 0) * frame_delta * 0.5f)) * (1.f + softness);
    field_pos[1] = (pos + ((field_order == 0 ? 0 : 1) * frame_delta * 0.5f)) * (1.f + softness);

    register uint8_t *p;
    register uint8_t *q;
    uint16_t *l;
    int32_t x_diff = (luma_width << 16) / *width;
    int32_t y_diff = (luma_height << 16) / *height;
    int32_t x_offset = 0;
    int32_t y_offset = 0;
    uint8_t *p_row;
    uint8_t *q_row;
    uint32_t i_softness = softness * (1 << 16);
    int field_count = field_order < 0 ? 1 : 2;
    int field_stride_src = field_count * stride_src;
    int field_stride_dest = field_count * stride_dest;
    int field = 0;
    float mix_a, mix_b;

    // composite using luma map
    while (field < field_count) {
        p_row = p_src + field * stride_src;
        q_row = p_dest + field * stride_dest;
        y_offset = field << 16;
        i = field;

        while (i < height_src) {
            p = p_row;
            q = q_row;
            l = luma_bitmap + (y_offset >> 16) * (luma_width * field_count);
            x_offset = 0;
            j = width_src;

            if (is_translucent) {
                while (j--) {
                    float weight = l[x_offset >> 16] / 65535.f;
                    float value = smoothstep_float(weight, softness + weight, field_pos[field]);
                    mix_a = calculate_mix(1.0f - value, alpha_dest ? *alpha_dest : 255);
                    mix_b = calculate_mix(value, alpha_src ? *alpha_src : 255);
                    if (invert && alpha_src) {
                        float mix2 = mix_b + mix_a - mix_b * mix_a;
                        *alpha_src = 255 * mix2;
                        if (mix2 != 0.f)
                            mix_b /= mix2;
                    } else if (!invert && alpha_dest) {
                        float mix2 = mix_b + mix_a - mix_b * mix_a;
                        *alpha_dest = 255 * mix2;
                        if (mix2 != 0.f)
                            mix_b /= mix2;
                    }
                    *q = sample_mix(*q, *p++, mix_b);
                    q++;
                    *q = sample_mix(*q, *p++, mix_b);
                    q++;
                    if (alpha_dest)
                        alpha_dest++;
                    if (alpha_src)
                        alpha_src++;
                    x_offset += x_diff;
                }
            } else {
                while (j--) {
                    uint16_t weight = l[x_offset >> 16];
                    uint32_t value = smoothstep(weight,
                                                i_softness + weight,
                                                (1 << 16) * field_pos[field]);
                    *q = (*p++ * value + *q * ((1 << 16) - value)) >> 16;
                    q++;
                    *q = (*p++ * value + *q * ((1 << 16) - value)) >> 16;
                    q++;
                    x_offset += x_diff;
                }
            }
            y_offset += y_diff;
            i += field_count;
            p_row += field_stride_src;
            q_row += field_stride_dest;
        }

        field++;
    }
}

void yuv422_to_luma16(uint8_t *image, uint16_t **map, int width, int height, int full_range)
{
    // allocate the luma bitmap
    *map = (uint16_t *) mlt_pool_alloc(width * height * sizeof(uint16_t));
    if (!*map)
        return;

    int i;
    int n = width * height;
    uint8_t offset = full_range ? 0 : 16;
    uint8_t max = full_range ? 255 : 235 - offset;
    int factor = full_range ? 256 : 299; // 299 = 65535 / 219

    // process the image data into the luma bitmap
    for (i = 0; i < n; i++) {
        (*map)[i] = CLAMP(image[i << 1] - offset, 0, max) * factor;
    }
}

static int transition_get_image(mlt_frame a_frame,
                                uint8_t **image,
                                mlt_image_format *format,
                                int *width,
                                int *height,
                                int writable)
{
    // Get the b frame from the stack
    mlt_frame b_frame = mlt_frame_pop_frame(a_frame);

    // Get the transition object
    mlt_transition transition = mlt_frame_pop_service(a_frame);

    // Get the properties of the transition
    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);

    // Get the properties of the a frame
    mlt_properties a_props = MLT_FRAME_PROPERTIES(a_frame);

    // Get the properties of the b frame
    mlt_properties b_props = MLT_FRAME_PROPERTIES(b_frame);

    // This compositer is yuv422 only
    *format = mlt_image_yuv422;

    mlt_service_lock(MLT_TRANSITION_SERVICE(transition));

    // The cached luma map information
    int luma_width = mlt_properties_get_int(properties, "width");
    int luma_height = mlt_properties_get_int(properties, "height");
    uint16_t *luma_bitmap = mlt_properties_get_data(properties, "bitmap", NULL);
    char *current_resource = mlt_properties_get(properties, "_resource");
    mlt_producer producer = mlt_properties_get_data(properties, "producer", NULL);

    // If the filename property changed, reload the map
    char *resource = mlt_properties_get(properties, "resource");

    // Correct width/height if not specified
    if (luma_width == 0 || luma_height == 0) {
        luma_width = *width;
        luma_height = *height;
    }

    if (resource && (producer || !current_resource || strcmp(resource, current_resource))) {
        char temp[PATH_MAX];
        char *extension = strrchr(resource, '.');
        char *orig_resource = resource;
        mlt_profile profile = mlt_service_profile(MLT_TRANSITION_SERVICE(transition));

        if (strchr(resource, '%')) {
            snprintf(temp,
                     sizeof(temp),
                     "%s/lumas/%s/%s",
                     mlt_environment("MLT_DATA"),
                     mlt_profile_lumas_dir(profile),
                     strchr(resource, '%') + 1);
            FILE *test = mlt_fopen(temp, "r");
            if (!test) {
                strcat(temp, ".png");
                test = mlt_fopen(temp, "r");
            }
            if (test) {
                fclose(test);
                resource = temp;
            }
            extension = strrchr(resource, '.');
        }

        // See if it is a PGM
        if (extension != NULL && strcmp(extension, ".pgm") == 0) {
            // Load from PGM
            luma_bitmap = NULL;
            if (mlt_luma_map_from_pgm(resource, &luma_bitmap, &luma_width, &luma_height)) {
                // Failed to read file; generate it.
                mlt_luma_map luma = mlt_luma_map_new(orig_resource);
                if (profile) {
                    luma->w = profile->width;
                    luma->h = profile->height;
                }
                luma_bitmap = mlt_luma_map_render(luma);
                luma_width = luma->w;
                luma_height = luma->h;
                free(luma);
            }

            // Set the transition properties
            mlt_properties_set_int(properties, "width", luma_width);
            mlt_properties_set_int(properties, "height", luma_height);
            mlt_properties_set(properties, "_resource", orig_resource);
            mlt_properties_set_data(properties,
                                    "bitmap",
                                    luma_bitmap,
                                    luma_width * luma_height * 2,
                                    mlt_pool_release,
                                    NULL);
            mlt_properties_clear(properties, "producer");
        } else if (!*resource) {
            luma_bitmap = NULL;
            mlt_properties_set(properties, "_resource", NULL);
            mlt_properties_set_data(properties, "bitmap", luma_bitmap, 0, mlt_pool_release, NULL);
            mlt_properties_clear(properties, "producer");
        } else {
            if (!producer || !current_resource || strcmp(resource, current_resource)) {
                // Get the factory producer service
                char *factory = mlt_properties_get(properties, "factory");

                // Create the producer
                producer = mlt_factory_producer(profile, factory, resource);
                if (producer)
                    mlt_properties_set(properties, "_resource", resource);
            }

            // If we have one
            if (producer) {
                // Get the producer properties
                mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);

                // Ensure that we loop
                mlt_properties_set(producer_properties, "eof", "loop");

                // Now pass all producer. properties on the transition down
                mlt_properties_pass(producer_properties, properties, "producer.");

                // We will get the alpha frame from the producer
                mlt_frame luma_frame = NULL;

                // Determine if producer is a video clip or still image
                const char *service_name = mlt_properties_get(producer_properties, "mlt_service");
                int is_clip = mlt_producer_get_length(producer) > 1
                              && mlt_properties_get_int(producer_properties, "video_index") >= 0
                              && service_name && !strncmp("avformat", service_name, 8);
                if (is_clip) {
                    if (!mlt_properties_get(producer_properties, "producer.eof"))
                        mlt_properties_set(producer_properties, "eof", "pause");
                    mlt_producer_seek(producer, mlt_transition_get_position(transition, a_frame));
                }

                // Get the luma frame
                if (mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &luma_frame, 0) == 0) {
                    uint8_t *luma_image = NULL;
                    mlt_image_format luma_format = mlt_image_yuv422;

                    // Get image from the luma producer
                    mlt_frame_get_image(luma_frame,
                                        &luma_image,
                                        &luma_format,
                                        &luma_width,
                                        &luma_height,
                                        0);

                    // Generate the luma map
                    if (luma_image) {
                        if (is_clip) {
                            yuv422_to_luma16(luma_image,
                                             &luma_bitmap,
                                             luma_width,
                                             luma_height,
                                             mlt_properties_get_int(MLT_FRAME_PROPERTIES(luma_frame),
                                                                    "full_range"));
                        } else {
                            mlt_luma_map_from_yuv422(luma_image,
                                                     &luma_bitmap,
                                                     luma_width,
                                                     luma_height);
                        }
                    }

                    // Set the transition properties
                    mlt_properties_set_int(properties, "width", luma_width);
                    mlt_properties_set_int(properties, "height", luma_height);
                    mlt_properties_set_data(properties,
                                            "bitmap",
                                            luma_bitmap,
                                            luma_width * luma_height * 2,
                                            mlt_pool_release,
                                            NULL);

                    // Cleanup the luma frame
                    mlt_frame_close(luma_frame);
                }

                if (is_clip) {
                    // Save the producer for getting next frame
                    mlt_properties_set_data(properties,
                                            "producer",
                                            producer,
                                            0,
                                            (mlt_destructor) mlt_producer_close,
                                            NULL);
                } else {
                    // Cleanup the luma producer
                    mlt_properties_clear(properties, "producer");
                    mlt_producer_close(producer);
                    producer = NULL;
                }
            }
        }
    }

    // Arbitrary composite defaults
    float mix = mlt_transition_get_progress(transition, a_frame);
    float frame_delta = mlt_transition_get_progress_delta(transition, a_frame);
    float luma_softness = mlt_properties_get_double(properties, "softness");
    int progressive = mlt_properties_get_int(a_props, "consumer.progressive")
                      || mlt_properties_get_int(properties, "progressive")
                      || mlt_properties_get_int(b_props, "luma.progressive");
    int top_field_first = mlt_properties_get_int(b_props, "top_field_first");
    int reverse = mlt_properties_get_int(properties, "reverse");
    int invert = mlt_properties_get_int(properties, "invert");
    int threads = CLAMP(mlt_properties_get_int(properties, "threads"), 0, mlt_slices_count_normal());
    int alpha_over = mlt_properties_get_int(properties, "alpha_over");

    // Honour the reverse here
    if (mix >= 1.0)
        mix -= floor(mix);

    if (mlt_properties_get(properties, "fixed"))
        mix = mlt_properties_get_double(properties, "fixed");

    if (producer) {
        invert = !invert;
        mix = 0.5f;
    } else {
        mlt_service_unlock(MLT_TRANSITION_SERVICE(transition));
    }

    int fix_background_alpha = mlt_properties_get_int(properties, "fix_background_alpha");
    if (luma_width > 0 && luma_height > 0 && luma_bitmap != NULL) {
        reverse = invert ? !reverse : reverse;
        mix = reverse ? 1 - mix : mix;
        frame_delta *= reverse ? -1.0 : 1.0;
        // Composite the frames using a luma map
        luma_composite(!invert ? a_frame : b_frame,
                       !invert ? b_frame : a_frame,
                       luma_width,
                       luma_height,
                       luma_bitmap,
                       mix,
                       frame_delta,
                       luma_softness,
                       progressive ? -1 : top_field_first,
                       width,
                       height,
                       invert,
                       fix_background_alpha);
    } else {
        mix = (reverse || invert) ? 1 - mix : mix;
        invert = 0;
        // Dissolve the frames using the time offset for mix value
        dissolve_yuv(a_frame,
                     b_frame,
                     mix,
                     *width,
                     *height,
                     threads,
                     alpha_over,
                     fix_background_alpha);
    }
    if (producer) {
        mlt_service_unlock(MLT_TRANSITION_SERVICE(transition));
    }

    // Extract the a_frame image info
    *width = mlt_properties_get_int(!invert ? a_props : b_props, "width");
    *height = mlt_properties_get_int(!invert ? a_props : b_props, "height");
    *image = mlt_properties_get_data(!invert ? a_props : b_props, "image", NULL);

    return 0;
}

/** Luma transition processing.
*/

static mlt_frame transition_process(mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame)
{
    // Push the transition on to the frame
    mlt_frame_push_service(a_frame, transition);

    // Push the b_frame on to the stack
    mlt_frame_push_frame(a_frame, b_frame);

    // Push the transition method
    mlt_frame_push_get_image(a_frame, transition_get_image);

    return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_luma_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *lumafile)
{
    mlt_transition transition = mlt_transition_new();
    if (transition != NULL) {
        // Set the methods
        transition->process = transition_process;

        // Default factory
        mlt_properties_set(MLT_TRANSITION_PROPERTIES(transition),
                           "factory",
                           mlt_environment("MLT_PRODUCER"));

        // Set the main property
        mlt_properties_set(MLT_TRANSITION_PROPERTIES(transition), "resource", lumafile);

        // Inform apps and framework that this is a video only transition
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "_transition_type", 1);

        return transition;
    }
    return NULL;
}
