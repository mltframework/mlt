/*
 * consumer_westley.c -- a libxml2 serialiser of mlt service networks
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

#include "consumer_westley.h"
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <libxml/tree.h>

#define ID_SIZE 128

// This maintains counters for adding ids to elements
struct serialise_context_s
{
	mlt_properties id_map;
	int producer_count;
	int multitrack_count;
	int playlist_count;
	int tractor_count;
	int filter_count;
	int transition_count;
	int pass;
	mlt_properties hide_map;
	char *root;
};
typedef struct serialise_context_s* serialise_context;

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer this );
static void serialise_service( serialise_context context, mlt_service service, xmlNode *node );

typedef enum 
{
	westley_existing,
	westley_producer,
	westley_multitrack,
	westley_playlist,
	westley_tractor,
	westley_filter,
	westley_transition
}
westley_type;

/** Create or retrieve an id associated to this service.
*/

static char *westley_get_id( serialise_context context, mlt_service service, westley_type type )
{
	char *id = NULL;
	int i = 0;
	mlt_properties map = context->id_map;

	// Search the map for the service
	for ( i = 0; i < mlt_properties_count( map ); i ++ )
		if ( mlt_properties_get_data_at( map, i, NULL ) == service )
			break;

	// If the service is not in the map, and the type indicates a new id is needed...
	if ( i >= mlt_properties_count( map ) && type != westley_existing )
	{
		// Attempt to reuse existing id
		id = mlt_properties_get( mlt_service_properties( service ), "id" );

		// If no id, or the id is used in the map (for another service), then 
		// create a new one.
		if ( id == NULL || mlt_properties_get_data( map, id, NULL ) != NULL )
		{
			char temp[ ID_SIZE ];
			do
			{
				switch( type )
				{
					case westley_producer:
						sprintf( temp, "producer%d", context->producer_count ++ );
						break;
					case westley_multitrack:
						sprintf( temp, "multitrack%d", context->multitrack_count ++ );
						break;
					case westley_playlist:
						sprintf( temp, "playlist%d", context->playlist_count ++ );
						break;
					case westley_tractor:
						sprintf( temp, "tractor%d", context->tractor_count ++ );
						break;
					case westley_filter:
						sprintf( temp, "filter%d", context->filter_count ++ );
						break;
					case westley_transition:
						sprintf( temp, "transition%d", context->transition_count ++ );
						break;
					case westley_existing:
						// Never gets here
						break;
				}
			}
			while( mlt_properties_get_data( map, temp, NULL ) != NULL );

			// Set the data at the generated name
			mlt_properties_set_data( map, temp, service, 0, NULL, NULL );

			// Get the pointer to the name (i is the end of the list)
			id = mlt_properties_get_name( map, i );
		}
		else
		{
			// Store the existing id in the map
			mlt_properties_set_data( map, id, service, 0, NULL, NULL );
		}
	}
	else if ( type == westley_existing )
	{
		id = mlt_properties_get_name( map, i );
	}

	return id;
}

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_westley_init( char *arg )
{
	// Create the consumer object
	mlt_consumer this = calloc( sizeof( struct mlt_consumer_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( this, NULL ) == 0 )
	{
		// Allow thread to be started/stopped
		this->start = consumer_start;
		this->is_stopped = consumer_is_stopped;

		mlt_properties_set( mlt_consumer_properties( this ), "resource", arg );

		// Return the consumer produced
		return this;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

static inline void serialise_properties( serialise_context context, mlt_properties properties, xmlNode *node )
{
	int i;
	xmlNode *p;
	
	// Enumerate the properties
	for ( i = 0; i < mlt_properties_count( properties ); i++ )
	{
		char *name = mlt_properties_get_name( properties, i );
		if ( name != NULL &&
			 name[ 0 ] != '_' &&
			 mlt_properties_get_value( properties, i ) != NULL &&
			 strcmp( name, "westley" ) != 0 &&
			 strcmp( name, "in" ) != 0 &&
			 strcmp( name, "out" ) != 0 && 
			 strcmp( name, "id" ) != 0 && 
			 strcmp( name, "root" ) != 0 && 
			 strcmp( name, "width" ) != 0 &&
			 strcmp( name, "height" ) != 0 )
		{
			char *value = mlt_properties_get_value( properties, i );
			p = xmlNewChild( node, NULL, "property", NULL );
			xmlNewProp( p, "name", mlt_properties_get_name( properties, i ) );
			if ( strcmp( context->root, "" ) && !strncmp( value, context->root, strlen( context->root ) ) )
				xmlNodeSetContent( p, value + strlen( context->root ) + 1 );
			else
				xmlNodeSetContent( p, value );
		}
	}
}

static inline void serialise_service_filters( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;
	xmlNode *p;
	mlt_filter filter = NULL;
	
	// Enumerate the filters
	for ( i = 0; ( filter = mlt_producer_filter( MLT_PRODUCER( service ), i ) ) != NULL; i ++ )
	{
		mlt_properties properties = mlt_filter_properties( filter );
		if ( mlt_properties_get_int( properties, "_fezzik" ) == 0 )
		{
			// Get a new id - if already allocated, do nothing
			char *id = westley_get_id( context, mlt_filter_service( filter ), westley_filter );
			if ( id != NULL )
			{
				int in = mlt_properties_get_position( properties, "in" );
				int out = mlt_properties_get_position( properties, "out" );
				p = xmlNewChild( node, NULL, "filter", NULL );
				xmlNewProp( p, "id", id );
				if ( in != 0 || out != 0 )
				{
					char temp[ 20 ];
					sprintf( temp, "%d", in );
					xmlNewProp( p, "in", temp );
					sprintf( temp, "%d", out );
					xmlNewProp( p, "out", temp );
				}
				serialise_properties( context, properties, p );
				serialise_service_filters( context, mlt_filter_service( filter ), p );
			}
		}
	}
}

static void serialise_producer( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = mlt_service_properties( service );
	
	if ( context->pass == 0 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_producer );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, "producer", NULL );

		// Set the id
		xmlNewProp( child, "id", id );
		serialise_properties( context, properties, child );
		serialise_service_filters( context, service, child );

		// Add producer to the map
		mlt_properties_set_int( context->hide_map, id, mlt_properties_get_int( properties, "hide" ) );
	}
	else
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_existing );
		xmlNewProp( node, "producer", id );
	}
}

static void serialise_multitrack( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;
	
	if ( context->pass == 0 )
	{
		// Iterate over the tracks to collect the producers
		for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
			serialise_service( context, MLT_SERVICE( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) ), node );
	}
	else
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_multitrack );
		if ( id == NULL )
			return;

		// Serialise the tracks
		for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
		{
			xmlNode *track = xmlNewChild( node, NULL, "track", NULL );
			int hide = 0;
			mlt_producer producer = mlt_multitrack_track( MLT_MULTITRACK( service ), i );

			char *id = westley_get_id( context, MLT_SERVICE( producer ), westley_existing );
			xmlNewProp( track, "producer", id );
			
			hide = mlt_properties_get_int( context->hide_map, id );
			if ( hide )
				xmlNewProp( track, "hide", hide == 1 ? "video" : ( hide == 2 ? "audio" : "both" ) );
		}
		serialise_service_filters( context, service, node );
	}
}

static void serialise_tractor( serialise_context context, mlt_service service, xmlNode *node );

static void serialise_playlist( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;
	xmlNode *child = node;
	mlt_playlist_clip_info info;
	mlt_properties properties = mlt_service_properties( service );
	
	if ( context->pass == 0 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_playlist );
		if ( id == NULL )
			return;

		// Iterate over the playlist entries to collect the producers
		for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
		{
			if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
			{
				if ( info.producer != NULL )
				{
					char *service_s = mlt_properties_get( mlt_producer_properties( info.producer ), "mlt_service" );
					char *resource_s = mlt_properties_get( mlt_producer_properties( info.producer ), "resource" );
					if ( resource_s != NULL && !strcmp( resource_s, "<tractor>" ) )
					{
						serialise_tractor( context, MLT_SERVICE( info.producer ), node );
						context->pass ++;
						serialise_tractor( context, MLT_SERVICE( info.producer ), node );
						context->pass --;
					}
					else if ( service_s != NULL && strcmp( service_s, "blank" ) != 0 )
						serialise_service( context, MLT_SERVICE( info.producer ), node );
					else if ( resource_s != NULL && !strcmp( resource_s, "<playlist>" ) )
						serialise_playlist( context, MLT_SERVICE( info.producer ), node );
				}
			}
		}
		
		child = xmlNewChild( node, NULL, "playlist", NULL );

		// Set the id
		xmlNewProp( child, "id", id );

		// Add producer to the map
		mlt_properties_set_int( context->hide_map, id, mlt_properties_get_int( properties, "hide" ) );
	
		// Iterate over the playlist entries
		for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
		{
			if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
			{
				char *service_s = mlt_properties_get( mlt_producer_properties( info.producer ), "mlt_service" );
				if ( service_s != NULL && strcmp( service_s, "blank" ) == 0 )
				{
					char length[ 20 ];
					length[ 19 ] = '\0';
					xmlNode *entry = xmlNewChild( child, NULL, "blank", NULL );
					snprintf( length, 19, "%d", info.frame_count );
					xmlNewProp( entry, "length", length );
				}
				else
				{
					char temp[ 20 ];
					xmlNode *entry = xmlNewChild( child, NULL, "entry", NULL );
					id = westley_get_id( context, MLT_SERVICE( info.producer ), westley_existing );
					xmlNewProp( entry, "producer", id );
					sprintf( temp, "%d", info.frame_in );
					xmlNewProp( entry, "in", temp );
					sprintf( temp, "%d", info.frame_out );
					xmlNewProp( entry, "out", temp );
				}
			}
		}

		serialise_service_filters( context, service, child );
	}
	else if ( strcmp( (const char*) node->name, "tractor" ) != 0 )
	{
		char *id = westley_get_id( context, service, westley_existing );
		xmlNewProp( node, "producer", id );
	}
}

static void serialise_tractor( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = mlt_service_properties( service );
	
	if ( context->pass == 0 )
	{
		// Recurse on connected producer
		serialise_service( context, mlt_service_producer( service ), node );
	}
	else
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_tractor );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, "tractor", NULL );

		// Set the id
		xmlNewProp( child, "id", id );
		xmlNewProp( child, "in", mlt_properties_get( properties, "in" ) );
		xmlNewProp( child, "out", mlt_properties_get( properties, "out" ) );

		// Recurse on connected producer
		serialise_service( context, mlt_service_producer( service ), child );
		serialise_service_filters( context, service, child );
	}
}

static void serialise_filter( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = mlt_service_properties( service );
	
	// Recurse on connected producer
	serialise_service( context, mlt_service_producer( service ), node );

	if ( context->pass == 1 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_filter );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, "filter", NULL );

		// Set the id
		xmlNewProp( child, "id", id );
		xmlNewProp( child, "in", mlt_properties_get( properties, "in" ) );
		xmlNewProp( child, "out", mlt_properties_get( properties, "out" ) );

		serialise_properties( context, properties, child );
		serialise_service_filters( context, service, child );
	}
}

static void serialise_transition( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = mlt_service_properties( service );
	
	// Recurse on connected producer
	serialise_service( context, MLT_SERVICE( MLT_TRANSITION( service )->producer ), node );

	if ( context->pass == 1 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = westley_get_id( context, service, westley_transition );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, "transition", NULL );
	
		// Set the id
		xmlNewProp( child, "id", id );
		xmlNewProp( child, "in", mlt_properties_get( properties, "in" ) );
		xmlNewProp( child, "out", mlt_properties_get( properties, "out" ) );

		serialise_properties( context, properties, child );
		serialise_service_filters( context, service, child );
	}
}

static void serialise_service( serialise_context context, mlt_service service, xmlNode *node )
{
	// Iterate over consumer/producer connections
	while ( service != NULL )
	{
		mlt_properties properties = mlt_service_properties( service );
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );
		
		// Tell about the producer
		if ( strcmp( mlt_type, "producer" ) == 0 )
		{
			serialise_producer( context, service, node );
			if ( mlt_properties_get( properties, "westley" ) != NULL )
				break;
		}

		// Tell about the framework container producers
		else if ( strcmp( mlt_type, "mlt_producer" ) == 0 )
		{
			char *resource = mlt_properties_get( properties, "resource" );
			
			// Recurse on multitrack's tracks
			if ( strcmp( resource, "<multitrack>" ) == 0 )
			{
				serialise_multitrack( context, service, node );
				break;
			}
			
			// Recurse on playlist's clips
			else if ( strcmp( resource, "<playlist>" ) == 0 )
			{
				serialise_playlist( context, service, node );
			}
			
			// Recurse on tractor's producer
			else if ( strcmp( resource, "<tractor>" ) == 0 )
			{
				serialise_tractor( context, service, node );
				break;
			}

			// Treat it as a normal producer
			else
			{
				serialise_producer( context, service, node );
			}
		}
		
		// Tell about a filter
		else if ( strcmp( mlt_type, "filter" ) == 0 )
		{
			serialise_filter( context, service, node );
			break;
		}
		
		// Tell about a transition
		else if ( strcmp( mlt_type, "transition" ) == 0 )
		{
			serialise_transition( context, service, node );
			break;
		}
		
		// Get the next connected service
		service = mlt_service_producer( service );
	}
}

xmlDocPtr westley_make_doc( mlt_service service )
{
	mlt_properties properties = mlt_service_properties( service );
	xmlDocPtr doc = xmlNewDoc( "1.0" );
	xmlNodePtr root = xmlNewNode( NULL, "westley" );
	struct serialise_context_s *context = calloc( 1, sizeof( struct serialise_context_s ) );
	
	xmlDocSetRootElement( doc, root );

	// If we have root, then deal with it now
	if ( mlt_properties_get( properties, "root" ) != NULL )
	{
		xmlNewProp( root, "root", mlt_properties_get( properties, "root" ) );
		context->root = strdup( mlt_properties_get( properties, "root" ) );
	}
	else
	{
		context->root = strdup( "" );
	}

	// Assign a title property
	if ( mlt_properties_get( properties, "title" ) != NULL )
		xmlNewProp( root, "title", mlt_properties_get( properties, "title" ) );

	// Construct the context maps
	context->id_map = mlt_properties_new();
	context->hide_map = mlt_properties_new();
	
	// Ensure producer is a framework producer
	mlt_properties_set( mlt_service_properties( service ), "mlt_type", "mlt_producer" );

	// In pass one, we serialise the end producers and playlists,
	// adding them to a map keyed by address.
	serialise_service( context, service, root );

	// In pass two, we serialise the tractor and reference the
	// producers and playlists
	context->pass++;
	serialise_service( context, service, root );

	// Cleanup resource
	mlt_properties_close( context->id_map );
	mlt_properties_close( context->hide_map );
	free( context->root );
	free( context );
	
	return doc;
}

static int consumer_start( mlt_consumer this )
{
	xmlDocPtr doc = NULL;
	
	// Get the producer service
	mlt_service service = mlt_service_producer( mlt_consumer_service( this ) );
	if ( service != NULL )
	{
		mlt_properties properties = mlt_consumer_properties( this );
		char *resource =  mlt_properties_get( properties, "resource" );

		// Set the title if provided
		if ( mlt_properties_get( properties, "title" ) )
			mlt_properties_set( mlt_service_properties( service ), "title", mlt_properties_get( properties, "title" ) );
		else if ( mlt_properties_get( mlt_service_properties( service ), "title" ) == NULL )
			mlt_properties_set( mlt_service_properties( service ), "title", "Anonymous Submission" );

		// Check for a root on the consumer properties and pass to service
		if ( mlt_properties_get( properties, "root" ) )
			mlt_properties_set( mlt_service_properties( service ), "root", mlt_properties_get( properties, "root" ) );

		// Specify roots in other cases...
		if ( resource != NULL && mlt_properties_get( properties, "root" ) == NULL )
		{
			// Get the current working directory
			char *cwd = getcwd( NULL, 0 );
			mlt_properties_set( mlt_service_properties( service ), "root", cwd );
			free( cwd );
		}

		// Make the document
		doc = westley_make_doc( service );

		// Handle the output
		if ( resource == NULL || !strcmp( resource, "" ) )
		{
			xmlDocFormatDump( stdout, doc, 1 );
		}
		else if ( !strcmp( resource, "buffer" ) )
		{
			xmlChar *buffer = NULL;
			int length = 0;
			xmlDocDumpMemory( doc, &buffer, &length );
			mlt_properties_set_data( properties, "buffer", buffer, length, xmlFree, NULL );
		}
		else
		{
			xmlSaveFormatFile( resource, doc, 1 );
		}
		
		// Close the document
		xmlFreeDoc( doc );
	}
	
	mlt_consumer_stop( this );

	mlt_consumer_stopped( this );

	return 0;
}

static int consumer_is_stopped( mlt_consumer this )
{
	return 1;
}
