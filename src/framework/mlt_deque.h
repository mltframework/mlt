/**
 * \file mlt_deque.h
 * \brief double ended queue
 * \see mlt_deque_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#ifndef MLT_DEQUE_H
#define MLT_DEQUE_H

#include "mlt_export.h"
#include "mlt_types.h"

/** The callback function used to compare items for insert sort.
 *
 * \public \memberof mlt_deque_s
 * \param a the first object
 * \param b the second object
 * \returns 0 if equal, < 0 if a < b, or > 0 if a > b
*/
typedef int (*mlt_deque_compare)(void *a, void *b);

MLT_EXPORT mlt_deque mlt_deque_init();
MLT_EXPORT int mlt_deque_count(mlt_deque self);
MLT_EXPORT int mlt_deque_push_back(mlt_deque self, void *item);
MLT_EXPORT void *mlt_deque_pop_back(mlt_deque self);
MLT_EXPORT int mlt_deque_push_front(mlt_deque self, void *item);
MLT_EXPORT void *mlt_deque_pop_front(mlt_deque self);
MLT_EXPORT void *mlt_deque_peek_back(mlt_deque self);
MLT_EXPORT void *mlt_deque_peek_front(mlt_deque self);
MLT_EXPORT void *mlt_deque_peek(mlt_deque self, int index);
MLT_EXPORT int mlt_deque_insert(mlt_deque self, void *item, mlt_deque_compare);

MLT_EXPORT int mlt_deque_push_back_int(mlt_deque self, int item);
MLT_EXPORT int mlt_deque_pop_back_int(mlt_deque self);
MLT_EXPORT int mlt_deque_push_front_int(mlt_deque self, int item);
MLT_EXPORT int mlt_deque_pop_front_int(mlt_deque self);
MLT_EXPORT int mlt_deque_peek_back_int(mlt_deque self);
MLT_EXPORT int mlt_deque_peek_front_int(mlt_deque self);

MLT_EXPORT int mlt_deque_push_back_double(mlt_deque self, double item);
MLT_EXPORT double mlt_deque_pop_back_double(mlt_deque self);
MLT_EXPORT int mlt_deque_push_front_double(mlt_deque self, double item);
MLT_EXPORT double mlt_deque_pop_front_double(mlt_deque self);
MLT_EXPORT double mlt_deque_peek_back_double(mlt_deque self);
MLT_EXPORT double mlt_deque_peek_front_double(mlt_deque self);

MLT_EXPORT void mlt_deque_close(mlt_deque self);

#endif
