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
#include <libxml/tree.h>

#define ID_SIZE 128

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );

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
		// We have stuff to clean up, so override the close method
		//parent->close = consumer_close;

		// Allow thread to be started/stopped
		this->start = consumer_start;

		mlt_properties_set( mlt_consumer_properties( this ), "resource", arg );

		// Return the consumer produced
		return this;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}


// This maintains counters for adding ids to elements
struct serialise_context_s
{
	int producer_count;
	int multitrack_count;
	int playlist_count;
	int tractor_count;
	int filter_count;
	int transition_count;
	int pass;
	mlt_properties producer_map;
};
typedef struct serialise_context_s* serialise_context;


static inline void serialise_properties( mlt_properties properties, xmlNode *node )
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
			 strcmp( name, "westley" ) != 0 )
		{
#if 0
			p = xmlNewChild( node, NULL, "prop", NULL );
#else
			p = node;
#endif
			xmlNewProp( p, mlt_properties_get_name( properties, i ), mlt_properties_get_value( properties, i ) );
		}
	}
}

static void serialise_service( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;
	xmlNode *child = node;
	char id[ ID_SIZE + 1 ];
	char key[ 11 ];
	id[ ID_SIZE ] = '\0';
	key[ 10 ] = '\0';
	
	// Iterate over consumer/producer connections
	while ( service != NULL )
	{
		mlt_properties properties = mlt_service_properties( service );
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );
		
		// Tell about the producer
		if ( strcmp( mlt_type, "producer" ) == 0 )
		{
			if ( context->pass == 0 )
			{
				child = xmlNewChild( node, NULL, "producer", NULL );

				// Set the id
				if ( mlt_properties_get( properties, "id" ) == NULL )
				{
					snprintf( id, ID_SIZE, "producer%d", context->producer_count++ );
					xmlNewProp( child, "id", id );
				}
				else
					strncpy( id, mlt_properties_get( properties, "id" ), ID_SIZE );
				serialise_properties( properties, child );

				// Add producer to the map
				snprintf( key, 10, "%p", service );
				mlt_properties_set( context->producer_map, key, id );
			}
			else
			{
				snprintf( key, 10, "%p", service );
				xmlNewProp( node, "producer", mlt_properties_get( context->producer_map, key ) );
			}
			if ( mlt_properties_get( properties, "westley" ) != NULL )
				break;
		}

		// Tell about the framework container producers
		else if ( strcmp( mlt_type, "mlt_producer" ) == 0 )
		{
			// Recurse on multitrack's tracks
			if ( strcmp( mlt_properties_get( properties, "resource" ), "<multitrack>" ) == 0 )
			{
				if ( context->pass == 0 )
				{
					// Iterate over the tracks
					for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
					{
						serialise_service( context, MLT_SERVICE( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) ), node );
					}
				}
				else
				{
					// Iterate over the tracks to collect the producers
					for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
					{
						serialise_service( context, MLT_SERVICE( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) ), node );
					}

					child = xmlNewChild( node, NULL, "multitrack", NULL );
				
					// Set the id
					if ( mlt_properties_get( properties, "id" ) == NULL )
					{
						snprintf( id, ID_SIZE, "multitrack%d", context->multitrack_count++ );
						xmlNewProp( child, "id", id );
					}

					// Iterate over the tracks
					for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
					{
						xmlNode *track = xmlNewChild( child, NULL, "track", NULL );
						snprintf( key, 10, "%p", MLT_SERVICE( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) ) );
						xmlNewProp( track, "producer", mlt_properties_get( context->producer_map, key ) );
					}
				}
				break;
			}
			
			// Recurse on playlist's clips
			else if ( strcmp( mlt_properties_get( properties, "resource" ), "<playlist>" ) == 0 )
			{
				mlt_playlist_clip_info info;

				if ( context->pass == 0 )
				{
					// Iterate over the playlist entries to collect the producers
					for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
					{
						if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
						{
							if ( strcmp( mlt_properties_get( mlt_producer_properties( info.producer ), "mlt_service" ), "blank" ) != 0 )
							{
								serialise_service( context, MLT_SERVICE( info.producer ), node );
							}
						}
					}
					
					child = xmlNewChild( node, NULL, "playlist", NULL );

					// Set the id
					if ( mlt_properties_get( properties, "id" ) == NULL )
					{
						snprintf( id, ID_SIZE, "playlist%d", context->playlist_count++ );
						xmlNewProp( child, "id", id );
					}
					else
						strncpy( id, mlt_properties_get( properties, "id" ), ID_SIZE );

					xmlNewProp( child, "in", mlt_properties_get( properties, "in" ) );
					xmlNewProp( child, "out", mlt_properties_get( properties, "out" ) );

					// Add producer to the map
					snprintf( key, 10, "%p", service );
					mlt_properties_set( context->producer_map, key, id );
				
					// Iterate over the playlist entries
					for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
					{
						if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
						{
							if ( strcmp( mlt_properties_get( mlt_producer_properties( info.producer ), "mlt_service" ), "blank" ) == 0 )
							{
								char length[ 20 ];
								length[ 19 ] = '\0';
								xmlNode *entry = xmlNewChild( child, NULL, "blank", NULL );
								snprintf( length, 19, "%lld", info.frame_count );
								xmlNewProp( entry, "length", length );
							}
							else
							{
								xmlNode *entry = xmlNewChild( child, NULL, "entry", NULL );
								snprintf( key, 10, "%p", MLT_SERVICE( info.producer ) );
								xmlNewProp( entry, "producer", mlt_properties_get( context->producer_map, key ) );
							}
						}
					}
				}
				else if ( strcmp( (const char*) node->name, "tractor" ) != 0 )
				{
					snprintf( key, 10, "%p", service );
					xmlNewProp( node, "producer", mlt_properties_get( context->producer_map, key ) );
				}
			}
			
			// Recurse on tractor's producer
			else if ( strcmp( mlt_properties_get( properties, "resource" ), "<tractor>" ) == 0 )
			{
				if ( context->pass == 0 )
				{
					// Recurse on connected producer
					serialise_service( context, mlt_service_get_producer( service ), node );
				}
				else
				{
					child = xmlNewChild( node, NULL, "tractor", NULL );

					// Set the id
					if ( mlt_properties_get( properties, "id" ) == NULL )
					{
						snprintf( id, ID_SIZE, "tractor%d", context->tractor_count++ );
						xmlNewProp( child, "id", id );
					}
					
					xmlNewProp( child, "in", mlt_properties_get( properties, "in" ) );
					xmlNewProp( child, "out", mlt_properties_get( properties, "out" ) );

					// Recurse on connected producer
					serialise_service( context, mlt_service_get_producer( service ), child );
				}
				break;
			}
		}
		
		// Tell about a filter
		else if ( strcmp( mlt_type, "filter" ) == 0 )
		{
			// Recurse on connected producer
			serialise_service( context, MLT_SERVICE( MLT_FILTER( service )->producer ), node );

			if ( context->pass == 1 )
			{
				child = xmlNewChild( node, NULL, "filter", NULL );

				// Set the id
				if ( mlt_properties_get( properties, "id" ) == NULL )
				{
					snprintf( id, ID_SIZE, "filter%d", context->filter_count++ );
					xmlNewProp( child, "id", id );
				}

				serialise_properties( properties, child );
			}
			break;
		}
		
		// Tell about a transition
		else if ( strcmp( mlt_type, "transition" ) == 0 )
		{
			// Recurse on connected producer
			serialise_service( context, MLT_SERVICE( MLT_TRANSITION( service )->producer ), node );

			if ( context->pass == 1 )
			{
				child = xmlNewChild( node, NULL, "transition", NULL );
			
				// Set the id
				if ( mlt_properties_get( properties, "id" ) == NULL )
				{
					snprintf( id, ID_SIZE, "transition%d", context->transition_count++ );
					xmlNewProp( child, "id", id );
				}

				serialise_properties( properties, child );
			}
			break;
		}
		
		// Get the next connected service
		service = mlt_service_get_producer( service );
	}
}

static int consumer_start( mlt_consumer this )
{
	mlt_service inigo = NULL;
	xmlDoc *doc = xmlNewDoc( "1.0" );
	xmlNode *root = xmlNewNode( NULL, "westley" );
	xmlDocSetRootElement( doc, root );
	
	// Get the producer service
	mlt_service service = mlt_service_get_producer( mlt_consumer_service( this ) );
	if ( service != NULL )
	{
		struct serialise_context_s *context = calloc( 1, sizeof( struct serialise_context_s ) );
		context->producer_map = mlt_properties_new();
		
		// Remember inigo
		if ( mlt_properties_get( mlt_service_properties( service ), "mlt_service" ) != NULL &&
				strcmp( mlt_properties_get( mlt_service_properties( service ), "mlt_service" ), "inigo" ) == 0 )
			inigo = service;
		
		// Ensure producer is a framework producer
		mlt_properties_set( mlt_service_properties( service ), "mlt_type", "mlt_producer" );

		// In pass one, we serialise the end producers and playlists,
		// adding them to a map keyed by address.
		serialise_service( context, service, root );

		// In pass two, we serialise the tractor and reference the
		// producers and playlists
		context->pass++;
		serialise_service( context, service, root );

		mlt_properties_close( context->producer_map );
		free( context );
		
		if ( mlt_properties_get( mlt_consumer_properties( this ), "resource" ) == NULL )
			xmlDocFormatDump( stdout, doc, 1 );
		else
			xmlSaveFormatFile( mlt_properties_get( mlt_consumer_properties( this ), "resource" ), doc, 1 );
	}

	xmlFreeDoc( doc );
	mlt_consumer_stop( this );

	// Tell inigo, enough already!
	if ( inigo != NULL )
		mlt_properties_set_int( mlt_service_properties( inigo ), "done", 1 );
	
	return 0;
}

