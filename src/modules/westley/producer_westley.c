/*
 * producer_libdv.c -- a libxml2 parser of mlt service networks
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
	int track_count;
	mlt_properties producer_map;
	mlt_properties destructors;
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

	track_service( context->destructors, service, (mlt_destructor) mlt_tractor_close );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( mlt_service_properties( service ), (char*) atts[0], (char*) atts[1] );

	context_push_service( context, service );
}

static void on_start_multitrack( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_service service = mlt_multitrack_service( mlt_multitrack_init() );

	track_service( context->destructors, service, (mlt_destructor) mlt_multitrack_close );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( mlt_service_properties( service ), (char*) atts[0], (char*) atts[1] );

	context_push_service( context, service );
}

static void on_start_playlist( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_service service = mlt_playlist_service( mlt_playlist_init() );
	mlt_properties properties = mlt_service_properties( service );

	track_service( context->destructors, service, (mlt_destructor) mlt_playlist_close );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );

	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

	context_push_service( context, service );
}

static void on_start_producer( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_properties properties = mlt_properties_new();
	mlt_service service = NULL;

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );

	if ( mlt_properties_get( properties, "mlt_service" ) != NULL )
	{
		service = MLT_SERVICE( mlt_factory_producer( mlt_properties_get( properties, "mlt_service" ),
			mlt_properties_get( properties, "resource" ) ) );
	}
	else
	{
		// Unspecified producer, use inigo
		char *args[2] = { mlt_properties_get( properties, "resource" ), 0 };
		service = MLT_SERVICE( mlt_factory_producer( "inigo", args ) );
	}

	track_service( context->destructors, service, (mlt_destructor) mlt_producer_close );

	// Add the producer to the producer map	
	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );

	context_push_service( context, service );
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
	mlt_playlist_blank( MLT_PLAYLIST( service ), length );

	// Push the playlist back onto the stack
	context_push_service( context, service );
}

static void on_start_entry_track( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// Look for the producer attribute
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		if ( strcmp( atts[0], "producer" ) == 0 )
		{
			if ( mlt_properties_get_data( context->producer_map, (char*) atts[1], NULL ) !=  NULL )
				// Push the referenced producer onto the stack
				context_push_service( context, MLT_SERVICE( mlt_properties_get_data( context->producer_map, (char*) atts[1], NULL ) ) );
			break;
		}
	}
}

static void on_start_filter( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	char *id;
	mlt_properties properties = mlt_properties_new();
	char key[11];
	key[ 10 ] = '\0';

	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );

	// Create the filter
	mlt_service service = MLT_SERVICE( mlt_factory_filter( mlt_properties_get( properties, "mlt_service" ), NULL ) );

	track_service( context->destructors, service, (mlt_destructor) mlt_filter_close );
	
	// Connect the filter to the producer
	mlt_filter_connect( MLT_FILTER( service ), producer,
		mlt_properties_get_int( properties, "track" ) );

	// Propogate the properties
	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );

	// Get the parent producer from the stack
	mlt_service tractor = context_pop_service( context );

	if ( tractor != NULL )
	{
		// Connect the filter to the tractor
		if ( strcmp( mlt_properties_get( mlt_service_properties( tractor ), "resource" ), "<tractor>" ) == 0 )
			mlt_tractor_connect( MLT_TRACTOR( tractor ), service );

		// Push the parent producer back onto the stack
		context_push_service( context, tractor );
	}

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

static void on_start_transition( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_properties properties = mlt_properties_new();

	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (char*) atts[0], (char*) atts[1] );

	// Create the transition
	mlt_service service = MLT_SERVICE( mlt_factory_transition( mlt_properties_get( properties, "mlt_service" ), NULL ) );

	track_service( context->destructors, service, (mlt_destructor) mlt_transition_close );

	// Propogate the properties
	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );

	// Connect the filter to the producer
	mlt_transition_connect( MLT_TRANSITION( service ), producer,
		mlt_properties_get_int( mlt_service_properties( service ), "a_track" ),
		mlt_properties_get_int( mlt_service_properties( service ), "b_track" ) );

	// Get the tractor from the stack
	mlt_service tractor = context_pop_service( context );

	// Connect the tractor to the transition
	mlt_tractor_connect( MLT_TRACTOR( tractor ), service );

	// Push the tractor back onto the stack
	context_push_service( context, tractor );

	// Push the transition onto the stack
	context_push_service( context, service );
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
	// Get the producer (playlist) from the stack
	mlt_service producer = context_pop_service( context );

	// Push the producer back onto the stack
	context_push_service( context, producer );
}

static void on_end_track( deserialise_context context, const xmlChar *name )
{
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Get the multitrack from the stack
	mlt_service service = context_pop_service( context );

	// Set the track on the multitrack
	mlt_multitrack_connect( MLT_MULTITRACK( service ),
		MLT_PRODUCER( producer ),
		context->track_count++ );

	// Push the multitrack back onto the stack
	context_push_service( context, service );
}

static void on_end_entry( deserialise_context context, const xmlChar *name )
{
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );

	// Get the playlist from the stack
	mlt_service service = context_pop_service( context );

	// Append the producer to the playlist
	mlt_playlist_append_io( MLT_PLAYLIST( service ),
		MLT_PRODUCER( producer ),
		mlt_properties_get_position( mlt_service_properties( producer ), "in" ),
		mlt_properties_get_position( mlt_service_properties( producer ), "out" ) );

	// Push the playlist back onto the stack
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
	{
		// Discard the last producer
		context_pop_service( context );
	}
}


mlt_producer producer_westley_init( char *filename )
{
	xmlSAXHandler *sax = calloc( 1, sizeof( xmlSAXHandler ) );
	struct deserialise_context_s *context = calloc( 1, sizeof( struct deserialise_context_s ) );

	context->producer_map = mlt_properties_new();
	context->destructors = mlt_properties_new();
	// We need to track the number of registered filters
	mlt_properties_set_int( context->destructors, "registered", 0 );
	sax->startElement = on_start_element;
	sax->endElement = on_end_element;

	xmlInitParser();
	xmlSAXUserParseFile( sax, context, filename );
	xmlCleanupParser();
	free( sax );

	mlt_properties_close( context->producer_map );

	mlt_service service = context_pop_service( context );
	// make the returned service destroy the connected services
	mlt_properties_set_data( mlt_service_properties( service ), "__destructors__", context->destructors, 0, (mlt_destructor) mlt_properties_close, NULL );
	free( context );

	return MLT_PRODUCER( service );
}

