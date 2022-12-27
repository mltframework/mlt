/**
 * \file mlt_chain.c
 * \brief link service class
 * \see mlt_chain_s
 *
 * Copyright (C) 2020-2022 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mlt_chain.h"
#include "mlt_factory.h"
#include "mlt_frame.h"
#include "mlt_log.h"
#include "mlt_tokeniser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** \brief private service definition */

typedef struct
{
	int link_count;
	int link_size;
	mlt_link* links;
	mlt_producer source;
	mlt_profile source_profile;
	mlt_properties source_parameters;
	mlt_producer begin;
}
mlt_chain_base;

/* Forward references to static methods.
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void relink_chain( mlt_chain self );
static void chain_property_changed( mlt_service owner, mlt_chain self, char *name );
static void source_property_changed( mlt_service owner, mlt_chain self, char *name );

/** Construct a chain.
 *
 * \public \memberof mlt_chain_s
 * \return the new chain
 */

mlt_chain mlt_chain_init( mlt_profile profile )
{
	mlt_chain self = calloc( 1, sizeof( struct mlt_chain_s ) );
	if ( self != NULL )
	{
		mlt_producer producer = &self->parent;
		if ( mlt_producer_init( producer, self ) == 0 )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			mlt_properties_set( properties, "mlt_type", "chain" );
			mlt_properties_clear( properties, "resource");
			mlt_properties_clear( properties, "mlt_service");
			mlt_properties_clear( properties, "in");
			mlt_properties_clear( properties, "out");
			mlt_properties_clear( properties, "length");

			producer->get_frame = producer_get_frame;
			producer->close = ( mlt_destructor )mlt_chain_close;
			producer->close_object = self;

			mlt_service_set_profile( MLT_CHAIN_SERVICE( self ), profile );

			// Generate local space
			self->local = calloc( 1, sizeof( mlt_chain_base ) );
			mlt_chain_base* base = self->local;
			base->source_profile = NULL;

			// Listen to property changes to pass along to the source
			mlt_events_listen( MLT_CHAIN_PROPERTIES(self), self, "property-changed", ( mlt_listener )chain_property_changed );
		}
		else
		{
			free( self );
			self = NULL;
		}
	}
	return self;
}

/** Set the source producer.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \param source the new source producer
 */

void mlt_chain_set_source( mlt_chain self, mlt_producer source )
{
	int error = self == NULL || source == NULL;
	if ( error == 0 )
	{
		mlt_chain_base* base = self->local;
		mlt_properties source_properties = MLT_PRODUCER_PROPERTIES( source );
		int n = 0;
		int i = 0;

		// Clean up from previous source
		mlt_producer_close( base->source );
		mlt_properties_close( base->source_parameters );
		mlt_profile_close( base->source_profile );

		// Save the source producer
		base->source = source;
		mlt_properties_inc_ref( source_properties );

		// Create a list of all parameters used by the source producer so that
		// they can be passed between the source producer and this chain.
		base->source_parameters = mlt_properties_new();
		mlt_repository repository = mlt_factory_repository();
		char* source_metadata_name = strdup( mlt_properties_get( source_properties, "mlt_service" ) );
		// If the service name ends in "-novalidate", then drop the ending and search for the metadata of the main service.
		// E.g. "avformat-novalidate" -> "avformat"
		char* novalidate_ptr = strstr(source_metadata_name, "-novalidate");
		if ( novalidate_ptr ) *novalidate_ptr = '\0';
		mlt_properties source_metadata = mlt_repository_metadata( repository, mlt_service_producer_type, source_metadata_name );
		free( source_metadata_name );
		if ( source_metadata )
		{
			mlt_properties params = (mlt_properties) mlt_properties_get_data( source_metadata, "parameters", NULL );
			if ( params )
			{
				n = mlt_properties_count( params );
				for ( i = 0; i < n; i++ )
				{
					mlt_properties param = (mlt_properties) mlt_properties_get_data( params, mlt_properties_get_name( params, i ), NULL );
					char* identifier = mlt_properties_get( param, "identifier" );
					if ( identifier )
					{
						// Set the value to 1 to indicate the parameter exists.
						mlt_properties_set_int( base->source_parameters, identifier, 1 );
					}
				}
			}
		}

		// Pass parameters and properties from the source producer to this chain.
		// Some properties may have been set during source initialization.
		n = mlt_properties_count( source_properties );
		mlt_events_block( MLT_CHAIN_PROPERTIES(self), self );
		for ( i = 0; i < n; i++ )
		{
			char* name = mlt_properties_get_name( source_properties, i );
			if ( mlt_properties_get_int( base->source_parameters, name ) ||
				 !strcmp( name, "mlt_service" ) ||
				 !strcmp( name, "_mlt_service_hidden" ) ||
				 !strcmp( name, "seekable" ) ||
				 !strcmp( name, "creation_time" ) ||
				 !strncmp( name, "meta.", 5 ) )
			{
				mlt_properties_pass_property( MLT_CHAIN_PROPERTIES(self), source_properties, name );
			}
		}
		// If a length has not been specified for this chain, copy in/out/length from the source producer
		if ( !mlt_producer_get_length( MLT_CHAIN_PRODUCER(self) ) ) {
			mlt_properties_set_position( MLT_CHAIN_PROPERTIES(self), "length", mlt_producer_get_length( base->source ) );
			mlt_producer_set_in_and_out( MLT_CHAIN_PRODUCER(self), mlt_producer_get_in( base->source ), mlt_producer_get_out( base->source ) );
		}
		mlt_events_unblock( MLT_CHAIN_PROPERTIES(self), self );

		// Monitor property changes from the source to pass to the chain.
		mlt_events_listen( source_properties, self, "property-changed", ( mlt_listener )source_property_changed );

		// This chain will control the speed and in/out
		mlt_producer_set_speed( base->source, 0.0 );
		// Approximate infinite length
		mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( base->source ), "length", 0x7fffffff );
		mlt_producer_set_in_and_out( base->source, 0, mlt_producer_get_length( base->source ) - 1 );

		// Reconfigure the chain
		relink_chain( self );
		mlt_events_fire( MLT_CHAIN_PROPERTIES(self), "chain-changed", mlt_event_data_none() );
	}
}

/** Get the source producer.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \return the source producer
 */

extern mlt_producer mlt_chain_get_source( mlt_chain self )
{
	mlt_producer source = NULL;
	if ( self && self->local )
	{
		mlt_chain_base* base = self->local;
		source = base->source;
	}
	return source;
}

/** Attach a link.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \param link the link to attach
 * \return true if there was an error
 */

int mlt_chain_attach( mlt_chain self, mlt_link link )
{
	int error = self == NULL || link == NULL;
	if ( error == 0 )
	{
		int i = 0;
		mlt_chain_base *base = self->local;

		for ( i = 0; error == 0 && i < base->link_count; i ++ )
			if ( base->links[ i ] == link )
				error = 1;

		if ( error == 0 )
		{
			if ( base->link_count == base->link_size )
			{
				base->link_size += 10;
				base->links = realloc( base->links, base->link_size * sizeof( mlt_link ) );
			}

			if ( base->links != NULL )
			{
				mlt_properties_inc_ref( MLT_LINK_PROPERTIES( link ) );
				mlt_properties_set_data( MLT_LINK_PROPERTIES( link ), "chain", self, 0, NULL, NULL );
				base->links[ base->link_count ++ ] = link;
				relink_chain( self );
				mlt_events_fire( MLT_CHAIN_PROPERTIES(self), "chain-changed", mlt_event_data_none() );
			}
			else
			{
				error = 2;
			}
		}
	}
	return error;
}

/** Detach a link.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \param link the link to detach
 * \return true if there was an error
 */

int mlt_chain_detach( mlt_chain self, mlt_link link )
{
	int error = self == NULL || link == NULL;
	if ( error == 0 )
	{
		int i = 0;
		mlt_chain_base *base = self->local;

		for ( i = 0; i < base->link_count; i ++ )
			if ( base->links[ i ] == link )
				break;

		if ( i < base->link_count )
		{
			base->links[ i ] = NULL;
			for ( i ++ ; i < base->link_count; i ++ )
				base->links[ i - 1 ] = base->links[ i ];
			base->link_count --;
			mlt_link_close( link );
			relink_chain( self );
			mlt_events_fire( MLT_CHAIN_PROPERTIES(self), "chain-changed", mlt_event_data_none() );
		}
	}
	return error;
}

/** Get the number of links attached.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \return the number of attached links or -1 if there was an error
 */

int mlt_chain_link_count( mlt_chain self )
{
	int result = -1;
	if ( self )
	{
		mlt_chain_base *base = self->local;
		result = base->link_count;
	}
	return result;
}

/** Reorder the attached links.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \param from the current index value of the link to move
 * \param to the new index value for the link specified in \p from
 * \return true if there was an error
 */

int mlt_chain_move_link( mlt_chain self, int from, int to )
{
	int error = -1;
	if ( self )
	{
		mlt_chain_base *base = self->local;
		if ( from < 0 ) from = 0;
		if ( from >= base->link_count ) from = base->link_count - 1;
		if ( to < 0 ) to = 0;
		if ( to >= base->link_count ) to = base->link_count - 1;
		if ( from != to && base->link_count > 1 )
		{
			mlt_link link = base->links[from];
			int i;
			if ( from > to )
			{
				for ( i = from; i > to; i-- )
					base->links[i] = base->links[i - 1];
			}
			else
			{
				for ( i = from; i < to; i++ )
					base->links[i] = base->links[i + 1];
			}
			base->links[to] = link;
			relink_chain( self );
			mlt_events_fire( MLT_CHAIN_PROPERTIES(self), "chain-changed", mlt_event_data_none() );
			error = 0;
		}
	}
	return error;
}

/** Retrieve an attached link.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 * \param index which one of potentially multiple links
 * \return the link or null if there was an error
 */

mlt_link mlt_chain_link( mlt_chain self, int index )
{
	mlt_link link = NULL;
	if ( self != NULL )
	{
		mlt_chain_base *base = self->local;
		if ( index >= 0 && index < base->link_count )
			link = base->links[ index ];
	}
	return link;
}

/** Close the chain and free its resources.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 */

void mlt_chain_close( mlt_chain self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_CHAIN_PROPERTIES( self ) ) <= 0 )
	{
		int i = 0;
		mlt_chain_base *base = self->local;
		mlt_events_block( MLT_CHAIN_PROPERTIES( self ), self );
		for ( i = 0; i < base->link_count; i ++ )
			mlt_link_close( base->links[ i ] );
		free( base->links );
		mlt_profile_close( base->source_profile );
		free( base );
		self->parent.close = NULL;
		mlt_producer_close( &self->parent );
		free( self );
	}
}

/** Attach normalizer links.
 *
 * \public \memberof mlt_chain_s
 * \param self a chain
 */

extern void mlt_chain_attach_normalizers( mlt_chain self )
{
	if ( !self ) return;

	if ( mlt_chain_link_count( self ) && mlt_properties_get_int( MLT_LINK_PROPERTIES( mlt_chain_link( self, 0 ) ), "_loader" ) )
	{
		// Chain already has normalizers
		return;
	}

	static mlt_properties normalisers = NULL;

	mlt_tokeniser tokenizer = mlt_tokeniser_init( );

	// We only need to load the normalising properties once
	if ( normalisers == NULL )
	{
		char temp[ 1024 ];
		sprintf( temp, "%s/core/chain_loader.ini", mlt_environment( "MLT_DATA" ) );
		normalisers = mlt_properties_load( temp );
		mlt_factory_register_for_clean_up( normalisers, ( mlt_destructor )mlt_properties_close );
	}

	// Apply normalisers
	for ( int i = 0; i < mlt_properties_count( normalisers ); i ++ )
	{
		char *value = mlt_properties_get_value( normalisers, i );
		mlt_tokeniser_parse_new( tokenizer, value, "," );
		for ( int j = 0; j < mlt_tokeniser_count( tokenizer ); j ++ )
		{
			mlt_link link = mlt_factory_link( mlt_tokeniser_get_string( tokenizer, j ), NULL );
			if ( link )
			{
				mlt_properties_set_int( MLT_LINK_PROPERTIES(link), "_loader", 1 );
				mlt_chain_attach( self, link );
				mlt_chain_move_link( self, mlt_chain_link_count( self ) - 1, 0 );
				break;
			}
		}
	}

	mlt_tokeniser_close( tokenizer );
}

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index )
{
	int result = 1;
	if ( parent && parent->child )
	{
		mlt_chain self = parent->child;
		if( self )
		{
			mlt_chain_base *base = self->local;
			mlt_producer_seek( base->begin, mlt_producer_frame( parent ) );
			result = mlt_service_get_frame( MLT_PRODUCER_SERVICE( base->begin ), frame, index );
			mlt_producer_prepare_next( parent );
		}
	}
	return result;
}

static void relink_chain( mlt_chain self )
{
	mlt_chain_base *base = self->local;
	mlt_profile profile = mlt_service_profile( MLT_CHAIN_SERVICE(self) );

	if ( !base->source )
	{
		return;
	}

	if ( base->link_count == 0 )
	{
		base->begin = base->source;
		// If there are no links, the producer can operate in the final frame rate
		mlt_service_set_profile( MLT_PRODUCER_SERVICE(base->source), profile );
	}
	else
	{
		if ( !base->source_profile )
		{
			// Save the native source producer profile
			base->source_profile = mlt_profile_init(NULL);
			mlt_profile_from_producer( base->source_profile, base->source );
		}

		// Set the producer to be in native frame rate
		mlt_service_set_profile( MLT_PRODUCER_SERVICE(base->source), base->source_profile );

		int i = 0;
		base->begin = MLT_LINK_PRODUCER( base->links[0] );
		for ( i = 0; i < base->link_count - 1; i++ )
		{
			mlt_link_connect_next( base->links[i], MLT_LINK_PRODUCER( base->links[i+1] ), profile );
		}
		mlt_link_connect_next( base->links[base->link_count -1], base->source, profile );
	}
}

static void chain_property_changed( mlt_service owner, mlt_chain self, char *name )
{
	mlt_chain_base *base = self->local;
	if ( !base->source ) return;
	if ( mlt_properties_get_int( base->source_parameters, name ) ||
		 !strncmp( name, "meta.", 5 ) )
	{
		// Pass parameter changes from this chain to the encapsulated source producer.
		mlt_properties chain_properties = MLT_CHAIN_PROPERTIES( self );
		mlt_properties source_properties = MLT_PRODUCER_PROPERTIES( base->source );
		mlt_events_block( source_properties, self );
		mlt_properties_pass_property( source_properties, chain_properties, name );
		mlt_events_unblock( source_properties, self );
	}
}

static void source_property_changed( mlt_service owner, mlt_chain self, char *name )
{
	mlt_chain_base *base = self->local;
	if ( mlt_properties_get_int( base->source_parameters, name ) ||
		 !strncmp( name, "meta.", 5 ) )
	{
		// The source producer might change its own parameters.
		// Pass those changes to this producer.
		mlt_properties chain_properties = MLT_CHAIN_PROPERTIES( self );
		mlt_properties source_properties = MLT_PRODUCER_PROPERTIES( base->source );
		mlt_events_block( chain_properties, self );
		mlt_properties_pass_property( chain_properties, source_properties, name );
		mlt_events_unblock( chain_properties, self );
	}
}
