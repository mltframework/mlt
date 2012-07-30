/*
 * consumer_xml.c -- a libxml2 serialiser of mlt service networks
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <libxml/tree.h>
#include <pthread.h>
#include <wchar.h>

#define ID_SIZE 128
#define TIME_PROPERTY "_consumer_xml"

#define _x (const xmlChar*)
#define _s (const char*)

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
	char *store;
	int no_meta;
	mlt_profile profile;
	mlt_time_format time_format;
};
typedef struct serialise_context_s* serialise_context;

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer this );
static void *consumer_thread( void *arg );
static void serialise_service( serialise_context context, mlt_service service, xmlNode *node );

static char* filter_restricted( const char *in )
{
	if ( !in ) return NULL;
	size_t n = strlen( in );
	char *out = calloc( 1, n + 1 );
	char *p = out;
	mbstate_t mbs;
	memset( &mbs, 0, sizeof(mbs) );
	while ( *in )
	{
		wchar_t w;
		size_t c = mbrtowc( &w, in, n, &mbs );
		if ( c <= 0 || c > n ) break;
		n -= c;
		in += c;
		if ( w == 0x9 || w == 0xA || w == 0xD ||
				( w >= 0x20 && w <= 0xD7FF ) ||
				( w >= 0xE000 && w <= 0xFFFD ) ||
				( w >= 0x10000 && w <= 0x10FFFF ) )
		{
			mbstate_t ps;
			memset( &ps, 0, sizeof(ps) );
			c = wcrtomb( p, w, &ps );
			if ( c > 0 )
				p += c;
		}
	}
	return out;
}

typedef enum
{
	xml_existing,
	xml_producer,
	xml_multitrack,
	xml_playlist,
	xml_tractor,
	xml_filter,
	xml_transition
}
xml_type;

/** Create or retrieve an id associated to this service.
*/

static char *xml_get_id( serialise_context context, mlt_service service, xml_type type )
{
	char *id = NULL;
	int i = 0;
	mlt_properties map = context->id_map;

	// Search the map for the service
	for ( i = 0; i < mlt_properties_count( map ); i ++ )
		if ( mlt_properties_get_data_at( map, i, NULL ) == service )
			break;

	// If the service is not in the map, and the type indicates a new id is needed...
	if ( i >= mlt_properties_count( map ) && type != xml_existing )
	{
		// Attempt to reuse existing id
		id = mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "id" );

		// If no id, or the id is used in the map (for another service), then
		// create a new one.
		if ( id == NULL || mlt_properties_get_data( map, id, NULL ) != NULL )
		{
			char temp[ ID_SIZE ];
			do
			{
				switch( type )
				{
					case xml_producer:
						sprintf( temp, "producer%d", context->producer_count ++ );
						break;
					case xml_multitrack:
						sprintf( temp, "multitrack%d", context->multitrack_count ++ );
						break;
					case xml_playlist:
						sprintf( temp, "playlist%d", context->playlist_count ++ );
						break;
					case xml_tractor:
						sprintf( temp, "tractor%d", context->tractor_count ++ );
						break;
					case xml_filter:
						sprintf( temp, "filter%d", context->filter_count ++ );
						break;
					case xml_transition:
						sprintf( temp, "transition%d", context->transition_count ++ );
						break;
					case xml_existing:
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
	else if ( type == xml_existing )
	{
		id = mlt_properties_get_name( map, i );
	}

	return id;
}

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_xml_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	mlt_consumer this = calloc( 1, sizeof( struct mlt_consumer_s ) );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( this, NULL, profile ) == 0 )
	{
		// Allow thread to be started/stopped
		this->start = consumer_start;
		this->stop = consumer_stop;
		this->is_stopped = consumer_is_stopped;

		mlt_properties_set( MLT_CONSUMER_PROPERTIES( this ), "resource", arg );
		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "real_time", -1 );
		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "prefill", 1 );
		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "terminate_on_pause", 1 );

		// Return the consumer produced
		return this;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

static void serialise_properties( serialise_context context, mlt_properties properties, xmlNode *node )
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
			 ( !context->no_meta || strncmp( name, "meta.", 5 ) ) &&
			 strcmp( name, "mlt" ) &&
			 strcmp( name, "in" ) &&
			 strcmp( name, "out" ) &&
			 strcmp( name, "id" ) &&
			 strcmp( name, "title" ) &&
			 strcmp( name, "root" ) &&
			 strcmp( name, "width" ) &&
			 strcmp( name, "height" ) )
		{
			char *value;
			if ( !strcmp( name, "length" ) )
				value = strdup( mlt_properties_get_time( properties, name, context->time_format ) );
			else
				value = filter_restricted( mlt_properties_get_value( properties, i ) );
			if ( value )
			{
				int rootlen = strlen( context->root );
				// convert absolute path to relative
				if ( rootlen && !strncmp( value, context->root, rootlen ) && value[ rootlen ] == '/' )
					p = xmlNewTextChild( node, NULL, _x("property"), _x(value + rootlen + 1 ) );
				else
					p = xmlNewTextChild( node, NULL, _x("property"), _x(value) );
				xmlNewProp( p, _x("name"), _x(name) );
				free( value );
			}
		}
	}
}

static void serialise_store_properties( serialise_context context, mlt_properties properties, xmlNode *node, const char *store )
{
	int i;
	xmlNode *p;

	// Enumerate the properties
	for ( i = 0; store != NULL && i < mlt_properties_count( properties ); i++ )
	{
		char *name = mlt_properties_get_name( properties, i );
		if ( !strncmp( name, store, strlen( store ) ) )
		{
			char *value = filter_restricted( mlt_properties_get_value( properties, i ) );
			if ( value )
			{
				int rootlen = strlen( context->root );
				// convert absolute path to relative
				if ( rootlen && !strncmp( value, context->root, rootlen ) && value[ rootlen ] == '/' )
					p = xmlNewTextChild( node, NULL, _x("property"), _x(value + rootlen + 1) );
				else
					p = xmlNewTextChild( node, NULL, _x("property"), _x(value) );
				xmlNewProp( p, _x("name"), _x(name) );
				free( value );
			}
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
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		if ( mlt_properties_get_int( properties, "_loader" ) == 0 )
		{
			// Get a new id - if already allocated, do nothing
			char *id = xml_get_id( context, MLT_FILTER_SERVICE( filter ), xml_filter );
			if ( id != NULL )
			{
				p = xmlNewChild( node, NULL, _x("filter"), NULL );
				xmlNewProp( p, _x("id"), _x(id) );
				if ( mlt_properties_get( properties, "title" ) )
					xmlNewProp( p, _x("title"), _x(mlt_properties_get( properties, "title" )) );
				if ( mlt_properties_get_position( properties, "in" ) )
					xmlNewProp( p, _x("in"), _x( mlt_properties_get_time( properties, "in", context->time_format ) ) );
				if ( mlt_properties_get_position( properties, "out" ) )
					xmlNewProp( p, _x("out"), _x( mlt_properties_get_time( properties, "out", context->time_format ) ) );
				serialise_properties( context, properties, p );
				serialise_service_filters( context, MLT_FILTER_SERVICE( filter ), p );
			}
		}
	}
}

static void serialise_producer( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_service parent = MLT_SERVICE( mlt_producer_cut_parent( MLT_PRODUCER( service ) ) );

	if ( context->pass == 0 )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( parent );
		// Get a new id - if already allocated, do nothing
		char *id = xml_get_id( context, parent, xml_producer );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, _x("producer"), NULL );

		// Set the id
		xmlNewProp( child, _x("id"), _x(id) );
		if ( mlt_properties_get( properties, "title" ) )
			xmlNewProp( child, _x("title"), _x(mlt_properties_get( properties, "title" )) );
		xmlNewProp( child, _x("in"), _x(mlt_properties_get_time( properties, "in", context->time_format )) );
		xmlNewProp( child, _x("out"), _x(mlt_properties_get_time( properties, "out", context->time_format )) );
		serialise_properties( context, properties, child );
		serialise_service_filters( context, service, child );

		// Add producer to the map
		mlt_properties_set_int( context->hide_map, id, mlt_properties_get_int( properties, "hide" ) );
	}
	else
	{
		char *id = xml_get_id( context, parent, xml_existing );
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
		xmlNewProp( node, _x("parent"), _x(id) );
		xmlNewProp( node, _x("in"), _x(mlt_properties_get_time( properties, "in", context->time_format )) );
		xmlNewProp( node, _x("out"), _x(mlt_properties_get_time( properties, "out", context->time_format )) );
	}
}

static void serialise_tractor( serialise_context context, mlt_service service, xmlNode *node );

static void serialise_multitrack( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;

	if ( context->pass == 0 )
	{
		// Iterate over the tracks to collect the producers
		for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
		{
			mlt_producer producer = mlt_producer_cut_parent( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) );
			serialise_service( context, MLT_SERVICE( producer ), node );
		}
	}
	else
	{
		// Get a new id - if already allocated, do nothing
		char *id = xml_get_id( context, service, xml_multitrack );
		if ( id == NULL )
			return;

		// Serialise the tracks
		for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
		{
			xmlNode *track = xmlNewChild( node, NULL, _x("track"), NULL );
			int hide = 0;
			mlt_producer producer = mlt_multitrack_track( MLT_MULTITRACK( service ), i );
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			mlt_service parent = MLT_SERVICE( mlt_producer_cut_parent( producer ) );

			char *id = xml_get_id( context, MLT_SERVICE( parent ), xml_existing );
			xmlNewProp( track, _x("producer"), _x(id) );
			if ( mlt_producer_is_cut( producer ) )
			{
				xmlNewProp( track, _x("in"), _x( mlt_properties_get_time( properties, "in", context->time_format ) ) );
				xmlNewProp( track, _x("out"), _x( mlt_properties_get_time( properties, "out", context->time_format ) ) );
				serialise_store_properties( context, MLT_PRODUCER_PROPERTIES( producer ), track, context->store );
				if ( !context->no_meta )
					serialise_store_properties( context, MLT_PRODUCER_PROPERTIES( producer ), track, "meta." );
				serialise_service_filters( context, MLT_PRODUCER_SERVICE( producer ), track );
			}

			hide = mlt_properties_get_int( context->hide_map, id );
			if ( hide )
				xmlNewProp( track, _x("hide"), _x( hide == 1 ? "video" : ( hide == 2 ? "audio" : "both" ) ) );
		}
		serialise_service_filters( context, service, node );
	}
}

static void serialise_playlist( serialise_context context, mlt_service service, xmlNode *node )
{
	int i;
	xmlNode *child = node;
	mlt_playlist_clip_info info;
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	if ( context->pass == 0 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = xml_get_id( context, service, xml_playlist );
		if ( id == NULL )
			return;

		// Iterate over the playlist entries to collect the producers
		for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
		{
			if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
			{
				if ( info.producer != NULL )
				{
					mlt_producer producer = mlt_producer_cut_parent( info.producer );
					char *service_s = mlt_properties_get( MLT_PRODUCER_PROPERTIES( producer ), "mlt_service" );
					char *resource_s = mlt_properties_get( MLT_PRODUCER_PROPERTIES( producer ), "resource" );
					if ( resource_s != NULL && !strcmp( resource_s, "<playlist>" ) )
						serialise_playlist( context, MLT_SERVICE( producer ), node );
					else if ( service_s != NULL && strcmp( service_s, "blank" ) != 0 )
						serialise_service( context, MLT_SERVICE( producer ), node );
				}
			}
		}

		child = xmlNewChild( node, NULL, _x("playlist"), NULL );

		// Set the id
		xmlNewProp( child, _x("id"), _x(id) );
		if ( mlt_properties_get( properties, "title" ) )
			xmlNewProp( child, _x("title"), _x(mlt_properties_get( properties, "title" )) );

		// Store application specific properties
		serialise_store_properties( context, properties, child, context->store );
		if ( !context->no_meta )
			serialise_store_properties( context, properties, child, "meta." );

		// Add producer to the map
		mlt_properties_set_int( context->hide_map, id, mlt_properties_get_int( properties, "hide" ) );

		// Iterate over the playlist entries
		for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
		{
			if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
			{
				mlt_producer producer = mlt_producer_cut_parent( info.producer );
				mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
				char *service_s = mlt_properties_get( producer_props, "mlt_service" );
				if ( service_s != NULL && strcmp( service_s, "blank" ) == 0 )
				{
					xmlNode *entry = xmlNewChild( child, NULL, _x("blank"), NULL );
					mlt_properties_set_data( producer_props, "_profile", context->profile, 0, NULL, NULL );
					mlt_properties_set_position( producer_props, TIME_PROPERTY, info.frame_count );
					xmlNewProp( entry, _x("length"), _x( mlt_properties_get_time( producer_props, TIME_PROPERTY, context->time_format ) ) );
				}
				else
				{
					char temp[ 20 ];
					xmlNode *entry = xmlNewChild( child, NULL, _x("entry"), NULL );
					id = xml_get_id( context, MLT_SERVICE( producer ), xml_existing );
					xmlNewProp( entry, _x("producer"), _x(id) );
					mlt_properties_set_position( producer_props, TIME_PROPERTY, info.frame_in );
					xmlNewProp( entry, _x("in"), _x( mlt_properties_get_time( producer_props, TIME_PROPERTY, context->time_format ) ) );
					mlt_properties_set_position( producer_props, TIME_PROPERTY, info.frame_out );
					xmlNewProp( entry, _x("out"), _x( mlt_properties_get_time( producer_props, TIME_PROPERTY, context->time_format ) ) );
					if ( info.repeat > 1 )
					{
						sprintf( temp, "%d", info.repeat );
						xmlNewProp( entry, _x("repeat"), _x(temp) );
					}
					if ( mlt_producer_is_cut( info.cut ) )
					{
						serialise_store_properties( context, MLT_PRODUCER_PROPERTIES( info.cut ), entry, context->store );
						if ( !context->no_meta )
							serialise_store_properties( context, MLT_PRODUCER_PROPERTIES( info.cut ), entry, "meta." );
						serialise_service_filters( context, MLT_PRODUCER_SERVICE( info.cut ), entry );
					}
				}
			}
		}

		serialise_service_filters( context, service, child );
	}
	else if ( xmlStrcmp( node->name, _x("tractor") ) != 0 )
	{
		char *id = xml_get_id( context, service, xml_existing );
		xmlNewProp( node, _x("producer"), _x(id) );
	}
}

static void serialise_tractor( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	if ( context->pass == 0 )
	{
		// Recurse on connected producer
		serialise_service( context, mlt_service_producer( service ), node );
	}
	else
	{
		// Get a new id - if already allocated, do nothing
		char *id = xml_get_id( context, service, xml_tractor );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, _x("tractor"), NULL );

		// Set the id
		xmlNewProp( child, _x("id"), _x(id) );
		if ( mlt_properties_get( properties, "title" ) )
			xmlNewProp( child, _x("title"), _x(mlt_properties_get( properties, "title" )) );
		if ( mlt_properties_get( properties, "global_feed" ) )
			xmlNewProp( child, _x("global_feed"), _x(mlt_properties_get( properties, "global_feed" )) );
		xmlNewProp( child, _x("in"), _x(mlt_properties_get_time( properties, "in", context->time_format )) );
		xmlNewProp( child, _x("out"), _x(mlt_properties_get_time( properties, "out", context->time_format )) );

		// Store application specific properties
		serialise_store_properties( context, MLT_SERVICE_PROPERTIES( service ), child, context->store );
		if ( !context->no_meta )
			serialise_store_properties( context, MLT_SERVICE_PROPERTIES( service ), child, "meta." );

		// Recurse on connected producer
		serialise_service( context, mlt_service_producer( service ), child );
		serialise_service_filters( context, service, child );
	}
}

static void serialise_filter( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	// Recurse on connected producer
	serialise_service( context, mlt_service_producer( service ), node );

	if ( context->pass == 1 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = xml_get_id( context, service, xml_filter );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, _x("filter"), NULL );

		// Set the id
		xmlNewProp( child, _x("id"), _x(id) );
		if ( mlt_properties_get( properties, "title" ) )
			xmlNewProp( child, _x("title"), _x(mlt_properties_get( properties, "title" )) );
		if ( mlt_properties_get_position( properties, "in" ) )
			xmlNewProp( child, _x("in"), _x( mlt_properties_get_time( properties, "in", context->time_format ) ) );
		if ( mlt_properties_get_position( properties, "out" ) )
			xmlNewProp( child, _x("out"), _x( mlt_properties_get_time( properties, "out", context->time_format ) ) );

		serialise_properties( context, properties, child );
		serialise_service_filters( context, service, child );
	}
}

static void serialise_transition( serialise_context context, mlt_service service, xmlNode *node )
{
	xmlNode *child = node;
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	// Recurse on connected producer
	serialise_service( context, MLT_SERVICE( MLT_TRANSITION( service )->producer ), node );

	if ( context->pass == 1 )
	{
		// Get a new id - if already allocated, do nothing
		char *id = xml_get_id( context, service, xml_transition );
		if ( id == NULL )
			return;

		child = xmlNewChild( node, NULL, _x("transition"), NULL );

		// Set the id
		xmlNewProp( child, _x("id"), _x(id) );
		if ( mlt_properties_get( properties, "title" ) )
			xmlNewProp( child, _x("title"), _x(mlt_properties_get( properties, "title" )) );
		if ( mlt_properties_get_position( properties, "in" ) )
			xmlNewProp( child, _x("in"), _x( mlt_properties_get_time( properties, "in", context->time_format ) ) );
		if ( mlt_properties_get_position( properties, "out" ) )
			xmlNewProp( child, _x("out"), _x( mlt_properties_get_time( properties, "out", context->time_format ) ) );

		serialise_properties( context, properties, child );
		serialise_service_filters( context, service, child );
	}
}

static void serialise_service( serialise_context context, mlt_service service, xmlNode *node )
{
	// Iterate over consumer/producer connections
	while ( service != NULL )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );

		// Tell about the producer
		if ( strcmp( mlt_type, "producer" ) == 0 )
		{
			char *mlt_service = mlt_properties_get( properties, "mlt_service" );
			if ( mlt_properties_get( properties, "xml" ) == NULL && ( mlt_service != NULL && !strcmp( mlt_service, "tractor" ) ) )
			{
				context->pass = 0;
				serialise_tractor( context, service, node );
				context->pass = 1;
				serialise_tractor( context, service, node );
				context->pass = 0;
				break;
			}
			else
			{
				serialise_producer( context, service, node );
			}
			if ( mlt_properties_get( properties, "xml" ) != NULL )
				break;
		}

		// Tell about the framework container producers
		else if ( strcmp( mlt_type, "mlt_producer" ) == 0 )
		{
			char *resource = mlt_properties_get( properties, "resource" );

			// Recurse on multitrack's tracks
			if ( resource && strcmp( resource, "<multitrack>" ) == 0 )
			{
				serialise_multitrack( context, service, node );
				break;
			}

			// Recurse on playlist's clips
			else if ( resource && strcmp( resource, "<playlist>" ) == 0 )
			{
				serialise_playlist( context, service, node );
			}

			// Recurse on tractor's producer
			else if ( resource && strcmp( resource, "<tractor>" ) == 0 )
			{
				context->pass = 0;
				serialise_tractor( context, service, node );
				context->pass = 1;
				serialise_tractor( context, service, node );
				context->pass = 0;
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

xmlDocPtr xml_make_doc( mlt_consumer consumer, mlt_service service )
{
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
	xmlDocPtr doc = xmlNewDoc( _x("1.0") );
	xmlNodePtr root = xmlNewNode( NULL, _x("mlt") );
	struct serialise_context_s *context = calloc( 1, sizeof( struct serialise_context_s ) );
	mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( consumer ) );
	char tmpstr[ 32 ];

	xmlDocSetRootElement( doc, root );

	// Indicate the numeric locale
	xmlNewProp( root, _x("LC_NUMERIC"), _x( setlocale( LC_NUMERIC, NULL ) ) );

	// Indicate the version
	xmlNewProp( root, _x("version"), _x( mlt_version_get_string() ) );

	// If we have root, then deal with it now
	if ( mlt_properties_get( properties, "root" ) != NULL )
	{
		xmlNewProp( root, _x("root"), _x(mlt_properties_get( properties, "root" )) );
		context->root = strdup( mlt_properties_get( properties, "root" ) );
	}
	else
	{
		context->root = strdup( "" );
	}

	// Assign the additional 'storage' pattern for properties
	context->store = mlt_properties_get( MLT_CONSUMER_PROPERTIES( consumer ), "store" );
	context->no_meta = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( consumer ), "no_meta" );
	const char *time_format = mlt_properties_get( MLT_CONSUMER_PROPERTIES( consumer ), "time_format" );
	if ( time_format && ( !strcmp( time_format, "smpte" ) || !strcmp( time_format, "SMPTE" )
			|| !strcmp( time_format, "timecode" ) ) )
		context->time_format = mlt_time_smpte;
	else if ( time_format && ( !strcmp( time_format, "clock" ) || !strcmp( time_format, "CLOCK" ) ) )
		context->time_format = mlt_time_clock;

	// Assign a title property
	if ( mlt_properties_get( properties, "title" ) != NULL )
		xmlNewProp( root, _x("title"), _x(mlt_properties_get( properties, "title" )) );
	mlt_properties_set_int( properties, "global_feed", 1 );

	// Add a profile child element
	if ( profile )
	{
		xmlNodePtr profile_node = xmlNewChild( root, NULL, _x("profile"), NULL );
		if ( profile->description )
			xmlNewProp( profile_node, _x("description"), _x(profile->description) );
		sprintf( tmpstr, "%d", profile->width );
		xmlNewProp( profile_node, _x("width"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->height );
		xmlNewProp( profile_node, _x("height"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->progressive );
		xmlNewProp( profile_node, _x("progressive"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->sample_aspect_num );
		xmlNewProp( profile_node, _x("sample_aspect_num"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->sample_aspect_den );
		xmlNewProp( profile_node, _x("sample_aspect_den"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->display_aspect_num );
		xmlNewProp( profile_node, _x("display_aspect_num"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->display_aspect_den );
		xmlNewProp( profile_node, _x("display_aspect_den"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->frame_rate_num );
		xmlNewProp( profile_node, _x("frame_rate_num"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->frame_rate_den );
		xmlNewProp( profile_node, _x("frame_rate_den"), _x(tmpstr) );
		sprintf( tmpstr, "%d", profile->colorspace );
		xmlNewProp( profile_node, _x("colorspace"), _x(tmpstr) );
		context->profile = profile;
	}

	// Construct the context maps
	context->id_map = mlt_properties_new();
	context->hide_map = mlt_properties_new();

	// Ensure producer is a framework producer
	mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "mlt_type", "mlt_producer" );

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


static void output_xml( mlt_consumer this )
{
	// Get the producer service
	mlt_service service = mlt_service_producer( MLT_CONSUMER_SERVICE( this ) );
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
	char *resource =  mlt_properties_get( properties, "resource" );
	xmlDocPtr doc = NULL;

	if ( !service ) return;

	// Set the title if provided
	if ( mlt_properties_get( properties, "title" ) )
		mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "title", mlt_properties_get( properties, "title" ) );
	else if ( mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "title" ) == NULL )
		mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "title", "Anonymous Submission" );

	// Check for a root on the consumer properties and pass to service
	if ( mlt_properties_get( properties, "root" ) )
		mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "root", mlt_properties_get( properties, "root" ) );

	// Specify roots in other cases...
	if ( resource != NULL && mlt_properties_get( properties, "root" ) == NULL )
	{
		// Get the current working directory
		char *cwd = getcwd( NULL, 0 );
		mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "root", cwd );
		free( cwd );
	}

	// Make the document
	doc = xml_make_doc( this, service );

	// Handle the output
	if ( resource == NULL || !strcmp( resource, "" ) )
	{
		xmlDocFormatDump( stdout, doc, 1 );
	}
	else if ( strchr( resource, '.' ) == NULL )
	{
		xmlChar *buffer = NULL;
		int length = 0;
		xmlDocDumpMemoryEnc( doc, &buffer, &length, "utf-8" );
		mlt_properties_set( properties, resource, _s(buffer) );
#ifdef WIN32
		xmlFreeFunc xmlFree = NULL;
		xmlMemGet( &xmlFree, NULL, NULL, NULL);
#endif
		xmlFree( buffer );
	}
	else
	{
		xmlSaveFormatFileEnc( resource, doc, "utf-8", 1 );
	}

	// Close the document
	xmlFreeDoc( doc );
}
static int consumer_start( mlt_consumer this )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	if ( mlt_properties_get_int( properties, "all" ) )
	{
		// Check that we're not already running
		if ( !mlt_properties_get_int( properties, "running" ) )
		{
			// Allocate a thread
			pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

			// Assign the thread to properties
			mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

			// Set the running state
			mlt_properties_set_int( properties, "running", 1 );
			mlt_properties_set_int( properties, "joined", 0 );

			// Create the thread
			pthread_create( thread, NULL, consumer_thread, this );
		}
	}
	else
	{
		output_xml( this );
		mlt_consumer_stop( this );
		mlt_consumer_stopped( this );
	}
	return 0;
}

static int consumer_is_stopped( mlt_consumer this )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
	return !mlt_properties_get_int( properties, "running" );
}

static int consumer_stop( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check that we're running
	if ( !mlt_properties_get_int( properties, "joined" ) )
	{
		// Get the thread
		pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );
		mlt_properties_set_int( properties, "joined", 1 );

		// Wait for termination
		if ( thread )
			pthread_join( *thread, NULL );
	}

	return 0;
}

static void *consumer_thread( void *arg )
{
	// Map the argument to the object
	mlt_consumer this = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Convenience functionality
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	// Frame and size
	mlt_frame frame = NULL;

	// Loop while running
	while( !terminated && mlt_properties_get_int( properties, "running" ) )
	{
		// Get the frame
		frame = mlt_consumer_rt_frame( this );

		// Check for termination
		if ( terminate_on_pause && frame != NULL )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Check that we have a frame to work with
		if ( frame )
		{
			int width = 0, height = 0;
			int frequency = mlt_properties_get_int( properties, "frequency" );
			int channels = mlt_properties_get_int( properties, "channels" );
			int samples = 0;
			mlt_image_format iformat = mlt_image_yuv422;
			mlt_audio_format aformat = mlt_audio_s16;
			uint8_t *buffer;

			mlt_frame_get_image( frame, &buffer, &iformat, &width, &height, 0 );
			mlt_frame_get_audio( frame, (void**) &buffer, &aformat, &frequency, &channels, &samples );

			// Close the frame
			mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			mlt_frame_close( frame );
		}
	}
	output_xml( this );

	// Indicate that the consumer is stopped
	mlt_properties_set_int( properties, "running", 0 );
	mlt_consumer_stopped( this );

	return NULL;
}
