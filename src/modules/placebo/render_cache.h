/*
 * render_cache.h -- keep one libplacebo renderer per kind of render call
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

#ifndef RENDER_CACHE_H
#define RENDER_CACHE_H

#include <libplacebo/renderer.h>

/* Which kind of call a render is for. */
enum placebo_renderer_site {
    PLACEBO_SITE_SCALE = 1,
    PLACEBO_SITE_RENDER,
    PLACEBO_SITE_COLOR_TRANSFORM,
    PLACEBO_SITE_CONVERT,
    PLACEBO_SITE_SHADER,
    PLACEBO_SITE_TRANSITION,
    PLACEBO_SITE_DEINTERLACE,
};

/* pl_render_image on a renderer reserved for calls like this one. Needs the
   render lock. A pl_renderer sizes its intermediate textures for its last call
   and reallocates them whenever the size or format changes. One renderer shared
   by filters working at different sizes therefore reallocates in every call, so
   renderers are kept per site and per source and target geometry. Returns
   non-zero on success. */
int placebo_render(enum placebo_renderer_site site,
                   const struct pl_frame *src,
                   const struct pl_frame *dst,
                   const struct pl_render_params *params);

/* Destroy every cached renderer. Needs the render lock, and has to run before
   the GPU is released. */
void placebo_render_cache_drain(void);

#endif /* RENDER_CACHE_H */
