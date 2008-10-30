/*
 * albino.c -- Local Valerie/Miracle Test Utility
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
#include <stdlib.h>
#include <string.h>
#include <sched.h>

/* Application header files */
#include <miracle/miracle_local.h>
#include <valerie/valerie_remote.h>
#include <valerie/valerie_util.h>

char *prompt( char *command, int length )
{
	printf( "> " );
	return fgets( command, length, stdin );
}

void report( valerie_response response )
{
	int index = 0;
	if ( response != NULL )
		for ( index = 0; index < valerie_response_count( response ); index ++ )
			printf( "%4d: %s\n", index, valerie_response_get_line( response, index ) );
}

int main( int argc, char **argv  )
{
	valerie_parser parser = NULL;
	valerie_response response = NULL;
	char temp[ 1024 ];
	int index = 1;

	if ( argc > 2 && !strcmp( argv[ 1 ], "-s" ) )
	{
		printf( "Miracle Client Instance\n" );
		parser = valerie_parser_init_remote( argv[ 2 ], 5250 );
		response = valerie_parser_connect( parser );
		index = 3;
	}
	else
	{
		printf( "Miracle Standalone Instance\n" );
		parser = miracle_parser_init_local( );
		response = valerie_parser_connect( parser );
	}

	if ( response != NULL )
	{
		/* process files on command lines before going into console mode */
		for ( ; index < argc; index ++ )
		{
			valerie_response_close( response );
			response = valerie_parser_run( parser, argv[ index ] );
			report( response );
		}
	
		while ( response != NULL && prompt( temp, 1024 ) )
		{
			valerie_util_trim( valerie_util_chomp( temp ) );
			if ( !strcasecmp( temp, "BYE" ) )
			{
				break;
			}
			else if ( strcmp( temp, "" ) )
			{
				valerie_response_close( response );
				response = valerie_parser_execute( parser, temp );
				report( response );
			}
		}
	}
	else
	{
		fprintf( stderr, "Unable to connect to a Miracle instance.\n" );
	}

	printf( "\n" );
	valerie_parser_close( parser );

	return 0;
}
