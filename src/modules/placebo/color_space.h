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

/* The transfer the consumer will encode the frame in, from consumer.color_trc.
   PL_COLOR_TRC_UNKNOWN when the consumer does not say. */
enum pl_color_transfer placebo_frame_output_transfer(mlt_frame frame);

/* Destination for converting src to another transfer, with the same primaries.
   Linear light has no peak or black level of its own, so libplacebo infers SDR
   white for it. That is right when the frame ends up SDR: the HDR source is
   tone mapped on the way to linear. It is wrong when the frame ends up HDR,
   where it would tone map the highlights away before the way back. So linear
   takes the levels of the HDR output transfer instead, and nothing is tone
   mapped until the output is reached. An output that is not HDR, or unknown,
   leaves the levels to libplacebo's inference. */
struct pl_color_space placebo_color_space_retarget(const struct pl_color_space *src,
                                                   enum pl_color_transfer transfer,
                                                   enum pl_color_transfer output);

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

/* How a finished texture is read back for display. Linear frames use
   rgba16hf. Every other frame is rgba8 and tagged sRGB, so the present pass
   does not apply a second transfer. The bytes themselves stay in the transfer
   placebo_output_format wrote. *fmt is NULL only when rgba8 itself is missing. */
void placebo_tex_encoding(pl_gpu gpu, mlt_frame frame, struct pl_color_space *color, pl_fmt *fmt);

/* Output texture for one filter. The format comes from placebo_tex_encoding.
   A linear frame stays linear. Any other known transfer is kept (bt709 is
   BT.1886); an unknown curve stays sRGB. An rgba64 request uses rgba16 unorm
   instead of rgba8 or half-float, so the download is 16-bit. *fmt is NULL
   only when rgba8 is missing. */
void placebo_output_format(pl_gpu gpu,
                           mlt_frame frame,
                           mlt_image_format requested,
                           struct pl_color_space *color,
                           pl_fmt *fmt);

#endif /* COLOR_SPACE_H */
