 /*
 * Copyright (C) 2001 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Deinterlace routines by Miguel Freitas
 * based of DScaler project sources (deinterlace.sourceforge.net)
 *
 * Currently only available for Xv driver and MMX extensions
 *
 */

#ifndef __DEINTERLACE_H__
#define __DEINTERLACE_H__

//#include "video_out.h"
#include <stdint.h>

int deinterlace_yuv_supported ( int method );
void deinterlace_yuv( uint8_t *pdst, uint8_t *psrc[],
    int width, int height, int method );

#define DEINTERLACE_NONE        0
#define DEINTERLACE_BOB         1
#define DEINTERLACE_WEAVE       2
#define DEINTERLACE_GREEDY      3
#define DEINTERLACE_ONEFIELD    4
#define DEINTERLACE_ONEFIELDXV  5
#define DEINTERLACE_LINEARBLEND 6
#define DEINTERLACE_YADIF       7
#define DEINTERLACE_YADIF_NOSPATIAL 8
		
extern const char *deinterlace_methods[];

#endif
