/**
 * \file mlt_service.h
 * \brief interface declaration for all service classes
 * \see mlt_service_s
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

#ifndef MLT_SERVICE_H
#define MLT_SERVICE_H

#include "mlt_properties.h"
#include "mlt_types.h"

/** \brief Service abstract base class
 *
 * \extends mlt_properties
 * The service is the base class for all of the interesting classes and
 * plugins for MLT. A service can have multiple inputs connections to
 * other services called its "producers" but only a single output to another
 * service called its "consumer." A service that has both producer and
 * consumer connections is called a filter. Any service can have zero or more
 * filters "attached" to it. We call any collection of services and their
 * connections a "service network," which is similar to what DirectShow calls
 * a filter graph or what gstreamer calls an element pipeline.
 *
 * \event \em service-changed a filter was attached or detached or a transition was connected or disconnected
 * \event \em property-changed
 * \properties \em mlt_type identifies the subclass
 * \properties \em _mlt_service_hidden a flag that indicates whether to hide the mlt_service
 * \properties \em mlt_service is the name of the implementation of the service
 * \properties \em resource is either the stream identifier or grandchild-class
 * \properties \em in when to start, what is started is service-specific
 * \properties \em out when to stop
 * \properties \em _filter_private Set this on a service to ensure that attached filters are handled privately.
 * See modules/core/filter_region.c and modules/core/filter_watermark.c for examples.
 * \properties \em _profile stores the mlt_profile for a service
 * \properties \em _unique_id is a unique identifier
 * \properties \em _need_previous_next boolean that instructs producers to get
 * preceding and following frames inside of \p mlt_service_get_frame
 */

struct mlt_service_s
{
	struct mlt_properties_s parent; /**< \private A service extends properties. */

	/** Get a frame of data (virtual function).
	 *
	 * \param mlt_producer a producer
	 * \param mlt_frame_ptr a frame pointer by reference
	 * \param int an index
	 * \return true if there was an error
	 */
	int ( *get_frame )( mlt_service self, mlt_frame_ptr frame, int index );

	/** the destructor virtual function */
	mlt_destructor close;
	void *close_object; /**< the object supplied to the close virtual function */

	void *local; /**< \private instance object */
	void *child; /**< \private the object of a subclass */
};

#define MLT_SERVICE_PROPERTIES( service )	( &( service )->parent )

extern int mlt_service_init( mlt_service self, void *child );
extern void mlt_service_lock( mlt_service self );
extern void mlt_service_unlock( mlt_service self );
extern mlt_service_type mlt_service_identify( mlt_service self );
extern int mlt_service_connect_producer( mlt_service self, mlt_service producer, int index );
extern int mlt_service_insert_producer( mlt_service self, mlt_service producer, int index );
extern int mlt_service_disconnect_producer( mlt_service self, int index );
extern mlt_service mlt_service_get_producer( mlt_service self );
extern int mlt_service_get_frame( mlt_service self, mlt_frame_ptr frame, int index );
extern mlt_properties mlt_service_properties( mlt_service self );
extern mlt_service mlt_service_consumer( mlt_service self );
extern mlt_service mlt_service_producer( mlt_service self );
extern int mlt_service_attach( mlt_service self, mlt_filter filter );
extern int mlt_service_detach( mlt_service self, mlt_filter filter );
extern void mlt_service_apply_filters( mlt_service self, mlt_frame frame, int index );
extern int mlt_service_filter_count( mlt_service self );
extern int mlt_service_move_filter( mlt_service self, int from, int to );
extern mlt_filter mlt_service_filter( mlt_service self, int index );
extern mlt_profile mlt_service_profile( mlt_service self );
extern void mlt_service_set_profile( mlt_service self, mlt_profile profile );
extern void mlt_service_close( mlt_service self );

extern void mlt_service_cache_put( mlt_service self, const char *name, void* data, int size, mlt_destructor destructor );
extern mlt_cache_item mlt_service_cache_get( mlt_service self, const char *name );
extern void mlt_service_cache_set_size( mlt_service self, const char *name, int size );
extern int mlt_service_cache_get_size( mlt_service self, const char *name );
extern void mlt_service_cache_purge( mlt_service self );

#endif

