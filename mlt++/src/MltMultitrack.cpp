/**
 * MltMultitrack.h - Multitrack wrapper
 * Copyright (C) 2004-2005 Charles Yates
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
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

#include "MltMultitrack.h"
#include "MltProducer.h"
using namespace Mlt;

Multitrack::Multitrack( mlt_multitrack multitrack ) :
	instance( multitrack )
{
	inc_ref( );
}

Multitrack::Multitrack( Multitrack &multitrack ) :
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

