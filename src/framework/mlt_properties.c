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
#include <ctype.h>

/* ---------------- // Private Implementation // ---------------- */

/** Private implementation of the property list.
*/

typedef struct
{
	int hash[ 199 ];
	char **name;
	mlt_property *value;
	int count;
	int size;
	mlt_properties mirror;
	int ref_count;
}
property_list;

/** Memory leak checks.
*/

//#define _MLT_PROPERTY_CHECKS_

#ifdef _MLT_PROPERTY_CHECKS_
static int properties_created = 0;
static int properties_destroyed = 0;
#endif

/** Basic implementation.
*/

int mlt_properties_init( mlt_properties this, void *child )
{
	if ( this != NULL )
	{
#ifdef _MLT_PROPERTY_CHECKS_
		// Increment number of properties created
		properties_created ++;
#endif

		// NULL all methods
		memset( this, 0, sizeof( struct mlt_properties_s ) );

		// Assign the child of the object
		this->child = child;

		// Allocate the local structure
		this->local = calloc( sizeof( property_list ), 1 );

		// Increment the ref count
		( ( property_list * )this->local )->ref_count = 1;
	}

	// Check that initialisation was successful
	return this != NULL && this->local == NULL;
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

/** Load properties from a file.
*/

mlt_properties mlt_properties_load( char *filename )
{
	// Construct a standalone properties object
	mlt_properties this = mlt_properties_new( );

	if ( this != NULL )
	{
		// Open the file
		FILE *file = fopen( filename, "r" );

		// Load contents of file
		if ( file != NULL )
		{
			// Temp string
			char temp[ 1024 ];
			char last[ 1024 ] = "";

			// Read each string from the file
			while( fgets( temp, 1024, file ) )
			{
				// Chomp the string
				temp[ strlen( temp ) - 1 ] = '\0';

				// Check if the line starts with a .
				if ( temp[ 0 ] == '.' )
				{
					char temp2[ 1024 ];
					sprintf( temp2, "%s%s", last, temp );
					strcpy( temp, temp2 );
				}
				else if ( strchr( temp, '=' ) )
				{
					strcpy( last, temp );
					*( strchr( last, '=' ) ) = '\0';
				}

				// Parse and set the property
				if ( strcmp( temp, "" ) && temp[ 0 ] != '#' )
					mlt_properties_parse( this, temp );
			}

			// Close the file
			fclose( file );
		}
	}

	// Return the pointer
	return this;
}

static inline int generate_hash( char *name )
{
	int hash = 0;
	int i = 1;
	while ( *name )
		hash = ( hash + ( i ++ * ( *name ++ & 31 ) ) ) % 199;
	return hash;
}

/** Special case - when a container (such as fezzik) is protecting another 
	producer, we need to ensure that properties are passed through to the
	real producer.
*/

static inline void mlt_properties_do_mirror( mlt_properties this, char *name )
{
	property_list *list = this->local;
	if ( list->mirror != NULL ) 
	{
		char *value = mlt_properties_get( this, name );
		if ( value != NULL )
			mlt_properties_set( list->mirror, name, value );
	}
}

/** Maintain ref count to allow multiple uses of an mlt object.
*/

int mlt_properties_inc_ref( mlt_properties this )
{
	if ( this != NULL )
	{
		property_list *list = this->local;
		return ++ list->ref_count;
	}
	return 0;
}

/** Maintain ref count to allow multiple uses of an mlt object.
*/

int mlt_properties_dec_ref( mlt_properties this )
{
	if ( this != NULL )
	{
		property_list *list = this->local;
		return -- list->ref_count;
	}
	return 0;
}

/** Return the ref count of this object.
*/

int mlt_properties_ref_count( mlt_properties this )
{
	if ( this != NULL )
	{
		property_list *list = this->local;
		return list->ref_count;
	}
	return 0;
}

/** Allow the specification of a mirror.
*/

void mlt_properties_mirror( mlt_properties this, mlt_properties that )
{
	property_list *list = this->local;
	list->mirror = that;
}

/** Inherit all serialisable properties from that into this.
*/

int mlt_properties_inherit( mlt_properties this, mlt_properties that )
{
	int count = mlt_properties_count( that );
	int i = 0;
	for ( i = 0; i < count; i ++ )
	{
		char *value = mlt_properties_get_value( that, i );
		if ( value != NULL )
		{
			char *name = mlt_properties_get_name( that, i );
			mlt_properties_set( this, name, value );
		}
	}
	return 0;
}

/** Pass all properties from 'that' that match the prefix to 'this' (excluding the prefix).
*/

int mlt_properties_pass( mlt_properties this, mlt_properties that, char *prefix )
{
	int count = mlt_properties_count( that );
	int length = strlen( prefix );
	int i = 0;
	for ( i = 0; i < count; i ++ )
	{
		char *name = mlt_properties_get_name( that, i );
		if ( !strncmp( name, prefix, length ) )
		{
			char *value = mlt_properties_get_value( that, i );
			if ( value != NULL )
				mlt_properties_set( this, name + length, value );
		}
	}
	return 0;
}

/** Locate a property by name
*/

static inline mlt_property mlt_properties_find( mlt_properties this, char *name )
{
	property_list *list = this->local;
	mlt_property value = NULL;
	int key = generate_hash( name );
	int i = list->hash[ key ] - 1;

	if ( i >= 0 )
	{
		// Check if we're hashed
		if ( list->count > 0 &&
		 	name[ 0 ] == list->name[ i ][ 0 ] && 
		 	!strcmp( list->name[ i ], name ) )
			value = list->value[ i ];

		// Locate the item 
		for ( i = list->count - 1; value == NULL && i >= 0; i -- )
			if ( name[ 0 ] == list->name[ i ][ 0 ] && !strcmp( list->name[ i ], name ) )
				value = list->value[ i ];
	}

	return value;
}

/** Add a new property.
*/

static mlt_property mlt_properties_add( mlt_properties this, char *name )
{
	property_list *list = this->local;
	int key = generate_hash( name );

	// Check that we have space and resize if necessary
	if ( list->count == list->size )
	{
		list->size += 50;
		list->name = realloc( list->name, list->size * sizeof( char * ) );
		list->value = realloc( list->value, list->size * sizeof( mlt_property ) );
	}

	// Assign name/value pair
	list->name[ list->count ] = strdup( name );
	list->value[ list->count ] = mlt_property_init( );

	// Assign to hash table
	if ( list->hash[ key ] == 0 )
		list->hash[ key ] = list->count + 1;

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
	if ( property != NULL && ( value == NULL || value[ 0 ] != '@' ) )
	{
		error = mlt_property_set_string( property, value );
		mlt_properties_do_mirror( this, name );
	}
	else if ( property != NULL && value[ 0 ] == '@' )
	{
		int total = 0;
		int current = 0;
		char id[ 255 ];
		char op = '+';

		value ++;

		while ( *value != '\0' )
		{
			int length = strcspn( value, "+-*/" );

			// Get the identifier
			strncpy( id, value, length );
			id[ length ] = '\0';
			value += length;

			// Determine the value
			if ( isdigit( id[ 0 ] ) )
				current = atof( id );
			else
				current = mlt_properties_get_int( this, id );

			// Apply the operation
			switch( op )
			{
				case '+':
					total += current;
					break;
				case '-':
					total -= current;
					break;
				case '*':
					total *= current;
					break;
				case '/':
					total /= current;
					break;
			}

			// Get the next op
			op = *value != '\0' ? *value ++ : ' ';
		}

		error = mlt_property_set_int( property, total );
		mlt_properties_do_mirror( this, name );
	}

	mlt_events_fire( this, "property-changed", name, NULL );

	return error;
}

/** Set or default the property.
*/

int mlt_properties_set_or_default( mlt_properties this, char *name, char *value, char *def )
{
	return mlt_properties_set( this, name, value == NULL ? def : value );
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
	property_list *list = this->local;
	if ( index >= 0 && index < list->count )
		return list->name[ index ];
	return NULL;
}

/** Get a string value by index.
*/

char *mlt_properties_get_value( mlt_properties this, int index )
{
	property_list *list = this->local;
	if ( index >= 0 && index < list->count )
		return mlt_property_get_string( list->value[ index ] );
	return NULL;
}

/** Get a data value by index.
*/

void *mlt_properties_get_data_at( mlt_properties this, int index, int *size )
{
	property_list *list = this->local;
	if ( index >= 0 && index < list->count )
		return mlt_property_get_data( list->value[ index ], size );
	return NULL;
}

/** Return the number of items in the list.
*/

int mlt_properties_count( mlt_properties this )
{
	property_list *list = this->local;
	return list->count;
}

/** Set a value by parsing a name=value string
*/

int mlt_properties_parse( mlt_properties this, char *namevalue )
{
	char *name = strdup( namevalue );
	char *value = NULL;
	int error = 0;
	char *ptr = strchr( name, '=' );

	if ( ptr )
	{
		*( ptr ++ ) = '\0';

		if ( *ptr != '\"' )
		{
			value = strdup( ptr );
		}
		else
		{
			ptr ++;
			value = strdup( ptr );
			if ( value != NULL && value[ strlen( value ) - 1 ] == '\"' )
				value[ strlen( value ) - 1 ] = '\0';
		}
	}
	else
	{
		value = strdup( "" );
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
	{
		error = mlt_property_set_int( property, value );
		mlt_properties_do_mirror( this, name );
	}

	mlt_events_fire( this, "property-changed", name, NULL );

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
	{
		error = mlt_property_set_double( property, value );
		mlt_properties_do_mirror( this, name );
	}

	mlt_events_fire( this, "property-changed", name, NULL );

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
	{
		error = mlt_property_set_position( property, value );
		mlt_properties_do_mirror( this, name );
	}

	mlt_events_fire( this, "property-changed", name, NULL );

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

	mlt_events_fire( this, "property-changed", name, NULL );

	return error;
}

/** Rename a property.
*/

int mlt_properties_rename( mlt_properties this, char *source, char *dest )
{
	mlt_property value = mlt_properties_find( this, dest );

	if ( value == NULL )
	{
		property_list *list = this->local;
		int i = 0;

		// Locate the item 
		for ( i = 0; i < list->count; i ++ )
		{
			if ( !strcmp( list->name[ i ], source ) )
			{
				free( list->name[ i ] );
				list->name[ i ] = strdup( dest );
				list->hash[ generate_hash( dest ) ] = i + 1;
				break;
			}
		}
	}

	return value != NULL;
}

/** Dump the properties.
*/

void mlt_properties_dump( mlt_properties this, FILE *output )
{
	property_list *list = this->local;
	int i = 0;
	for ( i = 0; i < list->count; i ++ )
		if ( mlt_properties_get( this, list->name[ i ] ) != NULL )
			fprintf( output, "%s=%s\n", list->name[ i ], mlt_properties_get( this, list->name[ i ] ) );
}

void mlt_properties_debug( mlt_properties this, char *title, FILE *output )
{
	fprintf( stderr, "%s: ", title );
	if ( this != NULL )
	{
		property_list *list = this->local;
		int i = 0;
		fprintf( output, "[ ref=%d", list->ref_count );
		for ( i = 0; i < list->count; i ++ )
			if ( mlt_properties_get( this, list->name[ i ] ) != NULL )
				fprintf( output, ", %s=%s", list->name[ i ], mlt_properties_get( this, list->name[ i ] ) );
			else
				fprintf( output, ", %s=%p", list->name[ i ], mlt_properties_get_data( this, list->name[ i ], NULL ) );
		fprintf( output, " ]" );
	}
	fprintf( stderr, "\n" );
}

/** Close the list.
*/

void mlt_properties_close( mlt_properties this )
{
	if ( this != NULL && mlt_properties_dec_ref( this ) <= 0 )
	{
		if ( this->close != NULL )
		{
			this->close( this->close_object );
		}
		else
		{
			property_list *list = this->local;
			int index = 0;

#ifdef _MLT_PROPERTY_CHECKS_
			// Show debug info
			mlt_properties_debug( this, "Closing", stderr );

			// Increment destroyed count
			properties_destroyed ++;

			// Show current stats - these should match when the app is closed
			fprintf( stderr, "Created %d, destroyed %d\n", properties_created, properties_destroyed );
#endif

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
	}
}

