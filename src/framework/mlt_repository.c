/*
 * repository.c -- provides a map between service and shared objects
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

#include "mlt_repository.h"
#include "mlt_properties.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

struct mlt_repository_s
{
	struct mlt_properties_s parent;
	mlt_properties object_list;
};

static char *construct_full_file( char *output, char *prefix, char *file )
{
	strcpy( output, prefix );
	if ( prefix[ strlen( prefix ) - 1 ] != '/' )
		strcat( output, "/" );
	strcat( output, file );
	return output;
}

static char *chomp( char *input )
{
	if ( input[ strlen( input ) - 1 ] == '\n' )
		input[ strlen( input ) - 1 ] = '\0';
	return input;
}

static mlt_properties construct_object( char *prefix, char *id )
{
	mlt_properties output = calloc( sizeof( struct mlt_properties_s ), 1 );
	mlt_properties_init( output, NULL );
	mlt_properties_set( output, "prefix", prefix );
	mlt_properties_set( output, "id", id );
	return output;
}

static mlt_properties construct_service( mlt_properties object, char *id )
{
	mlt_properties output = calloc( sizeof( struct mlt_properties_s ), 1 );
	mlt_properties_init( output, NULL );
	mlt_properties_set_data( output, "object", object, 0, NULL, NULL );
	mlt_properties_set( output, "id", id );
	return output;
}

void *construct_instance( mlt_properties service_properties, char *symbol, void *input )
{
	// Extract the service
	char *service = mlt_properties_get( service_properties, "id" );

	// Get the object properties
	void *object_properties = mlt_properties_get_data( service_properties, "object", NULL );

	// Get the dlopen'd object
	void *object = mlt_properties_get_data( object_properties, "dlopen", NULL );

	// Get the dlsym'd symbol
	void *( *symbol_ptr )( char *, void * ) = mlt_properties_get_data( object_properties, symbol, NULL );

	// Check that we have object and open if we don't
	if ( object == NULL )
	{
		char full_file[ 512 ];

		// Get the prefix and id of the shared object
		char *prefix = mlt_properties_get( object_properties, "prefix" );
		char *file = mlt_properties_get( object_properties, "id" );

		// Construct the full file
		construct_full_file( full_file, prefix, file );

		// Open the shared object
		object = dlopen( full_file, RTLD_NOW | RTLD_GLOBAL );

		// Set it on the properties
		mlt_properties_set_data( object_properties, "dlopen", object, 0, ( void (*)( void * ) )dlclose, NULL );
	}

	// Now check if we have this symbol pointer
	if ( object != NULL && symbol_ptr == NULL )
	{
		// Construct it now
		symbol_ptr = dlsym( object, symbol );

		// Set it on the properties
		mlt_properties_set_data( object_properties, "dlsym", symbol_ptr, 0, NULL, NULL );
	}

	// Construct the service
	return symbol_ptr != NULL ? symbol_ptr( service, input ) : NULL;
}

void destroy_properties( void *arg )
{
	mlt_properties_close( arg );
	free( arg );
}

mlt_repository mlt_repository_init( mlt_properties object_list, char *prefix, char *data, char *symbol )
{
	char full_file[ 512 ];
	FILE *file;

	// Construct the repository
	mlt_repository this = calloc( sizeof( struct mlt_repository_s ), 1 );
	mlt_properties_init( &this->parent, NULL );

	// Add the symbol to THIS repository properties.
	mlt_properties_set( &this->parent, "_symbol", symbol );

	// Asociate the repository to the global object_list
	this->object_list = object_list;

	// Construct full file
	construct_full_file( full_file, prefix, data );

	// Open the file
	file = fopen( full_file, "r" );

	// Parse the contents
	if ( file != NULL )
	{
		char full[ 512 ];
		char service[ 256 ];
		char object[ 256 ];

		while( fgets( full, 512, file ) )
		{
			chomp( full );

			if ( full[ 0 ] != '#' && full[ 0 ] != '\0' && sscanf( full, "%s %s", service, object ) == 2 )
			{
				// Get the object properties first
				mlt_properties object_properties = mlt_properties_get_data( object_list, object, NULL );

				// If their are no properties, create them now
				if ( object_properties == NULL )
				{
					// Construct the object
					object_properties = construct_object( prefix, object );

					// Add it to the object list
					mlt_properties_set_data( object_list, object, object_properties, 0, destroy_properties, NULL );
				}

				// Now construct a property for the service
				mlt_properties service_properties = construct_service( object_properties, service );

				// Add it to the repository
				mlt_properties_set_data( &this->parent, service, service_properties, 0, destroy_properties, NULL );
			}
		}

		// Close the file
		fclose( file );
	}

	return this;
}

void *mlt_repository_fetch( mlt_repository this, char *service, void *input )
{
	// Get the service properties
	mlt_properties service_properties = mlt_properties_get_data( &this->parent, service, NULL );

	// If the service exists
	if ( service_properties != NULL )
	{
		// Get the symbol that is used to generate this service
		char *symbol = mlt_properties_get( &this->parent, "_symbol" );

		// Now get an instance of the service
		return construct_instance( service_properties, symbol, input );
	}

	return NULL;
}

void mlt_repository_close( mlt_repository this )
{
	mlt_properties_close( &this->parent );
	free( this );
}


