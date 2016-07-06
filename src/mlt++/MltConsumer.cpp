/**
 * MltConsumer.cpp - MLT Wrapper
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
#include "MltConsumer.h"
#include "MltEvent.h"
#include "MltProfile.h"
using namespace Mlt;

Consumer::Consumer( ) :
	instance( NULL )
{
	instance = mlt_factory_consumer( NULL, NULL, NULL );
}

Consumer::Consumer( Profile& profile ) :
	instance( NULL )
{
	instance = mlt_factory_consumer( profile.get_profile(), NULL, NULL );
}

Consumer::Consumer( Profile& profile, const char *id, const char *arg ) :
	instance( NULL )
{
	if ( id == NULL || arg != NULL )
	{
		instance = mlt_factory_consumer( profile.get_profile(), id, arg );
	}
	else
	{
		if ( strchr( id, ':' ) )
		{
			char *temp = strdup( id );
			char *arg = strchr( temp, ':' ) + 1;
			*( arg - 1 ) = '\0';
			instance = mlt_factory_consumer( profile.get_profile(), temp, arg );
			free( temp );
		}
		else
		{
			instance = mlt_factory_consumer( profile.get_profile(), id, NULL );
		}
	}
}

Consumer::Consumer( Service &consumer ) :
	instance( NULL )
{
	if ( consumer.type( ) == consumer_type )
	{
		instance = ( mlt_consumer )consumer.get_service( );
		inc_ref( );
	}
}

Consumer::Consumer( Consumer &consumer ) :
	Mlt::Service( consumer ),
	instance( consumer.get_consumer( ) )
{
	inc_ref( );
}

Consumer::Consumer( mlt_consumer consumer ) :
	instance( consumer )
{
	inc_ref( );
}

Consumer::~Consumer( )
{
	mlt_consumer_close( instance );
}

mlt_consumer Consumer::get_consumer( )
{
	return instance;
}

mlt_service Consumer::get_service( )
{
	return mlt_consumer_service( get_consumer( ) );
}

int Consumer::connect( Service &service )
{
	return connect_producer( service );
}

int Consumer::start( )
{
	return mlt_consumer_start( get_consumer( ) );
}

void Consumer::purge( )
{
	mlt_consumer_purge( get_consumer( ) );
}

int Consumer::stop( )
{
	return mlt_consumer_stop( get_consumer( ) );
}

bool Consumer::is_stopped( )
{
	return mlt_consumer_is_stopped( get_consumer( ) ) != 0;
}

int Consumer::run( )
{
	int ret = start( );
	if ( !is_stopped( ) )
	{
		Event *e = setup_wait_for( "consumer-stopped" );
		wait_for( e );
		delete e;
	}
	return ret;
}

int Consumer::position( )
{
	return mlt_consumer_position( get_consumer() );
}
