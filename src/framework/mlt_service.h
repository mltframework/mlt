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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MLT_SERVICE_H_
#define _MLT_SERVICE_H_

#include "mlt_properties.h"

/** State of a service.

    Note that a service may be dormant even though it's fully connected,
	providing or consuming.
*/

typedef enum
{
	mlt_state_unknown   = 0,
	mlt_state_dormant   = 1,
	mlt_state_connected = 2,
	mlt_state_providing = 4,
	mlt_state_consuming = 8
}
mlt_service_state;

/** The interface definition for all services.
*/

struct mlt_service_s
{
	// We're extending properties here
	struct mlt_properties_s parent;

	// Protected virtual
	int ( *accepts_input )( mlt_service this );
	int ( *accepts_output )( mlt_service this );
	int ( *has_input )( mlt_service this );
	int ( *has_output )( mlt_service this );
	int ( *get_frame )( mlt_service this, mlt_frame_ptr frame, int index );

	// Private data
	void *private;
	void *child;
};

/** The public API.
*/

extern int mlt_service_init( mlt_service this, void *child );
extern mlt_properties mlt_service_properties( mlt_service this );
extern int mlt_service_connect_producer( mlt_service this, mlt_service producer, int index );
extern mlt_service_state mlt_service_get_state( mlt_service this );
extern void mlt_service_close( mlt_service this );

extern int mlt_service_accepts_input( mlt_service this );
extern int mlt_service_accepts_output( mlt_service this );
extern int mlt_service_has_input( mlt_service this );
extern int mlt_service_has_output( mlt_service this );
extern int mlt_service_get_frame( mlt_service this, mlt_frame_ptr frame, int index );
extern int mlt_service_is_active( mlt_service this );

#endif

