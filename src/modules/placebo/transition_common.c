/*
 * transition_common.c -- shared libplacebo transition helpers
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

#include "transition_common.h"

#include "color_space.h"
#include "gpu_context.h"
#include "render_params.h"
#include "tex_pool.h"
#include "vulkan_frame.h"

#include <libplacebo/config.h>

const float placebo_transition_uv[4][2] = {
    {0.f, 0.f},
    {1.f, 0.f},
    {0.f, 1.f},
    {1.f, 1.f},
};

struct pl_frame placebo_transition_texture_frame(pl_tex texture, const struct pl_color_space *color)
{
    struct pl_color_repr repr = pl_color_repr_rgb;
    repr.alpha = PL_ALPHA_INDEPENDENT;
    return (struct pl_frame){
        .num_planes = 1,
        .planes = {{
            .texture = texture,
            .components = 4,
            .component_mapping = {0, 1, 2, 3},
        }},
        .repr = repr,
        .color = *color,
    };
}

int placebo_transition_render(enum placebo_renderer_site site,
                              const struct pl_frame *source,
                              const struct pl_frame *target)
{
    struct pl_render_params params = pl_render_fast_params;
    params.dither_params = NULL;
    params.deband_params = NULL;
    /* Preserve partial alpha instead of compositing onto the background. */
    placebo_render_params_preserve_alpha(&params);
#if PL_API_VER >= 346
    params.border = PL_CLEAR_SKIP;
#endif
    return placebo_render(site, source, target, &params) ? 0 : 1;
};

/**
 * Check whether a texture already matches the requested normalized layout.
 *
 * \param tex the candidate texture
 * \param w required width
 * \param h required height
 * \param fmt required texture format
 * \param have current texture color space
 * \param want required texture color space
 * \return nonzero if the texture can be reused without conversion
 */
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

int placebo_transition_normalize(pl_gpu gpu,
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
    struct pl_color_space src_color = *dst_color;
    pl_tex temp = NULL;
    int mapped_vulkan = 0;
    int error;

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
        src = placebo_transition_texture_frame(tex, &src_color);
    } else if (!placebo_map_vulkan_image(gpu, image, &mapped)) {
        return 1;
    } else {
        mapped_vulkan = 1;
    }

    temp = placebo_tex_acquire_work(gpu, out_w, out_h, dst_fmt, PLACEBO_TEX_STORABLE);
    if (!temp) {
        if (mapped_vulkan)
            placebo_unmap_vulkan_image(gpu, &mapped);
        return 1;
    }

    dst = placebo_transition_texture_frame(temp, dst_color);
    error = placebo_transition_render(PLACEBO_SITE_TRANSITION, mapped_vulkan ? &mapped : &src, &dst);
    if (mapped_vulkan)
        placebo_unmap_vulkan_image(gpu, &mapped);
    if (error) {
        placebo_tex_recycle(gpu, &temp);
        return 1;
    }
    *out_tex = temp;
    *owned = 1;
    return 0;
}
