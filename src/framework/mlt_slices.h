/**
 * \file mlt_slices.h
 * \brief sliced threading processing helper
 * \see mlt_slices_s
 *
 * Copyright (C) 2016 Meltytech, LLC
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

#ifndef MLT_SLICES_H
#define MLT_SLICES_H

struct mlt_slices_s;
typedef struct mlt_slices_s *mlt_slices;                  /**< pointer to Sliced processing context object */

typedef int (*mlt_slices_proc)( int id, int idx, int jobs, void* cookie );

extern mlt_slices mlt_slices_init( int threads, int policy, int priority );

extern void mlt_slices_close( mlt_slices ctx );

extern void mlt_slices_run( mlt_slices ctx, int jobs, mlt_slices_proc proc, void* cookie );

#endif
