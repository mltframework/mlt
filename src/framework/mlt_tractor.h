/*
 * mlt_tractor.h -- tractor service class
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

#ifndef _MLT_TRACTOR_H_
#define _MLT_TRACTOR_H_

#include "mlt_producer.h"

/** Private structure.
*/

struct mlt_tractor_s
{
	struct mlt_producer_s parent;
	mlt_service producer;
};

#define MLT_TRACTOR_PRODUCER( tractor )		( &( tractor )->parent )
#define MLT_TRACTOR_SERVICE( tractor )		MLT_PRODUCER_SERVICE( MLT_TRACTOR_PRODUCER( tractor ) )
#define MLT_TRACTOR_PROPERTIES( tractor )	MLT_SERVICE_PROPERTIES( MLT_TRACTOR_SERVICE( tractor ) )

extern mlt_tractor mlt_tractor_init( );
extern mlt_tractor mlt_tractor_new( );
extern mlt_service mlt_tractor_service( mlt_tractor self );
extern mlt_producer mlt_tractor_producer( mlt_tractor self );
extern mlt_properties mlt_tractor_properties( mlt_tractor self );
extern mlt_field mlt_tractor_field( mlt_tractor self );
extern mlt_multitrack mlt_tractor_multitrack( mlt_tractor self );
extern int mlt_tractor_connect( mlt_tractor self, mlt_service service );
extern void mlt_tractor_refresh( mlt_tractor self );
extern int mlt_tractor_set_track( mlt_tractor self, mlt_producer producer, int index );
extern mlt_producer mlt_tractor_get_track( mlt_tractor self, int index );
extern void mlt_tractor_close( mlt_tractor self );

#endif
