/*
 * producer_fezzik.c -- a normalising filter
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

#include "producer_fezzik.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>

#include <framework/mlt.h>

static mlt_properties dictionary = NULL;
static mlt_properties normalisers = NULL;

static void track_service( mlt_tractor tractor, void *service, mlt_destructor destructor )
{
	mlt_properties properties = mlt_tractor_properties( tractor );
	int registered = mlt_properties_get_int( properties, "_registered" );
	char *key = mlt_properties_get( properties, "_registered" );
	char *real = malloc( strlen( key ) + 2 );
	sprintf( real, "_%s", key );
	mlt_properties_set_data( properties, real, service, 0, destructor, NULL );
	mlt_properties_set_int( properties, "_registered", ++ registered );
	free( real );
}

static mlt_producer create_from( char *file, char *services )
{
	mlt_producer producer = NULL;
	char *temp = strdup( services );
	char *service = temp;
	do
	{
		char *p = strchr( service, ',' );
		if ( p != NULL )
			*p ++ = '\0';
		producer = mlt_factory_producer( service, file );
		service = p;
	}
	while ( producer == NULL && service != NULL );
	free( temp );
	return producer;
}

static mlt_producer create_producer( char *file )
{
	mlt_producer result = NULL;

	// 1st Line - check for service:resource handling
	if ( strchr( file, ':' ) )
	{
		char *temp = strdup( file );
		char *service = temp;
		char *resource = strchr( temp, ':' );
		*resource ++ = '\0';
		result = mlt_factory_producer( service, resource );
		free( temp );
	}

	// 2nd Line preferences
	if ( result == NULL )
	{
		int i = 0;
		char *lookup = strdup( file );
		char *p = lookup;

		// We only need to load the dictionary once
		if ( dictionary == NULL )
		{
			char temp[ 1024 ];
			sprintf( temp, "%s/fezzik.dict", mlt_factory_prefix( ) );
			dictionary = mlt_properties_load( temp );
			mlt_factory_register_for_clean_up( dictionary, ( mlt_destructor )mlt_properties_close );
		}

		// Convert the lookup string to lower case
		while ( *p )
		{
			*p = tolower( *p );
			p ++;
		}

		// Iterate through the dictionary
		for ( i = 0; result == NULL && i < mlt_properties_count( dictionary ); i ++ )
		{
			char *name = mlt_properties_get_name( dictionary, i );
			if ( fnmatch( name, lookup, 0 ) == 0 )
				result = create_from( file, mlt_properties_get_value( dictionary, i ) );
		}

		free( lookup );
	}

	// Finally, try just loading as service
	if ( result == NULL )
		result = mlt_factory_producer( file, NULL );

	return result;
}

static mlt_service create_filter( mlt_tractor tractor, mlt_service last, char *effect, int *created )
{
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_filter filter = mlt_factory_filter( id, arg );
	if ( filter != NULL )
	{
		mlt_filter_connect( filter, last, 0 );
		track_service( tractor, filter, ( mlt_destructor )mlt_filter_close );
		last = mlt_filter_service( filter );
		*created = 1;
	}
	free( id );
	return last;
}

static mlt_service attach_normalisers( mlt_tractor tractor, mlt_service last )
{
	// Loop variable
	int i;

	// Tokeniser
	mlt_tokeniser tokeniser = mlt_tokeniser_init( );

	// We only need to load the normalising properties once
	if ( normalisers == NULL )
	{
		char temp[ 1024 ];
		sprintf( temp, "%s/fezzik.ini", mlt_factory_prefix( ) );
		normalisers = mlt_properties_load( temp );
		mlt_factory_register_for_clean_up( normalisers, ( mlt_destructor )mlt_properties_close );
	}

	// Apply normalisers
	for ( i = 0; i < mlt_properties_count( normalisers ); i ++ )
	{
		int j = 0;
		int created = 0;
		char *value = mlt_properties_get_value( normalisers, i );
		mlt_tokeniser_parse_new( tokeniser, value, "," );
		for ( j = 0; !created && j < mlt_tokeniser_count( tokeniser ); j ++ )
			last = create_filter( tractor, last, mlt_tokeniser_get_string( tokeniser, j ), &created );
	}

	// Close the tokeniser
	mlt_tokeniser_close( tokeniser );

	return last;
}

mlt_producer producer_fezzik_init( char *arg )
{
	// Create the producer that the tractor will contain
	mlt_producer producer = NULL;

	if ( arg != NULL )
		producer = create_producer( arg );

	// Build the tractor if we have a producer and it isn't already westley'd :-)
	if ( producer != NULL && mlt_properties_get( mlt_producer_properties( producer ), "westley" ) == NULL )
	{
		// Construct the tractor
		mlt_tractor tractor = mlt_tractor_init( );

		// Sanity check
		if ( tractor != NULL )
		{
			// Extract the tractor properties
			mlt_properties properties = mlt_tractor_properties( tractor );

			// Our producer will be the last service
			mlt_service last = mlt_producer_service( producer );

			// Set the registered count
			mlt_properties_set_int( properties, "_registered", 0 );

			// Register our producer for seeking in the tractor
			mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

			// Now attach normalising filters
			last = attach_normalisers( tractor, last );

			// Connect the tractor to the last
			mlt_tractor_connect( tractor, last );

			// Finally, inherit properties from producer
			mlt_properties_inherit( properties, mlt_producer_properties( producer ) );

			// Now make sure we don't lose our inherited identity
			mlt_properties_set_int( properties, "_mlt_service_hidden", 1 );

			// This is a temporary hack to ensure that westley doesn't dig too deep
			// and fezzik doesn't overdo it with throwing rocks...
			mlt_properties_set( properties, "westley", "was here" );

			// We need to ensure that all further properties are mirrored in the producer
			mlt_properties_mirror( properties, mlt_producer_properties( producer ) );

			// Ensure that the inner producer ignores the in point
			mlt_properties_set_int( mlt_producer_properties( producer ), "ignore_points", 1 );

			// Now, we return the producer of the tractor
			producer = mlt_tractor_producer( tractor );
		}
	}

	// Return the tractor's producer
	return producer;
}
