/*
 * kino_wrapper.h -- c wrapper for kino file handler
 * Copyright (C) 2005-2014 Meltytech, LLC
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

#ifndef MLT_PRODUCER_KINO_WRAPPER_H_
#define MLT_PRODUCER_KINO_WRAPPER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct kino_wrapper_s *kino_wrapper;

extern kino_wrapper kino_wrapper_init( );
extern int kino_wrapper_open( kino_wrapper, char * );
extern int kino_wrapper_is_open( kino_wrapper );
extern int kino_wrapper_is_pal( kino_wrapper );
extern int kino_wrapper_get_frame_count( kino_wrapper );
extern int kino_wrapper_get_frame( kino_wrapper, uint8_t *, int );
extern void kino_wrapper_close( kino_wrapper );

#ifdef __cplusplus
}
#endif

#endif
