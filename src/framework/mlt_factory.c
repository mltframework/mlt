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
#include "mlt_factory.h"
#include "mlt_repository.h"
#include "mlt_properties.h"

#include <stdlib.h>

/** Singleton repositories
*/

static mlt_properties object_list = NULL;
static mlt_repository producers = NULL;
static mlt_repository filters = NULL;
static mlt_repository transitions = NULL;
static mlt_repository consumers = NULL;

/** Construct the factories.
*/

int mlt_factory_init( char *prefix )
{
	// If no directory is specified, default to install directory
	if ( prefix == NULL )
		prefix = PREFIX_DATA;

	// Create the object list.
	object_list = calloc( sizeof( struct mlt_properties_s ), 1 );
	mlt_properties_init( object_list, NULL );

	// Create a repository for each service type
	producers = mlt_repository_init( object_list, prefix, "producers.dat", "mlt_create_producer" );
	filters = mlt_repository_init( object_list, prefix, "filters.dat", "mlt_create_filter" );
	transitions = mlt_repository_init( object_list, prefix, "transitions.dat", "mlt_create_transition" );
	consumers = mlt_repository_init( object_list, prefix, "consumers.dat", "mlt_create_consumer" );

	return 0;
}

/** Fetch a producer from the repository.
*/

mlt_producer mlt_factory_producer( char *service, void *input )
{
	return ( mlt_producer )mlt_repository_fetch( producers, service, input );
}

/** Fetch a filter from the repository.
*/

mlt_filter mlt_factory_filter( char *service, void *input )
{
	return ( mlt_filter )mlt_repository_fetch( filters, service, input );
}

/** Fetch a transition from the repository.
*/

mlt_transition mlt_factory_transition( char *service, void *input )
{
	return ( mlt_transition )mlt_repository_fetch( transitions, service, input );
}

/** Fetch a consumer from the repository
*/

mlt_consumer mlt_factory_consumer( char *service, void *input )
{
	return ( mlt_consumer )mlt_repository_fetch( consumers, service, input );
}

/** Close the factory.
*/

void mlt_factory_close( )
{
	mlt_repository_close( producers );
	mlt_repository_close( filters );
	mlt_repository_close( transitions );
	mlt_repository_close( consumers );
	mlt_properties_close( object_list );
	free( object_list );
}

