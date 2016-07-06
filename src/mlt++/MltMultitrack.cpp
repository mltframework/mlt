/**
 * MltMultitrack.h - Multitrack wrapper
 * Copyright (C) 2004-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@gmail.com>
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

#include "MltMultitrack.h"
#include "MltProducer.h"
using namespace Mlt;

Multitrack::Multitrack( mlt_multitrack multitrack ) :
	instance( multitrack )
{
	inc_ref( );
}

Multitrack::Multitrack( Service &multitrack ) :
	instance( NULL )
{
	if ( multitrack.type( ) == multitrack_type )
	{
		instance = ( mlt_multitrack )multitrack.get_service( );
		inc_ref( );
	}
}

Multitrack::Multitrack( Multitrack &multitrack ) :
	Mlt::Producer( multitrack ),
	instance( multitrack.get_multitrack( ) )
{
	inc_ref( );
}

Multitrack::~Multitrack( )
{
	mlt_multitrack_close( instance );
}

mlt_multitrack Multitrack::get_multitrack( )
{
	return instance;
}

mlt_producer Multitrack::get_producer( )
{
	return mlt_multitrack_producer( get_multitrack( ) );
}

int Multitrack::connect( Producer &producer, int index )
{
	return mlt_multitrack_connect( get_multitrack( ), producer.get_producer( ), index );
}

int Multitrack::insert( Producer &producer, int index )
{
	return mlt_multitrack_insert( get_multitrack( ), producer.get_producer( ), index );
}

int Multitrack::disconnect( int index )
{
	return mlt_multitrack_disconnect( get_multitrack(), index );
}

int Multitrack::clip( mlt_whence whence, int index )
{
	return mlt_multitrack_clip( get_multitrack( ), whence, index );
}

int Multitrack::count( )
{
	return mlt_multitrack_count( get_multitrack( ) );
}

Producer *Multitrack::track( int index )
{
	return new Producer( mlt_multitrack_track( get_multitrack( ), index ) );
}

void Multitrack::refresh( )
{
	return mlt_multitrack_refresh( get_multitrack( ) );
}
