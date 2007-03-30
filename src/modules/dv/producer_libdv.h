/*
 * producer_libdv.h -- a DV decoder based on libdv
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _PRODUCER_LIBDV_H_
#define _PRODUCER_LIBDV_H_

#include <framework/mlt_producer.h>

extern mlt_producer producer_libdv_init( char *filename );

#define FRAME_SIZE_525_60 	10 * 150 * 80
#define FRAME_SIZE_625_50 	12 * 150 * 80

#endif
