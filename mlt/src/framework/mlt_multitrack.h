/*
 * mlt_multitrack.h -- multitrack service class
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

#ifndef _MLT_MULITRACK_H_
#define _MLT_MULITRACK_H_

#include "mlt_producer.h"

/** Public final methods
*/

extern mlt_multitrack mlt_multitrack_init( );
extern mlt_producer mlt_multitrack_producer( mlt_multitrack this );
extern mlt_service mlt_multitrack_service( mlt_multitrack this );
extern int mlt_multitrack_connect( mlt_multitrack this, mlt_producer producer, int track );
extern void mlt_multitrack_close( mlt_multitrack this );

#endif

