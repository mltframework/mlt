/**
 * MltFilter.cpp - MLT Wrapper
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

#include <stdlib.h>
#include <string.h>
#include "MltFilter.h"
#include "MltProfile.h"
using namespace Mlt;

Filter::Filter( Profile& profile, const char *id, const char *arg ) :
	instance( NULL )
{
	if ( arg != NULL )
	{
		instance = mlt_factory_filter( profile.get_profile(), id, arg );
	}
	else
	{
		if ( strchr( id, ':' ) )
		{
			char *temp = strdup( id );
			char *arg = strchr( temp, ':' ) + 1;
			*( arg - 1 ) = '\0';
			instance = mlt_factory_filter( profile.get_profile(), temp, arg );
			free( temp );
		}
		else
		{
			instance = mlt_factory_filter( profile.get_profile(), id, NULL );
		}
	}
}

Filter::Filter( Service &filter ) :
	instance( NULL )
{
	if ( filter.type( ) == filter_type )
	{
		instance = ( mlt_filter )filter.get_service( );
		inc_ref( );
	}
}

Filter::Filter( Filter &filter ) :
	Mlt::Service( filter ),
	instance( filter.get_filter( ) )
{
	inc_ref( );
}

Filter::Filter( mlt_filter filter ) :
	instance( filter )
{
	inc_ref( );
}

Filter::~Filter( )
{
	mlt_filter_close( instance );
}

mlt_filter Filter::get_filter( )
{
	return instance;
}

mlt_service Filter::get_service( )
{
	return mlt_filter_service( get_filter( ) );
}

int Filter::connect( Service &service, int index )
{
	return mlt_filter_connect( get_filter( ), service.get_service( ), index );
}

void Filter::set_in_and_out( int in, int out )
{
	mlt_filter_set_in_and_out( get_filter( ), in, out );
}

int Filter::get_in( )
{
	return mlt_filter_get_in( get_filter( ) );
}

int Filter::get_out( )
{
	return mlt_filter_get_out( get_filter( ) );
}

int Filter::get_length( )
{
	return mlt_filter_get_length( get_filter( ) );
}

int Filter::get_length2( Frame &frame )
{
	return mlt_filter_get_length2( get_filter( ), frame.get_frame() );
}

int Filter::get_track( )
{
	return mlt_filter_get_track( get_filter( ) );
}

int Filter::get_position( Frame &frame )
{
	return mlt_filter_get_position( get_filter( ), frame.get_frame( ) );
}

double Filter::get_progress( Frame &frame )
{
	return mlt_filter_get_progress( get_filter( ), frame.get_frame( ) );
}

void Filter::process( Frame &frame )
{
	mlt_filter_process( get_filter( ), frame.get_frame() );
}
