/*
 * producer_xml.c -- a libxml2 parser of mlt service networks
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

// TODO: destroy unreferenced producers (they are currently destroyed
//       when the returned producer is closed).

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h> // for xmlCreateFileParserCtxt
#include <libxml/tree.h>

#define STACK_SIZE 1000
#define BRANCH_SIG_LEN 4000

#define _x (const xmlChar*)
#define _s (const char*)

#undef DEBUG
#ifdef DEBUG
extern xmlDocPtr xml_make_doc( mlt_service service );
#endif

enum service_type
{
	mlt_invalid_type,
	mlt_unknown_type,
	mlt_producer_type,
	mlt_playlist_type,
	mlt_entry_type,
	mlt_tractor_type,
	mlt_multitrack_type,
	mlt_filter_type,
	mlt_transition_type,
	mlt_consumer_type,
	mlt_field_type,
	mlt_services_type,
	mlt_dummy_filter_type,
	mlt_dummy_transition_type,
	mlt_dummy_producer_type,
};

struct deserialise_context_s
{
	enum service_type stack_types[ STACK_SIZE ];
	mlt_service stack_service[ STACK_SIZE ];
	int stack_service_size;
	mlt_properties producer_map;
	mlt_properties destructors;
	char *property;
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
	mlt_profile profile;
	int pass;
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

static int context_push_service( deserialise_context this, mlt_service that, enum service_type type )
{
	int ret = this->stack_service_size >= STACK_SIZE - 1;
	if ( ret == 0 )
	{
		this->stack_service[ this->stack_service_size ] = that;
		this->stack_types[ this->stack_service_size++ ] = type;
		
		// Record the tree branch on which this service lives
		if ( that != NULL && mlt_properties_get( MLT_SERVICE_PROPERTIES( that ), "_xml_branch" ) == NULL )
		{
			char s[ BRANCH_SIG_LEN ];
			mlt_properties_set( MLT_SERVICE_PROPERTIES( that ), "_xml_branch", serialise_branch( this, s ) );
		}
	}
	return ret;
}

/** Pop a service.
*/

static mlt_service context_pop_service( deserialise_context this, enum service_type *type )
{
	mlt_service result = NULL;
	
	*type = invalid_type;
	if ( this->stack_service_size > 0 )
	{
		result = this->stack_service[ -- this->stack_service_size ];
		if ( type != NULL )
			*type = this->stack_types[ this->stack_service_size ];
	}
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


// Prepend the property value with the document root
static inline void qualify_property( deserialise_context context, mlt_properties properties, const char *name )
{
	char *resource = mlt_properties_get( properties, name );
	if ( resource != NULL && resource[0] )
	{
		// Qualify file name properties	
		char *root = mlt_properties_get( context->producer_map, "root" );
		if ( root != NULL && strcmp( root, "" ) )
		{
			char *full_resource = malloc( strlen( root ) + strlen( resource ) + 2 );
			if ( resource[ 0 ] != '/' && strchr( resource, ':' ) == NULL )
			{
				strcpy( full_resource, root );
				strcat( full_resource, "/" );
				strcat( full_resource, resource );
			}
			else
			{
				strcpy( full_resource, resource );
			}
			mlt_properties_set( properties, name, full_resource );
			free( full_resource );
		}
	}
}


/** This function adds a producer to a playlist or multitrack when
    there is no entry or track element.
*/

static int add_producer( deserialise_context context, mlt_service service, mlt_position in, mlt_position out )
{
	// Return value (0 = service remains top of stack, 1 means it can be removed)
	int result = 0;

	// Get the parent producer
	enum service_type type = mlt_invalid_type;
	mlt_service container = context_pop_service( context, &type );
	int contained = 0;

	if ( service != NULL && container != NULL )
	{
		char *container_branch = mlt_properties_get( MLT_SERVICE_PROPERTIES( container ), "_xml_branch" );
		char *service_branch = mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "_xml_branch" );
		contained = !strncmp( container_branch, service_branch, strlen( container_branch ) );
	}

	if ( contained )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
		char *hide_s = mlt_properties_get( properties, "hide" );

		// Indicate that this service is no longer top of stack
		result = 1;

		switch( type )
		{
			case mlt_tractor_type: 
				{
					mlt_multitrack multitrack = mlt_tractor_multitrack( MLT_TRACTOR( container ) );
					mlt_multitrack_connect( multitrack, MLT_PRODUCER( service ), mlt_multitrack_count( multitrack ) );
				}
				break;
			case mlt_multitrack_type:
				{
					mlt_multitrack_connect( MLT_MULTITRACK( container ),
						MLT_PRODUCER( service ),
						mlt_multitrack_count( MLT_MULTITRACK( container ) ) );
				}
				break;
			case mlt_playlist_type:
				{
					mlt_playlist_append_io( MLT_PLAYLIST( container ), MLT_PRODUCER( service ), in, out );
				}
				break;
			default:
				result = 0;
				fprintf( stderr, "Producer defined inside something that isn't a container\n" );
				break;
		};

		// Set the hide state of the track producer
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

	// Put the parent producer back
	if ( container != NULL )
		context_push_service( context, container, type );

	return result;
}

/** Attach filters defined on that to this.
*/

static void attach_filters( mlt_service this, mlt_service that )
{
	if ( that != NULL )
	{
		int i = 0;
		mlt_filter filter = NULL;
		for ( i = 0; ( filter = mlt_service_filter( that, i ) ) != NULL; i ++ )
		{
			mlt_service_attach( this, filter );
			attach_filters( MLT_FILTER_SERVICE( filter ), MLT_FILTER_SERVICE( filter ) );
		}
	}
}

static void on_start_profile( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_profile p = context->profile;
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		if ( xmlStrcmp( atts[ 0 ], _x("name") ) == 0 || xmlStrcmp( atts[ 0 ], _x("profile") ) == 0 )
		{
			mlt_profile my_profile = mlt_profile_init( _s(atts[ 1 ]) );
			if ( my_profile )
			{
				p->description = strdup( my_profile->description );
				p->display_aspect_den = my_profile->display_aspect_den;
				p->display_aspect_num = my_profile->display_aspect_num;
				p->frame_rate_den = my_profile->frame_rate_den;
				p->frame_rate_num = my_profile->frame_rate_num;
				p->width = my_profile->width;
				p->height = my_profile->height;
				p->progressive = my_profile->progressive;
				p->sample_aspect_den = my_profile->sample_aspect_den;
				p->sample_aspect_num = my_profile->sample_aspect_num;
				p->colorspace = my_profile->colorspace;
				p->is_explicit = 1;
				mlt_profile_close( my_profile );
			}
		}
		else if ( xmlStrcmp( atts[ 0 ], _x("description") ) == 0 )
		{
			if ( p->description )
				free( p->description );
			p->description = strdup( _s(atts[ 1 ]) );
			p->is_explicit = 1;
		}
		else if ( xmlStrcmp( atts[ 0 ], _x("display_aspect_den") ) == 0 )
			p->display_aspect_den = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("display_aspect_num") ) == 0 )
			p->display_aspect_num = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("sample_aspect_num") ) == 0 )
			p->sample_aspect_num = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("sample_aspect_den") ) == 0 )
			p->sample_aspect_den = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("width") ) == 0 )
			p->width = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("height") ) == 0 )
			p->height = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("progressive") ) == 0 )
			p->progressive = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("frame_rate_num") ) == 0 )
			p->frame_rate_num = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("frame_rate_den") ) == 0 )
			p->frame_rate_den = strtol( _s(atts[ 1 ]), NULL, 0 );
		else if ( xmlStrcmp( atts[ 0 ], _x("colorspace") ) == 0 )
			p->colorspace = strtol( _s(atts[ 1 ]), NULL, 0 );
	}
}

static void on_start_tractor( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_tractor tractor = mlt_tractor_new( );
	mlt_service service = MLT_TRACTOR_SERVICE( tractor );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	track_service( context->destructors, service, (mlt_destructor) mlt_tractor_close );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), (const char*) atts[0], atts[1] == NULL ? "" : (const char*) atts[1] );

	mlt_properties_set_int( MLT_TRACTOR_PROPERTIES( tractor ), "global_feed", 1 );

	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );
	
	context_push_service( context, service, mlt_tractor_type );
}

static void on_end_tractor( deserialise_context context, const xmlChar *name )
{
	// Get the tractor
	enum service_type type;
	mlt_service tractor = context_pop_service( context, &type );

	if ( tractor != NULL && type == mlt_tractor_type )
	{
		// See if the tractor should be added to a playlist or multitrack
		if ( add_producer( context, tractor, 0, mlt_producer_get_out( MLT_PRODUCER( tractor ) ) ) == 0 )
			context_push_service( context, tractor, type );
	}
	else
	{
		fprintf( stderr, "Invalid state for tractor\n" );
	}
}

static void on_start_multitrack( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	enum service_type type;
	mlt_service parent = context_pop_service( context, &type );

	// If we don't have a parent, then create one now, providing we're in a state where we can
	if ( parent == NULL || ( type == mlt_playlist_type || type == mlt_multitrack_type ) )
	{
		mlt_tractor tractor = NULL;
		// Push the parent back
		if ( parent != NULL )
			context_push_service( context, parent, type );

		// Create a tractor to contain the multitrack
		tractor = mlt_tractor_new( );
		parent = MLT_TRACTOR_SERVICE( tractor );
		track_service( context->destructors, parent, (mlt_destructor) mlt_tractor_close );
		type = mlt_tractor_type;

		// Flag it as a synthesised tractor for clean up later
		mlt_properties_set_int( MLT_SERVICE_PROPERTIES( parent ), "loader_synth", 1 );
	}

	if ( type == mlt_tractor_type )
	{
		mlt_service service = MLT_SERVICE( mlt_tractor_multitrack( MLT_TRACTOR( parent ) ) );
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
		for ( ; atts != NULL && *atts != NULL; atts += 2 )
			mlt_properties_set( properties, (const char*) atts[0], atts[1] == NULL ? "" : (const char*) atts[1] );

		if ( mlt_properties_get( properties, "id" ) != NULL )
			mlt_properties_set_data( context->producer_map, mlt_properties_get( properties,"id" ), service, 0, NULL, NULL );

		context_push_service( context, parent, type );
		context_push_service( context, service, mlt_multitrack_type );
	}
	else
	{
		fprintf( stderr, "Invalid multitrack position\n" );
	}
}

static void on_end_multitrack( deserialise_context context, const xmlChar *name )
{
	// Get the multitrack from the stack
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );

	if ( service == NULL || type != mlt_multitrack_type )
		fprintf( stderr, "End multitrack in the wrong state...\n" );
}

static void on_start_playlist( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_playlist playlist = mlt_playlist_init( );
	mlt_service service = MLT_PLAYLIST_SERVICE( playlist );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	track_service( context->destructors, service, (mlt_destructor) mlt_playlist_close );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( properties, (const char*) atts[0], atts[1] == NULL ? "" : (const char*) atts[1] );

		// Out will be overwritten later as we append, so we need to save it
		if ( xmlStrcmp( atts[ 0 ], _x("out") ) == 0 )
			mlt_properties_set( properties, "_xml.out", ( const char* )atts[ 1 ] );
	}

	if ( mlt_properties_get( properties, "id" ) != NULL )
		mlt_properties_set_data( context->producer_map, mlt_properties_get( properties, "id" ), service, 0, NULL, NULL );

	context_push_service( context, service, mlt_playlist_type );
}

static void on_end_playlist( deserialise_context context, const xmlChar *name )
{
	// Get the playlist from the stack
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );

	if ( service != NULL && type == mlt_playlist_type )
	{
		mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
		mlt_position in = -1;
		mlt_position out = -1;

		if ( mlt_properties_get( properties, "in" ) )
			in = mlt_properties_get_position( properties, "in" );
		if ( mlt_properties_get( properties, "out" ) )
			out = mlt_properties_get_position( properties, "out" );

		// See if the playlist should be added to a playlist or multitrack
		if ( add_producer( context, service, in, out ) == 0 )
			context_push_service( context, service, type );
	}
	else
	{
		fprintf( stderr, "Invalid state of playlist end %d\n", type );
	}
}

static void on_start_producer( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// use a dummy service to hold properties to allow arbitrary nesting
	mlt_service service = calloc( 1, sizeof( struct mlt_service_s ) );
	mlt_service_init( service, NULL );

	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	context_push_service( context, service, mlt_dummy_producer_type );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (const char*) atts[0], atts[1] == NULL ? "" : (const char*) atts[1] );
}

// Parse a SMIL clock value (as produced by Kino 0.9.1) and return position in frames
static mlt_position parse_clock_value( char *value, double fps )
{
	// This implementation expects a fully specified clock value - no optional
	// parts (e.g. 1:05)
	char *pos, *copy = strdup( value );
	int hh, mm, ss, ms;
	mlt_position result = -1;

	value = copy;
	pos = strchr( value, ':' );
	if ( !pos )
		return result;
	*pos = '\0';
	hh = atoi( value );
	value = pos + 1;

	pos = strchr( value, ':' );
	if ( !pos )
		return result;
	*pos = '\0';
	mm = atoi( value );
	value = pos + 1;
	
	pos = strchr( value, '.' );
	if ( !pos )
		return result;
	*pos = '\0';
	ss = atoi( value );
	value = pos + 1;
	
	ms = atoi( value );
	free( copy );
	result = ( fps * ( ( (hh * 3600) + (mm * 60) + ss ) * 1000  + ms ) / 1000 + 0.5 );
	
	return result;
}

static void on_end_producer( deserialise_context context, const xmlChar *name )
{
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	if ( service != NULL && type == mlt_dummy_producer_type )
	{
		mlt_service producer = NULL;

		qualify_property( context, properties, "resource" );
		char *resource = mlt_properties_get( properties, "resource" );

		// Let Kino-SMIL src be a synonym for resource
		if ( resource == NULL )
		{
			qualify_property( context, properties, "src" );
			resource = mlt_properties_get( properties, "src" );
		}

		// Instantiate the producer
		if ( mlt_properties_get( properties, "mlt_service" ) != NULL )
		{
			char *service_name = mlt_properties_get( properties, "mlt_service" );
			if ( resource )
			{
				char *temp = calloc( 1, strlen( service_name ) + strlen( resource ) + 2 );
				strcat( temp, service_name );
				strcat( temp, ":" );
				strcat( temp, resource );
				producer = MLT_SERVICE( mlt_factory_producer( context->profile, NULL, temp ) );
				free( temp );
			}
			else
			{
				producer = MLT_SERVICE( mlt_factory_producer( context->profile, NULL, service_name ) );
			}
		}

		// Just in case the plugin requested doesn't exist...
		if ( producer == NULL && resource != NULL )
			producer = MLT_SERVICE( mlt_factory_producer( context->profile, NULL, resource ) );
	
		if ( producer == NULL )
			producer = MLT_SERVICE( mlt_factory_producer( context->profile, NULL, "+INVALID.txt" ) );

		if ( producer == NULL )
			producer = MLT_SERVICE( mlt_factory_producer( context->profile, NULL, "colour:red" ) );

		// Track this producer
		track_service( context->destructors, producer, (mlt_destructor) mlt_producer_close );

		// Propogate the properties
		qualify_property( context, properties, "resource" );
		qualify_property( context, properties, "luma" );
		qualify_property( context, properties, "luma.resource" );
		qualify_property( context, properties, "composite.luma" );
		qualify_property( context, properties, "producer.resource" );

		// Handle in/out properties separately
		mlt_position in = -1;
		mlt_position out = -1;
	
		// Get in
		if ( mlt_properties_get( properties, "in" ) != NULL )
			in = mlt_properties_get_position( properties, "in" );
		// Let Kino-SMIL clipBegin be a synonym for in
		if ( mlt_properties_get( properties, "clipBegin" ) != NULL )
		{
			if ( strchr( mlt_properties_get( properties, "clipBegin" ), ':' ) )
				// Parse clock value
				in = parse_clock_value( mlt_properties_get( properties, "clipBegin" ),
					mlt_producer_get_fps( MLT_PRODUCER(  producer ) ) );
			else
				// Parse frames value
				in = mlt_properties_get_position( properties, "clipBegin" );
		}
		// Get out
		if ( mlt_properties_get( properties, "out" ) != NULL )
			out = mlt_properties_get_position( properties, "out" );
		// Let Kino-SMIL clipEnd be a synonym for out
		if ( mlt_properties_get( properties, "clipEnd" ) != NULL )
		{
			if ( strchr( mlt_properties_get( properties, "clipEnd" ), ':' ) )
				// Parse clock value
				out = parse_clock_value( mlt_properties_get( properties, "clipEnd" ),
					mlt_producer_get_fps( MLT_PRODUCER( producer ) ) );
			else
				// Parse frames value
				out = mlt_properties_get_position( properties, "clipEnd" );
		}
		// Remove in and out
		mlt_properties_set( properties, "in", NULL );
		mlt_properties_set( properties, "out", NULL );

		// Inherit the properties
		mlt_properties_inherit( MLT_SERVICE_PROPERTIES( producer ), properties );

		// Attach all filters from service onto producer
		attach_filters( producer, service );

		// Add the producer to the producer map
		if ( mlt_properties_get( properties, "id" ) != NULL )
			mlt_properties_set_data( context->producer_map, mlt_properties_get(properties, "id"), producer, 0, NULL, NULL );

		// See if the producer should be added to a playlist or multitrack
		if ( add_producer( context, producer, in, out ) == 0 )
		{
			// Otherwise, set in and out on...
			if ( in != -1 || out != -1 )
			{
				// Get the parent service
				enum service_type type;
				mlt_service parent = context_pop_service( context, &type );
				if ( parent != NULL )
				{
					// Get the parent properties
					properties = MLT_SERVICE_PROPERTIES( parent );
				
					char *resource = mlt_properties_get( properties, "resource" );
				
					// Put the parent producer back
					context_push_service( context, parent, type );
					
					// If the parent is a track or entry
					if ( resource && ( strcmp( resource, "<entry>" ) == 0 ) )
					{
						if ( in > -1 ) mlt_properties_set_position( properties, "in", in );
						if ( out > -1 ) mlt_properties_set_position( properties, "out", out );
					}
					else
					{
						// Otherwise, set in and out on producer directly
						mlt_producer_set_in_and_out( MLT_PRODUCER( producer ), in, out );
					}
				}
				else
				{
					// Otherwise, set in and out on producer directly
					mlt_producer_set_in_and_out( MLT_PRODUCER( producer ), in, out );
				}
			}

			// Push the producer onto the stack
			context_push_service( context, producer, mlt_producer_type );
		}

		mlt_service_close( service );
	}
}

static void on_start_blank( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// Get the playlist from the stack
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_position length = 0;
	
	if ( type == mlt_playlist_type && service != NULL )
	{
		// Look for the length attribute
		for ( ; atts != NULL && *atts != NULL; atts += 2 )
		{
			if ( xmlStrcmp( atts[0], _x("length") ) == 0 )
			{
				length = atoll( _s(atts[1]) );
				break;
			}
		}

		// Append a blank to the playlist
		mlt_playlist_blank( MLT_PLAYLIST( service ), length - 1 );

		// Push the playlist back onto the stack
		context_push_service( context, service, type );
	}
	else
	{
		fprintf( stderr, "blank without a playlist - a definite no no\n" );
	}
}

static void on_start_entry( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	mlt_producer entry = NULL;
	mlt_properties temp = mlt_properties_new( );

	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( temp, (const char*) atts[0], atts[1] == NULL ? "" : (const char*) atts[1] );
		
		// Look for the producer attribute
		if ( xmlStrcmp( atts[ 0 ], _x("producer") ) == 0 )
		{
			mlt_producer producer = mlt_properties_get_data( context->producer_map, (const char*) atts[1], NULL );
			if ( producer !=  NULL )
				mlt_properties_set_data( temp, "producer", producer, 0, NULL, NULL );
		}
	}

	// If we have a valid entry
	if ( mlt_properties_get_data( temp, "producer", NULL ) != NULL )
	{
		mlt_playlist_clip_info info;
		enum service_type parent_type = invalid_type;
		mlt_service parent = context_pop_service( context, &parent_type );
		mlt_producer producer = mlt_properties_get_data( temp, "producer", NULL );

		if ( parent_type == mlt_playlist_type )
		{
			// Append the producer to the playlist
			mlt_position in = -1;
			mlt_position out = -1;
			if ( mlt_properties_get( temp, "in" ) )
				in = mlt_properties_get_position( temp, "in" );
			if ( mlt_properties_get( temp, "out" ) )
				out = mlt_properties_get_position( temp, "out" );
			mlt_playlist_append_io( MLT_PLAYLIST( parent ), producer, in, out );

			// Handle the repeat property
			if ( mlt_properties_get_int( temp, "repeat" ) > 0 )
			{
				mlt_playlist_repeat_clip( MLT_PLAYLIST( parent ),
										  mlt_playlist_count( MLT_PLAYLIST( parent ) ) - 1,
										  mlt_properties_get_int( temp, "repeat" ) );
			}

			mlt_playlist_get_clip_info( MLT_PLAYLIST( parent ), &info, mlt_playlist_count( MLT_PLAYLIST( parent ) ) - 1 );
			entry = info.cut;
		}
		else
		{
			fprintf( stderr, "Entry not part of a playlist...\n" );
		}

		context_push_service( context, parent, parent_type );
	}

	// Push the cut onto the stack
	context_push_service( context, MLT_PRODUCER_SERVICE( entry ), mlt_entry_type );

	mlt_properties_close( temp );
}

static void on_end_entry( deserialise_context context, const xmlChar *name )
{
	// Get the entry from the stack
	enum service_type entry_type = invalid_type;
	mlt_service entry = context_pop_service( context, &entry_type );

	if ( entry == NULL && entry_type != mlt_entry_type )
	{
		fprintf( stderr, "Invalid state at end of entry\n" );
	}
}

static void on_start_track( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// use a dummy service to hold properties to allow arbitrary nesting
	mlt_service service = calloc( 1, sizeof( struct mlt_service_s ) );
	mlt_service_init( service, NULL );

	// Push the dummy service onto the stack
	context_push_service( context, service, mlt_entry_type );
	
	mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "resource", "<track>" );
	
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
	{
		mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), (const char*) atts[0], atts[1] == NULL ? "" : (const char*) atts[1] );
		
		// Look for the producer attribute
		if ( xmlStrcmp( atts[ 0 ], _x("producer") ) == 0 )
		{
			mlt_producer producer = mlt_properties_get_data( context->producer_map, (const char*) atts[1], NULL );
			if ( producer !=  NULL )
				mlt_properties_set_data( MLT_SERVICE_PROPERTIES( service ), "producer", producer, 0, NULL, NULL );
		}
	}
}

static void on_end_track( deserialise_context context, const xmlChar *name )
{
	// Get the track from the stack
	enum service_type track_type;
	mlt_service track = context_pop_service( context, &track_type );

	if ( track != NULL && track_type == mlt_entry_type )
	{
		mlt_properties track_props = MLT_SERVICE_PROPERTIES( track );
		enum service_type parent_type = invalid_type;
		mlt_service parent = context_pop_service( context, &parent_type );
		mlt_multitrack multitrack = NULL;

		mlt_producer producer = mlt_properties_get_data( track_props, "producer", NULL );
		mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

		if ( parent_type == mlt_tractor_type )
			multitrack = mlt_tractor_multitrack( MLT_TRACTOR( parent ) );
		else if ( parent_type == mlt_multitrack_type )
			multitrack = MLT_MULTITRACK( parent );
		else
			fprintf( stderr, "track contained in an invalid container\n" );

		if ( multitrack != NULL )
		{
			// Set producer i/o if specified
			if ( mlt_properties_get( track_props, "in" ) != NULL ||
				 mlt_properties_get( track_props, "out" ) != NULL )
			{
				mlt_position in = -1;
				mlt_position out = -1;
				if ( mlt_properties_get( track_props, "in" ) )
					in = mlt_properties_get_position( track_props, "in" );
				if ( mlt_properties_get( track_props, "out" ) )
					out = mlt_properties_get_position( track_props, "out" );
				mlt_producer cut = mlt_producer_cut( MLT_PRODUCER( producer ), in, out );
				mlt_multitrack_connect( multitrack, cut, mlt_multitrack_count( multitrack ) );
				mlt_properties_inherit( MLT_PRODUCER_PROPERTIES( cut ), track_props );
				track_props = MLT_PRODUCER_PROPERTIES( cut );
				mlt_producer_close( cut );
			}
			else
			{
				mlt_multitrack_connect( multitrack, producer, mlt_multitrack_count( multitrack ) );
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
		}

		if ( parent != NULL )
			context_push_service( context, parent, parent_type );

		mlt_service_close( track );
	}
	else
	{
		fprintf( stderr, "Invalid state at end of track\n" );
	}
}

static void on_start_filter( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// use a dummy service to hold properties to allow arbitrary nesting
	mlt_service service = calloc( 1, sizeof( struct mlt_service_s ) );
	mlt_service_init( service, NULL );

	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	context_push_service( context, service, mlt_dummy_filter_type );

	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (const char*) atts[0], (const char*) atts[1] );
}

static void on_end_filter( deserialise_context context, const xmlChar *name )
{
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	enum service_type parent_type = invalid_type;
	mlt_service parent = context_pop_service( context, &parent_type );

	if ( service != NULL && type == mlt_dummy_filter_type )
	{
		mlt_service filter = MLT_SERVICE( mlt_factory_filter( context->profile, mlt_properties_get( properties, "mlt_service" ), NULL ) );
		mlt_properties filter_props = MLT_SERVICE_PROPERTIES( filter );

		track_service( context->destructors, filter, (mlt_destructor) mlt_filter_close );

		// Propogate the properties
		qualify_property( context, properties, "resource" );
		qualify_property( context, properties, "luma" );
		qualify_property( context, properties, "luma.resource" );
		qualify_property( context, properties, "composite.luma" );
		qualify_property( context, properties, "producer.resource" );
		mlt_properties_inherit( filter_props, properties );

		// Attach all filters from service onto filter
		attach_filters( filter, service );

		// Associate the filter with the parent
		if ( parent != NULL )
		{
			if ( parent_type == mlt_tractor_type )
			{
				mlt_field field = mlt_tractor_field( MLT_TRACTOR( parent ) );
				mlt_field_plant_filter( field, MLT_FILTER( filter ), mlt_properties_get_int( properties, "track" ) );
				mlt_filter_set_in_and_out( MLT_FILTER( filter ), 
										   mlt_properties_get_int( properties, "in" ),
										   mlt_properties_get_int( properties, "out" ) );
			}
			else
			{
				mlt_service_attach( parent, MLT_FILTER( filter ) );
			}

			// Put the parent back on the stack
			context_push_service( context, parent, parent_type );
		}
		else
		{
			fprintf( stderr, "filter closed with invalid parent...\n" );
		}

		// Close the dummy filter service
		mlt_service_close( service );
	}
	else
	{
		fprintf( stderr, "Invalid top of stack on filter close\n" );
	}
}

static void on_start_transition( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	// use a dummy service to hold properties to allow arbitrary nesting
	mlt_service service = calloc( 1, sizeof( struct mlt_service_s ) );
	mlt_service_init( service, NULL );

	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	context_push_service( context, service, mlt_dummy_transition_type );

	// Set the properties
	for ( ; atts != NULL && *atts != NULL; atts += 2 )
		mlt_properties_set( properties, (const char*) atts[0], (const char*) atts[1] );
}

static void on_end_transition( deserialise_context context, const xmlChar *name )
{
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	enum service_type parent_type = invalid_type;
	mlt_service parent = context_pop_service( context, &parent_type );

	if ( service != NULL && type == mlt_dummy_transition_type )
	{
		char *id = mlt_properties_get( properties, "mlt_service" );
		mlt_service effect = MLT_SERVICE( mlt_factory_transition( context->profile, id, NULL ) );
		mlt_properties effect_props = MLT_SERVICE_PROPERTIES( effect );

		track_service( context->destructors, effect, (mlt_destructor) mlt_transition_close );

		// Propogate the properties
		qualify_property( context, properties, "resource" );
		qualify_property( context, properties, "luma" );
		qualify_property( context, properties, "luma.resource" );
		qualify_property( context, properties, "composite.luma" );
		qualify_property( context, properties, "producer.resource" );
		mlt_properties_inherit( effect_props, properties );

		// Attach all filters from service onto effect
		attach_filters( effect, service );

		// Associate the filter with the parent
		if ( parent != NULL )
		{
			if ( parent_type == mlt_tractor_type )
			{
				mlt_field field = mlt_tractor_field( MLT_TRACTOR( parent ) );
				if ( mlt_properties_get_int( properties, "a_track" ) == mlt_properties_get_int( properties, "b_track" ) )
					mlt_properties_set_int( properties, "b_track", mlt_properties_get_int( properties, "a_track" ) + 1 );
				mlt_field_plant_transition( field, MLT_TRANSITION( effect ), 
											mlt_properties_get_int( properties, "a_track" ),
											mlt_properties_get_int( properties, "b_track" ) );
				mlt_transition_set_in_and_out( MLT_TRANSITION( effect ), 
										   mlt_properties_get_int( properties, "in" ),
										   mlt_properties_get_int( properties, "out" ) );
			}
			else
			{
				fprintf( stderr, "Misplaced transition - ignoring\n" );
			}

			// Put the parent back on the stack
			context_push_service( context, parent, parent_type );
		}
		else
		{
			fprintf( stderr, "transition closed with invalid parent...\n" );
		}

		// Close the dummy filter service
		mlt_service_close( service );
	}
	else
	{
		fprintf( stderr, "Invalid top of stack on transition close\n" );
	}
}

static void on_start_property( deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
	const char *value = NULL;

	if ( service != NULL )
	{
		// Set the properties
		for ( ; atts != NULL && *atts != NULL; atts += 2 )
		{
			if ( xmlStrcmp( atts[ 0 ], _x("name") ) == 0 )
				context->property = strdup( _s(atts[ 1 ]) );
			else if ( xmlStrcmp( atts[ 0 ], _x("value") ) == 0 )
				value = _s(atts[ 1 ]);
		}

		if ( context->property != NULL )
			mlt_properties_set( properties, context->property, value == NULL ? "" : value );

		// Tell parser to collect any further nodes for serialisation
		context->is_value = 1;

		context_push_service( context, service, type );
	}
	else
	{
		fprintf( stderr, "Property without a service '%s'?\n", ( const char * )name );
	}
}

static void on_end_property( deserialise_context context, const xmlChar *name )
{
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	if ( service != NULL )
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
			mlt_properties_set( properties, context->property, _s(value) );
#ifdef WIN32
			xmlFreeFunc xmlFree = NULL;
			xmlMemGet( &xmlFree, NULL, NULL, NULL);
#endif
			xmlFree( value );
			xmlFreeDoc( context->value_doc );
			context->value_doc = NULL;
		}

		// Close this property handling
		free( context->property );
		context->property = NULL;

		context_push_service( context, service, type );
	}
	else
	{
		fprintf( stderr, "Property without a service '%s'??\n", (const char *)name );
	}
}

static void on_start_element( void *ctx, const xmlChar *name, const xmlChar **atts)
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
//printf("on_start_element: %s\n", name );
	if ( context->pass == 0 )
	{
		if ( xmlStrcmp( name, _x("mlt") ) == 0 ||
		     xmlStrcmp( name, _x("profile") ) == 0 ||
		     xmlStrcmp( name, _x("profileinfo") ) == 0 )
			on_start_profile( context, name, atts );
		return;
	}
	context->branch[ context->depth ] ++;
	context->depth ++;
	
	// Build a tree from nodes within a property value
	if ( context->is_value == 1 && context->pass == 1 )
	{
		xmlNodePtr node = xmlNewNode( NULL, name );
		
		if ( context->value_doc == NULL )
		{
			// Start a new tree
			context->value_doc = xmlNewDoc( _x("1.0") );
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
	else if ( xmlStrcmp( name, _x("tractor") ) == 0 )
		on_start_tractor( context, name, atts );
	else if ( xmlStrcmp( name, _x("multitrack") ) == 0 )
		on_start_multitrack( context, name, atts );
	else if ( xmlStrcmp( name, _x("playlist") ) == 0 || xmlStrcmp( name, _x("seq") ) == 0 || xmlStrcmp( name, _x("smil") ) == 0 )
		on_start_playlist( context, name, atts );
	else if ( xmlStrcmp( name, _x("producer") ) == 0 || xmlStrcmp( name, _x("video") ) == 0 )
		on_start_producer( context, name, atts );
	else if ( xmlStrcmp( name, _x("blank") ) == 0 )
		on_start_blank( context, name, atts );
	else if ( xmlStrcmp( name, _x("entry") ) == 0 )
		on_start_entry( context, name, atts );
	else if ( xmlStrcmp( name, _x("track") ) == 0 )
		on_start_track( context, name, atts );
	else if ( xmlStrcmp( name, _x("filter") ) == 0 )
		on_start_filter( context, name, atts );
	else if ( xmlStrcmp( name, _x("transition") ) == 0 )
		on_start_transition( context, name, atts );
	else if ( xmlStrcmp( name, _x("property") ) == 0 )
		on_start_property( context, name, atts );
	else if ( xmlStrcmp( name, _x("westley") ) == 0 || xmlStrcmp( name, _x("mlt") ) == 0 )
		for ( ; atts != NULL && *atts != NULL; atts += 2 )
			mlt_properties_set( context->producer_map, ( const char * )atts[ 0 ], ( const char * )atts[ 1 ] );
}

static void on_end_element( void *ctx, const xmlChar *name )
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
//printf("on_end_element: %s\n", name );
	if ( context->is_value == 1 && context->pass == 1 && xmlStrcmp( name, _x("property") ) != 0 )
		context_pop_node( context );
	else if ( xmlStrcmp( name, _x("multitrack") ) == 0 )
		on_end_multitrack( context, name );
	else if ( xmlStrcmp( name, _x("playlist") ) == 0 || xmlStrcmp( name, _x("seq") ) == 0 || xmlStrcmp( name, _x("smil") ) == 0 )
		on_end_playlist( context, name );
	else if ( xmlStrcmp( name, _x("track") ) == 0 )
		on_end_track( context, name );
	else if ( xmlStrcmp( name, _x("entry") ) == 0 )
		on_end_entry( context, name );
	else if ( xmlStrcmp( name, _x("tractor") ) == 0 )
		on_end_tractor( context, name );
	else if ( xmlStrcmp( name, _x("property") ) == 0 )
		on_end_property( context, name );
	else if ( xmlStrcmp( name, _x("producer") ) == 0 || xmlStrcmp( name, _x("video") ) == 0 )
		on_end_producer( context, name );
	else if ( xmlStrcmp( name, _x("filter") ) == 0 )
		on_end_filter( context, name );
	else if ( xmlStrcmp( name, _x("transition") ) == 0 )
		on_end_transition( context, name );

	context->branch[ context->depth ] = 0;
	context->depth --;
}

static void on_characters( void *ctx, const xmlChar *ch, int len )
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	char *value = calloc( len + 1, 1 );
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );

	if ( service != NULL )
		context_push_service( context, service, type );

	value[ len ] = 0;
	strncpy( value, (const char*) ch, len );

	if ( context->stack_node_size > 0 )
		xmlNodeAddContent( context->stack_node[ context->stack_node_size - 1 ], ( xmlChar* )value );

	// libxml2 generates an on_characters immediately after a get_entity within
	// an element value, and we ignore it because it is called again during
	// actual substitution.
	else if ( context->property != NULL && context->entity_is_replace == 0 )
	{
		char *s = mlt_properties_get( properties, context->property );
		if ( s != NULL )
		{
			// Append new text to existing content
			char *new = calloc( strlen( s ) + len + 1, 1 );
			strcat( new, s );
			strcat( new, value );
			mlt_properties_set( properties, context->property, new );
			free( new );
		}
		else
			mlt_properties_set( properties, context->property, value );
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
				context->publicId, context->systemId, ( xmlChar* )mlt_properties_get( context->params, _s(name) ) );
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

// TODO: Check this with Dan... I think this is for parameterisation
// but it's breaking standard escaped entities (like &lt; etc).
static void on_entity_declaration( void *ctx, const xmlChar* name, int type, 
	const xmlChar* publicId, const xmlChar* systemId, xmlChar* content)
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	
	xmlAddDocEntity( context->entity_doc, name, type, publicId, systemId, content );
}

// TODO: Check this functionality (see on_entity_declaration)
static xmlEntityPtr on_get_entity( void *ctx, const xmlChar* name )
{
	struct _xmlParserCtxt *xmlcontext = ( struct _xmlParserCtxt* )ctx;
	deserialise_context context = ( deserialise_context )( xmlcontext->_private );
	xmlEntityPtr e = NULL;

	// Setup for entity declarations if not ready
	if ( xmlGetIntSubset( context->entity_doc ) == NULL )
	{
		xmlCreateIntSubset( context->entity_doc, _x("mlt"), _x(""), _x("") );
		context->publicId = _x("");
		context->systemId = _x("");
	}

	// Add our parameters if not already
	params_to_entities( context );
	
	e = xmlGetPredefinedEntity( name );
	
	// Send signal to on_characters that an entity substitutin is pending
	if ( e == NULL )
	{
		e = xmlGetDocEntity( context->entity_doc, name );
		if ( e != NULL )
			context->entity_is_replace = 1;
	}
	
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

// Quick workaround to avoid unecessary libxml2 warnings
static int file_exists( char *file )
{
	char *name = strdup( file );
	int exists = 0;
	if ( name != NULL && strchr( name, '?' ) )
		*( strchr( name, '?' ) ) = '\0';
	if ( name != NULL )
	{
		FILE *f = fopen( name, "r" );
		exists = f != NULL;
		if ( exists ) fclose( f );
	}
	free( name );
	return exists;
}

mlt_producer producer_xml_init( mlt_profile profile, mlt_service_type servtype, const char *id, char *data )
{
	xmlSAXHandler *sax = calloc( 1, sizeof( xmlSAXHandler ) );
	struct deserialise_context_s *context = calloc( 1, sizeof( struct deserialise_context_s ) );
	mlt_properties properties = NULL;
	int i = 0;
	struct _xmlParserCtxt *xmlcontext;
	int well_formed = 0;
	char *filename = NULL;
	int info = strcmp( id, "xml-string" ) ? 0 : 1;

	if ( data == NULL || !strcmp( data, "" ) || ( info == 0 && !file_exists( data ) ) )
		return NULL;

	context = calloc( 1, sizeof( struct deserialise_context_s ) );
	if ( context == NULL )
		return NULL;

	context->producer_map = mlt_properties_new();
	context->destructors = mlt_properties_new();
	context->params = mlt_properties_new();
	context->profile = profile;

	// Decode URL and parse parameters
	mlt_properties_set( context->producer_map, "root", "" );
	if ( info == 0 )
	{
		filename = strdup( data );
		parse_url( context->params, url_decode( filename, data ) );

		// We need the directory prefix which was used for the xml
		if ( strchr( filename, '/' ) )
		{
			char *root = NULL;
			mlt_properties_set( context->producer_map, "root", filename );
			root = mlt_properties_get( context->producer_map, "root" );
			*( strrchr( root, '/' ) ) = '\0';

			// If we don't have an absolute path here, we're heading for disaster...
			if ( root[ 0 ] != '/' )
			{
				char *cwd = getcwd( NULL, 0 );
				char *real = malloc( strlen( cwd ) + strlen( root ) + 2 );
				sprintf( real, "%s/%s", cwd, root );
				mlt_properties_set( context->producer_map, "root", real );
				free( real );
				free( cwd );
			}
		}
	}

	// We need to track the number of registered filters
	mlt_properties_set_int( context->destructors, "registered", 0 );

	// Setup SAX callbacks
	sax->startElement = on_start_element;

	// Setup libxml2 SAX parsing
	xmlInitParser(); 
	xmlSubstituteEntitiesDefault( 1 );
	// This is used to facilitate entity substitution in the SAX parser
	context->entity_doc = xmlNewDoc( _x("1.0") );
	if ( info == 0 )
		xmlcontext = xmlCreateFileParserCtxt( filename );
	else
		xmlcontext = xmlCreateMemoryParserCtxt( data, strlen( data ) );

	// Invalid context - clean up and return NULL
	if ( xmlcontext == NULL )
	{
		mlt_properties_close( context->producer_map );
		mlt_properties_close( context->destructors );
		mlt_properties_close( context->params );
		free( context );
		free( sax );
		free( filename );
		return NULL;
	}

	// Parse
	xmlcontext->sax = sax;
	xmlcontext->_private = ( void* )context;	
	xmlParseDocument( xmlcontext );
	
	// Cleanup after parsing
	xmlcontext->sax = NULL;
	xmlcontext->_private = NULL;
	xmlFreeParserCtxt( xmlcontext );
	context->stack_node_size = 0;
	context->stack_service_size = 0;

	// Setup the second pass
	context->pass ++;
	if ( info == 0 )
		xmlcontext = xmlCreateFileParserCtxt( filename );
	else
		xmlcontext = xmlCreateMemoryParserCtxt( data, strlen( data ) );

	// Invalid context - clean up and return NULL
	if ( xmlcontext == NULL )
	{
		mlt_properties_close( context->producer_map );
		mlt_properties_close( context->destructors );
		mlt_properties_close( context->params );
		xmlFreeDoc( context->entity_doc );
		free( context );
		free( sax );
		free( filename );
		return NULL;
	}

	// Setup SAX callbacks
	sax->endElement = on_end_element;
	sax->characters = on_characters;
	sax->cdataBlock = on_characters;
	sax->internalSubset = on_internal_subset;
	sax->entityDecl = on_entity_declaration;
	sax->getEntity = on_get_entity;

	// Parse
	xmlcontext->sax = sax;
	xmlcontext->_private = ( void* )context;
	xmlParseDocument( xmlcontext );
	well_formed = xmlcontext->wellFormed;

	// Cleanup after parsing
	xmlFreeDoc( context->entity_doc );
	free( sax );
	xmlMemoryDump( ); // for debugging

	// Get the last producer on the stack
	enum service_type type;
	mlt_service service = context_pop_service( context, &type );
	if ( well_formed && service != NULL )
	{
		// Verify it is a producer service (mlt_type="mlt_producer")
		// (producer, playlist, multitrack)
		char *type = mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "mlt_type" );
		if ( type == NULL || ( strcmp( type, "mlt_producer" ) != 0 && strcmp( type, "producer" ) != 0 ) )
			service = NULL;
	}

#ifdef DEBUG
	xmlDocPtr doc = xml_make_doc( service );
	xmlDocFormatDump( stdout, doc, 1 );
	xmlFreeDoc( doc );
	service = NULL;
#endif
	
	if ( well_formed && service != NULL )
	{
		char *title = mlt_properties_get( context->producer_map, "title" );
		
		// Need the complete producer list for various reasons
		properties = context->destructors;

		// Now make sure we don't have a reference to the service in the properties
		for ( i = mlt_properties_count( properties ) - 1; i >= 1; i -- )
		{
			char *name = mlt_properties_get_name( properties, i );
			if ( mlt_properties_get_data_at( properties, i, NULL ) == service )
			{
				mlt_properties_set_data( properties, name, service, 0, NULL, NULL );
				break;
			}
		}

		// We are done referencing destructor property list
		// Set this var to service properties for convenience
		properties = MLT_SERVICE_PROPERTIES( service );
	
		// Assign the title
		mlt_properties_set( properties, "title", title );

		// Optimise for overlapping producers
		mlt_producer_optimise( MLT_PRODUCER( service ) );

		// Handle deep copies
		if ( getenv( "MLT_XML_DEEP" ) == NULL )
		{
			// Now assign additional properties
			if ( info == 0 )
				mlt_properties_set( properties, "resource", data );

			// This tells consumer_xml not to deep copy
			mlt_properties_set( properties, "xml", "was here" );
		}
		else
		{
			// Allow the project to be edited
			mlt_properties_set( properties, "_xml", "was here" );
			mlt_properties_set_int( properties, "_mlt_service_hidden", 1 );
		}
	}
	else
	{
		// Return null if not well formed
		service = NULL;
	}

	// Clean up
	mlt_properties_close( context->producer_map );
	if ( context->params != NULL )
		mlt_properties_close( context->params );
	mlt_properties_close( context->destructors );
	free( context );
	free( filename );

	return MLT_PRODUCER( service );
}
