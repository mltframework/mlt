/*
 * mlt_factory.c -- the factory method interfaces
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
#include "mlt.h"
#include "mlt_repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Singleton repositories
*/

static char *mlt_prefix = NULL;
static mlt_properties global_properties = NULL;
static mlt_properties object_list = NULL;
static mlt_repository producers = NULL;
static mlt_repository filters = NULL;
static mlt_repository transitions = NULL;
static mlt_repository consumers = NULL;
static int unique_id = 0;

/** Construct the factories.
*/

int mlt_factory_init( char *prefix )
{
	// Only initialise once
	if ( mlt_prefix == NULL )
	{
		// Allow user over rides
		if ( prefix == NULL )
			prefix = getenv( "MLT_REPOSITORY" );

		// If no directory is specified, default to install directory
		if ( prefix == NULL )
			prefix = PREFIX_DATA;

		// Store the prefix for later retrieval
		mlt_prefix = strdup( prefix );

		// Initialise the pool
		mlt_pool_init( );

		// Create the global properties
		global_properties = mlt_properties_new( );
		mlt_properties_set_or_default( global_properties, "MLT_NORMALISATION", getenv( "MLT_NORMALISATION" ), "PAL" );
		mlt_properties_set_or_default( global_properties, "MLT_PRODUCER", getenv( "MLT_PRODUCER" ), "fezzik" );
		mlt_properties_set_or_default( global_properties, "MLT_CONSUMER", getenv( "MLT_CONSUMER" ), "sdl" );

		// Create the object list.
		object_list = mlt_properties_new( );

		// Create a repository for each service type
		producers = mlt_repository_init( object_list, prefix, "producers.dat", "mlt_create_producer" );
		filters = mlt_repository_init( object_list, prefix, "filters.dat", "mlt_create_filter" );
		transitions = mlt_repository_init( object_list, prefix, "transitions.dat", "mlt_create_transition" );
		consumers = mlt_repository_init( object_list, prefix, "consumers.dat", "mlt_create_consumer" );
	}

	return 0;
}

/** Fetch the prefix used in this instance.
*/

const char *mlt_factory_prefix( )
{
	return mlt_prefix;
}

/** Get a value from the environment.
*/

char *mlt_environment( char *name )
{
	return mlt_properties_get( global_properties, name );
}

/** Fetch a producer from the repository.
*/

mlt_producer mlt_factory_producer( char *service, void *input )
{
	mlt_producer obj = NULL;

	// Pick up the default normalising producer if necessary
	if ( service == NULL )
		service = mlt_environment( "MLT_PRODUCER" );

	// Try to instantiate via the specified service
	obj = mlt_repository_fetch( producers, service, input );

	if ( obj != NULL )
	{
		mlt_properties properties = mlt_producer_properties( obj );
		mlt_properties_set_int( properties, "_unique_id", ++ unique_id );
		mlt_properties_set( properties, "mlt_type", "producer" );
		if ( mlt_properties_get_int( properties, "_mlt_service_hidden" ) == 0 )
			mlt_properties_set( properties, "mlt_service", service );
	}
	return obj;
}

/** Fetch a filter from the repository.
*/

mlt_filter mlt_factory_filter( char *service, void *input )
{
	mlt_filter obj = mlt_repository_fetch( filters, service, input );
	if ( obj != NULL )
	{
		mlt_properties properties = mlt_filter_properties( obj );
		mlt_properties_set_int( properties, "_unique_id", ++ unique_id );
		mlt_properties_set( properties, "mlt_type", "filter" );
		mlt_properties_set( properties, "mlt_service", service );
	}
	return obj;
}

/** Fetch a transition from the repository.
*/

mlt_transition mlt_factory_transition( char *service, void *input )
{
	mlt_transition obj = mlt_repository_fetch( transitions, service, input );
	if ( obj != NULL )
	{
		mlt_properties properties = mlt_transition_properties( obj );
		mlt_properties_set_int( properties, "_unique_id", ++ unique_id );
		mlt_properties_set( properties, "mlt_type", "transition" );
		mlt_properties_set( properties, "mlt_service", service );
	}
	return obj;
}

/** Fetch a consumer from the repository
*/

mlt_consumer mlt_factory_consumer( char *service, void *input )
{
	mlt_consumer obj = NULL;

	if ( service == NULL )
		service = mlt_environment( "MLT_CONSUMER" );

	obj = mlt_repository_fetch( consumers, service, input );

	if ( obj != NULL )
	{
		mlt_properties properties = mlt_consumer_properties( obj );
		mlt_properties_set_int( properties, "_unique_id", ++ unique_id );
		mlt_properties_set( properties, "mlt_type", "consumer" );
		mlt_properties_set( properties, "mlt_service", service );
	}
	return obj;
}

/** Register an object for clean up.
*/

void mlt_factory_register_for_clean_up( void *ptr, mlt_destructor destructor )
{
	char unique[ 256 ];
	sprintf( unique, "%08d", mlt_properties_count( global_properties ) );
	mlt_properties_set_data( global_properties, unique, ptr, 0, destructor, NULL );
}

/** Close the factory.
*/

void mlt_factory_close( )
{
	if ( mlt_prefix != NULL )
	{
		mlt_repository_close( producers );
		mlt_repository_close( filters );
		mlt_repository_close( transitions );
		mlt_repository_close( consumers );
		mlt_properties_close( global_properties );
		mlt_properties_close( object_list );
		free( mlt_prefix );
		mlt_prefix = NULL;
		mlt_pool_close( );
	}
}

