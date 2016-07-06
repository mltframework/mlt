/**
 * \file mlt_producer.h
 * \brief abstraction for all producer services
 * \see mlt_producer_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#ifndef MLT_PRODUCER_H
#define MLT_PRODUCER_H

#include "mlt_service.h"
#include "mlt_filter.h"
#include "mlt_profile.h"

/** \brief Producer abstract service class
 *
 * A producer is a service that generates audio, video, and metadata.
 * Some day it may also generate text (subtitles). This is not to say
 * a producer "synthesizes," rather that is an origin of data within the
 * service network - that could be through synthesis or reading a stream.
 *
 * \extends mlt_service
 * \event \em producer-changed either service-changed was fired or the timing of the producer changed
 * \properties \em mlt_type the name of the service subclass, e.g. mlt_producer
 * \properties \em mlt_service the name of a producer subclass
 * \properties \em _position the current position of the play head, relative to the in point
 * \properties \em _frame the current position of the play head, relative to the beginning of the resource
 * \properties \em _speed the current speed factor, where 1.0 is normal
 * \properties \em aspect_ratio sample aspect ratio
 * \properties \em length the duration of the cut in frames
 * \properties \em eof the end-of-file behavior, one of: pause, continue, loop
 * \properties \em resource the file name, stream address, or the class name in angle brackets
 * \properties \em _cut set if this producer is a "cut" producer
 * \properties \em mlt_mix stores the data for a "mix" producer
 * \properties \em _cut_parent holds a reference to the cut's parent producer
 * \properties \em ignore_points Set this to temporarily disable the in and out points.
 * \properties \em use_clone holds a reference to a clone's producer, as created by mlt_producer_optimise
 * \properties \em _clone is the index of the clone in the list of clones stored on the clone's producer
 * \properties \em _clones is the number of clones of the producer, as created by mlt_producer_optimise
 * \properties \em _clone.{N} holds a reference to the N'th clone of the producer, as created by mlt_producer_optimise
 * \properties \em meta.* holds metadata - there is a loose taxonomy to be defined
 * \properties \em set.* holds properties to set on a frame produced
 * \envvar \em MLT_DEFAULT_PRODUCER_LENGTH - the default duration of the producer in frames, defaults to 15000.
 * Most producers will set the producer length to something appropriate
 * like the real duration of an audio or video clip. However, some other things
 * like still images and generators do not have an intrinsic length besides one
 * or infinity. Those producers tend to not override the default length and one
 * expect the app or user to set the length. The default value of 15000 was chosen
 * to provide something useful - not too long or short and convenient to simply
 * set an out point without necessarily nedding to extend the length.
 * \todo define the media metadata taxonomy
 */

struct mlt_producer_s
{
	/** A producer is a service. */
	struct mlt_service_s parent;

	/** Get a frame of data (virtual function).
	 *
	 * \param mlt_producer a producer
	 * \param mlt_frame_ptr a frame pointer by reference
	 * \param int an index
	 * \return true if there was an error
	 */
	int ( *get_frame )( mlt_producer, mlt_frame_ptr, int );

	/** the destructor virtual function */
	mlt_destructor close;
	void *close_object; /**< the object supplied to the close virtual function */

	void *local; /**< \private instance object */
	void *child; /**< \private the object of a subclass */
};

/*
 *  Public final methods
 */

#define MLT_PRODUCER_SERVICE( producer )	( &( producer )->parent )
#define MLT_PRODUCER_PROPERTIES( producer )	MLT_SERVICE_PROPERTIES( MLT_PRODUCER_SERVICE( producer ) )

extern int mlt_producer_init( mlt_producer self, void *child );
extern mlt_producer mlt_producer_new( mlt_profile );
extern mlt_service mlt_producer_service( mlt_producer self );
extern mlt_properties mlt_producer_properties( mlt_producer self );
extern int mlt_producer_seek( mlt_producer self, mlt_position position );
extern int mlt_producer_seek_time( mlt_producer self, const char* time );
extern mlt_position mlt_producer_position( mlt_producer self );
extern mlt_position mlt_producer_frame( mlt_producer self );
char* mlt_producer_frame_time( mlt_producer self, mlt_time_format );
extern int mlt_producer_set_speed( mlt_producer self, double speed );
extern double mlt_producer_get_speed( mlt_producer self );
extern double mlt_producer_get_fps( mlt_producer self );
extern int mlt_producer_set_in_and_out( mlt_producer self, mlt_position in, mlt_position out );
extern int mlt_producer_clear( mlt_producer self );
extern mlt_position mlt_producer_get_in( mlt_producer self );
extern mlt_position mlt_producer_get_out( mlt_producer self );
extern mlt_position mlt_producer_get_playtime( mlt_producer self );
extern mlt_position mlt_producer_get_length( mlt_producer self );
extern char* mlt_producer_get_length_time( mlt_producer self, mlt_time_format );
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
