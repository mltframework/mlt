/* GdkPixbuf library - Scaling and compositing functions
 *
 * Original:
 * Copyright (C) 2000 Red Hat, Inc
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Modification for MLT:
 * Copyright (C) 2003-2014 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,  51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef PIXOPS_H
#define PIXOPS_H

#include <glib.h>

/* Interpolation modes; must match GdkInterpType */
typedef enum {
	PIXOPS_INTERP_NEAREST,
	PIXOPS_INTERP_TILES,
	PIXOPS_INTERP_BILINEAR,
	PIXOPS_INTERP_HYPER
} PixopsInterpType;

/* Scale src_buf from src_width / src_height by factors scale_x, scale_y
 * and composite the portion corresponding to
 * render_x, render_y, render_width, render_height in the new
 * coordinate system into dest_buf starting at 0, 0
 */
void yuv422_scale     (guchar         *dest_buf,
		       int             render_x0,
		       int             render_y0,
		       int             render_x1,
		       int             render_y1,
		       int             dest_rowstride,
		       int             dest_channels,
		       int             dest_has_alpha,
		       const guchar   *src_buf,
		       int             src_width,
		       int             src_height,
		       int             src_rowstride,
		       int             src_channels,
		       int             src_has_alpha,
		       double          scale_x,
		       double          scale_y,
		       PixopsInterpType   interp_type);

#define yuv422_scale_simple( dest_buf, dest_width, dest_height, dest_rowstride, src_buf, src_width, src_height, src_rowstride, interp_type ) \
	yuv422_scale( (dest_buf), 0, 0, \
		(dest_width), (dest_height), \
		(dest_rowstride), 2, 0, \
		(src_buf), (src_width), (src_height), \
		(src_rowstride), 2, 0, \
		(double) (dest_width) / (src_width), (double) (dest_height) / (src_height), \
		(PixopsInterpType) interp_type );

#endif
