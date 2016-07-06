/**
 * \file mlt_factory.h
 * \brief the factory method interfaces
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

#ifndef MLT_FACTORY_H
#define MLT_FACTORY_H

#include "mlt_types.h"
#include "mlt_profile.h"
#include "mlt_repository.h"

/**
 * \envvar \em MLT_PRODUCER the name of a default producer often used by other services, defaults to "loader"
 * \envvar \em MLT_CONSUMER the name of a default consumer, defaults to "sdl"
 * \envvar \em MLT_TEST_CARD the name of a producer or file to be played when nothing is available (all tracks blank)
 * \envvar \em MLT_DATA overrides the default full path to the MLT and module supplemental data files, defaults to \p PREFIX_DATA
 * \envvar \em MLT_PROFILE selects the default mlt_profile_s, defaults to "dv_pal"
 * \envvar \em MLT_REPOSITORY overrides the default location of the plugin modules, defaults to \p PREFIX_LIB.
 * MLT_REPOSITORY is ignored on Windows and OS X relocatable builds.
 * \envvar \em MLT_PRESETS_PATH overrides the default full path to the properties preset files, defaults to \p MLT_DATA/presets
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
extern mlt_repository mlt_factory_repository();
extern const char *mlt_factory_directory( );
extern char *mlt_environment( const char *name );
extern int mlt_environment_set( const char *name, const char *value );
extern mlt_properties mlt_factory_event_object( );
extern mlt_producer mlt_factory_producer( mlt_profile profile, const char *service, const void *resource );
extern mlt_filter mlt_factory_filter( mlt_profile profile, const char *name, const void *input );
extern mlt_transition mlt_factory_transition( mlt_profile profile, const char *name, const void *input );
extern mlt_consumer mlt_factory_consumer( mlt_profile profile, const char *name, const void *input );
extern void mlt_factory_register_for_clean_up( void *ptr, mlt_destructor destructor );
extern void mlt_factory_close( );
extern mlt_properties mlt_global_properties( );

#endif
