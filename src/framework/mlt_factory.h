/*
 * mlt_factory.h -- the factory method interfaces
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

#ifndef _MLT_FACTORY_H
#define _MLT_FACTORY_H

#include "mlt_types.h"

extern int mlt_factory_init( char *prefix );
extern const char *mlt_factory_prefix( );
extern char *mlt_environment( char *name );
extern mlt_properties mlt_factory_event_object( );
extern mlt_producer mlt_factory_producer( char *name, void *input );
extern mlt_filter mlt_factory_filter( char *name, void *input );
extern mlt_transition mlt_factory_transition( char *name, void *input );
extern mlt_consumer mlt_factory_consumer( char *name, void *input );
extern void mlt_factory_register_for_clean_up( void *ptr, mlt_destructor destructor );
extern void mlt_factory_close( );

#endif
