/**
 * \file mlt_factory.h
 * \brief the factory method interfaces
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
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

#ifndef _MLT_FACTORY_H
#define _MLT_FACTORY_H

#include "mlt_types.h"
#include "mlt_profile.h"
#include "mlt_repository.h"

/**
 * \event \em producer-create-request fired when mlt_factory_producer is called
 * \event \em producer-create-done fired when a producer registers itself
 * \event \em filter-create-request fired when mlt_factory_filter is called
 * \event \em filter-create-done fired when a filter registers itself
 * \event \em transition-create-request fired when mlt_factory_transition is called
 * \event \em transition-create-done fired when a transition registers itself
 * \event \em consumer-create-request fired when mlt_factory_consumer is called
 * \event \em consumer-create-done fired when a consumer registers itself
 */

extern mlt_repository mlt_factory_init( const char *directory );
extern const char *mlt_factory_directory( );
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
