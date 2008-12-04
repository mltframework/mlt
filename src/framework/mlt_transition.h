/**
 * \file mlt_transition.h
 * \brief abstraction for all transition services
 *
 * Copyright (C) 2003-2008 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
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

#ifndef _MLT_TRANSITION_H_
#define _MLT_TRANSITION_H_

#include "mlt_service.h"

/** \brief Transition abstract service class
 *
 * \extends mlt_service_s
 */

struct mlt_transition_s
{
	/* We're implementing service here */
	struct mlt_service_s parent;

	/* public virtual */
	void ( *close )( mlt_transition );

	/* protected transition method */
	mlt_frame ( *process )( mlt_transition, mlt_frame, mlt_frame );

	/* Protected */
	void *child;

	/* track and in/out points */
	mlt_service producer;

	/* Private */
	mlt_frame *frames;
	int held;
};

/** Public final methods
*/

#define MLT_TRANSITION_SERVICE( transition )		( &( transition )->parent )
#define MLT_TRANSITION_PROPERTIES( transition )		MLT_SERVICE_PROPERTIES( MLT_TRANSITION_SERVICE( transition ) )

extern int mlt_transition_init( mlt_transition self, void *child );
extern mlt_transition mlt_transition_new( );
extern mlt_service mlt_transition_service( mlt_transition self );
extern mlt_properties mlt_transition_properties( mlt_transition self );
extern int mlt_transition_connect( mlt_transition self, mlt_service producer, int a_track, int b_track );
extern void mlt_transition_set_in_and_out( mlt_transition self, mlt_position in, mlt_position out );
extern int mlt_transition_get_a_track( mlt_transition self );
extern int mlt_transition_get_b_track( mlt_transition self );
extern mlt_position mlt_transition_get_in( mlt_transition self );
extern mlt_position mlt_transition_get_out( mlt_transition self );
extern mlt_frame mlt_transition_process( mlt_transition self, mlt_frame a_frame, mlt_frame b_frame );
extern void mlt_transition_close( mlt_transition self );

#endif
