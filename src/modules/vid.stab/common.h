/*
 * common.h
 * Copyright (C) 2013 Jakub Ksiezniak <j.ksiezniak@gmail.com>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef VIDSTAB_COMMON_H_
#define VIDSTAB_COMMON_H_

extern "C" {
#include <vid.stab/libvidstab.h>
#include <framework/mlt.h>
}

inline VSPixelFormat convertImageFormat(mlt_image_format &format) {
	switch (format) {
	case mlt_image_rgb24:
		return PF_RGB24;
	case mlt_image_rgb24a:
		return PF_RGBA;
	case mlt_image_yuv420p:
		return PF_YUV420P;
	default:
		return PF_NONE;
	}
}

#endif /* VIDSTAB_COMMON_H_ */
