/**
 * MltFilter.cpp - MLT Wrapper
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

#include "MltFilter.h"
using namespace Mlt;

Filter::Filter( char *id, char *service ) :
	destroy( true ),
	instance( NULL )
{
	instance = mlt_factory_filter( id, service );
}

Filter::Filter( Filter &filter ) :
	destroy( false ),
	instance( filter.get_filter( ) )
{
}

Filter::Filter( mlt_filter filter ) :
	destroy( false ),
	instance( filter )
{
}

Filter::~Filter( )
{
	if ( destroy )
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

void Filter::set_in_and_out( mlt_position in, mlt_position out )
{
	mlt_filter_set_in_and_out( get_filter( ), in, out );
}

mlt_position Filter::get_in( )
{
	return mlt_filter_get_in( get_filter( ) );
}

mlt_position Filter::get_out( )
{
	return mlt_filter_get_out( get_filter( ) );
}

int Filter::get_track( )
{
	return mlt_filter_get_track( get_filter( ) );
}


