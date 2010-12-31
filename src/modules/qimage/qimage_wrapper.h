/*
 * qimage_wrapper.h -- a QT/QImage based producer for MLT
 * Copyright (C) 2006 Visual Media
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * NB: This module is designed to be functionally equivalent to the 
 * gtk2 image loading module so it can be used as replacement.
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

#ifndef MLT_QIMAGE_WRAPPER
#define MLT_QIMAGE_WRAPPER

#include <framework/mlt.h>

#include "config.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct producer_qimage_s
{
	struct mlt_producer_s parent;
	mlt_properties filenames;
	int count;
	int image_idx;
	int qimage_idx;
	uint8_t *current_image;
	int has_alpha;
	int current_width;
	int current_height;
	mlt_cache_item image_cache;
	pthread_mutex_t mutex;
};

typedef struct producer_qimage_s *producer_qimage;

extern void refresh_qimage( producer_qimage, mlt_frame, int width, int height );
extern void make_tempfile( producer_qimage, const char *xml );

#ifdef USE_KDE
extern void init_qimage();
#endif

#ifdef __cplusplus
}
#endif

#endif
