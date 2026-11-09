/*
 * convert.h -- libplacebo image format converter
 * Copyright (C) 2026 D-Ogi
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

#ifndef CONVERT_H
#define CONVERT_H

#include <framework/mlt_frame.h>

typedef struct
{
    uint8_t *image;
    mlt_image_format format;
    mlt_frame frame;
    char property[64];
} placebo_image_source;

/* Retain source identity across automatic upload. Release after validation.
   The captured pointer is for identity comparisons only, not dereferencing. */
int placebo_frame_get_gpu_image_source(mlt_frame frame,
                                       uint8_t **image,
                                       mlt_image_format *format,
                                       int *width,
                                       int *height,
                                       int writable,
                                       placebo_image_source *source);
void placebo_image_source_release(placebo_image_source *source);

int placebo_convert_image(mlt_frame frame,
                          uint8_t **image,
                          mlt_image_format *format,
                          mlt_image_format output_format);

#endif // CONVERT_H
