/*
 * producer_westley.c -- a libxml2 parser of mlt service networks
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

// TODO: destroy unreferenced producers
 
#include "producer_westley.h"
#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/parser.h>

#define STACK_SIZE 1000

struct deserialise_context_s
{
	mlt_service stack_service[ STACK_SIZE ];
	int stack_service_size;
	mlt_properties producer_map;
	mlt_properties destructors;
	char *property;
	mlt_properties producer_properties;
};
typedef struct deserialise_context_s *deserialise_context;


/** Push a service.
*/

static int context_push_service( deserialise_context this, mlt_service that )
{
	int ret = this->stack_service_size >= STACK_SIZE;
	if ( ret == 0 )
		this->stack_service[ this->stack_service_size ++ ] = that;
	return ret;
}

/** Pop a service.
*/

static mlt_service context_pop_service( deserialise_context this )
{
	mlt_service result = NULL;
	if ( this->stack_service_size > 0 )
		result = this->stack_service[ -- this->stack_service_size ];
	return result;
}

// Set the destructor on a new service
static void track_service( mlt_properties properties, void *service, mlt_destructor destructor )
{
	int registered = mlt_properties_get_int( properties, "registered" );
	char *key = mlt_properties_get( properties, "registered" );
	mlt_properties_set_data( properties, key, service, 0, destructor, NULL );
	mlt_properties_set_int( properties, "registered", ++ registered );
}

static void on_start_tractor( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_service service = mlt_tractor_service( mlt_tractor_init() );
	mlt_properties properties = mlt_service_properties( service );

	track_service( context->destructors, service, (mlt_destructor) mlt_tractor_close );

	mlt_properties_set_position( properties, "length", 0 );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( mlt_service_properties( service ), (char*) atts[0], (char*) atts[1] );

	if ( mlt_properties_get_position( properties, "length" ) < mlt_properties_get_position( properties, "out" ) )
	{
		mlt_position length = mlt_properties_get_position( properties, "out" ) + 1;
		mlt_properties_set_position( properties, "length", length );
	}

	context_push_service( context, service );
}

static void on_start_multitrack( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_service service = mlt_multitrack_service( mlt_multitrack_init() );
	mlt_properties properties = mlt_service_properties( service );

	track_service( context->destructors, service, (mlt_destructor) mlt_multitrack_close );

	mlt_properties_set_position( properties, "length", 0 );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );

	context_push_service( context, service );
}

static void on_start_playlist( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_service service = mlt_playlist_service( mlt_playlist_init() );
	mlt_properties properties = mlt_service_properties( service );

	track_service( context->destructors, service, (mlt_destructor) mlt_playlist_close );

	mlt_properties_set_position( properties, "length", 0 );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( properties, ( char* )atts[0], ( char* )atts[1] );

		// Out will be overwritten later as we append, so we need to save it
		if ( strcmp( atts[ 0 ], "out" ) == 0 )
			mlt_properties_set( properties, "_westley.out", ( char* )atts[ 1 ] );
	}

	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

	context_push_service( context, service );
}

static void on_start_producer( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_properties properties = context->producer_properties = mlt_properties_new();

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );
	}
}

static void on_start_blank( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// Get the playlist from the stack
	mlt_service service = context_pop_service( context );
	mlt_position length = 0;

	// Look for the length attribute
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		if ( strcmp( atts[0], "length" ) == 0 )
		{
			length = atoll( atts[1] );
			break;
		}
	}

	// Append a blank to the playlist
	mlt_playlist_blank( MLT_PLAYLIST( service ), length - 1 );

	// Push the playlist back onto the stack
	context_push_service( context, service );
}

static void on_start_entry_track( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// Use a dummy service to hold properties to allow arbitratry nesting
	mlt_service service = calloc( 1, sizeof( struct mlt_service_s ) );
	mlt_service_init( service, NULL );

	// Push the dummy service onto the stack
	context_push_service( context, service );
	
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( mlt_service_properties( service ), (char*) atts[0], (char*) atts[1] );
		
		// Look for the producer attribute
		if ( strcmp( atts[ 0 ], "producer" ) == 0 )
		{
			if ( mlt_properties_get_data( context->producer_map, (char*) atts[1], NULL ) !=  NULL )
				// Push the referenced producer onto the stack
				context_push_service( context, MLT_SERVICE( mlt_properties_get_data( context->producer_map, (char*) atts[1], NULL ) ) );
		}
	}
}

static void on_start_filter( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_properties properties = context->producer_properties = mlt_properties_new();

	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );
}

static void on_start_transition( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_properties properties = context->producer_properties = mlt_properties_new();

	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );
}

static void on_start_property( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_properties properties = context->producer_properties;
	char *value = NULL;

	if ( properties == NULL )
		return;
	
	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		if ( strcmp( atts[ 0 ], "name" ) == 0 )
		{
			context->property = strdup( atts[ 1 ] );
		}
		else if ( strcmp( atts[ 0 ], "value" ) == 0 )
		{
			value = (char*) atts[ 1 ];
		}
	}

	if ( context->property != NULL && value != NULL )
		mlt_properties_set( properties, context->property, value );
}

static void on_end_multitrack( deserialise_context context, const xmlChar *name )
{
	// Get the producer (multitrack) from the stack
	mlt_service producer = context_pop_service( context );

	// Get the tractor from the stack
	mlt_service service = context_pop_service( context );

	// Connect the tractor to the producer
	mlt_tractor_connect( MLT_TRACTOR( service ), producer );
	mlt_properties_set_data( mlt_service_properties( service ), "multitrack",
		MLT_MULTITRACK( producer ), 0, NULL, NULL );

	// Push the tractor back onto the stack
	context_push_service( context, service );

	// Push the producer back onto the stack
	context_push_service( context, producer );
}

static void on_end_playlist( deserialise_context context, const xmlChar *name )
{
	// Get the playlist from the stack
	mlt_service service = context_pop_service( context );
	mlt_properties properties = mlt_service_properties( service );

	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out;

	if ( mlt_properties_get( properties, "_westley.out" ) != NULL )
		out = mlt_properties_get_position( properties, "_westley.out" );
	else
		out = mlt_properties_get_position( properties, "length" ) - 1;

	if ( mlt_properties_get_position( properties, "length" ) < out )
		mlt_properties_set_position( properties, "length", out  + 1 );

	mlt_producer_set_in_and_out( MLT_PRODUCER( service ), in, out );

	// Push the playlist back onto the stack
	context_push_service( context, service );
}

static void on_end_track( deserialise_context context, const xmlChar *name )
{
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Get the dummy track service from the stack
	mlt_service track = context_pop_service( context );

	// Get the multitrack from the stack
	mlt_service service = context_pop_service( context );

	// Set the track on the multitrack
	mlt_multitrack_connect( MLT_MULTITRACK( service ),
		MLT_PRODUCER( producer ),
		mlt_multitrack_count( MLT_MULTITRACK( service ) ) );

	// Set producer i/o if specified
	if ( mlt_properties_get( mlt_service_properties( track ), "in" ) != NULL ||
		mlt_properties_get( mlt_service_properties( track ), "out" ) != NULL )
	{
		mlt_producer_set_in_and_out( MLT_PRODUCER( producer ),
			mlt_properties_get_position( mlt_service_properties( track ), "in" ),
			mlt_properties_get_position( mlt_service_properties( track ), "out" ) );
	}

	// Push the multitrack back onto the stack
	context_push_service( context, service );

	mlt_service_close( track );
}

static void on_end_entry( deserialise_context context, const xmlChar *name )
{
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Get the dummy entry service from the stack
	mlt_service entry = context_pop_service( context );

	// Get the playlist from the stack
	mlt_service service = context_pop_service( context );

	// Append the producer to the playlist
	if ( mlt_properties_get( mlt_service_properties( entry ), "in" ) != NULL ||
		mlt_properties_get( mlt_service_properties( entry ), "out" ) != NULL )
	{
		mlt_playlist_append_io( MLT_PLAYLIST( service ),
			MLT_PRODUCER( producer ),
			mlt_properties_get_position( mlt_service_properties( entry ), "in" ), 
			mlt_properties_get_position( mlt_service_properties( entry ), "out" ) );
	}
	else
	{
		mlt_playlist_append( MLT_PLAYLIST( service ), MLT_PRODUCER( producer ) );
	}

	// Push the playlist back onto the stack
	context_push_service( context, service );

	mlt_service_close( entry );
}

static void on_end_tractor( deserialise_context context, const xmlChar *name )
{
	// Get and discard the last producer
	mlt_producer multitrack = MLT_PRODUCER( context_pop_service( context ) );

	// Get the tractor
	mlt_service tractor = context_pop_service( context );
	multitrack = mlt_properties_get_data( mlt_service_properties( tractor ), "multitrack", NULL );

	// Inherit the producer's properties
	mlt_properties properties = mlt_producer_properties( multitrack );
	mlt_properties_set_position( properties, "length", mlt_producer_get_out( multitrack ) + 1 );
	mlt_producer_set_in_and_out( multitrack, 0, mlt_producer_get_out( multitrack ) );
	mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( multitrack ) );

	// Push the playlist back onto the stack
	context_push_service( context, tractor );
}

static void on_end_property( deserialise_context context, const xmlChar *name )
{
	// Close this property handling
	free( context->property );
	context->property = NULL;
}

static void on_end_producer( deserialise_context context, const xmlChar *name )
{
	mlt_properties properties = context->producer_properties;
	mlt_service service = NULL;
	
	if ( properties == NULL )
		return;
		
	// Instatiate the producer
	if ( mlt_properties_get( properties, "resource" ) != NULL )
	{
		char *root = mlt_properties_get( context->producer_map, "_root" );
		char *resource = mlt_properties_get( properties, "resource" );
		char *full_resource = malloc( strlen( root ) + strlen( resource ) + 1 );
		if ( resource[ 0 ] != '/' )
		{
			strcpy( full_resource, root );
			strcat( full_resource, resource );
		}
		else
		{
			strcpy( full_resource, resource );
		}
		service = MLT_SERVICE( mlt_factory_producer( "fezzik", full_resource ) );
		free( full_resource );
	}
	if ( service == NULL && mlt_properties_get( properties, "mlt_service" ) != NULL )
	{
		service = MLT_SERVICE( mlt_factory_producer( mlt_properties_get( properties, "mlt_service" ),
			mlt_properties_get( properties, "resource" ) ) );
	}

	track_service( context->destructors, service, (mlt_destructor) mlt_producer_close );

	// Add the producer to the producer map
	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );
	context->producer_properties = NULL;
	properties = mlt_service_properties( service );

	// Set in and out
	mlt_producer_set_in_and_out( MLT_PRODUCER( service ),
		mlt_properties_get_position( properties, "in" ),
		mlt_properties_get_position( properties, "out" ) );

	// Push the new producer onto the stack
	context_push_service( context, service );
}

static void on_end_filter( deserialise_context context, const xmlChar *name )
{
	mlt_properties properties = context->producer_properties;
	if ( properties == NULL )
		return;

	char *id;
	char key[11];
	key[ 10 ] = '\0';

	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );
//fprintf( stderr, "connecting filter to %s\n", mlt_properties_get( mlt_service_properties( producer ), "resource" ) );

	// Create the filter
	mlt_service service = MLT_SERVICE( mlt_factory_filter( mlt_properties_get( properties, "mlt_service" ), NULL ) );

	track_service( context->destructors, service, (mlt_destructor) mlt_filter_close );

	// Connect the filter to the producer
	mlt_filter_connect( MLT_FILTER( service ), producer,
		mlt_properties_get_int( properties, "track" ) );

	// Set in and out from producer if non existant
	if ( mlt_properties_get( properties, "in" ) == NULL )
		mlt_properties_set_position( properties, "in", mlt_producer_get_in( MLT_PRODUCER( producer ) ) );
	if ( mlt_properties_get( properties, "out" ) == NULL )
		mlt_properties_set_position( properties, "out", mlt_producer_get_out( MLT_PRODUCER( producer ) ) );

	// Propogate the properties
	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );
	context->producer_properties = NULL;
	properties = mlt_service_properties( service );

	// Set in and out
//fprintf( stderr, "setting filter in %lld out %lld\n", mlt_properties_get_position( properties, "in" ), mlt_properties_get_position( properties, "out" ) );
	mlt_filter_set_in_and_out( MLT_FILTER( service ), 
		mlt_properties_get_position( properties, "in" ),
		mlt_properties_get_position( properties, "out" ) );

	// Get the parent producer from the stack
	mlt_service tractor = context_pop_service( context );

	if ( tractor != NULL )
	{
//fprintf( stderr, "connecting tractor %s to filter\n", mlt_properties_get( mlt_service_properties( tractor ), "resource" ) );
		// Connect the tractor to the filter
		if ( strcmp( mlt_properties_get( mlt_service_properties( tractor ), "resource" ), "<tractor>" ) == 0 )
			mlt_tractor_connect( MLT_TRACTOR( tractor ), service );

		// Push the parent producer back onto the stack
		context_push_service( context, tractor );
	}

//fprintf( stderr, "setting filter in %lld out %lld\n", mlt_properties_get_position( properties, "in" ), mlt_properties_get_position( properties, "out" ) );
	// If a producer alias is in the producer_map, get it
	snprintf( key, 10, "%p", producer );
	if ( mlt_properties_get_data( context->producer_map, key, NULL ) != NULL )
		producer = mlt_properties_get_data( context->producer_map, key, NULL );

	// Put the producer in the producer map
	id = mlt_properties_get( mlt_service_properties( producer ), "id" );
	if ( id != NULL )
		mlt_properties_set_data( context->producer_map, id, service, 0, NULL, NULL );

	// For filter chain support, add an alias to the producer map
	snprintf( key, 10, "%p", service );
	mlt_properties_set_data( context->producer_map, key, producer, 0, NULL, NULL );

	// Push the filter onto the stack
	context_push_service( context, service );

}

static void on_end_transition( deserialise_context context, const xmlChar *name )
{
	mlt_properties properties = context->producer_properties;
	if ( properties == NULL )
		return;

	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Create the transition
	mlt_service service = MLT_SERVICE( mlt_factory_transition( mlt_properties_get( properties, "mlt_service" ), NULL ) );

	track_service( context->destructors, service, (mlt_destructor) mlt_transition_close );

	// Propogate the properties
	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );
	context->producer_properties = NULL;
	properties = mlt_service_properties( service );

	// Set in and out
	mlt_transition_set_in_and_out( MLT_TRANSITION( service ),
		mlt_properties_get_position( properties, "in" ),
		mlt_properties_get_position( properties, "out" ) );

	// Connect the filter to the producer
	mlt_transition_connect( MLT_TRANSITION( service ), producer,
		mlt_properties_get_int( properties, "a_track" ),
		mlt_properties_get_int( properties, "b_track" ) );

	// Get the tractor from the stack
	mlt_service tractor = context_pop_service( context );

	// Connect the tractor to the transition
	mlt_tractor_connect( MLT_TRACTOR( tractor ), service );

	// Push the tractor back onto the stack
	context_push_service( context, tractor );

	// Push the transition onto the stack
	context_push_service( context, service );
}

static void on_start_element( void *ctx, const xmlChar *name, const xmlChar **atts)
{
	deserialise_context context = ( deserialise_context ) ctx;
	
	if ( strcmp( name, "tractor" ) == 0 )
		on_start_tractor( context, name, atts );
	else if ( strcmp( name, "multitrack" ) == 0 )
		on_start_multitrack( context, name, atts );
	else if ( strcmp( name, "playlist" ) == 0 )
		on_start_playlist( context, name, atts );
	else if ( strcmp( name, "producer" ) == 0 )
		on_start_producer( context, name, atts );
	else if ( strcmp( name, "blank" ) == 0 )
		on_start_blank( context, name, atts );
	else if ( strcmp( name, "entry" ) == 0 || strcmp( name, "track" ) == 0 )
		on_start_entry_track( context, name, atts );
	else if ( strcmp( name, "filter" ) == 0 )
		on_start_filter( context, name, atts );
	else if ( strcmp( name, "transition" ) == 0 )
		on_start_transition( context, name, atts );
	else if ( strcmp( name, "property" ) == 0 )
		on_start_property( context, name, atts );
}

static void on_end_element( void *ctx, const xmlChar *name )
{
	deserialise_context context = ( deserialise_context ) ctx;
	
	if ( strcmp( name, "multitrack" ) == 0 )
		on_end_multitrack( context, name );
	else if ( strcmp( name, "playlist" ) == 0 )
		on_end_playlist( context, name );
	else if ( strcmp( name, "track" ) == 0 )
		on_end_track( context, name );
	else if ( strcmp( name, "entry" ) == 0 )
		on_end_entry( context, name );
	else if ( strcmp( name, "tractor" ) == 0 )
		on_end_tractor( context, name );
	else if ( strcmp( name, "property" ) == 0 )
		on_end_property( context, name );
	else if ( strcmp( name, "producer" ) == 0 )
		on_end_producer( context, name );
	else if ( strcmp( name, "filter" ) == 0 )
		on_end_filter( context, name );
	else if ( strcmp( name, "transition" ) == 0 )
		on_end_transition( context, name );
}

static void on_characters( void *ctx, const xmlChar *ch, int len )
{
	deserialise_context context = ( deserialise_context ) ctx;
	char *value = calloc( len + 1, 1 );

	value[ len ] = 0;
	strncpy( value, (const char*) ch, len );

	if ( context->property != NULL && context->producer_properties != NULL )
		mlt_properties_set( context->producer_properties, context->property, value );
		
	free( value);
}

mlt_producer producer_westley_init( char *filename )
{
	xmlSAXHandler *sax = calloc( 1, sizeof( xmlSAXHandler ) );
	struct deserialise_context_s *context = calloc( 1, sizeof( struct deserialise_context_s ) );
	mlt_properties properties = NULL;
	int i = 0;

	context->producer_map = mlt_properties_new();
	context->destructors = mlt_properties_new();

	// We need to track the number of registered filters
	mlt_properties_set_int( context->destructors, "registered", 0 );

	// We need the directory prefix which was used for the westley
	mlt_properties_set( context->producer_map, "_root", "" );
	if ( strchr( filename, '/' ) )
	{
		char *root = NULL;
		mlt_properties_set( context->producer_map, "_root", filename );
		root = mlt_properties_get( context->producer_map, "_root" );
		*( strrchr( root, '/' ) + 1 ) = '\0';
	}

	sax->startElement = on_start_element;
	sax->endElement = on_end_element;
	sax->characters = on_characters;

	// I REALLY DON'T GET THIS - HOW THE HELL CAN YOU REFERENCE A WESTLEY IN A WESTLEY???
	xmlInitParser();

	xmlSAXUserParseFile( sax, context, filename );

	// Need the complete producer list for various reasons
	properties = context->destructors;

	// Get the last producer on the stack
	mlt_service service = context_pop_service( context );

	// Do we actually have a producer here?
	if ( service != NULL )
	{
		// Now make sure we don't have a reference to the service in the properties
		for ( i = mlt_properties_count( properties ) - 1; i >= 1; i -- )
		{
			char *name = mlt_properties_get_name( properties, i );
			if ( mlt_properties_get_data( properties, name, NULL ) == service )
			{
				mlt_properties_set_data( properties, name, service, 0, NULL, NULL );
				break;
			}
		}

		// We are done referencing destructor property list
		// Set this var to service properties for convenience
		properties = mlt_service_properties( service );
	
		// make the returned service destroy the connected services
		mlt_properties_set_data( properties, "__destructors__", context->destructors, 0, (mlt_destructor) mlt_properties_close, NULL );

		// Now assign additional properties
		mlt_properties_set( properties, "resource", filename );

		// This tells consumer_westley not to deep copy
		mlt_properties_set( properties, "westley", "was here" );
	}
	else
	{
		// Clean up
		mlt_properties_close( properties );
	}

	free( context->stack_service );
	mlt_properties_close( context->producer_map );
	//free( context );
	free( sax );
	xmlCleanupParser();
	xmlMemoryDump( );


	return MLT_PRODUCER( service );
}

