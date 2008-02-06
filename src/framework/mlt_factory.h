/*
 * mlt_factory.h -- the factory method interfaces
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

#ifndef _MLT_FACTORY_H
#define _MLT_FACTORY_H

#include "mlt_types.h"
#include "mlt_profile.h"
#include "mlt_repository.h"

extern mlt_repository mlt_factory_init( const char *prefix );
extern const char *mlt_factory_prefix( );
extern char *mlt_environment( const char *name );
extern int mlt_environment_set( const char *name, const char *value );
extern mlt_properties mlt_factory_event_object( );
extern mlt_producer mlt_factory_producer( mlt_profile profile, const char *name, void *input );
extern mlt_filter mlt_factory_filter( mlt_profile profile, const char *name, void *input );
extern mlt_transition mlt_factory_transition( mlt_profile profile, const char *name, void *input );
extern mlt_consumer mlt_factory_consumer( mlt_profile profile, const char *name, void *input );
extern void mlt_factory_register_for_clean_up( void *ptr, mlt_destructor destructor );
extern void mlt_factory_close( );

#endif
