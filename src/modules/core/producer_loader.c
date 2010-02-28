/*
 * producer_loader.c -- auto-load producer by file name extension
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>
#include <assert.h>

#include <framework/mlt.h>

static mlt_properties dictionary = NULL;
static mlt_properties normalisers = NULL;

static mlt_producer create_from( mlt_profile profile, char *file, char *services )
{
	mlt_producer producer = NULL;
	char *temp = strdup( services );
	char *service = temp;
	do
	{
		char *p = strchr( service, ',' );
		if ( p != NULL )
			*p ++ = '\0';
		producer = mlt_factory_producer( profile, service, file );
		service = p;
	}
	while ( producer == NULL && service != NULL );
	free( temp );
	return producer;
}

static mlt_producer create_producer( mlt_profile profile, char *file )
{
	mlt_producer result = NULL;

	// 1st Line - check for service:resource handling
	if ( strchr( file, ':' ) )
	{
		char *temp = strdup( file );
		char *service = temp;
		char *resource = strchr( temp, ':' );
		*resource ++ = '\0';
		result = mlt_factory_producer( profile, service, resource );
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
			sprintf( temp, "%s/core/loader.dict", mlt_environment( "MLT_DATA" ) );
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
				result = create_from( profile, file, mlt_properties_get_value( dictionary, i ) );
		}

		free( lookup );
	}

	// Finally, try just loading as service
	if ( result == NULL )
		result = mlt_factory_producer( profile, file, NULL );

	return result;
}

static void create_filter( mlt_profile profile, mlt_producer producer, char *effect, int *created )
{
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';

	// The swscale and avcolor_space filters require resolution as arg to test compatibility
	if ( strncmp( effect, "swscale", 7 ) == 0 || strncmp( effect, "avcolo", 6 ) == 0 )
		arg = (char*) mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( producer ), "_real_width" );

	mlt_filter filter = mlt_factory_filter( profile, id, arg );
	if ( filter != NULL )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_loader", 1 );
		mlt_producer_attach( producer, filter );
		mlt_filter_close( filter );
		*created = 1;
	}
	free( id );
}

static void attach_normalisers( mlt_profile profile, mlt_producer producer )
{
	// Loop variable
	int i;

	// Tokeniser
	mlt_tokeniser tokeniser = mlt_tokeniser_init( );

	// We only need to load the normalising properties once
	if ( normalisers == NULL )
	{
		char temp[ 1024 ];
		sprintf( temp, "%s/core/loader.ini", mlt_environment( "MLT_DATA" ) );
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
			create_filter( profile, producer, mlt_tokeniser_get_string( tokeniser, j ), &created );
	}

	// Close the tokeniser
	mlt_tokeniser_close( tokeniser );
}

mlt_producer producer_loader_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the producer 
	mlt_producer producer = NULL;
	mlt_properties properties = NULL;

	if ( arg != NULL )
		producer = create_producer( profile, arg );

	if ( producer != NULL )
		properties = MLT_PRODUCER_PROPERTIES( producer );

	// Attach filters if we have a producer and it isn't already xml'd :-)
	if ( producer && strcmp( id, "abnormal" ) &&
		mlt_properties_get( properties, "xml" ) == NULL &&
		mlt_properties_get( properties, "_xml" ) == NULL &&
		mlt_properties_get( properties, "loader_normalised" ) == NULL )
		attach_normalisers( profile, producer );
	
	// Always let the image and audio be converted
	int created = 0;
#ifndef __DARWIN__
	create_filter( profile, producer, "avcolor_space", &created );
	if ( !created )
#endif
		create_filter( profile, producer, "imageconvert", &created );
	create_filter( profile, producer, "audioconvert", &created );

	// Now make sure we don't lose our identity
	if ( properties != NULL )
		mlt_properties_set_int( properties, "_mlt_service_hidden", 1 );

	// Return the producer
	return producer;
}
