/**
 * \file mlt_events.h
 * \brief event handling
 * \see mlt_events_struct
 *
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#ifndef MLT_EVENTS_H
#define MLT_EVENTS_H

#include "mlt_types.h"

#if GCC_VERSION >= 40000
typedef void ( *mlt_transmitter )( void *, ... );
typedef void ( *mlt_listener )( void *, ... );
#else
/** callback function to send an event message
 *
 */
typedef void ( *mlt_transmitter )( );
/** event handler when receiving an event message
 * \param the properties object on which the event was registered
 * \param an opaque pointer to a service or really an object
 * \param variable args supplied by the transmitter
 */
typedef void ( *mlt_listener )( );
#endif

extern void mlt_events_init( mlt_properties self );
extern int mlt_events_register( mlt_properties self, const char *id, mlt_transmitter transmitter );
extern int mlt_events_fire( mlt_properties self, const char *id, ... );
extern mlt_event mlt_events_listen( mlt_properties self, void *service, const char *id, mlt_listener listener );
extern void mlt_events_block( mlt_properties self, void *service );
extern void mlt_events_unblock( mlt_properties self, void *service );
extern void mlt_events_disconnect( mlt_properties self, void *service );

extern mlt_event mlt_events_setup_wait_for( mlt_properties self, const char *id );
extern void mlt_events_wait_for( mlt_properties self, mlt_event event );
extern void mlt_events_close_wait_for( mlt_properties self, mlt_event event );

extern void mlt_event_inc_ref( mlt_event self );
extern void mlt_event_block( mlt_event self );
extern void mlt_event_unblock( mlt_event self );
extern void mlt_event_close( mlt_event self );

#endif

