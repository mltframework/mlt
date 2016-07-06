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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MLT_QIMAGE_WRAPPER
#define MLT_QIMAGE_WRAPPER

#include <framework/mlt.h>

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
	uint8_t *current_alpha;
	int current_width;
	int current_height;
	mlt_cache_item image_cache;
	mlt_cache_item alpha_cache;
	mlt_cache_item qimage_cache;
	void *qimage;
	mlt_image_format format;
};

typedef struct producer_qimage_s *producer_qimage;

extern int refresh_qimage( producer_qimage self, mlt_frame frame );
extern void refresh_image( producer_qimage, mlt_frame, mlt_image_format, int width, int height );
extern void make_tempfile( producer_qimage, const char *xml );
extern int init_qimage(const char *filename);


#ifdef __cplusplus
}
#endif

#endif
