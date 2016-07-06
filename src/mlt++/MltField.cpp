/**
 * MltField.cpp - Field wrapper
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

#include "MltField.h"
#include "MltFilter.h"
#include "MltTransition.h"
using namespace Mlt;

Field::Field( mlt_field field ) :
	instance( field )
{
	inc_ref( );
}

Field::Field( Field &field ) :
	Mlt::Service( field ),
	instance( field.get_field( ) )
{
	inc_ref( );
}

Field::~Field( )
{
	mlt_field_close( instance );
}

mlt_field Field::get_field( )
{
	return instance;
}

mlt_service Field::get_service( )
{
	return mlt_field_service( get_field( ) );
}

int Field::plant_filter( Filter &filter, int track )
{
	return mlt_field_plant_filter( get_field( ), filter.get_filter( ), track );
}

int Field::plant_transition( Transition &transition, int a_track, int b_track )
{
	return mlt_field_plant_transition( get_field( ), transition.get_transition( ), a_track, b_track );
}

void Field::disconnect_service( Service &service )
{
	mlt_field_disconnect_service( get_field(), service.get_service() );
}

