/*
 * dvserver.c -- DV Server
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <netinet/in.h>
#include "log.h"
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

/* Application header files */
#include "dvserver.h"
#include "dvconnection.h"
#include "dvlocal.h"
#include "log.h"
#include <dvremote.h>
#include <dvtokeniser.h>

/** Initialise a server structure.
*/

dv_server dv_server_init( char *id )
{
	dv_server server = malloc( sizeof( dv_server_t ) );
	if ( server != NULL )
	{
		memset( server, 0, sizeof( dv_server_t ) );
		server->id = id;
		server->port = DEFAULT_TCP_PORT;
		server->socket = -1;
	}
	return server;
}

/** Set the port of the server.
*/

void dv_server_set_port( dv_server server, int port )
{
	server->port = port;
}

void dv_server_set_proxy( dv_server server, char *proxy )
{
	dv_tokeniser tokeniser = dv_tokeniser_init( );
	server->proxy = 1;
	server->remote_port = DEFAULT_TCP_PORT;
	dv_tokeniser_parse_new( tokeniser, proxy, ":" );
	strcpy( server->remote_server, dv_tokeniser_get_string( tokeniser, 0 ) );
	if ( dv_tokeniser_count( tokeniser ) == 2 )
		server->remote_port = atoi( dv_tokeniser_get_string( tokeniser, 1 ) );
	dv_tokeniser_close( tokeniser );
}

/** Wait for a connection.
*/

static int dv_server_wait_for_connect( dv_server server )
{
    struct timeval tv;
    fd_set rfds;

    /* Wait for a 1 second. */
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO( &rfds );
    FD_SET( server->socket, &rfds );

    return select( server->socket + 1, &rfds, NULL, NULL, &tv);
}

/** Run the server thread.
*/

static void *dv_server_run( void *arg )
{
	dv_server server = arg;
	pthread_t cmd_parse_info;
	connection_t *tmp = NULL;
	pthread_attr_t thread_attributes;
	int socksize;

	socksize = sizeof( struct sockaddr );

	dv1394d_log( LOG_NOTICE, "%s version %s listening on port %i", server->id, VERSION, server->port );

	/* Create the initial thread. We want all threads to be created detached so
	   their resources get freed automatically. (CY: ... hmmph...) */
	pthread_attr_init( &thread_attributes );
	pthread_attr_setdetachstate( &thread_attributes, PTHREAD_CREATE_DETACHED );
	pthread_attr_init( &thread_attributes );
	pthread_attr_setinheritsched( &thread_attributes, PTHREAD_INHERIT_SCHED );
	/* pthread_attr_setschedpolicy( &thread_attributes, SCHED_RR ); */

	while ( !server->shutdown )
	{
		/* Wait for a new connection. */
		if ( dv_server_wait_for_connect( server ) )
		{
			/* Create a new block of data to hold a copy of the incoming connection for
			   our server thread. The thread should free this when it terminates. */

			tmp = (connection_t*) malloc( sizeof(connection_t) );
			tmp->parser = server->parser;
			tmp->fd = accept( server->socket, (struct sockaddr*) &(tmp->sin), &socksize );

			/* Pass the connection to a parser thread :-/ */
			if ( tmp->fd != -1 )
				pthread_create( &cmd_parse_info, &thread_attributes, parser_thread, tmp );
		}
	}

	dv1394d_log( LOG_NOTICE, "%s version %s server terminated.", server->id, VERSION );

	return NULL;
}

/** Execute the server thread.
*/

int dv_server_execute( dv_server server )
{
	int error = 0;
	dv_response response = NULL;
	int index = 0;
	struct sockaddr_in ServerAddr;
	int flag = 1;

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons( server->port );
	ServerAddr.sin_addr.s_addr = INADDR_ANY;
	
	/* Create socket, and bind to port. Listen there. Backlog = 5
	   should be sufficient for listen (). */
	server->socket = socket( AF_INET, SOCK_STREAM, 0 );

	if ( server->socket == -1 )
	{
		server->shutdown = 1;
		perror( "socket" );
		dv1394d_log( LOG_ERR, "%s unable to create socket.", server->id );
		return -1;
	}

    setsockopt( server->socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof( int ) );

	if ( bind( server->socket, (struct sockaddr *) &ServerAddr, sizeof (ServerAddr) ) != 0 )
	{
		server->shutdown = 1;
		perror( "bind" );
		dv1394d_log( LOG_ERR, "%s unable to bind to port %d.", server->id, server->port );
		return -1;
	}

	if ( listen( server->socket, 5 ) != 0 )
	{
		server->shutdown = 1;
		perror( "listen" );
		dv1394d_log( LOG_ERR, "%s unable to listen on port %d.", server->id, server->port );
		return -1;
	}

	fcntl( server->socket, F_SETFL, O_NONBLOCK );

	if ( !server->proxy )
	{
		dv1394d_log( LOG_NOTICE, "Starting server on %d.", server->port );
		server->parser = dv_parser_init_local( );
	}
	else
	{
		dv1394d_log( LOG_NOTICE, "Starting proxy for %s:%d on %d.", server->remote_server, server->remote_port, server->port );
		server->parser = dv_parser_init_remote( server->remote_server, server->remote_port );
	}

	response = dv_parser_connect( server->parser );

	if ( response != NULL && dv_response_get_error_code( response ) == 100 )
	{
		/* read configuration file */
		if ( response != NULL && !server->proxy )
		{
			dv_response_close( response );
			response = dv_parser_run( server->parser, "/etc/dv1394d.conf" );

			if ( dv_response_count( response ) > 1 )
			{
				if ( dv_response_get_error_code( response ) > 299 )
					dv1394d_log( LOG_ERR, "Error evaluating server configuration. Processing stopped." );
				for ( index = 0; index < dv_response_count( response ); index ++ )
					dv1394d_log( LOG_DEBUG, "%4d: %s", index, dv_response_get_line( response, index ) );
			}
		}

		if ( response != NULL )
		{
			pthread_attr_t attr;
			int result;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
			pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED );
			pthread_attr_setschedpolicy( &attr, SCHED_FIFO );
			pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
			dv_response_close( response );
			result = pthread_create( &server->thread, &attr, dv_server_run, server );
			if ( result )
			{
				dv1394d_log( LOG_WARNING, "Failed to schedule realtime (%s)", strerror(errno) );
				pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
				result = pthread_create( &server->thread, &attr, dv_server_run, server );
				if ( result )
				{
					dv1394d_log( LOG_CRIT, "Failed to launch TCP listener thread" );
					error = -1;
				}
			}
		}
	}
	else
	{
		dv1394d_log( LOG_ERR, "Error connecting to parser. Processing stopped." );
		server->shutdown = 1;
		error = -1;
	}

	return error;
}

/** Shutdown the server.
*/

void dv_server_shutdown( dv_server server )
{
	if ( server != NULL && !server->shutdown )
	{
		server->shutdown = 1;
		pthread_join( server->thread, NULL );
		dv_parser_close( server->parser );
		close( server->socket );
	}
}
