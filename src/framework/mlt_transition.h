/**
 * \file mlt_transition.h
 * \brief abstraction for all transition services
 * \see mlt_transition_s
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

#ifndef MLT_TRANSITION_H
#define MLT_TRANSITION_H

#include "mlt_service.h"

/** \brief Transition abstract service class
 *
 * A transition may modify the output of a producer based on the output of a second producer.
 *
 * \extends mlt_service_s
 * \properties \em a_track the track index (0-based) of a multitrack of the first producer
 * \properties \em b_track the track index (0-based) of a multitrack of the second producer
 * \properties \em accepts_blanks a flag to indicate if the transition should accept blank frames
 * \properties \em always_active a flag to indicate that the in and out points do not apply
 * \properties \em _transition_type 1 for video, 2 for audio, 3 for both audio and video
 * \properties \em disable Set this to disable the transition while keeping it in the object model.
 */

struct mlt_transition_s
{
	/** We're implementing service here */
	struct mlt_service_s parent;

	/** public virtual */
	void ( *close )( mlt_transition );

	/** protected transition method */
	mlt_frame ( *process )( mlt_transition, mlt_frame, mlt_frame );

	/** Protected */
	void *child;

	/** track and in/out points */
	mlt_service producer;

	/** Private */
	mlt_frame *frames;
	int held;
};

#define MLT_TRANSITION_SERVICE( transition )		( &( transition )->parent )
#define MLT_TRANSITION_PROPERTIES( transition )		MLT_SERVICE_PROPERTIES( MLT_TRANSITION_SERVICE( transition ) )

extern int mlt_transition_init( mlt_transition self, void *child );
extern mlt_transition mlt_transition_new( );
extern mlt_service mlt_transition_service( mlt_transition self );
extern mlt_properties mlt_transition_properties( mlt_transition self );
extern int mlt_transition_connect( mlt_transition self, mlt_service producer, int a_track, int b_track );
extern void mlt_transition_set_in_and_out( mlt_transition self, mlt_position in, mlt_position out );
extern void mlt_transition_set_tracks( mlt_transition self, int a_track, int b_track );
extern int mlt_transition_get_a_track( mlt_transition self );
extern int mlt_transition_get_b_track( mlt_transition self );
extern mlt_position mlt_transition_get_in( mlt_transition self );
extern mlt_position mlt_transition_get_out( mlt_transition self );
extern mlt_position mlt_transition_get_length( mlt_transition self );
extern mlt_position mlt_transition_get_position( mlt_transition self, mlt_frame frame );
extern double mlt_transition_get_progress( mlt_transition self, mlt_frame frame );
extern double mlt_transition_get_progress_delta( mlt_transition self, mlt_frame frame );
extern mlt_frame mlt_transition_process( mlt_transition self, mlt_frame a_frame, mlt_frame b_frame );
extern void mlt_transition_close( mlt_transition self );

#endif
