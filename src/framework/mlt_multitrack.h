/*
 * mlt_multitrack.h -- multitrack service class
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#ifndef _MLT_MULITRACK_H_
#define _MLT_MULITRACK_H_

#include "mlt_producer.h"

/** Private definition.
*/

struct mlt_track_s
{
	mlt_producer producer;
	mlt_event event;
};

typedef struct mlt_track_s *mlt_track;

struct mlt_multitrack_s
{
	/* We're extending producer here */
	struct mlt_producer_s parent;
	mlt_track *list;
	int size;
	int count;
};

/** Public final methods
*/

#define MLT_MULTITRACK_PRODUCER( multitrack )	( &( multitrack )->parent )
#define MLT_MULTITRACK_SERVICE( multitrack )	MLT_PRODUCER_SERVICE( MLT_MULTITRACK_PRODUCER( multitrack ) )
#define MLT_MULTITRACK_PROPERTIES( multitrack )	MLT_SERVICE_PROPERTIES( MLT_MULTITRACK_SERVICE( multitrack ) )

extern mlt_multitrack mlt_multitrack_init( );
extern mlt_producer mlt_multitrack_producer( mlt_multitrack self );
extern mlt_service mlt_multitrack_service( mlt_multitrack self );
extern mlt_properties mlt_multitrack_properties( mlt_multitrack self );
extern int mlt_multitrack_connect( mlt_multitrack self, mlt_producer producer, int track );
extern mlt_position mlt_multitrack_clip( mlt_multitrack self, mlt_whence whence, int index );
extern void mlt_multitrack_close( mlt_multitrack self );
extern int mlt_multitrack_count( mlt_multitrack self );
extern void mlt_multitrack_refresh( mlt_multitrack self );
extern mlt_producer mlt_multitrack_track( mlt_multitrack self, int track );

#endif

