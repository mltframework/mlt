/**
 * MltChain.cpp - Chain wrapper
 * Copyright (C) 2020 Meltytech, LLC
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

#include "MltChain.h"

using namespace Mlt;

Chain::Chain( ) :
	instance( nullptr )
{
}

Chain::Chain( Profile& profile, const char *id, const char *service ) :
	instance( nullptr )
{
	if ( id == NULL || service == NULL )
	{
		service = id != NULL ? id : service;
		id = NULL;
	}

	mlt_producer source = mlt_factory_producer( profile.get_profile(), id, service );
	if ( source )
	{
		instance = mlt_chain_init( profile.get_profile() );
		mlt_chain_set_source( instance, source );
		if ( id == NULL )
			mlt_chain_attach_normalisers( instance );
		mlt_producer_close ( source );
	}
}

Chain::Chain( Profile& profile ) :
	instance( mlt_chain_init( profile.get_profile() ) )
{
}

Chain::Chain( mlt_chain chain ) :
	instance( chain )
{
	inc_ref( );
}

Chain::Chain( Chain& chain ) :
	Mlt::Producer( chain ),
	instance( chain.get_chain( ) )
{
	inc_ref( );
}

Chain::Chain( Chain* chain ) :
	Mlt::Producer( chain ),
	instance( chain != NULL ? chain->get_chain( ) : NULL )
{
	if ( is_valid( ) )
		inc_ref( );
}

Chain::Chain( Service& chain ) :
	instance( NULL )
{
	if ( chain.type( ) == mlt_service_chain_type )
	{
		instance = ( mlt_chain )chain.get_service( );
		inc_ref( );
	}
}

Chain::~Chain( )
{
	mlt_chain_close( instance );
	instance = nullptr;
}

mlt_chain Chain::get_chain( )
{
	return instance;
}

mlt_producer Chain::get_producer( )
{
	return MLT_CHAIN_PRODUCER( instance );
}

void Chain::set_source( Mlt::Producer& source )
{
	mlt_chain_set_source( instance, source.get_producer() );
}

Mlt::Producer Chain::get_source( )
{
	return Mlt::Producer( mlt_chain_get_source(instance) );
}

int Chain::attach( Mlt::Link& link )
{
	return mlt_chain_attach( instance, link.get_link() );
}

int Chain::detach( Mlt::Link& link )
{
	return mlt_chain_detach( instance, link.get_link() );
}

int Chain::link_count() const
{
	return mlt_chain_link_count( instance );
}

bool Chain::move_link( int from, int to )
{
	return (bool)mlt_chain_move_link( instance, from, to );
}

Mlt::Link* Chain::link( int index )
{
	mlt_link result = mlt_chain_link( instance, index );
	return result == NULL ? NULL : new Link( result );

}
