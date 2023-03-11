/*
 * common.h
 * Copyright (C) 2023 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <framework/mlt_image.h>

mlt_deinterlacer supported_method( mlt_deinterlacer method );
int deinterlace_image( mlt_image dst, mlt_image src, mlt_image prev, mlt_image next, int tff, mlt_deinterlacer method );

#endif
