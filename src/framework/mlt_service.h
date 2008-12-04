/**
 * \file mlt_service.h
 * \brief interface declaration for all service classes
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

#ifndef _MLT_SERVICE_H_
#define _MLT_SERVICE_H_

#include "mlt_properties.h"
#include "mlt_profile.h"

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
 * \event \em service-changed
 * \event \em property-changed
 * \properties \em mlt_type identifies the subclass
 * \properties \em resource is either the stream identifier or grandchild-class
 * \properties \em in where to start playing
 * \properties \em _filter_private Set this on a service to ensure that attached filters are handled privately.
 * See modules/core/filter_region.c and modules/core/filter_watermark.c for examples.
 * \properties \em disable Set this on a filter to disable it while keeping it in the object model.
 * \properties \em _profile stores the mlt_profile for a service
 */

struct mlt_service_s
{
	struct mlt_properties_s parent; /**< \private */

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
extern mlt_service mlt_service_get_producer( mlt_service self );
extern int mlt_service_get_frame( mlt_service self, mlt_frame_ptr frame, int index );
extern mlt_properties mlt_service_properties( mlt_service self );
extern mlt_service mlt_service_consumer( mlt_service self );
extern mlt_service mlt_service_producer( mlt_service self );
extern int mlt_service_attach( mlt_service self, mlt_filter filter );
extern int mlt_service_detach( mlt_service self, mlt_filter filter );
extern void mlt_service_apply_filters( mlt_service self, mlt_frame frame, int index );
extern mlt_filter mlt_service_filter( mlt_service self, int index );
extern mlt_profile mlt_service_profile( mlt_service self );
extern void mlt_service_close( mlt_service self );

#endif

