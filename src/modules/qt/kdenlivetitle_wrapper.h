/*
 * kdenlivetitle_wrapper.h -- kdenlivetitle wrapper
 * Copyright (c) 2009 Marco Gittler <g.marco@freenet.de>
 * Copyright (c) 2009 Jean-Baptiste Mardelle <jb@kdenlive.org>
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

#include <framework/mlt.h>

#ifdef __cplusplus
extern "C" {
#endif
  
#include <framework/mlt_producer.h>
#include <framework/mlt_cache.h>
#include <framework/mlt_frame.h>


struct producer_ktitle_s
{
	struct mlt_producer_s parent;
	uint8_t *rgba_image;
	uint8_t *current_image;
	uint8_t *current_alpha;
	mlt_image_format format;
	int current_width;
	int current_height;
	pthread_mutex_t mutex;
};

typedef struct producer_ktitle_s *producer_ktitle;

extern void drawKdenliveTitle( producer_ktitle self, mlt_frame frame, mlt_image_format format, int, int, double, int );


#ifdef __cplusplus
}
#endif



