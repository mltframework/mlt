/**
 * MltTractor.cpp - Tractor wrapper
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

#include "MltTractor.h"
#include "MltMultitrack.h"
#include "MltField.h"
using namespace Mlt;

Tractor::Tractor( ) :
	instance( mlt_tractor_new( ) )
{
}

Tractor::Tractor( mlt_tractor tractor ) :
	instance( tractor )
{
	inc_ref( );
}

Tractor::Tractor( Tractor &tractor ) :
	instance( tractor.get_tractor( ) )
{
	inc_ref( );
}

Tractor::~Tractor( )
{
	mlt_tractor_close( instance );
}

mlt_tractor Tractor::get_tractor( )
{
	return instance;
}

mlt_producer Tractor::get_producer( )
{
	return mlt_tractor_producer( get_tractor( ) );
}

Multitrack *Tractor::multitrack( )
{
	return new Multitrack( mlt_tractor_multitrack( get_tractor( ) ) );
}

Field *Tractor::field( )
{
	return new Field( mlt_tractor_field( get_tractor( ) ) );
}

