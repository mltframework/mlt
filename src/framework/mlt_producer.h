/*
 * mlt_producer.h -- abstraction for all producer services
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

#ifndef _MLT_PRODUCER_H_
#define _MLT_PRODUCER_H_

#include "mlt_service.h"
#include "mlt_filter.h"

/** The interface definition for all producers.
*/

struct mlt_producer_s
{
	// We're implementing service here
	struct mlt_service_s parent;

	// Public virtual methods
	int ( *get_frame )( mlt_producer, mlt_frame_ptr, int );
	mlt_destructor close;
	void *close_object;

	// Private data
	void *local;
	void *child;
};

/** Public final methods
*/

#define MLT_PRODUCER_SERVICE( producer )	( &( producer )->parent )
#define MLT_PRODUCER_PROPERTIES( producer )	MLT_SERVICE_PROPERTIES( MLT_PRODUCER_SERVICE( producer ) )

extern int mlt_producer_init( mlt_producer self, void *child );
extern mlt_producer mlt_producer_new( );
extern mlt_service mlt_producer_service( mlt_producer self );
extern mlt_properties mlt_producer_properties( mlt_producer self );
extern int mlt_producer_seek( mlt_producer self, mlt_position position );
extern mlt_position mlt_producer_position( mlt_producer self );
extern mlt_position mlt_producer_frame( mlt_producer self );
extern int mlt_producer_set_speed( mlt_producer self, double speed );
extern double mlt_producer_get_speed( mlt_producer self );
extern double mlt_producer_get_fps( mlt_producer self );
extern int mlt_producer_set_in_and_out( mlt_producer self, mlt_position in, mlt_position out );
extern int mlt_producer_clear( mlt_producer self );
extern mlt_position mlt_producer_get_in( mlt_producer self );
extern mlt_position mlt_producer_get_out( mlt_producer self );
extern mlt_position mlt_producer_get_playtime( mlt_producer self );
extern mlt_position mlt_producer_get_length( mlt_producer self );
extern void mlt_producer_prepare_next( mlt_producer self );
extern int mlt_producer_attach( mlt_producer self, mlt_filter filter );
extern int mlt_producer_detach( mlt_producer self, mlt_filter filter );
extern mlt_filter mlt_producer_filter( mlt_producer self, int index );
extern mlt_producer mlt_producer_cut( mlt_producer self, int in, int out );
extern int mlt_producer_is_cut( mlt_producer self );
extern int mlt_producer_is_mix( mlt_producer self );
extern int mlt_producer_is_blank( mlt_producer self );
extern mlt_producer mlt_producer_cut_parent( mlt_producer self );
extern int mlt_producer_optimise( mlt_producer self );
extern void mlt_producer_close( mlt_producer self );

#endif
