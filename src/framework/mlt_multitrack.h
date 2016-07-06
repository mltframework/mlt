/**
 * \file mlt_multitrack.h
 * \brief multitrack service class
 * \see mlt_multitrack_s
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

#ifndef MLT_MULITRACK_H
#define MLT_MULITRACK_H

#include "mlt_producer.h"

/** \brief Track class used by mlt_multitrack_s
 */

struct mlt_track_s
{
	mlt_producer producer;
	mlt_event event;
};

typedef struct mlt_track_s *mlt_track;

/** \brief Multitrack class
 *
 * A multitrack is a parallel container of producers that acts a single producer.
 *
 * \extends mlt_producer_s
 * \properties \em log_id not currently used, but sets it to "mulitrack"
 */

struct mlt_multitrack_s
{
	/** We're extending producer here */
	struct mlt_producer_s parent;
	mlt_track *list;
	int size;
	int count;
};

#define MLT_MULTITRACK_PRODUCER( multitrack )	( &( multitrack )->parent )
#define MLT_MULTITRACK_SERVICE( multitrack )	MLT_PRODUCER_SERVICE( MLT_MULTITRACK_PRODUCER( multitrack ) )
#define MLT_MULTITRACK_PROPERTIES( multitrack )	MLT_SERVICE_PROPERTIES( MLT_MULTITRACK_SERVICE( multitrack ) )

extern mlt_multitrack mlt_multitrack_init( );
extern mlt_producer mlt_multitrack_producer( mlt_multitrack self );
extern mlt_service mlt_multitrack_service( mlt_multitrack self );
extern mlt_properties mlt_multitrack_properties( mlt_multitrack self );
extern int mlt_multitrack_connect( mlt_multitrack self, mlt_producer producer, int track );
extern int mlt_multitrack_insert( mlt_multitrack self, mlt_producer producer, int track );
extern int mlt_multitrack_disconnect( mlt_multitrack self, int track );
extern mlt_position mlt_multitrack_clip( mlt_multitrack self, mlt_whence whence, int index );
extern void mlt_multitrack_close( mlt_multitrack self );
extern int mlt_multitrack_count( mlt_multitrack self );
extern void mlt_multitrack_refresh( mlt_multitrack self );
extern mlt_producer mlt_multitrack_track( mlt_multitrack self, int track );

#endif

