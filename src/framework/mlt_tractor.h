/**
 * \file mlt_tractor.h
 * \brief tractor service class
 * \see mlt_tractor_s
 *
 * Copyright (C) 2003-2015 Meltytech, LLC
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

#ifndef MLT_TRACTOR_H
#define MLT_TRACTOR_H

#include "mlt_producer.h"

/** \brief Tractor class
 *
 * The tractor is a convenience class that works with the field class
 * to manage a multitrack, track filters, and transitions.
 *
 * \extends mlt_producer_s
 * \properties \em multitrack holds a reference to the mulitrack object that a tractor manages
 * \properties \em field holds a reference to the field object that a tractor manages
 * \properties \em producer holds a reference to an encapsulated producer
 * \properties \em global_feed a flag to indicate whether this tractor feeds to the consumer or stops here
 * \properties \em global_queue is something for the data_feed functionality in the core module
 * \properties \em data_queue is something for the data_feed functionality in the core module
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
extern int mlt_tractor_insert_track( mlt_tractor self, mlt_producer producer, int index );
extern int mlt_tractor_remove_track( mlt_tractor self, int index );
extern mlt_producer mlt_tractor_get_track( mlt_tractor self, int index );
extern void mlt_tractor_close( mlt_tractor self );

#endif
