/*
 * mlt_properties.h -- base properties class
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

#ifndef _MLT_PROPERTIES_H_
#define _MLT_PROPERTIES_H_

#include "mlt_types.h"
#include <stdio.h>

/** The properties base class defines the basic property propagation and
	handling.
*/

struct mlt_properties_s
{
	void *child;
	void *private;
};

/** Public interface.
*/

extern int mlt_properties_init( mlt_properties, void *child );
extern mlt_properties mlt_properties_new( );
extern void mlt_properties_mirror( mlt_properties this, mlt_properties that );
extern int mlt_properties_inherit( mlt_properties this, mlt_properties that );
extern int mlt_properties_set( mlt_properties this, char *name, char *value );
extern int mlt_properties_parse( mlt_properties this, char *namevalue );
extern char *mlt_properties_get( mlt_properties this, char *name );
extern char *mlt_properties_get_name( mlt_properties this, int index );
extern char *mlt_properties_get_value( mlt_properties this, int index );
extern void *mlt_properties_get_data_at( mlt_properties this, int index, int *size );
extern int mlt_properties_get_int( mlt_properties this, char *name );
extern int mlt_properties_set_int( mlt_properties this, char *name, int value );
extern double mlt_properties_get_double( mlt_properties this, char *name );
extern int mlt_properties_set_double( mlt_properties this, char *name, double value );
extern mlt_position mlt_properties_get_position( mlt_properties this, char *name );
extern int mlt_properties_set_position( mlt_properties this, char *name, mlt_position value );
extern int mlt_properties_set_data( mlt_properties this, char *name, void *value, int length, mlt_destructor, mlt_serialiser );
extern void *mlt_properties_get_data( mlt_properties this, char *name, int *length );
extern int mlt_properties_rename( mlt_properties this, char *source, char *dest );
extern int mlt_properties_count( mlt_properties this );
extern void mlt_properties_dump( mlt_properties this, FILE *output );
extern void mlt_properties_close( mlt_properties this );

#endif
