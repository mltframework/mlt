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

static void create_filter( mlt_producer producer, char *effect, int *created )
{
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_filter filter = mlt_factory_filter( id, arg );
	if ( filter != NULL )
	{
		mlt_properties_set_int( mlt_filter_properties( filter ), "_fezzik", 1 );
		mlt_producer_attach( producer, filter );
		mlt_filter_close( filter );
		*created = 1;
	}
	free( id );
}

static void attach_normalisers( mlt_producer producer )
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
			create_filter( producer, mlt_tokeniser_get_string( tokeniser, j ), &created );
	}

	// Close the tokeniser
	mlt_tokeniser_close( tokeniser );
}

mlt_producer producer_fezzik_init( char *arg )
{
	// Create the producer 
	mlt_producer producer = NULL;
	mlt_properties properties = NULL;

	if ( arg != NULL )
		producer = create_producer( arg );

	if ( producer != NULL )
		properties = mlt_producer_properties( producer );

	// Attach filters if we have a producer and it isn't already westley'd :-)
	if ( producer != NULL && mlt_properties_get( properties, "westley" ) == NULL )
		attach_normalisers( producer );

	// Now make sure we don't lose our identity
	if ( properties != NULL )
		mlt_properties_set_int( properties, "_mlt_service_hidden", 1 );

	// Return the producer
	return producer;
}
