/*
 * mlt_deque.h -- double ended queue
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#ifndef _MLT_DEQUE_H_
#define _MLT_DEQUE_H_

#include "mlt_types.h"

extern mlt_deque mlt_deque_init( );
extern int mlt_deque_count( mlt_deque this );
extern int mlt_deque_push_back( mlt_deque this, void *item );
extern void *mlt_deque_pop_back( mlt_deque this );
extern int mlt_deque_push_front( mlt_deque this, void *item );
extern void *mlt_deque_pop_front( mlt_deque this );
extern void *mlt_deque_peek_back( mlt_deque this );
extern void *mlt_deque_peek_front( mlt_deque this );
extern void mlt_deque_close( mlt_deque this );

#endif
