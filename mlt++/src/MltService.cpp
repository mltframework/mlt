/**
 * MltService.cpp - MLT Wrapper
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

#include <string.h>
#include "MltService.h"
using namespace Mlt;

Service::Service( ) :
	Properties( false ),
	instance( NULL )
{
}

Service::Service( Service &service ) :
	Properties( false ),
	instance( service.get_service( ) )
{
	inc_ref( );
}

Service::Service( mlt_service service ) :
	Properties( false ),
	instance( service )
{
	inc_ref( );
}

Service::~Service( )
{
	mlt_service_close( instance );
}

mlt_service Service::get_service( )
{
	return instance;
}

mlt_properties Service::get_properties( )
{
	return mlt_service_properties( get_service( ) );
}

int Service::connect_producer( Service &producer, int index )
{
	return mlt_service_connect_producer( get_service( ), producer.get_service( ), index );
}

Service *Service::producer( )
{
	return new Service( mlt_service_producer( get_service( ) ) );
}

Service *Service::consumer( )
{
	return new Service( mlt_service_consumer( get_service( ) ) );
}

Frame *Service::get_frame( int index )
{
	mlt_frame frame = NULL;
	mlt_service_get_frame( get_service( ), &frame, index );
	return new Frame( frame );
}

service_type Service::type( )
{
	service_type type = invalid_type;
	if ( is_valid( ) )
	{
		char *mlt_type = get( "mlt_type" );
		char *resource = get( "resource" );
		if ( mlt_type == NULL )
			type = unknown_type;
		else if ( !strcmp( mlt_type, "producer" ) )
			type = producer_type;
		else if ( !strcmp( mlt_type, "mlt_producer" ) )
		{
			if ( resource == NULL )
				type = producer_type;
			else if ( !strcmp( resource, "<playlist>" ) )
				type = playlist_type;
			else if ( !strcmp( resource, "<tractor>" ) )
				type = tractor_type;
			else if ( !strcmp( resource, "<multitrack>" ) )
				type = multitrack_type;
		}
		else if ( !strcmp( mlt_type, "filter" ) )
			type = filter_type;
		else if ( !strcmp( mlt_type, "transition" ) )
			type = transition_type;
		else if ( !strcmp( mlt_type, "consumer" ) )
			type = consumer_type;
		else
			type = unknown_type;
	}
	return type;
}
