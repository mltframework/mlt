/*
 * dvconnection.c -- DV Connection Handler
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
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <arpa/inet.h>

/* Application header files */
#include "global_commands.h"
#include "dvconnection.h"
#include "dvsocket.h"
#include "dvserver.h"
#include "log.h"

/** This is a generic replacement for fgets which operates on a file
   descriptor. Unlike fgets, we can also specify a line terminator. Maximum
   of (max - 1) chars can be read into buf from fd. If we reach the
   end-of-file, *eof_chk is set to 1. 
*/

int fdgetline( int fd, char *buf, int max, char line_terminator, int *eof_chk )
{
	int count = 0;
	char tmp [1];
	*eof_chk = 0;
	
	if (fd)
		while (count < max - 1) {
			if (read (fd, tmp, 1) > 0) {
				if (tmp [0] != line_terminator)
					buf [count++] = tmp [0];
				else
					break;

/* Is it an EOF character (ctrl-D, i.e. ascii 4)? If so we definitely want
   to break. */

				if (tmp [0] == 4) {
					*eof_chk = 1;
					break;
				}
			} else {
				*eof_chk = 1;
				break;
			}
		}
		
	buf [count] = '\0';
	
	return count;
}

static int connection_initiate( int );
static int connection_send( int, dv_response );
static int connection_read( int, char *, int );
static void connection_close( int );

static int connection_initiate( int fd )
{
	int error = 0;
	dv_response response = dv_response_init( );
	dv_response_set_error( response, 100, "VTR Ready" );
	error = connection_send( fd, response );
	dv_response_close( response );
	return error;
}

static int connection_send( int fd, dv_response response )
{
	int error = 0;
	int index = 0;
	int code = dv_response_get_error_code( response );

	if ( code != -1 )
	{
		int items = dv_response_count( response );

		if ( items == 0 )
			dv_response_set_error( response, 500, "Unknown error" );

		if ( code == 200 && items > 2 )
			dv_response_set_error( response, 201, "OK" );
		else if ( code == 200 && items > 1 )
			dv_response_set_error( response, 202, "OK" );

		code = dv_response_get_error_code( response );
		items = dv_response_count( response );

		for ( index = 0; !error && index < items; index ++ )
		{
			char *line = dv_response_get_line( response, index );
			int length = strlen( line );
			if ( length == 0 && index != dv_response_count( response ) - 1 && write( fd, " ", 1 ) != 1 )
				error = -1;
			else if ( length > 0 && write( fd, line, length ) != length )
				error = -1;
			if ( write( fd, "\r\n", 2 ) != 2 )
				error = -1;			
		}

		if ( ( code == 201 || code == 500 ) && strcmp( dv_response_get_line( response, items - 1 ), "" ) )
			write( fd, "\r\n", 2 );
	}
	else
	{
		char *message = "500 Empty Response\r\n\r\n";
		write( fd, message, strlen( message ) );
	}

	return error;
}

static int connection_read( int fd, char *command, int length )
{
	int eof_chk;
	int nchars = fdgetline( fd, command, length, '\n', &eof_chk );
	char *cr = strchr( command, '\r');
	if ( cr != NULL ) 
		cr[0] = '\0';
	if ( eof_chk || strncmp( command, "BYE", 3 ) == 0 ) 
		nchars = 0;
	return nchars;
}

int connection_status( int fd, dv1394_notifier notifier )
{
	int error = 0;
	int index = 0;
	dv1394_status_t status;
	char text[ 10240 ];
	dv_socket socket = dv_socket_init_fd( fd );
	
	for ( index = 0; !error && index < MAX_UNITS; index ++ )
	{
		dv1394_notifier_get( notifier, &status, index );
		dv1394_status_serialise( &status, text, sizeof( text ) );
		error = dv_socket_write_data( socket, text, strlen( text )  ) != strlen( text );
	}

	while ( !error )
	{
		if ( dv1394_notifier_wait( notifier, &status ) == 0 )
		{
			dv1394_status_serialise( &status, text, sizeof( text ) );
			error = dv_socket_write_data( socket, text, strlen( text ) ) != strlen( text );
		}
		else
		{
			struct timeval tv = { 0, 0 };
			fd_set rfds;

		    FD_ZERO( &rfds );
		    FD_SET( fd, &rfds );

			if ( select( socket->fd + 1, &rfds, NULL, NULL, &tv ) )
				error = 1;
		}
	}

	dv_socket_close( socket );
	
	return error;
}

static void connection_close( int fd )
{
	close( fd );
}

void *parser_thread( void *arg )
{
	struct hostent *he;
	connection_t *connection = arg;
	char address[ 512 ];
	char command[ 1024 ];
	int fd = connection->fd;
	dv_parser parser = connection->parser;
	dv_response response = NULL;

	/* We definitely want to ignore broken pipes. */
    signal( SIGPIPE, SIG_IGN );

	/* Get the connecting clients ip information */
	he = gethostbyaddr( (char *) &( connection->sin.sin_addr.s_addr ), sizeof(u_int32_t), AF_INET); 
	if ( he != NULL )
		strcpy( address, he->h_name );
	else
		inet_ntop( AF_INET, &( connection->sin.sin_addr.s_addr), address, 32 );

	dv1394d_log( LOG_NOTICE, "Connection established with %s (%d)", address, fd );

	/* Execute the commands received. */
	if ( connection_initiate( fd ) == 0 )
	{
		int error = 0;

		while( !error && connection_read( fd, command, 1024 ) )
		{
			if ( strncmp( command, "STATUS", 6 ) )
			{
				response = dv_parser_execute( parser, command );
				dv1394d_log( LOG_INFO, "%s \"%s\" %d", address, command, dv_response_get_error_code( response ) );
				error = connection_send( fd, response );
				dv_response_close( response );
			}
			else
			{
				error = connection_status( fd, dv_parser_get_notifier( parser ) );
			}
		}
	}

	/* Free the resources associated with this connection. */
	connection_close( fd );

	dv1394d_log( LOG_NOTICE, "Connection with %s (%d) closed", address, fd );

	free( connection );

	return NULL;
}
