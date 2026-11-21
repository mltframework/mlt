/*
 * render_params.h -- libplacebo render parameters shared by the filters
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

#ifndef RENDER_PARAMS_H
#define RENDER_PARAMS_H

#include <framework/mlt_frame.h>

#include <libplacebo/renderer.h>

/* libplacebo defaults, with cheap scalers when the consumer's rescale is unset,
   "bilinear" or "nearest". The upscaler also upsamples chroma, so this matters
   on every render of a 4K YUV frame. Any other rescale, including "none",
   keeps the quality scalers. */
struct pl_render_params placebo_render_params(mlt_frame frame);

/* Draw onto the target as it is instead of clearing it or compositing the
   picture's alpha onto a background first. */
void placebo_render_params_skip_background(struct pl_render_params *params);

/* Skip the background and keep the picture's partial alpha. */
void placebo_render_params_preserve_alpha(struct pl_render_params *params);

#endif /* RENDER_PARAMS_H */
