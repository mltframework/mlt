/**
 * \file mlt_image.h
 * \brief Image class
 * \see mlt_image_s
 *
 * Copyright (C) 2022 Meltytech, LLC
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

#ifndef MLT_IMAGE_H
#define MLT_IMAGE_H

#include "mlt_types.h"

/** \brief Image class
 *
 * Image is the data object that represents image for a period of time.
 */
#define MLT_IMAGE_MAX_PLANES 4

struct mlt_image_s
{
    mlt_image_format format;
    int width;
    int height;
    int colorspace;
    uint8_t *planes[MLT_IMAGE_MAX_PLANES];
    int strides[MLT_IMAGE_MAX_PLANES];
    void *data;
    mlt_destructor release_data;
    void *alpha;
    mlt_destructor release_alpha;
    mlt_destructor close;
};

extern mlt_image mlt_image_new();
extern void mlt_image_close(mlt_image self);
extern void mlt_image_set_values(
    mlt_image self, void *data, mlt_image_format format, int width, int height);
extern void mlt_image_get_values(
    mlt_image self, void **data, mlt_image_format *format, int *width, int *height);
extern void mlt_image_alloc_data(mlt_image self);
extern void mlt_image_alloc_alpha(mlt_image self);
extern int mlt_image_calculate_size(mlt_image self);
extern void mlt_image_fill_black(mlt_image self);
extern void mlt_image_fill_checkerboard(mlt_image self, double sample_aspect_ratio);
extern void mlt_image_fill_white(mlt_image self, int full_range);
extern void mlt_image_fill_opaque(mlt_image self);
extern const char *mlt_image_format_name(mlt_image_format format);
extern mlt_image_format mlt_image_format_id(const char *name);
extern int mlt_image_rgba_opaque(uint8_t *image, int width, int height);

// Deprecated functions
extern int mlt_image_format_size(mlt_image_format format, int width, int height, int *bpp);
extern void mlt_image_format_planes(
    mlt_image_format format, int width, int height, void *data, uint8_t *planes[4], int strides[4]);

#endif
