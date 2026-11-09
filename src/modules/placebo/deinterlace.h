/*
 * deinterlace.h -- shared libplacebo deinterlacing for filters and links
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

#ifndef PLACEBO_DEINTERLACE_H
#define PLACEBO_DEINTERLACE_H

#include <framework/mlt_frame.h>
#include <framework/mlt_service.h>

/** Adjacent source frames retained by the link on the current frame. */
typedef struct
{
    mlt_frame previous;
    mlt_frame next;
} placebo_deinterlace_frames;

int placebo_deinterlace_setup(mlt_service service);

/** Deinterlace \p frame according to its consumer properties.
 *
 * \param default_method bob for the filter, yadif for the temporal link
 * \param references provides temporal references, or NULL if the service
 * has none
 */
int placebo_deinterlace_get_image(mlt_service service,
                                  mlt_frame frame,
                                  mlt_deinterlacer default_method,
                                  const placebo_deinterlace_frames *references,
                                  uint8_t **image,
                                  mlt_image_format *format,
                                  int *width,
                                  int *height,
                                  int writable);

#endif
