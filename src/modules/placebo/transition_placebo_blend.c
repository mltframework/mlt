/*
 * transition_placebo_blend.c -- Porter-Duff and SVG blend via libplacebo
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
#include "vulkan_frame.h"

#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_transition.h>

#include <libplacebo/dispatch.h>
#include <libplacebo/gpu.h>
#include <libplacebo/renderer.h>
#include <libplacebo/shaders.h>

#include <string.h>

/* Premultiplied Porter-Duff and SVG 1.2 blend modes, numbered the same as
   Qt's QPainter::CompositionMode and movit.overlay compositing. Stored
   samples stay straight. vf_overlay treats an unspecified alpha_mode as
   straight, and overlay_vulkan multiplies the overlay color by its alpha.
   The shader premultiplies, blends, then writes straight alpha. */
static const char *k_blend_header
    = "vec4 premultiply(vec4 c) {                             \n"
      "    return vec4(c.rgb * c.a, c.a);                     \n"
      "}                                                      \n"
      "vec4 unpremultiply(vec4 c) {                           \n"
      "    float inv = c.a > 1e-4 ? 1.0 / c.a : 0.0;          \n"
      "    return vec4(c.rgb * inv, c.a);                     \n"
      "}                                                      \n"
      "float coverage(float da, float sa) {                   \n"
      "    return sa + da - sa * da;                          \n"
      "}                                                      \n"
      "vec4 blend_clear(vec4 d, vec4 s) {                     \n"
      "    return vec4(0.0);                                  \n"
      "}                                                      \n"
      "vec4 blend_source(vec4 d, vec4 s) { return s; }        \n"
      "vec4 blend_destination(vec4 d, vec4 s) { return d; }   \n"
      "vec4 blend_source_over(vec4 d, vec4 s) {               \n"
      "    return s + d * (1.0 - s.a);                        \n"
      "}                                                      \n"
      "vec4 blend_destination_over(vec4 d, vec4 s) {          \n"
      "    return d + s * (1.0 - d.a);                        \n"
      "}                                                      \n"
      "vec4 blend_source_in(vec4 d, vec4 s) {                 \n"
      "    return vec4(s.rgb * d.a, s.a * d.a);               \n"
      "}                                                      \n"
      "vec4 blend_destination_in(vec4 d, vec4 s) {            \n"
      "    return vec4(d.rgb * s.a, d.a * s.a);               \n"
      "}                                                      \n"
      "vec4 blend_source_out(vec4 d, vec4 s) {                \n"
      "    return vec4(s.rgb * (1.0 - d.a), s.a * (1.0 - d.a));\n"
      "}                                                      \n"
      "vec4 blend_destination_out(vec4 d, vec4 s) {           \n"
      "    return vec4(d.rgb * (1.0 - s.a), d.a * (1.0 - s.a));\n"
      "}                                                      \n"
      "vec4 blend_source_atop(vec4 d, vec4 s) {               \n"
      "    return vec4(s.rgb * d.a + d.rgb * (1.0 - s.a), d.a);\n"
      "}                                                      \n"
      "vec4 blend_destination_atop(vec4 d, vec4 s) {          \n"
      "    return vec4(d.rgb * s.a + s.rgb * (1.0 - d.a), s.a);\n"
      "}                                                      \n"
      "vec4 blend_xor(vec4 d, vec4 s) {                       \n"
      "    return s * (1.0 - d.a) + d * (1.0 - s.a);          \n"
      "}                                                      \n"
      "vec4 blend_plus(vec4 d, vec4 s) {                      \n"
      "    return min(d + s, vec4(1.0));                      \n"
      "}                                                      \n"
      "vec4 blend_multiply(vec4 d, vec4 s) {                  \n"
      "    return vec4(s.rgb * d.rgb + s.rgb * (1.0 - d.a)    \n"
      "                + d.rgb * (1.0 - s.a),                 \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "vec4 blend_screen(vec4 d, vec4 s) {                    \n"
      "    return vec4(s.rgb + d.rgb - s.rgb * d.rgb,         \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "vec4 blend_overlay(vec4 d, vec4 s) {                   \n"
      "    vec3 t = s.rgb * (1.0 - d.a) + d.rgb * (1.0 - s.a);\n"
      "    vec3 r;                                            \n"
      "    r.r = 2.0 * d.r < d.a                              \n"
      "        ? 2.0 * s.r * d.r + t.r                        \n"
      "        : s.a * d.a - 2.0 * (d.a - d.r) * (s.a - s.r) + t.r;\n"
      "    r.g = 2.0 * d.g < d.a                              \n"
      "        ? 2.0 * s.g * d.g + t.g                        \n"
      "        : s.a * d.a - 2.0 * (d.a - d.g) * (s.a - s.g) + t.g;\n"
      "    r.b = 2.0 * d.b < d.a                              \n"
      "        ? 2.0 * s.b * d.b + t.b                        \n"
      "        : s.a * d.a - 2.0 * (d.a - d.b) * (s.a - s.b) + t.b;\n"
      "    return vec4(r, coverage(d.a, s.a));                \n"
      "}                                                      \n"
      "vec4 blend_darken(vec4 d, vec4 s) {                    \n"
      "    return vec4(min(s.rgb * d.a, d.rgb * s.a)          \n"
      "                + s.rgb * (1.0 - d.a)                  \n"
      "                + d.rgb * (1.0 - s.a),                 \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "vec4 blend_lighten(vec4 d, vec4 s) {                   \n"
      "    return vec4(max(s.rgb * d.a, d.rgb * s.a)          \n"
      "                + s.rgb * (1.0 - d.a)                  \n"
      "                + d.rgb * (1.0 - s.a),                 \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "float dodge_channel(float dst, float src, float da, float sa) {\n"
      "    float under = src * (1.0 - da) + dst * (1.0 - sa);\n"
      "    if (src * da + dst * sa >= sa * da)                \n"
      "        return sa * da + under;                        \n"
      "    if (src >= sa - 0.001)                             \n"
      "        return dst * sa + under;                       \n"
      "    return dst * sa / max(1.0 - src / max(sa, 0.001), 0.001) + under;\n"
      "}                                                      \n"
      "vec4 blend_color_dodge(vec4 d, vec4 s) {               \n"
      "    return vec4(dodge_channel(d.r, s.r, d.a, s.a),     \n"
      "                dodge_channel(d.g, s.g, d.a, s.a),     \n"
      "                dodge_channel(d.b, s.b, d.a, s.a),     \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "float burn_channel(float dst, float src, float da, float sa) {\n"
      "    float under = src * (1.0 - da) + dst * (1.0 - sa);\n"
      "    if (src * da + dst * sa <= sa * da)                \n"
      "        return under;                                  \n"
      "    if (src <= 0.001)                                  \n"
      "        return dst * sa + under;                       \n"
      "    return sa * da - (da - dst) * sa / max(src, 0.001) + under;\n"
      "}                                                      \n"
      "vec4 blend_color_burn(vec4 d, vec4 s) {                \n"
      "    return vec4(burn_channel(d.r, s.r, d.a, s.a),      \n"
      "                burn_channel(d.g, s.g, d.a, s.a),      \n"
      "                burn_channel(d.b, s.b, d.a, s.a),      \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "vec4 blend_hard_light(vec4 d, vec4 s) {                \n"
      "    vec3 t = s.rgb * (1.0 - d.a) + d.rgb * (1.0 - s.a);\n"
      "    vec3 r;                                            \n"
      "    r.r = 2.0 * s.r < s.a                              \n"
      "        ? 2.0 * s.r * d.r + t.r                        \n"
      "        : s.a * d.a - 2.0 * (d.a - d.r) * (s.a - s.r) + t.r;\n"
      "    r.g = 2.0 * s.g < s.a                              \n"
      "        ? 2.0 * s.g * d.g + t.g                        \n"
      "        : s.a * d.a - 2.0 * (d.a - d.g) * (s.a - s.g) + t.g;\n"
      "    r.b = 2.0 * s.b < s.a                              \n"
      "        ? 2.0 * s.b * d.b + t.b                        \n"
      "        : s.a * d.a - 2.0 * (d.a - d.b) * (s.a - s.b) + t.b;\n"
      "    return vec4(r, coverage(d.a, s.a));                \n"
      "}                                                      \n"
      "float soft_channel(float dst, float src, float da, float sa) {\n"
      "    float dst_np = da > 0.001 ? dst / da : 0.0;        \n"
      "    float temp = src * (1.0 - da) + dst * (1.0 - sa);  \n"
      "    if (2.0 * src < sa)                                \n"
      "        return dst * (sa + (2.0 * src - sa) * (1.0 - dst_np)) + temp;\n"
      "    if (4.0 * dst <= da) {                             \n"
      "        float dst_sq = dst_np * dst_np;                \n"
      "        float dst_cb = dst_sq * dst_np;                \n"
      "        return dst * sa + da * (2.0 * src - sa)        \n"
      "               * (16.0 * dst_cb - 12.0 * dst_sq + 3.0 * dst_np) + temp;\n"
      "    }                                                  \n"
      "    return dst * sa + da * (2.0 * src - sa) * (sqrt(dst_np) - dst_np) + temp;\n"
      "}                                                      \n"
      "vec4 blend_soft_light(vec4 d, vec4 s) {                \n"
      "    return vec4(soft_channel(d.r, s.r, d.a, s.a),      \n"
      "                soft_channel(d.g, s.g, d.a, s.a),      \n"
      "                soft_channel(d.b, s.b, d.a, s.a),      \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "vec4 blend_difference(vec4 d, vec4 s) {                \n"
      "    return vec4(s.rgb + d.rgb - 2.0 * min(s.rgb * d.a, d.rgb * s.a),\n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n"
      "vec4 blend_exclusion(vec4 d, vec4 s) {                 \n"
      "    return vec4(s.rgb + d.rgb - 2.0 * s.rgb * d.rgb,   \n"
      "                coverage(d.a, s.a));                   \n"
      "}                                                      \n";

static const char *k_blend_body = "vec4 dst = premultiply(texture(bottom_tex, uv));      \n"
                                  "vec4 src = premultiply(texture(top_tex, uv));         \n"
                                  "vec4 blended;                                         \n"
                                  "switch (mode) {                                       \n"
                                  "case 1: blended = blend_destination_over(dst, src); break;\n"
                                  "case 2: blended = blend_clear(dst, src); break;       \n"
                                  "case 3: blended = blend_source(dst, src); break;      \n"
                                  "case 4: blended = blend_destination(dst, src); break; \n"
                                  "case 5: blended = blend_source_in(dst, src); break;   \n"
                                  "case 6: blended = blend_destination_in(dst, src); break;\n"
                                  "case 7: blended = blend_source_out(dst, src); break;  \n"
                                  "case 8: blended = blend_destination_out(dst, src); break;\n"
                                  "case 9: blended = blend_source_atop(dst, src); break; \n"
                                  "case 10: blended = blend_destination_atop(dst, src); break;\n"
                                  "case 11: blended = blend_xor(dst, src); break;        \n"
                                  "case 12: blended = blend_plus(dst, src); break;       \n"
                                  "case 13: blended = blend_multiply(dst, src); break;   \n"
                                  "case 14: blended = blend_screen(dst, src); break;     \n"
                                  "case 15: blended = blend_overlay(dst, src); break;    \n"
                                  "case 16: blended = blend_darken(dst, src); break;     \n"
                                  "case 17: blended = blend_lighten(dst, src); break;    \n"
                                  "case 18: blended = blend_color_dodge(dst, src); break;\n"
                                  "case 19: blended = blend_color_burn(dst, src); break; \n"
                                  "case 20: blended = blend_hard_light(dst, src); break; \n"
                                  "case 21: blended = blend_soft_light(dst, src); break; \n"
                                  "case 22: blended = blend_difference(dst, src); break; \n"
                                  "case 23: blended = blend_exclusion(dst, src); break;  \n"
                                  "default: blended = blend_source_over(dst, src); break;\n"
                                  "}                                                     \n"
                                  "color = unpremultiply(blended);                       \n";

static const float k_uv[4][2] = {
    {0.f, 0.f},
    {1.f, 0.f},
    {0.f, 1.f},
    {1.f, 1.f},
};

static struct pl_color_repr straight_rgb(void)
{
    struct pl_color_repr repr = pl_color_repr_rgb;
    repr.alpha = PL_ALPHA_INDEPENDENT;
    return repr;
}

static int same_texture(pl_tex tex,
                        int w,
                        int h,
                        pl_fmt fmt,
                        const struct pl_color_space *have,
                        const struct pl_color_space *want)
{
    return tex && tex->params.sampleable && tex->params.w == w && tex->params.h == h
           && tex->params.format == fmt && tex->params.format->num_components >= 4
           && pl_color_space_equal(have, want);
}

/* Make a sampleable RGBA texture at the output size in dst_color.
   A Vulkan image is rendered so crop and YUV are applied. A placebo texture
   that already matches is used as-is and *owned stays 0. Caller holds
   placebo_render_lock. */
static int normalize(pl_gpu gpu,
                     pl_renderer renderer,
                     mlt_frame frame,
                     const uint8_t *image,
                     mlt_image_format format,
                     mlt_image_format requested,
                     int out_w,
                     int out_h,
                     const struct pl_color_space *dst_color,
                     pl_fmt dst_fmt,
                     pl_tex *out_tex,
                     int *owned)
{
    int is_tex = placebo_frame_is_tex(frame, format);
    int is_vulkan = placebo_frame_is_vulkan(frame, format);
    struct pl_frame mapped = {0};
    struct pl_frame src = {0};
    struct pl_frame dst = {0};
    struct pl_render_params params;
    struct pl_color_space src_color = *dst_color;
    pl_tex temp = NULL;
    int mapped_vulkan = 0;
    int ok;

    *out_tex = NULL;
    *owned = 0;
    if (!is_tex && !is_vulkan)
        return 1;

    if (is_tex) {
        pl_fmt ignored = NULL;
        pl_tex tex = placebo_image_get_tex(image);
        if (!tex)
            return 1;
        placebo_output_format(gpu, frame, requested, &src_color, &ignored);
        if (same_texture(tex, out_w, out_h, dst_fmt, &src_color, dst_color)) {
            *out_tex = tex;
            return 0;
        }
        src = (struct pl_frame){
            .num_planes = 1,
            .planes = {{
                .texture = tex,
                .components = 4,
                .component_mapping = {0, 1, 2, 3},
            }},
            .repr = straight_rgb(),
            .color = src_color,
        };
    } else if (!placebo_map_vulkan_image(gpu, image, &mapped)) {
        return 1;
    } else {
        mapped_vulkan = 1;
    }

    temp = pl_tex_create(gpu,
                         pl_tex_params(.w = out_w,
                                       .h = out_h,
                                       .format = dst_fmt,
                                       .sampleable = true,
                                       .renderable = true,
                                       .storable = true, ));
    if (!temp) {
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        return 1;
    }

    dst = (struct pl_frame){
        .num_planes = 1,
        .planes = {{
            .texture = temp,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = straight_rgb(),
        .color = *dst_color,
    };
    params = pl_render_fast_params;
    params.dither_params = NULL;
    params.deband_params = NULL;
    /* Default background compositing flattens partial alpha onto black. */
    params.background = PL_CLEAR_SKIP;
    params.border = PL_CLEAR_SKIP;
    params.background_transparency = 1.f;
    ok = pl_render_image(renderer, mapped_vulkan ? &mapped : &src, &dst, &params);
    if (mapped_vulkan)
        placebo_unmap_vulkan_image(gpu, &mapped);
    if (!ok) {
        pl_tex_destroy(gpu, &temp);
        return 1;
    }
    *out_tex = temp;
    *owned = 1;
    return 0;
}

/* placebo.blend_mode writes placebo.blend.mode on the upper frame, and that
   wins. Producer properties are not copied onto the frame, so a value set
   on B's producer is checked too. */
static int blend_mode(mlt_transition transition, mlt_frame b_frame)
{
    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);
    mlt_properties b_props = MLT_FRAME_PROPERTIES(b_frame);
    mlt_producer producer = mlt_frame_get_original_producer(b_frame);
    mlt_producer source = producer ? mlt_producer_cut_parent(producer) : NULL;
    int mode = mlt_properties_get_int(properties, "mode");

    if (mlt_properties_exists(b_props, "placebo.blend.mode"))
        mode = mlt_properties_get_int(b_props, "placebo.blend.mode");
    else if (producer
             && mlt_properties_exists(MLT_PRODUCER_PROPERTIES(producer), "placebo.blend.mode"))
        mode = mlt_properties_get_int(MLT_PRODUCER_PROPERTIES(producer), "placebo.blend.mode");
    else if (source && source != producer
             && mlt_properties_exists(MLT_PRODUCER_PROPERTIES(source), "placebo.blend.mode"))
        mode = mlt_properties_get_int(MLT_PRODUCER_PROPERTIES(source), "placebo.blend.mode");
    if (mode < 0 || mode > 23)
        mode = 0;
    return mode;
}

static int blend(pl_gpu gpu, pl_dispatch dispatch, pl_tex bottom, pl_tex top, pl_tex target, int mode)
{
    pl_fmt uv_fmt = pl_find_vertex_fmt(gpu, PL_FMT_FLOAT, 2);
    pl_shader shader;
    struct pl_shader_desc descs[2] = {0};
    struct pl_shader_va uv = {0};
    struct pl_shader_var mode_var = {0};
    struct pl_custom_shader custom = {0};
    int ok;

    if (!uv_fmt || !dispatch)
        return 1;

    descs[0].desc.name = "bottom_tex";
    descs[0].desc.type = PL_DESC_SAMPLED_TEX;
    descs[0].binding.object = bottom;
    descs[0].binding.address_mode = PL_TEX_ADDRESS_CLAMP;
    descs[0].binding.sample_mode = PL_TEX_SAMPLE_NEAREST;
    descs[1].desc.name = "top_tex";
    descs[1].desc.type = PL_DESC_SAMPLED_TEX;
    descs[1].binding.object = top;
    descs[1].binding.address_mode = PL_TEX_ADDRESS_CLAMP;
    descs[1].binding.sample_mode = PL_TEX_SAMPLE_NEAREST;

    uv.attr.name = "uv";
    uv.attr.fmt = uv_fmt;
    uv.data[0] = k_uv[0];
    uv.data[1] = k_uv[1];
    uv.data[2] = k_uv[2];
    uv.data[3] = k_uv[3];

    mode_var.var = pl_var_int("mode");
    mode_var.data = &mode;
    mode_var.dynamic = true;

    custom.header = k_blend_header;
    custom.body = k_blend_body;
    custom.input = PL_SHADER_SIG_NONE;
    custom.output = PL_SHADER_SIG_COLOR;
    custom.description = "placebo.blend";
    custom.descriptors = descs;
    custom.num_descriptors = 2;
    custom.variables = &mode_var;
    custom.num_variables = 1;
    custom.vertex_attribs = &uv;
    custom.num_vertex_attribs = 1;

    shader = pl_dispatch_begin(dispatch);
    if (!pl_shader_custom(shader, &custom)) {
        pl_dispatch_abort(dispatch, &shader);
        return 1;
    }
    ok = pl_dispatch_finish(dispatch, pl_dispatch_params(.shader = &shader, .target = target));
    return ok ? 0 : 1;
}

static int get_image(mlt_frame a_frame,
                     uint8_t **image,
                     mlt_image_format *format,
                     int *width,
                     int *height,
                     int writable)
{
    mlt_frame b_frame = mlt_frame_pop_frame(a_frame);
    mlt_transition transition = mlt_frame_pop_service(a_frame);
    mlt_service service = MLT_TRANSITION_SERVICE(transition);
    pl_gpu gpu = placebo_gpu_get();
    pl_renderer renderer = placebo_renderer_get();
    pl_dispatch dispatch = placebo_dispatch_get();
    mlt_image_format requested;
    mlt_image_format b_format;
    uint8_t *b_image = NULL;
    int out_w;
    int out_h;
    int b_w;
    int b_h;
    int error;
    int b_error;
    int mode;
    int a_owned = 0;
    int b_owned = 0;
    pl_tex a_tex = NULL;
    pl_tex b_tex = NULL;
    pl_tex dst = NULL;
    struct pl_color_space dst_color = pl_color_space_srgb;
    pl_fmt dst_fmt = NULL;
    mlt_color_trc trc;

    if (!gpu || !renderer || !dispatch) {
        mlt_log_warning(service, "GPU not available, passing the lower frame through\n");
        error = mlt_frame_get_image(a_frame, image, format, width, height, writable);
        b_format = *format;
        b_w = *width;
        b_h = *height;
        mlt_frame_get_image(b_frame, &b_image, &b_format, &b_w, &b_h, writable);
        return error;
    }

    requested = *format;
    b_format = *format;
    placebo_frame_set_requested_tex(a_frame, 1);
    placebo_frame_set_requested_tex(b_frame, 1);
    placebo_request_gpu_image(a_frame, format);
    placebo_request_gpu_image(b_frame, &b_format);
    error = mlt_frame_get_image(a_frame, image, format, width, height, writable);
    out_w = *width;
    out_h = *height;
    b_w = out_w;
    b_h = out_h;
    b_error = mlt_frame_get_image(b_frame, &b_image, &b_format, &b_w, &b_h, writable);
    placebo_frame_set_requested_tex(a_frame, 0);
    placebo_frame_set_requested_tex(b_frame, 0);
    if (error || !*image)
        return error ? error : 1;
    if (b_error || !b_image)
        return b_error ? b_error : 1;
    if (out_w < 1 || out_h < 1) {
        mlt_log_error(service, "Invalid size for get_image: %dx%d\n", out_w, out_h);
        return 1;
    }

    mode = blend_mode(transition, b_frame);

    placebo_output_format(gpu, a_frame, requested, &dst_color, &dst_fmt);
    if (!dst_fmt) {
        mlt_log_error(service, "GPU does not support an RGBA output format\n");
        return 1;
    }

    placebo_render_lock();
    if (normalize(gpu,
                  renderer,
                  a_frame,
                  *image,
                  *format,
                  requested,
                  out_w,
                  out_h,
                  &dst_color,
                  dst_fmt,
                  &a_tex,
                  &a_owned)
        || normalize(gpu,
                     renderer,
                     b_frame,
                     b_image,
                     b_format,
                     requested,
                     out_w,
                     out_h,
                     &dst_color,
                     dst_fmt,
                     &b_tex,
                     &b_owned)) {
        mlt_log_error(service, "Failed to normalize blend inputs\n");
        goto fail_locked;
    }

    dst = pl_tex_create(gpu,
                        pl_tex_params(.w = out_w,
                                      .h = out_h,
                                      .format = dst_fmt,
                                      .sampleable = true,
                                      .renderable = true,
                                      .storable = true,
                                      .host_readable = true, ));
    if (!dst || blend(gpu, dispatch, a_tex, b_tex, dst, mode)) {
        mlt_log_error(service, "Blend dispatch failed\n");
        goto fail_locked;
    }
    if (a_owned)
        pl_tex_destroy(gpu, &a_tex);
    if (b_owned)
        pl_tex_destroy(gpu, &b_tex);
    placebo_render_unlock();

    trc = placebo_frame_color_trc(a_frame, *image, *format);
    *width = out_w;
    *height = out_h;
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(a_frame), "width", out_w);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(a_frame), "height", out_h);
    if (placebo_frame_publish_tex(a_frame, dst, requested, image, format, trc))
        return 1;
    return 0;

fail_locked:
    if (a_owned)
        pl_tex_destroy(gpu, &a_tex);
    if (b_owned)
        pl_tex_destroy(gpu, &b_tex);
    if (dst)
        pl_tex_destroy(gpu, &dst);
    placebo_render_unlock();
    return 1;
}

static mlt_frame process(mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame)
{
    mlt_frame_push_service(a_frame, transition);
    mlt_frame_push_frame(a_frame, b_frame);
    mlt_frame_push_get_image(a_frame, get_image);
    mlt_frame_prepend_convert_image(a_frame, placebo_convert_image);
    mlt_frame_prepend_convert_image(b_frame, placebo_convert_image);
    return a_frame;
}

mlt_transition transition_placebo_blend_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg)
{
    mlt_transition transition = mlt_transition_new();

    (void) profile;
    (void) type;
    (void) id;
    if (!transition)
        return NULL;

    transition->process = process;
    mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(transition), "_transition_type", 1);
    mlt_properties_set(MLT_TRANSITION_PROPERTIES(transition), "mode", arg && arg[0] ? arg : "0");
    if (placebo_gpu_get())
        placebo_vulkan_publish();
    return transition;
}
