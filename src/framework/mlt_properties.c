/*
 * mlt_properties.c -- base properties class
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

#include "config.h"
#include "mlt_properties.h"
#include "mlt_property.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- // Private Implementation // ---------------- */

/** Private implementation of the property list.
*/

typedef struct
{
	char **name;
	mlt_property *value;
	int count;
	int size;
}
property_list;

/** Basic implementation.
*/

int mlt_properties_init( mlt_properties this, void *child )
{
	// NULL all methods
	memset( this, 0, sizeof( struct mlt_properties_s ) );

	// Assign the child of the object
	this->child = child;

	// Allocate the private structure
	this->private = calloc( sizeof( property_list ), 1 );

	return this->private == NULL;
}

/** Constructor for stand alone object.
*/

mlt_properties mlt_properties_new( )
{
	// Construct a standalone properties object
	mlt_properties this = calloc( sizeof( struct mlt_properties_s ), 1 );

	// Initialise this
	mlt_properties_init( this, NULL );

	// Return the pointer
	return this;
}

/** Inherit all serialisable properties from that into this.
*/

int mlt_properties_inherit( mlt_properties this, mlt_properties that )
{
	int count = mlt_properties_count( that );
	while ( count -- )
	{
		char *value = mlt_properties_get_value( that, count );
		if ( value != NULL )
		{
			char *name = mlt_properties_get_name( that, count );
			mlt_properties_set( this, name, value );
		}
	}
	return 0;
}

/** Locate a property by name
*/

static mlt_property mlt_properties_find( mlt_properties this, char *name )
{
	mlt_property value = NULL;
	property_list *list = this->private;
	int i = 0;

	// Locate the item 
	for ( i = 0; value == NULL && i < list->count; i ++ )
		if ( !strcmp( list->name[ i ], name ) )
			value = list->value[ i ];

	return value;
}

/** Add a new property.
*/

static mlt_property mlt_properties_add( mlt_properties this, char *name )
{
	property_list *list = this->private;

	// Check that we have space and resize if necessary
	if ( list->count == list->size )
	{
		list->size += 10;
		list->name = realloc( list->name, list->size * sizeof( char * ) );
		list->value = realloc( list->value, list->size * sizeof( mlt_property ) );
	}

	// Assign name/value pair
	list->name[ list->count ] = strdup( name );
	list->value[ list->count ] = mlt_property_init( );

	// Return and increment count accordingly
	return list->value[ list->count ++ ];
}

/** Fetch a property by name - this includes add if not found.
*/

static mlt_property mlt_properties_fetch( mlt_properties this, char *name )
{
	// Try to find an existing property first
	mlt_property property = mlt_properties_find( this, name );

	// If it wasn't found, create one
	if ( property == NULL )
		property = mlt_properties_add( this, name );

	// Return the property
	return property;
}

/** Set the property.
*/

int mlt_properties_set( mlt_properties this, char *name, char *value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( this, name );

	// Set it if not NULL
	if ( property != NULL )
		error = mlt_property_set_string( property, value );

	return error;
}

/** Get a string value by name.
*/

char *mlt_properties_get( mlt_properties this, char *name )
{
	mlt_property value = mlt_properties_find( this, name );
	return value == NULL ? NULL : mlt_property_get_string( value );
}

/** Get a name by index.
*/

char *mlt_properties_get_name( mlt_properties this, int index )
{
	property_list *list = this->private;
	if ( index >= 0 && index < list->count )
		return list->name[ index ];
	return NULL;
}

/** Get a string value by index.
*/

char *mlt_properties_get_value( mlt_properties this, int index )
{
	property_list *list = this->private;
	if ( index >= 0 && index < list->count )
		return mlt_property_get_string( list->value[ index ] );
	return NULL;
}

/** Return the number of items in the list.
*/

int mlt_properties_count( mlt_properties this )
{
	property_list *list = this->private;
	return list->count;
}

/** Set a value by parsing a name=value string
*/

int mlt_properties_parse( mlt_properties this, char *namevalue )
{
	char *name = strdup( namevalue );
	char *value = strdup( namevalue );
	int error = 0;

	if ( strchr( name, '=' ) )
	{
		*( strchr( name, '=' ) ) = '\0';
		strcpy( value, strchr( value, '=' ) + 1 );
	}
	else
	{
		strcpy( value, "" );
	}

	error = mlt_properties_set( this, name, value );

	free( name );
	free( value );

	return error;
}

/** Get a value associated to the name.
*/

int mlt_properties_get_int( mlt_properties this, char *name )
{
	mlt_property value = mlt_properties_find( this, name );
	return value == NULL ? 0 : mlt_property_get_int( value );
}

/** Set a value associated to the name.
*/

int mlt_properties_set_int( mlt_properties this, char *name, int value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( this, name );

	// Set it if not NULL
	if ( property != NULL )
		error = mlt_property_set_int( property, value );

	return error;
}

/** Get a value associated to the name.
*/

double mlt_properties_get_double( mlt_properties this, char *name )
{
	mlt_property value = mlt_properties_find( this, name );
	return value == NULL ? 0 : mlt_property_get_double( value );
}

/** Set a value associated to the name.
*/

int mlt_properties_set_double( mlt_properties this, char *name, double value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( this, name );

	// Set it if not NULL
	if ( property != NULL )
		error = mlt_property_set_double( property, value );

	return error;
}

/** Get a value associated to the name.
*/

mlt_position mlt_properties_get_position( mlt_properties this, char *name )
{
	mlt_property value = mlt_properties_find( this, name );
	return value == NULL ? 0 : mlt_property_get_position( value );
}

/** Set a value associated to the name.
*/

int mlt_properties_set_position( mlt_properties this, char *name, mlt_position value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( this, name );

	// Set it if not NULL
	if ( property != NULL )
		error = mlt_property_set_position( property, value );

	return error;
}

/** Get a value associated to the name.
*/

void *mlt_properties_get_data( mlt_properties this, char *name, int *length )
{
	mlt_property value = mlt_properties_find( this, name );
	return value == NULL ? NULL : mlt_property_get_data( value, length );
}

/** Set a value associated to the name.
*/

int mlt_properties_set_data( mlt_properties this, char *name, void *value, int length, mlt_destructor destroy, mlt_serialiser serialise )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( this, name );

	// Set it if not NULL
	if ( property != NULL )
		error = mlt_property_set_data( property, value, length, destroy, serialise );

	return error;
}

/** Close the list.
*/

void mlt_properties_close( mlt_properties this )
{
	property_list *list = this->private;
	int index = 0;

	// Clean up names and values
	for ( index = list->count - 1; index >= 0; index -- )
	{
		free( list->name[ index ] );
		mlt_property_close( list->value[ index ] );
	}

	// Clear up the list
	free( list->name );
	free( list->value );
	free( list );

	// Free this now if this has no child
	if ( this->child == NULL )
		free( this );
}

