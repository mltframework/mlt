/*
 * remote.c -- Remote dv1394d client demo
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

/* System header files */
#include <stdio.h>
#include <stdint.h>

/* dv1394d header files */
#include <valerie/valerie_remote.h>

/* Application header files */
#include "client.h"
#include "io.h"

/** Connect to a remote server.
*/

static valerie_parser create_parser( )
{
	char server[ 132 ];
	int port;
	valerie_parser parser = NULL;

	printf( "Connecting to a Server\n\n" );

	printf( "Server [localhost]: " );

	if ( get_string( server, sizeof( server ), "localhost" ) != NULL )
	{
		printf( "Port        [5250]: " );

		if ( get_int( &port, 5250 ) != NULL )
			parser = valerie_parser_init_remote( server, port );
	}

	printf( "\n" );

	return parser;
}

/** Main function.
*/

int main( int argc, char **argv )
{
	valerie_parser parser = create_parser( );

	if ( parser != NULL )
	{
		dv_demo demo = dv_demo_init( parser );
		dv_demo_run( demo );
		dv_demo_close( demo );
		valerie_parser_close( parser );
	}

	return 0;
}
