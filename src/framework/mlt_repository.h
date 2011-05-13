/**
 * \file mlt_repository.h
 * \brief provides a map between service and shared objects
 * \see mlt_repository_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
 * \author Dan Dennedy <dan@dennedy.org>
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

#ifndef _MLT_REPOSITORY_H_
#define _MLT_REPOSITORY_H_

#include "mlt_types.h"
#include "mlt_profile.h"

/** This callback is the main entry point into a module, which must be exported
 *  with the symbol "mlt_register".
 *
 *  Inside the callback, the module registers the additional callbacks below.
 */

typedef void ( *mlt_repository_callback )( mlt_repository );

/** The callback function that modules implement to construct a service.
 */

typedef void *( *mlt_register_callback )( mlt_profile, mlt_service_type, const char * /* service name */, const void * /* arg */ );

/** The callback function that modules implement to supply metadata as a properties list.
 */

typedef mlt_properties ( *mlt_metadata_callback )( mlt_service_type, const char * /* service name */, void * /* callback_data */ );

/** A convenience macro to create an entry point for service registration. */
#define MLT_REPOSITORY void mlt_register( mlt_repository repository )

/** A convenience macro to a register service in a more declarative manner. */
#define MLT_REGISTER( type, service, symbol  ) ( mlt_repository_register( repository, (type), (service), ( mlt_register_callback )(symbol) ) )

/** A convenience macro to a register metadata in a more declarative manner. */
#define MLT_REGISTER_METADATA( type, service, callback, data ) ( mlt_repository_register_metadata( repository, (type), (service), ( mlt_metadata_callback )(callback), (data) ) )

extern mlt_repository mlt_repository_init( const char *directory );
extern void mlt_repository_register( mlt_repository self, mlt_service_type service_type, const char *service, mlt_register_callback );
extern void *mlt_repository_create( mlt_repository self, mlt_profile profile, mlt_service_type type, const char *service, const void *arg );
extern void mlt_repository_close( mlt_repository self );
extern mlt_properties mlt_repository_consumers( mlt_repository self );
extern mlt_properties mlt_repository_filters( mlt_repository self );
extern mlt_properties mlt_repository_producers( mlt_repository self );
extern mlt_properties mlt_repository_transitions( mlt_repository self );
extern void mlt_repository_register_metadata( mlt_repository self, mlt_service_type type, const char *service, mlt_metadata_callback, void *callback_data );
extern mlt_properties mlt_repository_metadata( mlt_repository self, mlt_service_type type, const char *service );
extern mlt_properties mlt_repository_languages( mlt_repository self );
extern mlt_properties mlt_repository_presets( );

#endif

