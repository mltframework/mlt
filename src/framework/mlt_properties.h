/**
 * \file mlt_properties.h
 * \brief Properties class declaration
 * \see mlt_properties_s
 *
 * Copyright (C) 2003-2011 Ushodaya Enterprises Limited
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

#ifndef _MLT_PROPERTIES_H_
#define _MLT_PROPERTIES_H_

#include "mlt_types.h"
#include "mlt_events.h"
#include <stdio.h>

/** \brief Properties class
 *
 * Properties is a combination list/dictionary of name/::mlt_property pairs.
 * It is also a base class for many of the other MLT classes.
 */

struct mlt_properties_s
{
	void *child; /**< \private the object of a subclass */
	void *local; /**< \private instance object */

	/** the destructor virtual function */
	mlt_destructor close;
	void *close_object;  /**< the object supplied to the close virtual function */
};

extern int mlt_properties_init( mlt_properties, void *child );
extern mlt_properties mlt_properties_new( );
extern mlt_properties mlt_properties_load( const char *file );
extern int mlt_properties_preset( mlt_properties self, const char *name );
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
extern int mlt_properties_is_sequence( mlt_properties self );
extern mlt_properties mlt_properties_parse_yaml( const char *file );
extern char *mlt_properties_serialise_yaml( mlt_properties self );
extern void mlt_properties_lock( mlt_properties self );
extern void mlt_properties_unlock( mlt_properties self );

#endif
