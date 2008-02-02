/*
 * valerie_remote.c -- Remote Parser
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

/* Application header files */
#include <framework/mlt.h>
#include "valerie_remote.h"
#include "valerie_socket.h"
#include "valerie_tokeniser.h"
#include "valerie_util.h"

/** Private valerie_remote structure.
*/

typedef struct
{
	int terminated;
	char *server;
	int port;
	valerie_socket socket;
	valerie_socket status;
	pthread_t thread;
	valerie_parser parser;
	pthread_mutex_t mutex;
	int connected;
}
*valerie_remote, valerie_remote_t;

/** Forward declarations.
*/

static valerie_response valerie_remote_connect( valerie_remote );
static valerie_response valerie_remote_execute( valerie_remote, char * );
static valerie_response valerie_remote_receive( valerie_remote, char *, char * );
static valerie_response valerie_remote_push( valerie_remote, char *, mlt_service );
static void valerie_remote_close( valerie_remote );
static int valerie_remote_read_response( valerie_socket, valerie_response );

/** DV Parser constructor.
*/

valerie_parser valerie_parser_init_remote( char *server, int port )
{
	valerie_parser parser = calloc( 1, sizeof( valerie_parser_t ) );
	valerie_remote remote = calloc( 1, sizeof( valerie_remote_t ) );

	if ( parser != NULL )
	{
		parser->connect = (parser_connect)valerie_remote_connect;
		parser->execute = (parser_execute)valerie_remote_execute;
		parser->push = (parser_push)valerie_remote_push;
		parser->received = (parser_received)valerie_remote_receive;
		parser->close = (parser_close)valerie_remote_close;
		parser->real = remote;

		if ( remote != NULL )
		{
			remote->parser = parser;
			remote->server = strdup( server );
			remote->port = port;
			pthread_mutex_init( &remote->mutex, NULL );
		}
	}
	return parser;
}

/** Thread for receiving and distributing the status information.
*/

static void *valerie_remote_status_thread( void *arg )
{
	valerie_remote remote = arg;
	char temp[ 10240 ];
	int length = 0;
	int offset = 0;
	valerie_tokeniser tokeniser = valerie_tokeniser_init( );
	valerie_notifier notifier = valerie_parser_get_notifier( remote->parser );
	valerie_status_t status;
	int index = 0;

	valerie_socket_write_data( remote->status, "STATUS\r\n", 8 );

	while ( !remote->terminated && 
			( length = valerie_socket_read_data( remote->status, temp + offset, sizeof( temp ) ) ) >= 0 )
	{
		if ( strchr( temp, '\n' ) == NULL )
		{
			offset = length;
			continue;
		}
		offset = 0;
		valerie_tokeniser_parse_new( tokeniser, temp, "\n" );
		for ( index = 0; index < valerie_tokeniser_count( tokeniser ); index ++ )
		{
			char *line = valerie_tokeniser_get_string( tokeniser, index );
			if ( line[ strlen( line ) - 1 ] == '\r' )
			{
				valerie_util_chomp( line );
				valerie_status_parse( &status, line );
				valerie_notifier_put( notifier, &status );
			}
			else
			{
				strcpy( temp, line );
				offset = strlen( temp );
			}
		}
	}

	valerie_notifier_disconnected( notifier );
	valerie_tokeniser_close( tokeniser );
	remote->terminated = 1;

	return NULL;
}

/** Forward reference.
*/

static void valerie_remote_disconnect( valerie_remote remote );

/** Connect to the server.
*/

static valerie_response valerie_remote_connect( valerie_remote remote )
{
	valerie_response response = NULL;

	valerie_remote_disconnect( remote );

	if ( !remote->connected )
	{
		signal( SIGPIPE, SIG_IGN );

		remote->socket = valerie_socket_init( remote->server, remote->port );
		remote->status = valerie_socket_init( remote->server, remote->port );

		if ( valerie_socket_connect( remote->socket ) == 0 )
		{
			response = valerie_response_init( );
			valerie_remote_read_response( remote->socket, response );
		}

		if ( response != NULL && valerie_socket_connect( remote->status ) == 0 )
		{
			valerie_response status_response = valerie_response_init( );
			valerie_remote_read_response( remote->status, status_response );
			if ( valerie_response_get_error_code( status_response ) == 100 )
				pthread_create( &remote->thread, NULL, valerie_remote_status_thread, remote );
			valerie_response_close( status_response );
			remote->connected = 1;
		}
	}

	return response;
}

/** Execute the command.
*/

static valerie_response valerie_remote_execute( valerie_remote remote, char *command )
{
	valerie_response response = NULL;
	pthread_mutex_lock( &remote->mutex );
	if ( valerie_socket_write_data( remote->socket, command, strlen( command ) ) == strlen( command ) )
	{
		response = valerie_response_init( );
		valerie_socket_write_data( remote->socket, "\r\n", 2 );
		valerie_remote_read_response( remote->socket, response );
	}
	pthread_mutex_unlock( &remote->mutex );
	return response;
}

/** Push a westley document to the server.
*/

static valerie_response valerie_remote_receive( valerie_remote remote, char *command, char *buffer )
{
	valerie_response response = NULL;
	pthread_mutex_lock( &remote->mutex );
	if ( valerie_socket_write_data( remote->socket, command, strlen( command ) ) == strlen( command ) )
	{
		char temp[ 20 ];
		int length = strlen( buffer );
		response = valerie_response_init( );
		valerie_socket_write_data( remote->socket, "\r\n", 2 );
		sprintf( temp, "%d", length );
		valerie_socket_write_data( remote->socket, temp, strlen( temp ) );
		valerie_socket_write_data( remote->socket, "\r\n", 2 );
		valerie_socket_write_data( remote->socket, buffer, length );
		valerie_socket_write_data( remote->socket, "\r\n", 2 );
		valerie_remote_read_response( remote->socket, response );
	}
	pthread_mutex_unlock( &remote->mutex );
	return response;
}

/** Push a producer to the server.
*/

static valerie_response valerie_remote_push( valerie_remote remote, char *command, mlt_service service )
{
	valerie_response response = NULL;
	if ( service != NULL )
	{
		mlt_consumer consumer = mlt_factory_consumer( NULL, "westley", "buffer" );
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
		char *buffer = NULL;
		// Temporary hack
		mlt_properties_set( properties, "store", "nle_" );
		mlt_consumer_connect( consumer, service );
		mlt_consumer_start( consumer );
		buffer = mlt_properties_get( properties, "buffer" );
		response = valerie_remote_receive( remote, command, buffer );
		mlt_consumer_close( consumer );
	}
	return response;
}

/** Disconnect.
*/

static void valerie_remote_disconnect( valerie_remote remote )
{
	if ( remote != NULL && remote->terminated )
	{
		if ( remote->connected )
			pthread_join( remote->thread, NULL );
		valerie_socket_close( remote->status );
		valerie_socket_close( remote->socket );
		remote->connected = 0;
		remote->terminated = 0;
	}
}

/** Close the parser.
*/

static void valerie_remote_close( valerie_remote remote )
{
	if ( remote != NULL )
	{
		remote->terminated = 1;
		valerie_remote_disconnect( remote );
		pthread_mutex_destroy( &remote->mutex );
		free( remote->server );
		free( remote );
	}
}

/** Read response. 
*/

static int valerie_remote_read_response( valerie_socket socket, valerie_response response )
{
	char temp[ 10240 ];
	int length;
	int terminated = 0;

	while ( !terminated && ( length = valerie_socket_read_data( socket, temp, 10240 ) ) >= 0 )
	{
		int position = 0;
		temp[ length ] = '\0';
		valerie_response_write( response, temp, length );
		position = valerie_response_count( response ) - 1;
		if ( position < 0 || temp[ strlen( temp ) - 1 ] != '\n' )
			continue;
		switch( valerie_response_get_error_code( response ) )
		{
			case 201:
			case 500:
				terminated = !strcmp( valerie_response_get_line( response, position ), "" );
				break;
			case 202:
				terminated = valerie_response_count( response ) >= 2;
				break;
			default:
				terminated = 1;
				break;
		}
	}

	return 0;
}
