/*
 * color_space.h -- map MLT color metadata onto libplacebo
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

#ifndef COLOR_SPACE_H
#define COLOR_SPACE_H

#include <framework/mlt_frame.h>

#include <libplacebo/colorspace.h>
#include <libplacebo/gpu.h>

/* Same libplacebo transfer that its FFmpeg mapping uses for this TRC.
   PL_COLOR_TRC_UNKNOWN when libplacebo has no curve for it. */
enum pl_color_transfer placebo_transfer_from_mlt(mlt_color_trc trc);

/* Non-zero when the frame's color_trc property is linear. */
int placebo_frame_is_linear(mlt_frame frame);

/* Color space with the frame's primaries and an explicit transfer. */
struct pl_color_space placebo_color_space_for_trc(mlt_frame frame, enum pl_color_transfer transfer);

/* Same mapping when the caller already has the ids (a Vulkan texture). */
struct pl_color_space placebo_color_space(mlt_color_primaries primaries,
                                          mlt_colorspace colorspace,
                                          int height,
                                          enum pl_color_transfer transfer);

/* Half-float RGBA for linear light, or NULL when the GPU has none. */
pl_fmt placebo_linear_format(pl_gpu gpu);

/* 8-bit RGBA, or NULL. */
pl_fmt placebo_rgba8_format(pl_gpu gpu);

/* 16-bit unorm RGBA, the GPU layout of mlt_image_rgba64. NULL when absent. */
pl_fmt placebo_rgba16_format(pl_gpu gpu);

/* Non-zero when tex was created with this exact pl_fmt. */
int placebo_tex_is(pl_tex tex, pl_fmt fmt);

/* Encoding to keep a placebo texture's current transfer.
   Linear frames use rgba16hf. Every other frame stays rgba8 sRGB, which is
   what the scale, shader, and render filters wrote before linear light.
   *fmt is NULL only when rgba8 itself is missing. */
void placebo_tex_encoding(pl_gpu gpu, mlt_frame frame, struct pl_color_space *color, pl_fmt *fmt);

/* Output texture for one filter. The transfer comes from placebo_tex_encoding.
   An rgba64 request uses rgba16 unorm instead of rgba8 or half-float, so the
   download is 16-bit. *fmt is NULL only when rgba8 is missing. */
void placebo_output_format(pl_gpu gpu,
                           mlt_frame frame,
                           mlt_image_format requested,
                           struct pl_color_space *color,
                           pl_fmt *fmt);

#endif /* COLOR_SPACE_H */
