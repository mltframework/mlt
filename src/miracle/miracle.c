/*
 * miracle.c -- A DV over IEEE 1394 TCP Server
 *
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
 * Authors:
 *     Dan Dennedy <dan@dennedy.org>
 *     Charles Yates <charles.yates@pandora.be>
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

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <framework/mlt.h>

/* Application header files */
#include "miracle_server.h"
#include "miracle_log.h"

/** Our dv server.
*/

static miracle_server server = NULL;

/** atexit shutdown handler for the server.
*/

static void main_cleanup( )
{
	miracle_server_shutdown( server );
}

/** Report usage and exit.
*/

void usage( char *app )
{
	fprintf( stderr, "Usage: %s [-test] [-port NNNN]\n", app );
	exit( 0 );
}

/** The main function.
*/

int main( int argc, char **argv )
{
	int error = 0;
	int index = 0;
	int background = 1;
	struct timespec tm = { 5, 0 };

	server = miracle_server_init( argv[ 0 ] );

	for ( index = 1; index < argc; index ++ )
	{
		if ( !strcmp( argv[ index ], "-port" ) )
			miracle_server_set_port( server, atoi( argv[ ++ index ] ) );
		else if ( !strcmp( argv[ index ], "-proxy" ) )
			miracle_server_set_proxy( server, argv[ ++ index ] );
		else if ( !strcmp( argv[ index ], "-test" ) )
			background = 0;
		else
			usage( argv[ 0 ] );
	}

	/* Optionally detatch ourselves from the controlling tty */

	if ( background )
	{
		if ( fork() )
			return 0;
		setsid();
		miracle_log_init( log_syslog, LOG_INFO );
	}
	else
	{
		miracle_log_init( log_stderr, LOG_DEBUG );
	}

	atexit( main_cleanup );

	/* Execute the server */
	error = miracle_server_execute( server );

	/* We need to wait until we're exited.. */
	while ( !server->shutdown )
		nanosleep( &tm, NULL );

	return error;
}
