/*
 * mlt_filter.h -- abstraction for all filter services
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

#ifndef _MLT_FILTER_H_
#define _MLT_FILTER_H_

#include "mlt_service.h"

/** The interface definition for all filters.
*/

struct mlt_filter_s
{
	// We're implementing service here
	struct mlt_service_s parent;

	// public virtual
	void ( *close )( mlt_filter );

	// protected filter method
	mlt_frame ( *process )( mlt_filter, mlt_frame );

	// track and in/out points
	mlt_service producer;
	int track;
	mlt_timecode in;
	mlt_timecode out;

	// Protected
	void *child;
};

/** Public final methods
*/

extern int mlt_filter_init( mlt_filter this, void *child );
extern mlt_service mlt_filter_service( mlt_filter this );
extern int mlt_filter_connect( mlt_filter this, mlt_service producer, int index );
extern void mlt_filter_set_in_and_out( mlt_filter this, mlt_timecode in, mlt_timecode out );
extern int mlt_filter_get_track( mlt_filter this );
extern mlt_timecode mlt_filter_get_in( mlt_filter this );
extern mlt_timecode mlt_filter_get_out( mlt_filter this );
extern void mlt_filter_close( mlt_filter );

#endif
