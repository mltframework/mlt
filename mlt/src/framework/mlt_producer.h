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

/** The interface definition for all producers.
*/

struct mlt_producer_s
{
	// We're implementing service here
	struct mlt_service_s parent;

	// Public virtual methods
	int ( *get_frame )( mlt_producer, mlt_frame_ptr, int );
	void ( *close )( mlt_producer );

	// Private data
	void *private;
	void *child;
};

/** Public final methods
*/

//extern double mlt_producer_convert_position_to_time( mlt_producer this, int64_t frame );
//extern mlt_position mlt_producer_convert_time_to_position( mlt_producer this, double time );

extern int mlt_producer_init( mlt_producer this, void *child );
extern mlt_service mlt_producer_service( mlt_producer this );
extern mlt_properties mlt_producer_properties( mlt_producer this );
extern int mlt_producer_seek( mlt_producer this, mlt_position position );
extern mlt_position mlt_producer_position( mlt_producer this );
extern mlt_position mlt_producer_frame( mlt_producer this );
extern int mlt_producer_set_speed( mlt_producer this, double speed );
extern double mlt_producer_get_speed( mlt_producer this );
extern double mlt_producer_get_fps( mlt_producer this );
extern int mlt_producer_set_in_and_out( mlt_producer this, mlt_position in, mlt_position out );
extern mlt_position mlt_producer_get_in( mlt_producer this );
extern mlt_position mlt_producer_get_out( mlt_producer this );
extern mlt_position mlt_producer_get_playtime( mlt_producer this );
extern mlt_position mlt_producer_get_length( mlt_producer this );
extern void mlt_producer_prepare_next( mlt_producer this );
extern void mlt_producer_close( mlt_producer this );

#endif
