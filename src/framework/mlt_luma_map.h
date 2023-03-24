/*
 * \file mlt_luma_map.h
 * \brief functions to generate and read luma-wipe transition maps
 *
 * Copyright (C) 2003-2019 Meltytech, LLC
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

#ifndef MLT_LUMA_MAP_H
#define MLT_LUMA_MAP_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mlt_luma_map_s
{
    int type;
    int w;
    int h;
    int bands;
    int rband;
    int vmirror;
    int hmirror;
    int dmirror;
    int invert;
    int offset;
    int flip;
    int flop;
    int pflip;
    int pflop;
    int quart;
    int rotate;
};

typedef struct mlt_luma_map_s *mlt_luma_map;

extern void mlt_luma_map_init(mlt_luma_map self);
extern mlt_luma_map mlt_luma_map_new(const char *path);
extern uint16_t *mlt_luma_map_render(mlt_luma_map self);
extern int mlt_luma_map_from_pgm(const char *filename, uint16_t **map, int *width, int *height);
extern void mlt_luma_map_from_yuv422(uint8_t *image, uint16_t **map, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
