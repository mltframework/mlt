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
};
typedef struct serialise_context_s* serialise_context;


static inline void serialise_properties( mlt_properties properties, xmlNode *node )
{
	int i;
	xmlNode *p;
	
	// Enumerate the properties
	for ( i = 0; i < mlt_properties_count( properties ); i++ )
	{
#if 0
		p = xmlNewChild( node, NULL, "prop", NULL );
#else
		p = node;
#endif
		xmlNewProp( p, mlt_properties_get_name( properties, i ), mlt_properties_get_value( properties, i ) );
	}
}

static xmlNode* serialise_service( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;
	xmlNode *child = node;
	char id[ 31 ];
	id[ 30 ] = '\0';
	
	// Iterate over consumer/producer connections
	while ( service != NULL )
	{
		mlt_properties properties = mlt_service_properties( service );
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );
		
		// Tell about the producer
		if ( strcmp( mlt_type, "producer" ) == 0 )
		{
			child = xmlNewChild( node, NULL, "producer", NULL );

			// Set the id
			if ( mlt_properties_get( properties, "id" ) != NULL )
				xmlNewProp( child, "id", mlt_properties_get( properties, "mlt_service" ) );
			else
			{
				snprintf( id, 30, "producer%d", context->producer_count++ );
				xmlNewProp( child, "id", id );
			}
			
			//xmlNewProp( child, "type", mlt_properties_get( properties, "mlt_service" ) );
			//xmlNewProp( child, "src", mlt_properties_get( properties, "resource" ) );
            serialise_properties( properties, child );
		}

		// Tell about the framework container producers
		else if ( strcmp( mlt_type, "mlt_producer" ) == 0 )
		{
			// Recurse on multitrack's tracks
			if ( strcmp( mlt_properties_get( properties, "resource" ), "<multitrack>" ) == 0 )
			{
				child = xmlNewChild( node, NULL, "multitrack", NULL );
				
				// Set the id
				if ( mlt_properties_get( properties, "id" ) != NULL )
					xmlNewProp( child, "id", mlt_properties_get( properties, "mlt_service" ) );
				else
				{
					snprintf( id, 30, "multitrack%d", context->multitrack_count++ );
					xmlNewProp( child, "id", id );
				}

				// Iterate over the tracks
				for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
				{
					xmlNode *track = xmlNewChild( child, NULL, "track", NULL );
					serialise_service( context, MLT_SERVICE( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) ), track );
				}
				break;
			}
			
			// Recurse on playlist's clips
			else if ( strcmp( mlt_properties_get( properties, "resource" ), "<playlist>" ) == 0 )
			{
				mlt_playlist_clip_info info;
				child = xmlNewChild( node, NULL, "playlist", NULL );
				
				// Set the id
				if ( mlt_properties_get( properties, "id" ) != NULL )
					xmlNewProp( child, "id", mlt_properties_get( properties, "mlt_service" ) );
				else
				{
					snprintf( id, 30, "playlist%d", context->playlist_count++ );
					xmlNewProp( child, "id", id );
				}

				xmlNewProp( child, "in", mlt_properties_get( properties, "in" ) );
				xmlNewProp( child, "out", mlt_properties_get( properties, "out" ) );

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
							snprintf( length, 19, "%lld", info.length );
							xmlNewProp( entry, "length", length );
						}
						else
						{
							xmlNode *entry = xmlNewChild( child, NULL, "entry", NULL );
							serialise_service( context, MLT_SERVICE( info.producer ), entry );
						}
					}
				}
			}
			
			// Recurse on tractor's producer
			else if ( strcmp( mlt_properties_get( properties, "resource" ), "<tractor>" ) == 0 )
			{
				child = xmlNewChild( node, NULL, "tractor", NULL );

				// Set the id
				if ( mlt_properties_get( properties, "id" ) != NULL )
					xmlNewProp( child, "id", mlt_properties_get( properties, "mlt_service" ) );
				else
				{
					snprintf( id, 30, "tractor%d", context->tractor_count++ );
					xmlNewProp( child, "id", id );
				}

				// Recurse on connected producer
				serialise_service( context, mlt_service_get_producer( service ), child );
				
				break;
			}
		}
		
		// Tell about a filter
		else if ( strcmp( mlt_type, "filter" ) == 0 )
		{
			// Recurse on connected producer
			child = serialise_service( context, MLT_SERVICE( MLT_FILTER( service )->producer ), node );

			// sanity check
			if ( child == NULL )
				break;

			node = xmlNewChild( child, NULL, "filter", NULL );

			// Set the id
			if ( mlt_properties_get( properties, "id" ) != NULL )
				xmlNewProp( node, "id", mlt_properties_get( properties, "mlt_service" ) );
			else
			{
				snprintf( id, 30, "filter%d", context->filter_count++ );
				xmlNewProp( node, "id", id );
			}

			//xmlNewProp( node, "type", mlt_properties_get( properties, "mlt_service" ) );

			serialise_properties( properties, node );

			break;
		}
		
		// Tell about a transition
		else if ( strcmp( mlt_type, "transition" ) == 0 )
		{
			// Recurse on connected producer
			child = serialise_service( context, MLT_SERVICE( MLT_TRANSITION( service )->producer ), node );

			// sanity check
			if ( child == NULL )
				break;

			node = xmlNewChild( child, NULL, "transition", NULL );
			
			// Set the id
			if ( mlt_properties_get( properties, "id" ) != NULL )
				xmlNewProp( node, "id", mlt_properties_get( properties, "mlt_service" ) );
			else
			{
				snprintf( id, 30, "transition%d", context->transition_count++ );
				xmlNewProp( node, "id", id );
			}

			//xmlNewProp( node, "type", mlt_properties_get( properties, "mlt_service" ) );

			serialise_properties( properties, node );
			
			break;
		}
		
		// Get the next connected service
		service = mlt_service_get_producer( service );
	}
	return child;
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
		struct serialise_context_s context;
		memset( &context, 0, sizeof( struct serialise_context_s ) );
		
		// Remember inigo
		if ( mlt_properties_get( mlt_service_properties( service ), "mlt_service" ) != NULL &&
				strcmp( mlt_properties_get( mlt_service_properties( service ), "mlt_service" ), "inigo" ) == 0 )
		{
			inigo = service;

			// Turn inigo's producer into a framework producer
			mlt_properties_set( mlt_service_properties( service ), "mlt_type", "mlt_producer" );
		}
			
		serialise_service( &context, service, root );
		xmlDocFormatDump( stderr, doc, 1 );
	}

	xmlFreeDoc( doc );
	mlt_consumer_stop( this );

	// Tell inigo, enough already!
	if ( inigo != NULL )
		mlt_properties_set_int( mlt_service_properties( inigo ), "done", 1 );
	
	return 0;
}

