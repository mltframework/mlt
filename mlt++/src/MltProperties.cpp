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

Properties::Properties( ) :
	destroy( true ),
	instance( NULL )
{
	instance = mlt_properties_new( );
}

Properties::Properties( Properties &properties ) :
	destroy( false ),
	instance( properties.get_properties( ) )
{
}

Properties::Properties( mlt_properties properties ) :
	destroy( false ),
	instance( properties )
{
}

Properties::Properties( char *file ) :
	destroy( true ),
	instance( NULL )
{
	instance = mlt_properties_load( file );
}

Properties::~Properties( )
{
	if ( destroy )
		mlt_properties_close( instance );
}

mlt_properties Properties::get_properties( )
{
	return instance;
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

int Properties::pass_values( Properties &that, char *prefix )
{
	return mlt_properties_pass( get_properties( ), that.get_properties( ), prefix );
}

int Properties::parse( char *namevalue )
{
	return mlt_properties_parse( get_properties( ), namevalue );
}

char *Properties::get_name( int index )
{
	return mlt_properties_get_name( get_properties( ), index );
}

char *Properties::get( int index )
{
	return mlt_properties_get_value( get_properties( ), index );
}

void *Properties::get_data( int index, int &size )
{
	return mlt_properties_get_data_at( get_properties( ), index, &size );
}

void Properties::mirror( Properties &that )
{
	mlt_properties_mirror( get_properties( ), that.get_properties( ) );
}

int Properties::inherit( Properties &that )
{
	return mlt_properties_inherit( get_properties( ), that.get_properties( ) );
}

int Properties::rename( char *source, char *dest )
{
	return mlt_properties_rename( get_properties( ), source, dest );
}

void Properties::dump( FILE *output )
{
	mlt_properties_dump( get_properties( ), output );
}
