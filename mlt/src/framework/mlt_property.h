/*
 * mlt_property.h -- property class
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

#ifndef _MLT_PROPERTY_H_
#define _MLT_PROPERTY_H_

#include "mlt_types.h"

/** Bit pattern for properties.
*/

typedef enum
{
	mlt_prop_none = 0,
	mlt_prop_int = 1,
	mlt_prop_string = 2,
	mlt_prop_timecode = 4,
	mlt_prop_double = 8,
	mlt_prop_data = 16
}
mlt_property_type;

/** Property structure.
*/

typedef struct mlt_property_s
{
	// Stores a bit pattern of types available for this property
	mlt_property_type types;

	// Atomic type handling
	int prop_int;
	mlt_timecode prop_timecode;
	double prop_double;

	// String handling
	char *prop_string;

	// Generic type handling
	void *data;
	int length;
	mlt_destructor destructor;
	mlt_serialiser serialiser;
}
*mlt_property;

/** API
*/

extern mlt_property mlt_property_init( );
extern void mlt_property_clear( mlt_property this );
extern int mlt_property_set_int( mlt_property this, int value );
extern int mlt_property_set_double( mlt_property this, double value );
extern int mlt_property_set_timecode( mlt_property this, mlt_timecode value );
extern int mlt_property_set_string( mlt_property this, char *value );
extern int mlt_property_set_data( mlt_property this, void *value, int length, mlt_destructor destructor, mlt_serialiser serialiser );
extern int mlt_property_get_int( mlt_property this );
extern double mlt_property_get_double( mlt_property this );
extern mlt_timecode mlt_property_get_timecode( mlt_property this );
extern char *mlt_property_get_string( mlt_property this );
extern void *mlt_property_get_data( mlt_property this, int *length );
extern void mlt_property_close( mlt_property this );

#endif

