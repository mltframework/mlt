/**
 * MltProperties.cpp - MLT Wrapper
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

#include "MltProperties.h"
using namespace Mlt;

PropertiesInstance::PropertiesInstance( ) :
	destroy( true ),
	instance( NULL )
{
	instance = mlt_properties_new( );
}

PropertiesInstance::PropertiesInstance( Properties &properties ) :
	destroy( false ),
	instance( properties.get_properties( ) )
{
}

PropertiesInstance::PropertiesInstance( mlt_properties properties ) :
	destroy( false ),
	instance( properties )
{
}

PropertiesInstance::~PropertiesInstance( )
{
	if ( destroy )
		mlt_properties_close( instance );
}

bool Properties::is_valid( )
{
	return get_properties( ) != NULL;
}

int Properties::count( )
{
	return mlt_properties_count( get_properties( ) );
}

char *Properties::get( char *name )
{
	return mlt_properties_get( get_properties( ), name );
}

int Properties::get_int( char *name )
{
	return mlt_properties_get_int( get_properties( ), name );
}

double Properties::get_double( char *name )
{
	return mlt_properties_get_double( get_properties( ), name );
}

void *Properties::get_data( char *name, int &size )
{
	return mlt_properties_get_data( get_properties( ), name, &size );
}

int Properties::set( char *name, char *value )
{
	return mlt_properties_set( get_properties( ), name, value );
}

int Properties::set( char *name, int value )
{
	return mlt_properties_set_int( get_properties( ), name, value );
}

int Properties::set( char *name, double value )
{
	return mlt_properties_set_double( get_properties( ), name, value );
}

int Properties::set( char *name, void *value, int size, mlt_destructor destructor, mlt_serialiser serialiser )
{
	return mlt_properties_set_data( get_properties( ), name, value, size, destructor, serialiser );
}

mlt_properties PropertiesInstance::get_properties( )
{
	return instance;
}


