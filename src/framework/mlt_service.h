/*
 * mlt_service.h -- interface for all service classes
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
 * along with self program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MLT_SERVICE_H_
#define _MLT_SERVICE_H_

#include "mlt_properties.h"

/** The interface definition for all services.
*/

struct mlt_service_s
{
	// We're extending properties here
	struct mlt_properties_s parent;

	// Protected virtual
	int ( *get_frame )( mlt_service self, mlt_frame_ptr frame, int index );

	// Private data
	void *local;
	void *child;
};

/** The public API.
*/

extern int mlt_service_init( mlt_service self, void *child );
extern int mlt_service_connect_producer( mlt_service self, mlt_service producer, int index );
extern int mlt_service_get_frame( mlt_service self, mlt_frame_ptr frame, int index );
extern mlt_properties mlt_service_properties( mlt_service self );
extern mlt_service mlt_service_consumer( mlt_service self );
extern mlt_service mlt_service_producer( mlt_service self );
extern void mlt_service_close( mlt_service self );

// I'm not sure about self one - leaving it out of docs for now (only used in consumer_westley)
extern mlt_service mlt_service_get_producer( mlt_service self );

#endif

