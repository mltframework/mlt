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
#include "mlt_events.h"
#include <stdio.h>

/** The properties base class defines the basic property propagation and
	handling.
*/

struct mlt_properties_s
{
	void *child;
	void *local;
	mlt_destructor close;
	void *close_object;
};

/** Public interface.
*/

extern int mlt_properties_init( mlt_properties, void *child );
extern mlt_properties mlt_properties_new( );
extern mlt_properties mlt_properties_load( const char *file );
extern int mlt_properties_inc_ref( mlt_properties self );
extern int mlt_properties_dec_ref( mlt_properties self );
extern int mlt_properties_ref_count( mlt_properties self );
extern void mlt_properties_mirror( mlt_properties self, mlt_properties that );
extern int mlt_properties_inherit( mlt_properties self, mlt_properties that );
extern int mlt_properties_pass( mlt_properties self, mlt_properties that, const char *prefix );
extern void mlt_properties_pass_property( mlt_properties self, mlt_properties that, const char *name );
extern int mlt_properties_pass_list( mlt_properties self, mlt_properties that, const char *list );
extern int mlt_properties_set( mlt_properties self, const char *name, const char *value );
extern int mlt_properties_set_or_default( mlt_properties self, const char *name, const char *value, const char *def );
extern int mlt_properties_parse( mlt_properties self, const char *namevalue );
extern char *mlt_properties_get( mlt_properties self, const char *name );
extern char *mlt_properties_get_name( mlt_properties self, int index );
extern char *mlt_properties_get_value( mlt_properties self, int index );
extern void *mlt_properties_get_data_at( mlt_properties self, int index, int *size );
extern int mlt_properties_get_int( mlt_properties self, const char *name );
extern int mlt_properties_set_int( mlt_properties self, const char *name, int value );
extern int64_t mlt_properties_get_int64( mlt_properties self, const char *name );
extern int mlt_properties_set_int64( mlt_properties self, const char *name, int64_t value );
extern double mlt_properties_get_double( mlt_properties self, const char *name );
extern int mlt_properties_set_double( mlt_properties self, const char *name, double value );
extern mlt_position mlt_properties_get_position( mlt_properties self, const char *name );
extern int mlt_properties_set_position( mlt_properties self, const char *name, mlt_position value );
extern int mlt_properties_set_data( mlt_properties self, const char *name, void *value, int length, mlt_destructor, mlt_serialiser );
extern void *mlt_properties_get_data( mlt_properties self, const char *name, int *length );
extern int mlt_properties_rename( mlt_properties self, const char *source, const char *dest );
extern int mlt_properties_count( mlt_properties self );
extern void mlt_properties_dump( mlt_properties self, FILE *output );
extern void mlt_properties_debug( mlt_properties self, const char *title, FILE *output );
extern int mlt_properties_save( mlt_properties, const char * );
extern int mlt_properties_dir_list( mlt_properties, const char *, const char *, int );
extern void mlt_properties_close( mlt_properties self );

#endif
