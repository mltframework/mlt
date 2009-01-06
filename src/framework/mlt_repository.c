/**
 * \file mlt_repository.c
 * \brief provides a map between service and shared objects
 *
 * Copyright (C) 2003-2008 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
 *         Dan Dennedy <dan@dennedy.org>
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
#include "mlt_tokeniser.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

/** \brief Repository class
 *
 * \extends mlt_properties_s
 */

struct mlt_repository_s
{
	struct mlt_properties_s parent; // a list of object files
	mlt_properties consumers; // lists of entry points
	mlt_properties filters;
	mlt_properties producers;
	mlt_properties transitions;
};

/** Construct a new repository
*/

mlt_repository mlt_repository_init( const char *directory )
{
	// Safety check
	if ( directory == NULL || strcmp( directory, "" ) == 0 )
		return NULL;

	// Construct the repository
	mlt_repository this = calloc( sizeof( struct mlt_repository_s ), 1 );
	mlt_properties_init( &this->parent, this );
	this->consumers = mlt_properties_new();
	this->filters = mlt_properties_new();
	this->producers = mlt_properties_new();
	this->transitions = mlt_properties_new();

	// Get the directory list
	mlt_properties dir = mlt_properties_new();
	int count = mlt_properties_dir_list( dir, directory, NULL, 0 );
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
			mlt_repository_callback symbol_ptr = dlsym( object, "mlt_register" );

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
		else if ( strstr( object_name, "libmlt" ) )
		{
			fprintf( stderr, "%s, %s: failed to dlopen %s\n  (%s)\n", __FILE__, __FUNCTION__, object_name, dlerror() );
		}
	}

	mlt_properties_close( dir );

	return this;
}

static mlt_properties new_service( void *symbol )
{
	mlt_properties properties = mlt_properties_new();
	mlt_properties_set_data( properties, "symbol", symbol, 0, NULL, NULL );
	return properties;
}

/** Register a service with the repository
    Typically, this is invoked by a module within its mlt_register().
*/

void mlt_repository_register( mlt_repository this, mlt_service_type service_type, const char *service, mlt_register_callback symbol )
{
	// Add the entry point to the corresponding service list
	switch ( service_type )
	{
		case consumer_type:
			mlt_properties_set_data( this->consumers, service, new_service( symbol ), 0, ( mlt_destructor )mlt_properties_close, NULL );
			break;
		case filter_type:
			mlt_properties_set_data( this->filters, service, new_service( symbol ), 0, ( mlt_destructor )mlt_properties_close, NULL );
			break;
		case producer_type:
			mlt_properties_set_data( this->producers, service, new_service( symbol ), 0, ( mlt_destructor )mlt_properties_close, NULL );
			break;
		case transition_type:
			mlt_properties_set_data( this->transitions, service, new_service( symbol ), 0, ( mlt_destructor )mlt_properties_close, NULL );
			break;
		default:
			break;
	}
}

static mlt_properties get_service_properties( mlt_repository this, mlt_service_type type, const char *service )
{
	mlt_properties service_properties = NULL;

	// Get the entry point from the corresponding service list
	switch ( type )
	{
		case consumer_type:
			service_properties = mlt_properties_get_data( this->consumers, service, NULL );
			break;
		case filter_type:
			service_properties = mlt_properties_get_data( this->filters, service, NULL );
			break;
		case producer_type:
			service_properties = mlt_properties_get_data( this->producers, service, NULL );
			break;
		case transition_type:
			service_properties = mlt_properties_get_data( this->transitions, service, NULL );
			break;
		default:
			break;
	}
	return service_properties;
}

/** Construct a new instance of a service
*/

void *mlt_repository_create( mlt_repository this, mlt_profile profile, mlt_service_type type, const char *service, void *input )
{
	mlt_properties properties = get_service_properties( this, type, service );
	if ( properties != NULL )
	{
		mlt_register_callback symbol_ptr = mlt_properties_get_data( properties, "symbol", NULL );

		// Construct the service
		return ( symbol_ptr != NULL ) ? symbol_ptr( profile, type, service, input ) : NULL;
	}
	return NULL;
}

/** Destroy a repository
*/

void mlt_repository_close( mlt_repository this )
{
	mlt_properties_close( this->consumers );
	mlt_properties_close( this->filters );
	mlt_properties_close( this->producers );
	mlt_properties_close( this->transitions );
	mlt_properties_close( &this->parent );
	free( this );
}

/** Get the list of registered consumers
*/

mlt_properties mlt_repository_consumers( mlt_repository self )
{
	return self->consumers;
}

/** Get the list of registered filters
*/

mlt_properties mlt_repository_filters( mlt_repository self )
{
	return self->filters;
}

/** Get the list of registered producers
*/

mlt_properties mlt_repository_producers( mlt_repository self )
{
	return self->producers;
}

/** Get the list of registered transitions
*/

mlt_properties mlt_repository_transitions( mlt_repository self )
{
	return self->transitions;
}

/** Register the metadata for a service
    IMPORTANT: mlt_repository will take responsibility for deallocating the metadata properties that you supply!
*/
void mlt_repository_register_metadata( mlt_repository self, mlt_service_type type, const char *service, mlt_metadata_callback callback, void *callback_data )
{
	mlt_properties service_properties = get_service_properties( self, type, service );
	mlt_properties_set_data( service_properties, "metadata_cb", callback, 0, NULL, NULL );
	mlt_properties_set_data( service_properties, "metadata_cb_data", callback_data, 0, NULL, NULL );
}

/** Get the metadata about a service
    Returns NULL if service or its metadata are unavailable.
*/

mlt_properties mlt_repository_metadata( mlt_repository self, mlt_service_type type, const char *service )
{
	mlt_properties metadata = NULL;
	mlt_properties properties = get_service_properties( self, type, service );

	// If this is a valid service
	if ( properties )
	{
		// Lookup cached metadata
		metadata = mlt_properties_get_data( properties, "metadata", NULL );
		if ( ! metadata )
		{
			// Not cached, so get the registered metadata callback function
			mlt_metadata_callback callback = mlt_properties_get_data( properties, "metadata_cb", NULL );

			// If a metadata callback function is registered
			if ( callback )
			{
				// Fetch the callback data arg
				void *data = mlt_properties_get_data( properties, "metadata_cb_data", NULL );

				// Fetch the metadata through the callback
				metadata = callback( type, service, data );

				// Cache the metadata
				if ( metadata )
					// Include dellocation and serialisation
					mlt_properties_set_data( properties, "metadata", metadata, 0, ( mlt_destructor )mlt_properties_close, ( mlt_serialiser )mlt_properties_serialise_yaml );
			}
		}
	}
	return metadata;
}

static char *getenv_locale()
{
	char *s = getenv( "LANGUAGE" );
	if ( s && s[0] )
		return s;
	s = getenv( "LC_ALL" );
	if ( s && s[0] )
		return s;
	s = getenv( "LC_MESSAGES" );
	if ( s && s[0] )
		return s;
	s = getenv( "LANG" );
	if ( s && s[0] )
		return s;
	return NULL;
}

/** Return a list of user-preferred language codes taken from environment variables.
*/

mlt_properties mlt_repository_languages( mlt_repository self )
{
	mlt_properties languages = mlt_properties_get_data( &self->parent, "languages", NULL );
	if ( languages )
		return languages;

	languages = mlt_properties_new();
	char *locale = getenv_locale();
	if ( locale )
	{
		locale = strdup( locale );
		mlt_tokeniser tokeniser = mlt_tokeniser_init();
		int count = mlt_tokeniser_parse_new( tokeniser, locale, ":" );
		if ( count )
		{
			int i;
			for ( i = 0; i < count; i++ )
			{
				char *locale = mlt_tokeniser_get_string( tokeniser, i );
				if ( strcmp( locale, "C" ) == 0 || strcmp( locale, "POSIX" ) == 0 )
					locale = "en";
				else if ( strlen( locale ) > 2 )
					locale[2] = 0;
				char string[21];
				snprintf( string, sizeof(string), "%d", i );
				mlt_properties_set( languages, string, locale );
			}
		}
		else
		{
			mlt_properties_set( languages, "0", "en" );
		}
		free( locale );
		mlt_tokeniser_close( tokeniser );
	}
	else
	{
		mlt_properties_set( languages, "0", "en" );
	}
	mlt_properties_set_data( &self->parent, "languages", languages, 0, ( mlt_destructor )mlt_properties_close, NULL );
	return languages;
}
