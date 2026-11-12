/*
 * transition_common.h -- shared libplacebo transition helpers
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

#ifndef PLACEBO_TRANSITION_COMMON_H
#define PLACEBO_TRANSITION_COMMON_H

#include <framework/mlt_frame.h>
#include <libplacebo/renderer.h>

#include "render_cache.h"

/**
 * UV coordinates for the four vertices of a full-frame quad.
 */
extern const float placebo_transition_uv[4][2];

/**
 * Create a single-plane RGBA frame view of a texture.
 *
 * The returned frame references \p texture and does not take ownership of it.
 *
 * \param texture the sampleable RGBA texture
 * \param color the color space associated with the texture
 * \return a libplacebo frame describing the texture
 */
struct pl_frame placebo_transition_texture_frame(pl_tex texture, const struct pl_color_space *color);

/**
 * Render a source frame into a target frame without compositing partial alpha.
 *
 * \pre The caller holds placebo_render_lock().
 * \param site which kind of call this is, to select a renderer
 * \param source the input frame
 * \param target the output frame
 * \return 0 on success; nonzero if rendering fails
 */
int placebo_transition_render(enum placebo_renderer_site site,
                              const struct pl_frame *source,
                              const struct pl_frame *target);

/**
 * Normalize a placebo or Vulkan image to the requested dimensions, format, and
 * color space.
 *
 * A matching placebo texture may be returned directly; otherwise, a new
 * texture is allocated and rendered into. The caller must hold
 * placebo_render_lock() and give the returned texture back with
 * placebo_tex_recycle() when \p owned is set.
 *
 * \pre The caller holds placebo_render_lock().
 * \param gpu the libplacebo GPU
 * \param frame the MLT frame that owns \p image
 * \param image the image payload returned for \p frame
 * \param format the format of \p image
 * \param requested the requested output format
 * \param out_w desired texture width
 * \param out_h desired texture height
 * \param dst_color desired texture color space
 * \param dst_fmt desired texture format
 * \param[out] out_tex receives the normalized texture on success
 * \param[out] owned receives nonzero if the caller owns the returned texture
 * \return 0 on success; nonzero if the input is unsupported or normalization fails
 */
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
                                 int *owned);

#endif
