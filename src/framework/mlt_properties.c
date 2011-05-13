/**
 * \file mlt_properties.c
 * \brief Properties class definition
 * \see mlt_properties_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
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

#include "mlt_properties.h"
#include "mlt_property.h"
#include "mlt_deque.h"
#include "mlt_log.h"
#include "mlt_factory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

/** \brief private implementation of the property list */

typedef struct
{
	int hash[ 199 ];
	char **name;
	mlt_property *value;
	int count;
	int size;
	mlt_properties mirror;
	int ref_count;
	pthread_mutex_t mutex;
}
property_list;

/* Memory leak checks */

//#define _MLT_PROPERTY_CHECKS_ 2
#ifdef _MLT_PROPERTY_CHECKS_
static int properties_created = 0;
static int properties_destroyed = 0;
#endif

/** Initialize a properties object that was already allocated.
 *
 * This does allocate its ::property_list, and it adds a reference count.
 * \public \memberof mlt_properties_s
 * \param self the properties structure to initialize
 * \param child an opaque pointer to a subclass object
 * \return true if failed
 */

int mlt_properties_init( mlt_properties self, void *child )
{
	if ( self != NULL )
	{
#ifdef _MLT_PROPERTY_CHECKS_
		// Increment number of properties created
		properties_created ++;
#endif

		// NULL all methods
		memset( self, 0, sizeof( struct mlt_properties_s ) );

		// Assign the child of the object
		self->child = child;

		// Allocate the local structure
		self->local = calloc( sizeof( property_list ), 1 );

		// Increment the ref count
		( ( property_list * )self->local )->ref_count = 1;
		pthread_mutex_init( &( ( property_list * )self->local )->mutex, NULL );;
	}

	// Check that initialisation was successful
	return self != NULL && self->local == NULL;
}

/** Create a properties object.
 *
 * This allocates the properties structure and calls mlt_properties_init() on it.
 * Free the properties object with mlt_properties_close().
 * \public \memberof mlt_properties_s
 * \return a new properties object
 */

mlt_properties mlt_properties_new( )
{
	// Construct a standalone properties object
	mlt_properties self = calloc( sizeof( struct mlt_properties_s ), 1 );

	// Initialise self
	mlt_properties_init( self, NULL );

	// Return the pointer
	return self;
}

static int load_properties( mlt_properties self, const char *filename )
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
				mlt_properties_parse( self, temp );
		}

		// Close the file
		fclose( file );
	}
	return file? 0 : errno;
}

/** Create a properties object by reading a .properties text file.
 *
 * Free the properties object with mlt_properties_close().
 * \deprecated Please start using mlt_properties_parse_yaml().
 * \public \memberof mlt_properties_s
 * \param filename the absolute file name
 * \return a new properties object
 */

mlt_properties mlt_properties_load( const char *filename )
{
	// Construct a standalone properties object
	mlt_properties self = mlt_properties_new( );

	if ( self != NULL )
		load_properties( self, filename );

	// Return the pointer
	return self;
}

/** Set properties from a preset.
 *
 * Presets are typically installed to $prefix/share/mlt/presets/{type}/{service}/[{profile}/]{name}.
 * For example, "/usr/share/mlt/presets/consumer/avformat/dv_ntsc_wide/DVD"
 * could be an encoding preset for a widescreen NTSC DVD Video.
 * Do not specify the type and service in the preset name parameter; these are
 * inferred automatically from the service to which you are applying the preset.
 * Using the example above and assuming you are calling this function on the
 * avformat consumer, the name passed to the function should simply be DVD.
 * Note that the profile portion of the path is optional, but a profile-specific
 * preset with the same name as a more generic one is given a higher priority.
 * \todo Look in a user-specific location - somewhere in the home directory.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the name of a preset in a well-known location or the explicit path
 * \return true if error
 */

int mlt_properties_preset( mlt_properties self, const char *name )
{
	struct stat stat_buff;

	// validate input
	if ( !( self && name && strlen( name ) ) )
		return 1;

	// See if name is an explicit file
	if ( ! stat( name, &stat_buff ) )
	{
		return load_properties( self, name );
	}
	else
	{
		// Look for profile-specific preset before a generic one.
		char *data          = getenv( "MLT_PRESETS_PATH" );
		const char *type    = mlt_properties_get( self, "mlt_type" );
		const char *service = mlt_properties_get( self, "mlt_service" );
		const char *profile = mlt_environment( "MLT_PROFILE" );
		int error = 0;

		if ( data )
		{
			data = strdup( data );
		}
		else
		{
			data = malloc( strlen( mlt_environment( "MLT_DATA" ) ) + 9 );
			strcpy( data, mlt_environment( "MLT_DATA" ) );
			strcat( data, "/presets" );
		}
		if ( data && type && service )
		{
			char *path = malloc( 5 + strlen(name) + strlen(data) + strlen(type) + strlen(service) + ( profile? strlen(profile) : 0 ) );
			sprintf( path, "%s/%s/%s/%s/%s", data, type, service, profile, name );
			if ( load_properties( self, path ) )
			{
				sprintf( path, "%s/%s/%s/%s", data, type, service, name );
				error = load_properties( self, path );
			}
			free( path );
		}
		else
		{
			error = 1;
		}
		free( data );
		return error;
	}
}

/** Generate a hash key.
 *
 * \private \memberof mlt_properties_s
 * \param name a string
 * \return an integer
 */

static inline int generate_hash( const char *name )
{
	int hash = 0;
	int i = 1;
	while ( *name )
		hash = ( hash + ( i ++ * ( *name ++ & 31 ) ) ) % 199;
	return hash;
}

/** Copy a serializable property to a properties list that is mirroring this one.
 *
 * Special case - when a container (such as loader) is protecting another
 * producer, we need to ensure that properties are passed through to the
 * real producer.
 * \private \memberof mlt_properties_s
 * \param self a properties list
 * \param name the name of the property to copy
 */

static inline void mlt_properties_do_mirror( mlt_properties self, const char *name )
{
	property_list *list = self->local;
	if ( list->mirror != NULL )
	{
		char *value = mlt_properties_get( self, name );
		if ( value != NULL )
			mlt_properties_set( list->mirror, name, value );
	}
}

/** Increment the reference count.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \return the new reference count
 */

int mlt_properties_inc_ref( mlt_properties self )
{
	int result = 0;
	if ( self != NULL )
	{
		property_list *list = self->local;
		pthread_mutex_lock( &list->mutex );
		result = ++ list->ref_count;
		pthread_mutex_unlock( &list->mutex );
	}
	return result;
}

/** Decrement the reference count.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \return the new reference count
 */

int mlt_properties_dec_ref( mlt_properties self )
{
	int result = 0;
	if ( self != NULL )
	{
		property_list *list = self->local;
		pthread_mutex_lock( &list->mutex );
		result = -- list->ref_count;
		pthread_mutex_unlock( &list->mutex );
	}
	return result;
}

/** Get the reference count.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \return the current reference count
 */

int mlt_properties_ref_count( mlt_properties self )
{
	if ( self != NULL )
	{
		property_list *list = self->local;
		return list->ref_count;
	}
	return 0;
}

/** Set a properties list to be a mirror copy of another.
 *
 * Note that this does not copy all existing properties. Rather, you must
 * call this before setting the properties that you wish to copy.
 * \public \memberof mlt_properties_s
 * \param that the properties which will receive copies of the properties as they are set.
 * \param self the properties to mirror
 */

void mlt_properties_mirror( mlt_properties self, mlt_properties that )
{
	property_list *list = self->local;
	list->mirror = that;
}

/** Copy all serializable properties to another properties list.
 *
 * \public \memberof mlt_properties_s
 * \param self The properties to copy to
 * \param that The properties to copy from
 * \return false
 */

int mlt_properties_inherit( mlt_properties self, mlt_properties that )
{
	int count = mlt_properties_count( that );
	int i = 0;
	for ( i = 0; i < count; i ++ )
	{
		char *value = mlt_properties_get_value( that, i );
		if ( value != NULL )
		{
			char *name = mlt_properties_get_name( that, i );
			mlt_properties_set( self, name, value );
		}
	}
	return 0;
}

/** Pass all serializable properties that match a prefix to another properties object
 *
 * \public \memberof mlt_properties_s
 * \param self the properties to copy to
 * \param that The properties to copy from
 * \param prefix the property names to match (required)
 * \return false
 */

int mlt_properties_pass( mlt_properties self, mlt_properties that, const char *prefix )
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
				mlt_properties_set( self, name + length, value );
		}
	}
	return 0;
}

/** Locate a property by name.
 *
 * \private \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to lookup by name
 * \return the property or NULL for failure
 */

static inline mlt_property mlt_properties_find( mlt_properties self, const char *name )
{
	property_list *list = self->local;
	mlt_property value = NULL;
	int key = generate_hash( name );

	mlt_properties_lock( self );

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
	mlt_properties_unlock( self );

	return value;
}

/** Add a new property.
 *
 * \private \memberof mlt_properties_s
 * \param self a properties list
 * \param name the name of the new property
 * \return the new property
 */

static mlt_property mlt_properties_add( mlt_properties self, const char *name )
{
	property_list *list = self->local;
	int key = generate_hash( name );
	mlt_property result;

	mlt_properties_lock( self );

	// Check that we have space and resize if necessary
	if ( list->count == list->size )
	{
		list->size += 50;
		list->name = realloc( list->name, list->size * sizeof( const char * ) );
		list->value = realloc( list->value, list->size * sizeof( mlt_property ) );
	}

	// Assign name/value pair
	list->name[ list->count ] = strdup( name );
	list->value[ list->count ] = mlt_property_init( );

	// Assign to hash table
	if ( list->hash[ key ] == 0 )
		list->hash[ key ] = list->count + 1;

	// Return and increment count accordingly
	result = list->value[ list->count ++ ];

	mlt_properties_unlock( self );

	return result;
}

/** Fetch a property by name and add one if not found.
 *
 * \private \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to lookup or add
 * \return the property
 */

static mlt_property mlt_properties_fetch( mlt_properties self, const char *name )
{
	// Try to find an existing property first
	mlt_property property = mlt_properties_find( self, name );

	// If it wasn't found, create one
	if ( property == NULL )
		property = mlt_properties_add( self, name );

	// Return the property
	return property;
}

/** Copy a property to another properties list.
 *
 * \public \memberof mlt_properties_s
 * \author Zach <zachary.drew@gmail.com>
 * \param self the properties to copy to
 * \param that the properties to copy from
 * \param name the name of the property to copy
 */

void mlt_properties_pass_property( mlt_properties self, mlt_properties that, const char *name )
{
	// Make sure the source property isn't null.
	mlt_property that_prop = mlt_properties_find( that, name );
	if( that_prop == NULL )
		return;

	mlt_property_pass( mlt_properties_fetch( self, name ), that_prop );
}

/** Copy all properties specified in a comma-separated list to another properties list.
 *
 * White space is also a delimiter.
 * \public \memberof mlt_properties_s
 * \author Zach <zachary.drew@gmail.com>
 * \param self the properties to copy to
 * \param that the properties to copy from
 * \param list a delimited list of property names
 * \return false
 */


int mlt_properties_pass_list( mlt_properties self, mlt_properties that, const char *list )
{
	char *props = strdup( list );
	char *ptr = props;
	const char *delim = " ,\t\n";	// Any combination of spaces, commas, tabs, and newlines
	int count, done = 0;

	while( !done )
	{
		count = strcspn( ptr, delim );

		if( ptr[count] == '\0' )
			done = 1;
		else
			ptr[count] = '\0';	// Make it a real string

		mlt_properties_pass_property( self, that, ptr );

		ptr += count + 1;
		ptr += strspn( ptr, delim );
	}

	free( props );

	return 0;
}


/** Set a property to a string.
 *
 * The property name "properties" is reserved to load the preset in \p value.
 * When the value begins with '@' then it is interpreted as a very simple math
 * expression containing only the +, -, *, and / operators.
 * The event "property-changed" is fired after the property has been set.
 *
 * This makes a copy of the string value you supply.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to set
 * \param value the property's new value
 * \return true if error
 */

int mlt_properties_set( mlt_properties self, const char *name, const char *value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( self, name );

	// Set it if not NULL
	if ( property == NULL )
	{
		mlt_log( NULL, MLT_LOG_FATAL, "Whoops - %s not found (should never occur)\n", name );
	}
	else if ( value == NULL )
	{
		error = mlt_property_set_string( property, value );
		mlt_properties_do_mirror( self, name );
	}
	else if ( *value != '@' )
	{
		error = mlt_property_set_string( property, value );
		mlt_properties_do_mirror( self, name );
		if ( !strcmp( name, "properties" ) )
			mlt_properties_preset( self, value );
	}
	else if ( value[ 0 ] == '@' )
	{
		double total = 0;
		double current = 0;
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
				current = mlt_properties_get_double( self, id );

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
					total = total / current;
					break;
			}

			// Get the next op
			op = *value != '\0' ? *value ++ : ' ';
		}

		error = mlt_property_set_double( property, total );
		mlt_properties_do_mirror( self, name );
	}

	mlt_events_fire( self, "property-changed", name, NULL );

	return error;
}

/** Set or default a property to a string.
 *
 * This makes a copy of the string value you supply.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to set
 * \param value the string value to set or NULL to use the default
 * \param def the default string if value is NULL
 * \return true if error
 */

int mlt_properties_set_or_default( mlt_properties self, const char *name, const char *value, const char *def )
{
	return mlt_properties_set( self, name, value == NULL ? def : value );
}

/** Get a string value by name.
 *
 * Do not free the returned string. It's lifetime is controlled by the property
 * and this properties object.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \return the property's string value or NULL if it does not exist
 */

char *mlt_properties_get( mlt_properties self, const char *name )
{
	mlt_property value = mlt_properties_find( self, name );
	return value == NULL ? NULL : mlt_property_get_string( value );
}

/** Get a property name by index.
 *
 * Do not free the returned string.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param index the numeric index of the property
 * \return the name of the property or NULL if index is out of range
 */

char *mlt_properties_get_name( mlt_properties self, int index )
{
	property_list *list = self->local;
	if ( index >= 0 && index < list->count )
		return list->name[ index ];
	return NULL;
}

/** Get a property's string value by index.
 *
 * Do not free the returned string.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param index the numeric index of the property
 * \return the property value as a string or NULL if the index is out of range
 */

char *mlt_properties_get_value( mlt_properties self, int index )
{
	property_list *list = self->local;
	if ( index >= 0 && index < list->count )
		return mlt_property_get_string( list->value[ index ] );
	return NULL;
}

/** Get a data value by index.
 *
 * Do not free the returned pointer if you supplied a destructor function when you
 * set this property.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param index the numeric index of the property
 * \param[out] size the size of the binary data in bytes or NULL if the index is out of range
 */

void *mlt_properties_get_data_at( mlt_properties self, int index, int *size )
{
	property_list *list = self->local;
	if ( index >= 0 && index < list->count )
		return mlt_property_get_data( list->value[ index ], size );
	return NULL;
}

/** Return the number of items in the list.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \return the number of property objects
 */

int mlt_properties_count( mlt_properties self )
{
	property_list *list = self->local;
	return list->count;
}

/** Set a value by parsing a name=value string.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param namevalue a string containing name and value delimited by '='
 * \return true if there was an error
 */

int mlt_properties_parse( mlt_properties self, const char *namevalue )
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

	error = mlt_properties_set( self, name, value );

	free( name );
	free( value );

	return error;
}

/** Get an integer associated to the name.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \return The integer value, 0 if not found (which may also be a legitimate value)
 */

int mlt_properties_get_int( mlt_properties self, const char *name )
{
	mlt_property value = mlt_properties_find( self, name );
	return value == NULL ? 0 : mlt_property_get_int( value );
}

/** Set a property to an integer value.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to set
 * \param value the integer
 * \return true if error
 */

int mlt_properties_set_int( mlt_properties self, const char *name, int value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( self, name );

	// Set it if not NULL
	if ( property != NULL )
	{
		error = mlt_property_set_int( property, value );
		mlt_properties_do_mirror( self, name );
	}

	mlt_events_fire( self, "property-changed", name, NULL );

	return error;
}

/** Get a 64-bit integer associated to the name.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \return the integer value, 0 if not found (which may also be a legitimate value)
 */

int64_t mlt_properties_get_int64( mlt_properties self, const char *name )
{
	mlt_property value = mlt_properties_find( self, name );
	return value == NULL ? 0 : mlt_property_get_int64( value );
}

/** Set a property to a 64-bit integer value.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to set
 * \param value the integer
 * \return true if error
 */

int mlt_properties_set_int64( mlt_properties self, const char *name, int64_t value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( self, name );

	// Set it if not NULL
	if ( property != NULL )
	{
		error = mlt_property_set_int64( property, value );
		mlt_properties_do_mirror( self, name );
	}

	mlt_events_fire( self, "property-changed", name, NULL );

	return error;
}

/** Get a floating point value associated to the name.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \return the floating point, 0 if not found (which may also be a legitimate value)
 */

double mlt_properties_get_double( mlt_properties self, const char *name )
{
	mlt_property value = mlt_properties_find( self, name );
	return value == NULL ? 0 : mlt_property_get_double( value );
}

/** Set a property to a floating point value.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to set
 * \param value the floating point value
 * \return true if error
 */

int mlt_properties_set_double( mlt_properties self, const char *name, double value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( self, name );

	// Set it if not NULL
	if ( property != NULL )
	{
		error = mlt_property_set_double( property, value );
		mlt_properties_do_mirror( self, name );
	}

	mlt_events_fire( self, "property-changed", name, NULL );

	return error;
}

/** Get a position value associated to the name.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \return the position, 0 if not found (which may also be a legitimate value)
 */

mlt_position mlt_properties_get_position( mlt_properties self, const char *name )
{
	mlt_property value = mlt_properties_find( self, name );
	return value == NULL ? 0 : mlt_property_get_position( value );
}

/** Set a property to a position value.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \param value the position
 * \return true if error
 */

int mlt_properties_set_position( mlt_properties self, const char *name, mlt_position value )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( self, name );

	// Set it if not NULL
	if ( property != NULL )
	{
		error = mlt_property_set_position( property, value );
		mlt_properties_do_mirror( self, name );
	}

	mlt_events_fire( self, "property-changed", name, NULL );

	return error;
}

/** Get a binary data value associated to the name.
 *
 * Do not free the returned pointer if you supplied a destructor function
 * when you set this property.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to get
 * \param[out] length The size of the binary data in bytes, if available (often it is not, you should know)
 */

void *mlt_properties_get_data( mlt_properties self, const char *name, int *length )
{
	mlt_property value = mlt_properties_find( self, name );
	return value == NULL ? NULL : mlt_property_get_data( value, length );
}

/** Store binary data as a property.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param name the property to set
 * \param value an opaque pointer to binary data
 * \param length the size of the binary data in bytes (optional)
 * \param destroy a function to deallocate the binary data when the property is closed (optional)
 * \param serialise a function that can serialize the binary data as text (optional)
 * \return true if error
 */

int mlt_properties_set_data( mlt_properties self, const char *name, void *value, int length, mlt_destructor destroy, mlt_serialiser serialise )
{
	int error = 1;

	// Fetch the property to work with
	mlt_property property = mlt_properties_fetch( self, name );

	// Set it if not NULL
	if ( property != NULL )
		error = mlt_property_set_data( property, value, length, destroy, serialise );

	mlt_events_fire( self, "property-changed", name, NULL );

	return error;
}

/** Rename a property.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param source the property to rename
 * \param dest the new name
 * \return true if the name is already in use
 */

int mlt_properties_rename( mlt_properties self, const char *source, const char *dest )
{
	mlt_property value = mlt_properties_find( self, dest );

	if ( value == NULL )
	{
		property_list *list = self->local;
		int i = 0;

		// Locate the item
		mlt_properties_lock( self );
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
		mlt_properties_unlock( self );
	}

	return value != NULL;
}

/** Dump the properties to a file handle.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param output a file handle
 */

void mlt_properties_dump( mlt_properties self, FILE *output )
{
	property_list *list = self->local;
	int i = 0;
	for ( i = 0; i < list->count; i ++ )
		if ( mlt_properties_get( self, list->name[ i ] ) != NULL )
			fprintf( output, "%s=%s\n", list->name[ i ], mlt_properties_get( self, list->name[ i ] ) );
}

/** Output the properties to a file handle.
 *
 * This version includes reference counts and does not put each property on a new line.
 * \public \memberof mlt_properties_s
 * \param self a properties pointer
 * \param title a string to preface the output
 * \param output a file handle
 */
void mlt_properties_debug( mlt_properties self, const char *title, FILE *output )
{
	if ( output == NULL ) output = stderr;
	fprintf( output, "%s: ", title );
	if ( self != NULL )
	{
		property_list *list = self->local;
		int i = 0;
		fprintf( output, "[ ref=%d", list->ref_count );
		for ( i = 0; i < list->count; i ++ )
			if ( mlt_properties_get( self, list->name[ i ] ) != NULL )
				fprintf( output, ", %s=%s", list->name[ i ], mlt_properties_get( self, list->name[ i ] ) );
			else
				fprintf( output, ", %s=%p", list->name[ i ], mlt_properties_get_data( self, list->name[ i ], NULL ) );
		fprintf( output, " ]" );
	}
	fprintf( output, "\n" );
}

/** Save the properties to a file by name.
 *
 * This uses the dump format - one line per property.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param filename the name of a file to create or overwrite
 * \return true if there was an error
 */

int mlt_properties_save( mlt_properties self, const char *filename )
{
	int error = 1;
	FILE *f = fopen( filename, "w" );
	if ( f != NULL )
	{
		mlt_properties_dump( self, f );
		fclose( f );
		error = 0;
	}
	return error;
}

/* This is a very basic cross platform fnmatch replacement - it will fail in
 * many cases, but for the basic *.XXX and YYY*.XXX, it will work ok.
 */

/** Test whether a filename or pathname matches a shell-style pattern.
 *
 * \private \memberof mlt_properties_s
 * \param wild a string containing a wildcard pattern
 * \param file the name of a file to test against
 * \return true if the file name matches the wildcard pattern
 */

static int mlt_fnmatch( const char *wild, const char *file )
{
	int f = 0;
	int w = 0;

	while( f < strlen( file ) && w < strlen( wild ) )
	{
		if ( wild[ w ] == '*' )
		{
			w ++;
			if ( w == strlen( wild ) )
				f = strlen( file );
			while ( f != strlen( file ) && tolower( file[ f ] ) != tolower( wild[ w ] ) )
				f ++;
		}
		else if ( wild[ w ] == '?' || tolower( file[ f ] ) == tolower( wild[ w ] ) )
		{
			f ++;
			w ++;
		}
		else if ( wild[ 0 ] == '*' )
		{
			w = 0;
		}
		else
		{
			return 0;
		}
	}

	return strlen( file ) == f &&  strlen( wild ) == w;
}

/** Compare the string or serialized value of two properties.
 *
 * \private \memberof mlt_properties_s
 * \param self a property
 * \param that a property
 * \return < 0 if \p self less than \p that, 0 if equal, or > 0 if \p self is greater than \p that
 */

static int mlt_compare( const void *self, const void *that )
{
	return strcmp( mlt_property_get_string( *( const mlt_property * )self ), mlt_property_get_string( *( const mlt_property * )that ) );
}

/** Get the contents of a directory.
 *
 * Obtains an optionally sorted list of the files found in a directory with a specific wild card.
 * Entries in the list have a numeric name (running from 0 to count - 1). Only values change
 * position if sort is enabled. Designed to be posix compatible (linux, os/x, mingw etc).
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \param dirname the name of the directory
 * \param pattern a wildcard pattern to filter the directory listing
 * \param sort Do you want to sort the directory listing?
 * \return the number of items in the directory listing
 */

int mlt_properties_dir_list( mlt_properties self, const char *dirname, const char *pattern, int sort )
{
	DIR *dir = opendir( dirname );

	if ( dir )
	{
		char key[ 20 ];
		struct dirent *de = readdir( dir );
		char fullname[ 1024 ];
		while( de != NULL )
		{
			sprintf( key, "%d", mlt_properties_count( self ) );
			snprintf( fullname, 1024, "%s/%s", dirname, de->d_name );
			if ( pattern == NULL )
				mlt_properties_set( self, key, fullname );
			else if ( de->d_name[ 0 ] != '.' && mlt_fnmatch( pattern, de->d_name ) )
				mlt_properties_set( self, key, fullname );
			de = readdir( dir );
		}

		closedir( dir );
	}

	if ( sort && mlt_properties_count( self ) )
	{
		property_list *list = self->local;
		mlt_properties_lock( self );
		qsort( list->value, mlt_properties_count( self ), sizeof( mlt_property ), mlt_compare );
		mlt_properties_unlock( self );
	}

	return mlt_properties_count( self );
}

/** Close a properties object.
 *
 * Deallocates the properties object and everything it contains.
 * \public \memberof mlt_properties_s
 * \param self a properties object
 */

void mlt_properties_close( mlt_properties self )
{
	if ( self != NULL && mlt_properties_dec_ref( self ) <= 0 )
	{
		if ( self->close != NULL )
		{
			self->close( self->close_object );
		}
		else
		{
			property_list *list = self->local;
			int index = 0;

#if _MLT_PROPERTY_CHECKS_ == 1
			// Show debug info
			mlt_properties_debug( self, "Closing", stderr );
#endif

#ifdef _MLT_PROPERTY_CHECKS_
			// Increment destroyed count
			properties_destroyed ++;

			// Show current stats - these should match when the app is closed
			mlt_log( NULL, MLT_LOG_DEBUG, "Created %d, destroyed %d\n", properties_created, properties_destroyed );
#endif

			// Clean up names and values
			for ( index = list->count - 1; index >= 0; index -- )
			{
				mlt_property_close( list->value[ index ] );
				free( list->name[ index ] );
			}

			// Clear up the list
			pthread_mutex_destroy( &list->mutex );
			free( list->name );
			free( list->value );
			free( list );

			// Free self now if self has no child
			if ( self->child == NULL )
				free( self );
		}
	}
}

/** Determine if the properties list is really just a sequence or ordered list.
 *
 * \public \memberof mlt_properties_s
 * \param properties a properties list
 * \return true if all of the property names are numeric (a sequence)
 */

int mlt_properties_is_sequence( mlt_properties properties )
{
	int i;
	int n = mlt_properties_count( properties );
	for ( i = 0; i < n; i++ )
		if ( ! isdigit( mlt_properties_get_name( properties, i )[0] ) )
			return 0;
	return 1;
}

/** \brief YAML Tiny Parser context structure
 *
 * YAML is a nifty text format popular in the Ruby world as a cleaner,
 * less verbose alternative to XML. See this Wikipedia topic for an overview:
 * http://en.wikipedia.org/wiki/YAML
 * The YAML specification is at:
 * http://yaml.org/
 * YAML::Tiny is a Perl module that specifies a subset of YAML that we are
 * using here (for the same reasons):
 * http://search.cpan.org/~adamk/YAML-Tiny-1.25/lib/YAML/Tiny.pm
 * \private
 */

struct yaml_parser_context
{
	mlt_deque stack;
	unsigned int level;
	unsigned int index;
	char block;
	char *block_name;
	unsigned int block_indent;

};
typedef struct yaml_parser_context *yaml_parser;

/** Remove spaces from the left side of a string.
 *
 * \param s the string to trim
 * \return the number of characters removed
 */

static unsigned int ltrim( char **s )
{
	unsigned int i = 0;
	char *c = *s;
	int n = strlen( c );
	for ( i = 0; i < n && *c == ' '; i++, c++ );
	*s = c;
	return i;
}

/** Remove spaces from the right side of a string.
 *
 * \param s the string to trim
 * \return the number of characters removed
 */

static unsigned int rtrim( char *s )
{
	int n = strlen( s );
	int i;
	for ( i = n; i > 0 && s[i - 1] == ' '; --i )
		s[i - 1] = 0;
	return n - i;
}

/** Parse a line of YAML Tiny.
 *
 * Adds a property if needed.
 * \private \memberof yaml_parser_context
 * \param context a YAML Tiny Parser context
 * \param namevalue a line of YAML Tiny
 * \return true if there was an error
 */

static int parse_yaml( yaml_parser context, const char *namevalue )
{
	char *name_ = strdup( namevalue );
	char *name = name_;
	char *value = NULL;
	int error = 0;
	char *ptr = strchr( name, ':' );
	unsigned int indent = ltrim( &name );
	mlt_properties properties = mlt_deque_peek_front( context->stack );

	// Ascending one more levels in the tree
	if ( indent < context->level )
	{
		unsigned int i;
		unsigned int n = ( context->level - indent ) / 2;
		for ( i = 0; i < n; i++ )
			mlt_deque_pop_front( context->stack );
		properties = mlt_deque_peek_front( context->stack );
		context->level = indent;
	}

	// Descending a level in the tree
	else if ( indent > context->level && context->block == 0 )
	{
		context->level = indent;
	}

	// If there is a colon that is not part of a block
	if ( ptr && ( indent == context->level ) )
	{
		// Reset block processing
		if ( context->block_name )
		{
			free( context->block_name );
			context->block_name = NULL;
			context->block = 0;
		}

		// Terminate the name and setup the value pointer
		*( ptr ++ ) = 0;

		// Trim comment
		char *comment = strchr( ptr, '#' );
		if ( comment )
		{
			*comment = 0;
		}

		// Trim leading and trailing spaces from bare value
		ltrim( &ptr );
		rtrim( ptr );

		// No value means a child
		if ( strcmp( ptr, "" ) == 0 )
		{
			mlt_properties child = mlt_properties_new();
			mlt_properties_set_data( properties, name, child, 0,
				( mlt_destructor )mlt_properties_close, NULL );
			mlt_deque_push_front( context->stack, child );
			context->index = 0;
			free( name_ );
			return error;
		}

		// A dash indicates a sequence item
		if ( name[0] == '-' )
		{
			mlt_properties child = mlt_properties_new();
			char key[20];

			snprintf( key, sizeof(key), "%d", context->index++ );
			mlt_properties_set_data( properties, key, child, 0,
				( mlt_destructor )mlt_properties_close, NULL );
			mlt_deque_push_front( context->stack, child );

			name ++;
			context->level += ltrim( &name ) + 1;
			properties = child;
		}

		// Value is quoted
		if ( *ptr == '\"' )
		{
			ptr ++;
			value = strdup( ptr );
			if ( value && value[ strlen( value ) - 1 ] == '\"' )
				value[ strlen( value ) - 1 ] = 0;
		}

		// Value is folded or unfolded block
		else if ( *ptr == '|' || *ptr == '>' )
		{
			context->block = *ptr;
			context->block_name = strdup( name );
			context->block_indent = 0;
			value = strdup( "" );
		}

		// Bare value
		else
		{
			value = strdup( ptr );
		}
	}

	// A list of scalars
	else if ( name[0] == '-' )
	{
		// Reset block processing
		if ( context->block_name )
		{
			free( context->block_name );
			context->block_name = NULL;
			context->block = 0;
		}

		char key[20];

		snprintf( key, sizeof(key), "%d", context->index++ );
		ptr = name + 1;

		// Trim comment
		char *comment = strchr( ptr, '#' );
		if ( comment )
			*comment = 0;

		// Trim leading and trailing spaces from bare value
		ltrim( &ptr );
		rtrim( ptr );

		// Value is quoted
		if ( *ptr == '\"' )
		{
			ptr ++;
			value = strdup( ptr );
			if ( value && value[ strlen( value ) - 1 ] == '\"' )
				value[ strlen( value ) - 1 ] = 0;
		}

		// Value is folded or unfolded block
		else if ( *ptr == '|' || *ptr == '>' )
		{
			context->block = *ptr;
			context->block_name = strdup( key );
			context->block_indent = 0;
			value = strdup( "" );
		}

		// Bare value
		else
		{
			value = strdup( ptr );
		}

		free( name_ );
		name = name_ = strdup( key );
	}

	// Non-folded block
	else if ( context->block == '|' )
	{
		if ( context->block_indent == 0 )
			context->block_indent = indent;
		if ( indent > context->block_indent )
			name = &name_[ context->block_indent ];
		rtrim( name );
		char *old_value = mlt_properties_get( properties, context->block_name );
		value = calloc( 1, strlen( old_value ) + strlen( name ) + 2 );
		strcpy( value, old_value );
		if ( strcmp( old_value, "" ) )
			strcat( value, "\n" );
		strcat( value, name );
		name = context->block_name;
	}

	// Folded block
	else if ( context->block == '>' )
	{
		ltrim( &name );
		rtrim( name );
		char *old_value = mlt_properties_get( properties, context->block_name );

		// Blank line (prepended with spaces) is new line
		if ( strcmp( name, "" ) == 0 )
		{
			value = calloc( 1, strlen( old_value ) + 2 );
			strcat( value, old_value );
			strcat( value, "\n" );
		}
		// Concatenate with space
		else
		{
			value = calloc( 1, strlen( old_value ) + strlen( name ) + 2 );
			strcat( value, old_value );
			if ( strcmp( old_value, "" ) && old_value[ strlen( old_value ) - 1 ] != '\n' )
				strcat( value, " " );
			strcat( value, name );
		}
		name = context->block_name;
	}

	else
	{
		value = strdup( "" );
	}

	error = mlt_properties_set( properties, name, value );

	free( name_ );
	free( value );

	return error;
}

/** Parse a YAML Tiny file by name.
 *
 * \public \memberof mlt_properties_s
 * \param filename the name of a text file containing YAML Tiny
 * \return a new properties list
 */

mlt_properties mlt_properties_parse_yaml( const char *filename )
{
	// Construct a standalone properties object
	mlt_properties self = mlt_properties_new( );

	if ( self )
	{
		// Open the file
		FILE *file = fopen( filename, "r" );

		// Load contents of file
		if ( file )
		{
			// Temp string
			char temp[ 1024 ];
			char *ptemp = &temp[ 0 ];

			// Parser context
			yaml_parser context = calloc( 1, sizeof( struct yaml_parser_context ) );
			context->stack = mlt_deque_init();
			mlt_deque_push_front( context->stack, self );

			// Read each string from the file
			while( fgets( temp, 1024, file ) )
			{
				// Check for end-of-stream
				if ( strncmp( ptemp, "...", 3 ) == 0 )
					break;

				// Chomp the string
				temp[ strlen( temp ) - 1 ] = '\0';

				// Skip blank lines, comment lines, and document separator
				if ( strcmp( ptemp, "" ) && ptemp[ 0 ] != '#' && strncmp( ptemp, "---", 3 )
				     && strncmp( ptemp, "%YAML", 5 ) && strncmp( ptemp, "% YAML", 6 ) )
					parse_yaml( context, temp );
			}

			// Close the file
			fclose( file );
			mlt_deque_close( context->stack );
			if ( context->block_name )
				free( context->block_name );
			free( context );
		}
	}

	// Return the pointer
	return self;
}

/*
 * YAML Tiny Serializer
 */

/** How many bytes to grow at a time */
#define STRBUF_GROWTH (1024)

/** \brief Private to mlt_properties_s, a self-growing buffer for building strings
 * \private
 */

struct strbuf_s
{
	size_t size;
	char *string;
};

typedef struct strbuf_s *strbuf;

/** Create a new string buffer
 *
 * \private \memberof strbuf_s
 * \return a new string buffer
 */

static strbuf strbuf_new( )
{
	strbuf buffer = calloc( 1, sizeof( struct strbuf_s ) );
	buffer->size = STRBUF_GROWTH;
	buffer->string = calloc( 1, buffer->size );
	return buffer;
}

/** Destroy a string buffer
 *
 * \private \memberof strbuf_s
 * \param buffer the string buffer to close
 */

static void strbuf_close( strbuf buffer )
{
	// We do not free buffer->string; strbuf user must save that pointer
	// and free it.
	if ( buffer )
		free( buffer );
}

/** Format a string into a string buffer
 *
 * A variable number of arguments follows the format string - one for each
 * format specifier.
 * \private \memberof strbuf_s
 * \param buffer the string buffer to write into
 * \param format a string that contains text and formatting instructions
 * \return the formatted string
 */

static char *strbuf_printf( strbuf buffer, const char *format, ... )
{
	while ( buffer->string )
	{
		va_list ap;
		va_start( ap, format );
		size_t len = strlen( buffer->string );
		size_t remain = buffer->size - len - 1;
		int need = vsnprintf( buffer->string + len, remain, format, ap );
		va_end( ap );
		if ( need > -1 && need < remain )
			break;
		buffer->string[ len ] = 0;
		buffer->size += need + STRBUF_GROWTH;
		buffer->string = realloc( buffer->string, buffer->size );
	}
	return buffer->string;
}

/** Indent a line of YAML Tiny.
 *
 * \private \memberof strbuf_s
 * \param output a string buffer
 * \param indent the number of spaces to indent
 */

static inline void indent_yaml( strbuf output, int indent )
{
	int j;
	for ( j = 0; j < indent; j++ )
		strbuf_printf( output, " " );
}

/** Convert a line string into a YAML block literal.
 *
 * \private \memberof strbuf_s
 * \param output a string buffer
 * \param value the string to format as a block literal
 * \param indent the number of spaces to indent
 */

static void output_yaml_block_literal( strbuf output, const char *value, int indent )
{
	char *v = strdup( value );
	char *sol = v;
	char *eol = strchr( sol, '\n' );

	while ( eol )
	{
		indent_yaml( output, indent );
		*eol = '\0';
		strbuf_printf( output, "%s\n", sol );
		sol = eol + 1;
		eol = strchr( sol, '\n' );
	}
	indent_yaml( output, indent );
	strbuf_printf( output, "%s\n", sol );
}

/** Recursively serialize a properties list into a string buffer as YAML Tiny.
 *
 * \private \memberof mlt_properties_s
 * \param self a properties list
 * \param output a string buffer to hold the serialized YAML Tiny
 * \param indent the number of spaces to indent (for recursion, initialize to 0)
 * \param is_parent_sequence Is this properties list really just a sequence (for recursion, initialize to 0)?
 */

static void serialise_yaml( mlt_properties self, strbuf output, int indent, int is_parent_sequence )
{
	property_list *list = self->local;
	int i = 0;

	for ( i = 0; i < list->count; i ++ )
	{
		// This implementation assumes that all data elements are property lists.
		// Unfortunately, we do not have run time type identification.
		mlt_properties child = mlt_property_get_data( list->value[ i ], NULL );

		if ( mlt_properties_is_sequence( self ) )
		{
			// Ignore hidden/non-serialisable items
			if ( list->name[ i ][ 0 ] != '_' )
			{
				// Indicate a sequence item
				indent_yaml( output, indent );
				strbuf_printf( output, "- " );

				// If the value can be represented as a string
				const char *value = mlt_properties_get( self, list->name[ i ] );
				if ( value && strcmp( value, "" ) )
				{
					// Determine if this is an unfolded block literal
					if ( strchr( value, '\n' ) )
					{
						strbuf_printf( output, "|\n" );
						output_yaml_block_literal( output, value, indent + strlen( list->name[ i ] ) + strlen( "|" ) );
					}
					else
					{
						strbuf_printf( output, "%s\n", value );
					}
				}
			}
			// Recurse on child
			if ( child )
				serialise_yaml( child, output, indent + 2, 1 );
		}
		else
		{
			// Assume this is a normal map-oriented properties list
			const char *value = mlt_properties_get( self, list->name[ i ] );

			// Ignore hidden/non-serialisable items
			// If the value can be represented as a string
			if ( list->name[ i ][ 0 ] != '_' && value && strcmp( value, "" ) )
			{
				if ( is_parent_sequence == 0 )
					indent_yaml( output, indent );
				else
					is_parent_sequence = 0;

				// Determine if this is an unfolded block literal
				if ( strchr( value, '\n' ) )
				{
					strbuf_printf( output, "%s: |\n", list->name[ i ] );
					output_yaml_block_literal( output, value, indent + strlen( list->name[ i ] ) + strlen( ": " ) );
				}
				else
				{
					strbuf_printf( output, "%s: %s\n", list->name[ i ], value );
				}
			}

			// Output a child as a map item
			if ( child )
			{
				indent_yaml( output, indent );
				strbuf_printf( output, "%s:\n", list->name[ i ] );

				// Recurse on child
				serialise_yaml( child, output, indent + 2, 0 );
			}
		}
	}
}

/** Serialize a properties list as a string of YAML Tiny.
 *
 * The caller MUST free the returned string!
 * This operates on properties containing properties as a hierarchical data
 * structure.
 * \public \memberof mlt_properties_s
 * \param self a properties list
 * \return a string containing YAML Tiny that represents the properties list
 */

char *mlt_properties_serialise_yaml( mlt_properties self )
{
	strbuf b = strbuf_new();
	strbuf_printf( b, "---\n" );
	serialise_yaml( self, b, 0, 0 );
	strbuf_printf( b, "...\n" );
	char *ret = b->string;
	strbuf_close( b );
	return ret;
}

/** Protect a properties list against concurrent access.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 */

void mlt_properties_lock( mlt_properties self )
{
	if ( self )
		pthread_mutex_lock( &( ( property_list* )( self->local ) )->mutex );
}

/** End protecting a properties list against concurrent access.
 *
 * \public \memberof mlt_properties_s
 * \param self a properties list
 */

void mlt_properties_unlock( mlt_properties self )
{
	if ( self )
		pthread_mutex_unlock( &( ( property_list* )( self->local ) )->mutex );
}
