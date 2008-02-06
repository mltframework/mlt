/*
 * repository.c -- provides a map between service and shared objects
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "mlt_repository.h"
#include "mlt_properties.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

struct mlt_repository_s
{
	struct mlt_properties_s parent; // a list of object files
	mlt_properties consumers; // lists of entry points
	mlt_properties filters;
	mlt_properties producers;
	mlt_properties transitions;
};

mlt_repository mlt_repository_init( const char *prefix )
{
	// Construct the repository
	mlt_repository this = calloc( sizeof( struct mlt_repository_s ), 1 );
	mlt_properties_init( &this->parent, this );
	this->consumers = mlt_properties_new();
	this->filters = mlt_properties_new();
	this->producers = mlt_properties_new();
	this->transitions = mlt_properties_new();
	
	// Get the directory list
	mlt_properties dir = mlt_properties_new();
	int count = mlt_properties_dir_list( dir, prefix, NULL, 0 );
	int i;
	
	// Iterate over files
	for ( i = 0; i < count; i++ )
	{
		int flags = RTLD_NOW;
		const char *object_name = mlt_properties_get_value( dir, i);

		// Very temporary hack to allow the quicktime plugins to work
		// TODO: extend repository to allow this to be used on a case by case basis
		if ( strstr( object_name, "libmltkino" ) )
			flags |= RTLD_GLOBAL;

		// Open the shared object
		void *object = dlopen( object_name, flags );		
		if ( object != NULL )
		{
			// Get the registration function
			int ( *symbol_ptr )( mlt_repository ) = dlsym( object, "mlt_register" );
			
			// Call the registration function
			if ( symbol_ptr != NULL )
			{
				symbol_ptr( this );
				
				// Register the object file for closure
				mlt_properties_set_data( &this->parent, object_name, object, 0, ( mlt_destructor )dlclose, NULL );
			}
			else
			{
				dlclose( object );
			}
		}
	}
	
	return this;
}

void mlt_repository_register( mlt_repository this, mlt_service_type service_type, const char *service, void *symbol )
{
	// Add the entry point to the corresponding service list
	switch ( service_type )
	{
		case consumer_type:
			mlt_properties_set_data( this->consumers, service, symbol, 0, NULL, NULL );
			break;
		case filter_type:
			mlt_properties_set_data( this->filters, service, symbol, 0, NULL, NULL );
			break;
		case producer_type:
			mlt_properties_set_data( this->producers, service, symbol, 0, NULL, NULL );
			break;
		case transition_type:
			mlt_properties_set_data( this->transitions, service, symbol, 0, NULL, NULL );
			break;
		default:
			break;
	}
}

void *mlt_repository_fetch( mlt_repository this, mlt_profile profile, mlt_service_type type, const char *service, void *input )
{
	void *( *symbol_ptr )( mlt_profile, mlt_service_type, const char *, void * ) = NULL;

	// Get the entry point from the corresponding service list
	switch ( type )
	{
		case consumer_type:
			symbol_ptr = mlt_properties_get_data( this->consumers, service, NULL );
			break;
		case filter_type:
			symbol_ptr = mlt_properties_get_data( this->filters, service, NULL );
			break;
		case producer_type:
			symbol_ptr = mlt_properties_get_data( this->producers, service, NULL );
			break;
		case transition_type:
			symbol_ptr = mlt_properties_get_data( this->transitions, service, NULL );
			break;
		default:
			break;
	}
	
	// Construct the service
	return ( symbol_ptr != NULL ) ? symbol_ptr( profile, type, service, input ) : NULL;
}

void mlt_repository_close( mlt_repository this )
{
	mlt_properties_close( this->consumers );
	mlt_properties_close( this->filters );
	mlt_properties_close( this->producers );
	mlt_properties_close( this->transitions );
	mlt_properties_close( &this->parent );
	free( this );
}
