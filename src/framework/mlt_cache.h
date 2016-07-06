/**
 * \file mlt_cache.h
 * \brief least recently used cache
 * \see mlt_cache_s
 *
 * Copyright (C) 2007-2014 Meltytech, LLC
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

#ifndef MLT_CACHE_H
#define MLT_CACHE_H

#include "mlt_types.h"

extern void *mlt_cache_item_data( mlt_cache_item item, int *size );
extern void mlt_cache_item_close( mlt_cache_item item );

extern mlt_cache mlt_cache_init();
extern void mlt_cache_set_size( mlt_cache cache, int size );
extern int mlt_cache_get_size( mlt_cache cache );
extern void mlt_cache_close( mlt_cache cache );
extern void mlt_cache_purge( mlt_cache cache, void *object );
extern void mlt_cache_put( mlt_cache cache, void *object, void* data, int size, mlt_destructor destructor );
extern mlt_cache_item mlt_cache_get( mlt_cache cache, void *object );
extern void mlt_cache_put_frame( mlt_cache cache, mlt_frame frame );
extern mlt_frame mlt_cache_get_frame( mlt_cache cache, mlt_position position );

#endif
