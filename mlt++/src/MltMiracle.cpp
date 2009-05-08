/**
 * MltMiracle.cpp - MLT Wrapper
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

#include "MltMiracle.h"
#include "MltService.h"
#include "MltResponse.h"
using namespace Mlt;

#include <time.h>

static valerie_response mlt_miracle_execute( void *arg, char *command )
{
	Miracle *miracle = ( Miracle * )arg;
	if ( miracle != NULL )
	{
		Response *response = miracle->execute( command );
		valerie_response real = valerie_response_clone( response->get_response( ) );
		delete response;
		return real;
	}
	else
	{
		valerie_response response = valerie_response_init( );
		valerie_response_set_error( response, 500, "Invalid server" );
		return response;
	}
}

static valerie_response mlt_miracle_received( void *arg, char *command, char *doc )
{
	Miracle *miracle = ( Miracle * )arg;
	if ( miracle != NULL )
	{
		Response *response = miracle->received( command, doc );
		if ( response != NULL )
		{
			valerie_response real = valerie_response_clone( response->get_response( ) );
			delete response;
			return real;
		}
		return NULL;
	}
	else
	{
		valerie_response response = valerie_response_init( );
		valerie_response_set_error( response, 500, "Invalid server" );
		return response;
	}
}

static valerie_response mlt_miracle_push( void *arg, char *command, mlt_service service )
{
	Miracle *miracle = ( Miracle * )arg;
	if ( miracle != NULL )
	{
		Service input( service );
		Response *response = miracle->push( command, &input );
		valerie_response real = valerie_response_clone( response->get_response( ) );
		delete response;
		return real;
	}
	else
	{
		valerie_response response = valerie_response_init( );
		valerie_response_set_error( response, 500, "Invalid server" );
		return response;
	}
}

Miracle::Miracle( char *name, int port, char *config ) :
	Properties( false )
{
	server = miracle_server_init( name );
	miracle_server_set_port( server, port );
	miracle_server_set_config( server, config );
}

Miracle::~Miracle( )
{
	miracle_server_close( server );
}

mlt_properties Miracle::get_properties( )
{
	return &server->parent;
}

bool Miracle::start( )
{
	if ( miracle_server_execute( server ) == 0 )
	{
		_real = server->parser->real;
		_execute = server->parser->execute;
		_received = server->parser->received;
		_push = server->parser->push;
		server->parser->real = this;
		server->parser->execute = mlt_miracle_execute;
		server->parser->received = mlt_miracle_received;
		server->parser->push = mlt_miracle_push;
	}
	return server->shutdown == 0;
}

bool Miracle::is_running( )
{
	return server->shutdown == 0;
}

Response *Miracle::execute( char *command )
{
	return new Response( _execute( _real, command ) );
}

Response *Miracle::received( char *command, char *doc )
{
	return new Response( _received( _real, command, doc ) );
}

Response *Miracle::push( char *command, Service *service )
{
	return new Response( _push( _real, command, service->get_service( ) ) );
}

void Miracle::wait_for_shutdown( )
{
	struct timespec tm = { 1, 0 };
	while ( !server->shutdown )
		nanosleep( &tm, NULL );
}

void Miracle::log_level( int threshold )
{
	miracle_log_init( log_stderr, threshold );
}

Properties *Miracle::unit( int index )
{
	mlt_properties properties = miracle_server_fetch_unit( server, index );
	return properties != NULL ? new Properties( properties ) : NULL;
}
