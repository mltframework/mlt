/**
 * MltProducer.cpp - MLT Wrapper
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

#include "MltProducer.h"
using namespace Mlt;

Producer::Producer( ) :
	destroy( false ),
	instance( NULL )
{
}

Producer::Producer( char *id, char *service ) :
	destroy( true ),
	instance( NULL )
{
	if ( id != NULL && service != NULL )
		instance = mlt_factory_producer( id, service );
	else
		instance = mlt_factory_producer( "fezzik", id != NULL ? id : service );
}

Producer::Producer( mlt_producer producer ) :
	destroy( false ),
	instance( producer )
{
}

Producer::Producer( Producer &producer ) :
	destroy( false ),
	instance( producer.get_producer( ) )
{
}

Producer::~Producer( )
{
	if ( destroy )
		mlt_producer_close( instance );
}
mlt_service Producer::get_service( )
{
	return mlt_producer_service( get_producer( ) );
}
	
int Producer::seek( mlt_position position )
{
	return mlt_producer_seek( get_producer( ), position );
}

mlt_position Producer::position( )
{
	return mlt_producer_position( get_producer( ) );
}

mlt_position Producer::frame( )
{
	return mlt_producer_frame( get_producer( ) );
}

int Producer::set_speed( double speed )
{
	return mlt_producer_set_speed( get_producer( ), speed );
}

double Producer::get_speed( )
{
	return mlt_producer_get_speed( get_producer( ) );
}

double Producer::get_fps( )
{
	return mlt_producer_get_fps( get_producer( ) );
}

int Producer::set_in_and_out( mlt_position in, mlt_position out )
{
	return mlt_producer_set_in_and_out( get_producer( ), in, out );
}

mlt_position Producer::get_in( )
{
	return mlt_producer_get_in( get_producer( ) );
}

mlt_position Producer::get_out( )
{
	return mlt_producer_get_out( get_producer( ) );
}

mlt_position Producer::get_length( )
{
	return mlt_producer_get_length( get_producer( ) );
}

mlt_position Producer::get_playtime( )
{
	return mlt_producer_get_playtime( get_producer( ) );
}

mlt_producer Producer::get_producer( )
{
	return instance;
}


