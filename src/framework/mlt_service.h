/*
 * mlt_service.h -- interface for all service classes
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

#ifndef _MLT_SERVICE_H_
#define _MLT_SERVICE_H_

#include "mlt_properties.h"
#include "mlt_profile.h"

/** The interface definition for all services.
*/

struct mlt_service_s
{
	/* We're extending properties here */
	struct mlt_properties_s parent;

	/* Protected virtual */
	int ( *get_frame )( mlt_service self, mlt_frame_ptr frame, int index );
	mlt_destructor close;
	void *close_object;

	/* Private data */
	void *local;
	void *child;
};

/** The public API.
*/

#define MLT_SERVICE_PROPERTIES( service )	( &( service )->parent )

extern int mlt_service_init( mlt_service self, void *child );
extern void mlt_service_lock( mlt_service self );
extern void mlt_service_unlock( mlt_service self );
extern mlt_service_type mlt_service_identify( mlt_service self );
extern int mlt_service_connect_producer( mlt_service self, mlt_service producer, int index );
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

/* I'm not sure about self one - leaving it out of docs for now (only used in consumer_westley) */
extern mlt_service mlt_service_get_producer( mlt_service self );

#endif

