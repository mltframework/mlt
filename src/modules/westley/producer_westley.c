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

// TODO: destroy unreferenced producers (they are currently destroyed
//       when the returned producer is closed).

#include "producer_westley.h"
#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h> // for xmlCreateFileParserCtxt
#include <libxml/tree.h>

#define STACK_SIZE 1000
#define BRANCH_SIG_LEN 4000

#undef DEBUG
#ifdef DEBUG
extern xmlDocPtr westley_make_doc( mlt_service service );
#endif

struct deserialise_context_s
{
	mlt_service stack_service[ STACK_SIZE ];
	int stack_service_size;
	mlt_properties producer_map;
	mlt_properties destructors;
	char *property;
	mlt_properties producer_properties;
	int is_value;
	xmlDocPtr value_doc;
	xmlNodePtr stack_node[ STACK_SIZE ];
	int stack_node_size;
	xmlDocPtr entity_doc;
	int entity_is_replace;
	int depth;
	int branch[ STACK_SIZE ];
	const xmlChar *publicId;
	const xmlChar *systemId;
	mlt_properties params;
};
typedef struct deserialise_context_s *deserialise_context;

/** Convert the numerical current branch address to a dot-delimited string.
*/
static char *serialise_branch( deserialise_context this, char *s )
{
	int i;
	
	s[0] = 0;
	for ( i = 0; i < this->depth; i++ )
	{
		int len = strlen( s );
		snprintf( s + len, BRANCH_SIG_LEN - len, "%d.", this->branch[ i ] );
	}
	return s;
}

/** Push a service.
*/

static int context_push_service( deserialise_context this, mlt_service that )
{
	int ret = this->stack_service_size >= STACK_SIZE - 1;
	if ( ret == 0 )
	{
		this->stack_service[ this->stack_service_size++ ] = that;
		
		// Record the tree branch on which this service lives
		if ( mlt_properties_get( mlt_service_properties( that ), "_westley_branch" ) == NULL )
		{
			char s[ BRANCH_SIG_LEN ];
			mlt_properties_set( mlt_service_properties( that ), "_westley_branch", serialise_branch( this, s ) );
		}
	}
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

/** Push a node.
*/

static int context_push_node( deserialise_context this, xmlNodePtr node )
{
	int ret = this->stack_node_size >= STACK_SIZE - 1;
	if ( ret == 0 )
		this->stack_node[ this->stack_node_size ++ ] = node;
	return ret;
}

/** Pop a node.
*/

static xmlNodePtr context_pop_node( deserialise_context this )
{
	xmlNodePtr result = NULL;
	if ( this->stack_node_size > 0 )
		result = this->stack_node[ -- this->stack_node_size ];
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


// Forward declarations
static void on_end_track( deserialise_context context, const xmlChar *name );
static void on_end_entry( deserialise_context context, const xmlChar *name );

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

	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );
	
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

	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

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
	
	if ( service == NULL )
		return;
	
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
	// Use a dummy service to hold properties to allow arbitrary nesting
	mlt_service service = calloc( 1, sizeof( struct mlt_service_s ) );
	mlt_service_init( service, NULL );

	// Push the dummy service onto the stack
	context_push_service( context, service );
	
	if ( strcmp( name, "entry" ) == 0 )
		mlt_properties_set( mlt_service_properties( service ), "resource", "<entry>" );
	else
		mlt_properties_set( mlt_service_properties( service ), "resource", "<track>" );
	
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( mlt_service_properties( service ), (char*) atts[0], (char*) atts[1] );
		
		// Look for the producer attribute
		if ( strcmp( atts[ 0 ], "producer" ) == 0 )
		{
			if ( mlt_properties_get_data( context->producer_map, (char*) atts[1], NULL ) !=  NULL )
				// Push the referenced producer onto the stack
				context_push_service( context, MLT_SERVICE( mlt_properties_get_data( context->producer_map, (char*) atts[1], NULL ) ) );
			else
				// Remove the dummy service to cause end element failure
				context_pop_service( context );
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
	
	// Tell parser to collect any further nodes for serialisation
	context->is_value = 1;
}


/** This function adds a producer to a playlist or multitrack when
    there is no entry or track element.
*/

static int add_producer( deserialise_context context, mlt_service service, mlt_position in, mlt_position out )
{
	// Get the parent producer
	mlt_service producer = context_pop_service( context );

	if ( producer != NULL )
	{
		char current_branch[ BRANCH_SIG_LEN ];
		char *service_branch = mlt_properties_get( mlt_service_properties( producer ), "_westley_branch" );

		// Validate the producer from the stack is an ancestor and not predecessor
		serialise_branch( context, current_branch );
		if ( service_branch != NULL && strncmp( service_branch, current_branch, strlen( service_branch ) ) == 0 )
		{
			char *resource = mlt_properties_get( mlt_service_properties( producer ), "resource" );
			
			// Put the parent producer back
			context_push_service( context, producer );
				
			// If the parent producer is a multitrack or playlist (not a track or entry)
			if ( resource && ( strcmp( resource, "<playlist>" ) == 0 ||
				strcmp( resource, "<multitrack>" ) == 0 ) )
			{
//printf( "add_producer: current branch %s service branch %s (%d)\n", current_branch, service_branch, strncmp( service_branch, current_branch, strlen( service_branch ) ) );
				if ( strcmp( resource, "<playlist>" ) == 0 )
				{
					// Append this producer to the playlist
					mlt_playlist_append_io( MLT_PLAYLIST( producer ), 
						MLT_PRODUCER( service ), in, out );
				}
				else
				{
					mlt_properties properties = mlt_service_properties( service );
					
					// Set this producer on the multitrack
					mlt_multitrack_connect( MLT_MULTITRACK( producer ),
						MLT_PRODUCER( service ),
						mlt_multitrack_count( MLT_MULTITRACK( producer ) ) );
					
					// Set the hide state of the track producer
					char *hide_s = mlt_properties_get( properties, "hide" );
					if ( hide_s != NULL )
					{
						if ( strcmp( hide_s, "video" ) == 0 )
							mlt_properties_set_int( properties, "hide", 1 );
						else if ( strcmp( hide_s, "audio" ) == 0 )
							mlt_properties_set_int( properties, "hide", 2 );
						else if ( strcmp( hide_s, "both" ) == 0 )
							mlt_properties_set_int( properties, "hide", 3 );
					}
	
				}
				// Normally, the enclosing entry or track will pop this service off
				// In its absence we do not push it on.
				return 1;
			}
		}
	}
	return 0;
}

static void on_end_multitrack( deserialise_context context, const xmlChar *name )
{
	// Get the multitrack from the stack
	mlt_service producer = context_pop_service( context );
	if ( producer == NULL )
		return;
	
	// Get the tractor from the stack
	mlt_service service = context_pop_service( context );
	
	// Create a tractor if one does not exist
	char *resource = NULL;
	if ( service != NULL )
		resource = mlt_properties_get( mlt_service_properties( service ), "resource" );
	if ( service == NULL || resource == NULL || strcmp( resource, "<tractor>" ) )
	{
//printf("creating a tractor\n");
		char current_branch[ BRANCH_SIG_LEN ];
		
		// Put the anonymous service back onto the stack!
		if ( service != NULL )
			context_push_service( context, service );
		
		// Fabricate the tractor
		service = mlt_tractor_service( mlt_tractor_init() );
		track_service( context->destructors, service, (mlt_destructor) mlt_tractor_close );
		
		// Inherit the producer's properties
		mlt_properties properties = mlt_service_properties( service );
		mlt_properties_set_position( properties, "length", mlt_producer_get_out( MLT_PRODUCER( producer ) ) + 1 );
		mlt_producer_set_in_and_out( MLT_PRODUCER( service ), 0, mlt_producer_get_out( MLT_PRODUCER( producer ) ) );
		mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( MLT_PRODUCER( producer ) ) );
		
		mlt_properties_set( properties, "_westley_branch", serialise_branch( context, current_branch ) );
	}
	
	// Connect the tractor to the multitrack
	mlt_tractor_connect( MLT_TRACTOR( service ), producer );
	mlt_properties_set_data( mlt_service_properties( service ), "multitrack",
		MLT_MULTITRACK( producer ), 0, NULL, NULL );

	// See if the tractor should be added to a playlist or multitrack
	add_producer( context, service, 0, mlt_producer_get_out( MLT_PRODUCER( producer ) ) );
	
	// Always push the multitrack back onto the stack for filters and transitions
	context_push_service( context, producer );
	
	// Always push the tractor back onto the stack for filters and transitions
	context_push_service( context, service );
}

static void on_end_playlist( deserialise_context context, const xmlChar *name )
{
	// Get the playlist from the stack
	mlt_service producer = context_pop_service( context );
	if ( producer == NULL )
		return;
	mlt_properties properties = mlt_service_properties( producer );

	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out;

	if ( mlt_properties_get( properties, "_westley.out" ) != NULL )
		out = mlt_properties_get_position( properties, "_westley.out" );
	else
		out = mlt_properties_get_position( properties, "length" ) - 1;

	if ( mlt_properties_get_position( properties, "length" ) < out )
		mlt_properties_set_position( properties, "length", out  + 1 );

	mlt_producer_set_in_and_out( MLT_PRODUCER( producer ), in, out );
	
	// See if the playlist should be added to a playlist or multitrack
	if ( add_producer( context, producer, in, out ) == 0 )
		
		// Otherwise, push the playlist back onto the stack
		context_push_service( context, producer );
}

static void on_end_track( deserialise_context context, const xmlChar *name )
{
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );
	if ( producer == NULL )
		return;
	mlt_properties producer_props = mlt_service_properties( producer );

	// See if the producer is a tractor
	char *resource = mlt_properties_get( producer_props, "resource" );
	if ( resource && strcmp( resource, "<tractor>" ) == 0 )
		// If so chomp its producer
		context_pop_service( context );

	// Get the dummy track service from the stack
	mlt_service track = context_pop_service( context );
	if ( track == NULL || strcmp( mlt_properties_get( mlt_service_properties( track ), "resource" ), "<track>" ) )
	{
		context_push_service( context, producer );
		return;
	}
	mlt_properties track_props = mlt_service_properties( track );

	// Get the multitrack from the stack
	mlt_service service = context_pop_service( context );
	if ( service == NULL )
	{
		context_push_service( context, producer );
		return;
	}
	
	// Set the track on the multitrack
	mlt_multitrack_connect( MLT_MULTITRACK( service ),
		MLT_PRODUCER( producer ),
		mlt_multitrack_count( MLT_MULTITRACK( service ) ) );

	// Set producer i/o if specified
	if ( mlt_properties_get( track_props, "in" ) != NULL ||
		mlt_properties_get( track_props, "out" ) != NULL )
	{
		mlt_producer_set_in_and_out( MLT_PRODUCER( producer ),
			mlt_properties_get_position( track_props, "in" ),
			mlt_properties_get_position( track_props, "out" ) );
	}
	
	// Set the hide state of the track producer
	char *hide_s = mlt_properties_get( track_props, "hide" );
	if ( hide_s != NULL )
	{
		if ( strcmp( hide_s, "video" ) == 0 )
			mlt_properties_set_int( producer_props, "hide", 1 );
		else if ( strcmp( hide_s, "audio" ) == 0 )
			mlt_properties_set_int( producer_props, "hide", 2 );
		else if ( strcmp( hide_s, "both" ) == 0 )
			mlt_properties_set_int( producer_props, "hide", 3 );
	}

	// Push the multitrack back onto the stack
	context_push_service( context, service );

	mlt_service_close( track );
}

static void on_end_entry( deserialise_context context, const xmlChar *name )
{
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );
	if ( producer == NULL )
		return;
	
	// See if the producer is a tractor
	char *resource = mlt_properties_get( mlt_service_properties( producer ), "resource" );
	if ( resource && strcmp( resource, "<tractor>" ) == 0 )
		// If so chomp its producer
		context_pop_service( context );

	// Get the dummy entry service from the stack
	mlt_service entry = context_pop_service( context );
	if ( entry == NULL || strcmp( mlt_properties_get( mlt_service_properties( entry ), "resource" ), "<entry>" ) )
	{
		context_push_service( context, producer );
		return;
	}

	// Get the playlist from the stack
	mlt_service service = context_pop_service( context );
	if ( service == NULL )
	{
		context_push_service( context, producer );
		return;
	}

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
	// Get the tractor
	mlt_service tractor = context_pop_service( context );
	if ( tractor == NULL )
		return;
	
	// Get the tractor's multitrack
	mlt_producer multitrack = mlt_properties_get_data( mlt_service_properties( tractor ), "multitrack", NULL );
	if ( multitrack != NULL )
	{
		// Inherit the producer's properties
		mlt_properties properties = mlt_producer_properties( MLT_PRODUCER( tractor ) );
		mlt_properties_set_position( properties, "length", mlt_producer_get_out( multitrack ) + 1 );
		mlt_producer_set_in_and_out( multitrack, 0, mlt_producer_get_out( multitrack ) );
		mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( multitrack ) );
	}

	// See if the tractor should be added to a playlist or multitrack
	if ( add_producer( context, tractor, 0, mlt_producer_get_out( MLT_PRODUCER( tractor ) ) ) == 0 )
		
		// Otherwise, push the tractor back onto the stack
		context_push_service( context, tractor );
}

static void on_end_property( deserialise_context context, const xmlChar *name )
{
	// Tell parser to stop building a tree
	context->is_value = 0;
	
	// See if there is a xml tree for the value
	if ( context->property != NULL && context->value_doc != NULL )
	{
		xmlChar *value;
		int size;
		
		// Serialise the tree to get value
		xmlDocDumpMemory( context->value_doc, &value, &size );
		mlt_properties_set( context->producer_properties, context->property, value );
		xmlFree( value );
		xmlFreeDoc( context->value_doc );
		context->value_doc = NULL;
	}
	
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
		
	char *resource = mlt_properties_get( properties, "resource" );
	// Let Kino-SMIL src be a synonym for resource
	if ( resource == NULL )
		resource = mlt_properties_get( properties, "src" );
	
	// Instantiate the producer
	if ( mlt_properties_get( properties, "mlt_service" ) != NULL )
	{
		char temp[ 1024 ];
		strncpy( temp, mlt_properties_get( properties, "mlt_service" ), 1024 );
		if ( resource != NULL )
		{
			strcat( temp, ":" );
			strncat( temp, resource, 1023 - strlen( temp ) );
		}
		service = MLT_SERVICE( mlt_factory_producer( "fezzik", temp ) );
	}
	if ( service == NULL && resource != NULL )
	{
		char *root = mlt_properties_get( context->producer_map, "_root" );
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
	if ( service == NULL )
		return;
	track_service( context->destructors, service, (mlt_destructor) mlt_producer_close );

	// Add the producer to the producer map
	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

	// Handle in/out properties separately
	mlt_position in = -1;
	mlt_position out = -1;
	
	// Get in
	if ( mlt_properties_get( properties, "in" ) != NULL )
		in = mlt_properties_get_position( properties, "in" );
	// Let Kino-SMIL clipBegin be a synonym for in
	if ( mlt_properties_get( properties, "clipBegin" ) != NULL )
		in = mlt_properties_get_position( properties, "clipBegin" );
	// Get out
	if ( mlt_properties_get( properties, "out" ) != NULL )
		out = mlt_properties_get_position( properties, "out" );
	// Let Kino-SMIL clipEnd be a synonym for out
	if ( mlt_properties_get( properties, "clipEnd" ) != NULL )
		out = mlt_properties_get_position( properties, "clipEnd" );
	
	// Remove in and out
	mlt_properties_set( properties, "in", NULL );
	mlt_properties_set( properties, "out", NULL );
	
	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );
	context->producer_properties = NULL;

	// See if the producer should be added to a playlist or multitrack
	if ( add_producer( context, service, in, out ) == 0 )
	{
		// Otherwise, set in and out on...
		if ( in != -1 || out != -1 )
		{
			// Get the parent service
			mlt_service parent = context_pop_service( context );
			if ( parent != NULL )
			{
				// Get the parent properties
				properties = mlt_service_properties( parent );
				
				char *resource = mlt_properties_get( properties, "resource" );
				
				// Put the parent producer back
				context_push_service( context, parent );
					
				// If the parent is a track or entry
				if ( resource && ( strcmp( resource, "<entry>" ) == 0 ) )
				{
					mlt_properties_set_position( properties, "in", in );
					mlt_properties_set_position( properties, "out", out );
				}
				else
				{
					// Otherwise, set in and out on producer directly
					mlt_producer_set_in_and_out( MLT_PRODUCER( service ), in, out );
				}
			}
			else
			{
				// Otherwise, set in and out on producer directly
				mlt_producer_set_in_and_out( MLT_PRODUCER( service ), in, out );
			}
		}
	
		// Push the producer onto the stack
		context_push_service( context, service );
	}
}

static void on_end_filter( deserialise_context context, const xmlChar *name )
{
	mlt_properties properties = context->producer_properties;
	if ( properties == NULL )
		return;

	char *id;
	char key[11];
	key[ 10 ] = '\0';
	mlt_service tractor = NULL;
	
	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );
	if ( producer == NULL )
		return;
	
	// See if the producer is a tractor
	char *resource = mlt_properties_get( mlt_service_properties( producer ), "resource" );
	if ( resource != NULL && strcmp( resource, "<tractor>" ) == 0 )
	{
		// If so, then get the next producer
		tractor = producer;
		producer = context_pop_service( context );
	}
	
//fprintf( stderr, "connecting filter to %s\n", mlt_properties_get( mlt_service_properties( producer ), "resource" ) );

	// Create the filter
	mlt_service service = MLT_SERVICE( mlt_factory_filter( mlt_properties_get( properties, "mlt_service" ), NULL ) );
	if ( service == NULL )
	{
		context_push_service( context, producer );
		return;
	}
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

	// Set in and out again due to inheritance
	mlt_filter_set_in_and_out( MLT_FILTER( service ), 
		mlt_properties_get_position( properties, "in" ),
		mlt_properties_get_position( properties, "out" ) );

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
	
	if ( tractor != NULL )
	{
		// Connect the tractor to the filter
		mlt_tractor_connect( MLT_TRACTOR( tractor ), service );

		// Push the tractor back onto the stack
		context_push_service( context, tractor );
	}
}

static void on_end_transition( deserialise_context context, const xmlChar *name )
{
	mlt_service tractor = NULL;
	mlt_properties properties = context->producer_properties;
	if ( properties == NULL )
		return;

	// Get the producer from the stack
	mlt_service producer = context_pop_service( context );
	if ( producer == NULL )
		return;

	// See if the producer is a tractor
	char *resource = mlt_properties_get( mlt_service_properties( producer ), "resource" );
	if ( resource != NULL && strcmp( resource, "<tractor>" ) == 0 )
	{
		// If so, then get the next producer
		tractor = producer;
		producer = context_pop_service( context );
	}
	
	// Create the transition
	mlt_service service = MLT_SERVICE( mlt_factory_transition( mlt_properties_get( properties, "mlt_service" ), NULL ) );
	if ( service == NULL )
	{
		context_push_service( context, producer );
		return;
	}
	track_service( context->destructors, service, (mlt_destructor) mlt_transition_close );

	// Propogate the properties
	mlt_properties_inherit( mlt_service_properties( service ), properties );
	mlt_properties_close( properties );
	context->producer_properties = NULL;
	properties = mlt_service_properties( service );

	// Set in and out again due to inheritance
	mlt_transition_set_in_and_out( MLT_TRANSITION( service ),
		mlt_properties_get_position( properties, "in" ),
		mlt_properties_get_position( properties, "out" ) );

	// Connect the transition to the producer
	mlt_transition_connect( MLT_TRANSITION( service ), producer,
		mlt_properties_get_int( properties, "a_track" ),
		mlt_properties_get_int( properties, "b_track" ) );

	// Push the transition onto the stack
	context_push_service( context, service );
	
	if ( tractor != NULL )
	{
		// Connect the tractor to the transition
		mlt_tractor_connect( MLT_TRACTOR( tractor ), service );

		// Push the tractor back onto the stack
		context_push_service( context, tractor );
	}
}

static void on_start_element( void *ctx, const xmlChar *name, const xmlChar **atts)
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
//printf("on_start_element: %s\n", name );
	context->branch[ context->depth ] ++;
	context->depth ++;
	
	// Build a tree from nodes within a property value
	if ( context->is_value == 1 )
	{
		xmlNodePtr node = xmlNewNode( NULL, name );
		
		if ( context->value_doc == NULL )
		{
			// Start a new tree
			context->value_doc = xmlNewDoc( "1.0" );
			xmlDocSetRootElement( context->value_doc, node );
		}
		else
		{
			// Append child to tree
			xmlAddChild( context->stack_node[ context->stack_node_size - 1 ], node );
		}
		context_push_node( context, node );
		
		// Set the attributes
		for ( ; atts != NULL && *atts != NULL; atts += 2 )
			xmlSetProp( node, atts[ 0 ], atts[ 1 ] );
	}
	else if ( strcmp( name, "tractor" ) == 0 )
		on_start_tractor( context, name, atts );
	else if ( strcmp( name, "multitrack" ) == 0 )
		on_start_multitrack( context, name, atts );
	else if ( strcmp( name, "playlist" ) == 0 || strcmp( name, "seq" ) == 0 || strcmp( name, "smil" ) == 0 )
		on_start_playlist( context, name, atts );
	else if ( strcmp( name, "producer" ) == 0 || strcmp( name, "video" ) == 0 )
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
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
//printf("on_end_element: %s\n", name );
	if ( context->is_value == 1 && strcmp( name, "property" ) != 0 )
		context_pop_node( context );
	else if ( strcmp( name, "multitrack" ) == 0 )
		on_end_multitrack( context, name );
	else if ( strcmp( name, "playlist" ) == 0 || strcmp( name, "seq" ) == 0 || strcmp( name, "smil" ) == 0 )
		on_end_playlist( context, name );
	else if ( strcmp( name, "track" ) == 0 )
		on_end_track( context, name );
	else if ( strcmp( name, "entry" ) == 0 )
		on_end_entry( context, name );
	else if ( strcmp( name, "tractor" ) == 0 )
		on_end_tractor( context, name );
	else if ( strcmp( name, "property" ) == 0 )
		on_end_property( context, name );
	else if ( strcmp( name, "producer" ) == 0 || strcmp( name, "video" ) == 0 )
		on_end_producer( context, name );
	else if ( strcmp( name, "filter" ) == 0 )
		on_end_filter( context, name );
	else if ( strcmp( name, "transition" ) == 0 )
		on_end_transition( context, name );

	context->branch[ context->depth ] = 0;
	context->depth --;
}

static void on_characters( void *ctx, const xmlChar *ch, int len )
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	char *value = calloc( len + 1, 1 );

	value[ len ] = 0;
	strncpy( value, (const char*) ch, len );

	if ( context->stack_node_size > 0 )
		xmlNodeAddContent( context->stack_node[ context->stack_node_size - 1 ], ( xmlChar* )value );

	// libxml2 generates an on_characters immediately after a get_entity within
	// an element value, and we ignore it because it is called again during
	// actual substitution.
	else if ( context->property != NULL && context->producer_properties != NULL
		&& context->entity_is_replace == 0 )
	{
		char *s = mlt_properties_get( context->producer_properties, context->property );
		if ( s != NULL )
		{
			// Append new text to existing content
			char *new = calloc( strlen( s ) + len + 1, 1 );
			strcat( new, s );
			strcat( new, value );
			mlt_properties_set( context->producer_properties, context->property, new );
			free( new );
		}
		else
			mlt_properties_set( context->producer_properties, context->property, value );
	}
	context->entity_is_replace = 0;
		
	free( value);
}

/** Convert parameters parsed from resource into entity declarations.
*/
static void params_to_entities( deserialise_context context )
{
	if ( context->params != NULL )
	{	
		int i;
		
		// Add our params as entitiy declarations
		for ( i = 0; i < mlt_properties_count( context->params ); i++ )
		{
			xmlChar *name = ( xmlChar* )mlt_properties_get_name( context->params, i );
			xmlAddDocEntity( context->entity_doc, name, XML_INTERNAL_GENERAL_ENTITY,
				context->publicId, context->systemId, ( xmlChar* )mlt_properties_get( context->params, name ) );
		}

		// Flag completion
		mlt_properties_close( context->params );
		context->params = NULL;
	}
}

// The following 3 facilitate entity substitution in the SAX parser
static void on_internal_subset( void *ctx, const xmlChar* name,
	const xmlChar* publicId, const xmlChar* systemId )
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
	context->publicId = publicId;
	context->systemId = systemId;
	xmlCreateIntSubset( context->entity_doc, name, publicId, systemId );
	
	// Override default entities with our parameters
	params_to_entities( context );
}

static void on_entity_declaration( void *ctx, const xmlChar* name, int type, 
	const xmlChar* publicId, const xmlChar* systemId, xmlChar* content)
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
	xmlAddDocEntity( context->entity_doc, name, type, publicId, systemId, content );
}

xmlEntityPtr on_get_entity( void *ctx, const xmlChar* name )
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	xmlEntityPtr e = NULL;

	// Setup for entity declarations if not ready
	if ( xmlGetIntSubset( context->entity_doc ) == NULL )
	{
		xmlCreateIntSubset( context->entity_doc, "westley", "", "" );
		context->publicId = "";
		context->systemId = "";
	}

	// Add our parameters if not already
	params_to_entities( context );
	
	e = xmlGetDocEntity( context->entity_doc, name );
	
	// Send signal to on_characters that an entity substitutin is pending
	if ( e != NULL )
		context->entity_is_replace = 1;
	
	return e;
}

/** Convert a hexadecimal character to its value.
*/
static int tohex( char p )
{
	return isdigit( p ) ? p - '0' : tolower( p ) - 'a' + 10;
}

/** Decode a url-encoded string containing hexadecimal character sequences.
*/
static char *url_decode( char *dest, char *src )
{
	char *p = dest;
	
	while ( *src )
	{
		if ( *src == '%' )
		{
			*p ++ = ( tohex( *( src + 1 ) ) << 4 ) | tohex( *( src + 2 ) );
			src += 3;
		}
		else
		{
			*p ++ = *src ++;
		}
	}

	*p = *src;
	return dest;
}

/** Extract the filename from a URL attaching parameters to a properties list.
*/
static void parse_url( mlt_properties properties, char *url )
{
	int i;
	int n = strlen( url );
	char *name = NULL;
	char *value = NULL;
	
	for ( i = 0; i < n; i++ )
	{
		switch ( url[ i ] )
		{
			case '?':
				url[ i++ ] = '\0';
				name = &url[ i ];
				break;
			
			case ':':
			case '=':
				url[ i++ ] = '\0';
				value = &url[ i ];
				break;
			
			case '&':
				url[ i++ ] = '\0';
				if ( name != NULL && value != NULL )
					mlt_properties_set( properties, name, value );
				name = &url[ i ];
				value = NULL;
				break;
		}
	}
	if ( name != NULL && value != NULL )
		mlt_properties_set( properties, name, value );
}

mlt_producer producer_westley_init( char *url )
{
	if ( url == NULL )
		return NULL;
	xmlSAXHandler *sax = calloc( 1, sizeof( xmlSAXHandler ) );
	struct deserialise_context_s *context = calloc( 1, sizeof( struct deserialise_context_s ) );
	mlt_properties properties = NULL;
	int i = 0;
	struct _xmlParserCtxt *xmlcontext;
	int well_formed = 0;
	char *filename = strdup( url );
	
	context->producer_map = mlt_properties_new();
	context->destructors = mlt_properties_new();
	context->params = mlt_properties_new();

	// Decode URL and parse parameters	
	parse_url( context->params, url_decode( filename, url ) );

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

	// Setup SAX callbacks
	sax->startElement = on_start_element;
	sax->endElement = on_end_element;
	sax->characters = on_characters;
	sax->cdataBlock = on_characters;
	sax->internalSubset = on_internal_subset;
	sax->entityDecl = on_entity_declaration;
	sax->getEntity = on_get_entity;

	// Setup libxml2 SAX parsing
	xmlInitParser(); 
	xmlSubstituteEntitiesDefault( 1 );
	// This is used to facilitate entity substitution in the SAX parser
	context->entity_doc = xmlNewDoc( "1.0" );
	xmlcontext = xmlCreateFileParserCtxt( filename );
	xmlcontext->sax = sax;
	xmlcontext->_private = ( void* )context;
	
	// Parse
	xmlParseDocument( xmlcontext );
	well_formed = xmlcontext->wellFormed;
	
	// Cleanup after parsing
	xmlFreeDoc( context->entity_doc );
	free( sax );
	xmlcontext->sax = NULL;
	xmlcontext->_private = NULL;
	xmlFreeParserCtxt( xmlcontext );
	xmlMemoryDump( ); // for debugging

	// Get the last producer on the stack
	mlt_service service = context_pop_service( context );
	if ( well_formed && service != NULL )
	{
		// Verify it is a producer service (mlt_type="mlt_producer")
		// (producer, playlist, multitrack)
		char *type = mlt_properties_get( mlt_service_properties( service ), "mlt_type" );
		if ( type == NULL || ( strcmp( type, "mlt_producer" ) != 0 && strcmp( type, "producer" ) != 0 ) )
			service = NULL;
	}

#ifdef DEBUG
	xmlDocPtr doc = westley_make_doc( service );
	xmlDocFormatDump( stdout, doc, 1 );
	xmlFreeDoc( doc );
	service = NULL;
#endif
	
	if ( well_formed && service != NULL )
	{
		
		// Need the complete producer list for various reasons
		properties = context->destructors;

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
		mlt_properties_set( properties, "resource", url );

		// This tells consumer_westley not to deep copy
		mlt_properties_set( properties, "westley", "was here" );
	}
	else
	{
		// Return null if not well formed
		service = NULL;
		
		// Clean up
		mlt_properties_close( context->destructors );
	}

	// Clean up
	mlt_properties_close( context->producer_map );
	if ( context->params != NULL )
		mlt_properties_close( context->params );
	free( context );
	free( filename );

	return MLT_PRODUCER( service );
}
