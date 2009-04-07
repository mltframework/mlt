/*
 * valerie_parser.c -- Valerie Parser for Miracle
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* Application header files */
#include "valerie_parser.h"
#include "valerie_util.h"

/** Connect to the parser.
*/

valerie_response valerie_parser_connect( valerie_parser parser )
{
	return parser->connect( parser->real );
}

/** Execute a command via the parser.
*/

valerie_response valerie_parser_execute( valerie_parser parser, char *command )
{
	return parser->execute( parser->real, command );
}

/** Push a service via the parser.
*/

valerie_response valerie_parser_received( valerie_parser parser, char *command, char *doc )
{
	return parser->received != NULL ? parser->received( parser->real, command, doc ) : NULL;
}

/** Push a service via the parser.
*/

valerie_response valerie_parser_push( valerie_parser parser, char *command, mlt_service service )
{
	return parser->push( parser->real, command, service );
}

/** Execute a formatted command via the parser.
*/

valerie_response valerie_parser_executef( valerie_parser parser, const char *format, ... )
{
	char *command = malloc( 10240 );
	valerie_response response = NULL;
 	if ( command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, 10240, format, list ) != 0 )
			response = valerie_parser_execute( parser, command );
		va_end( list );
		free( command );
	}
	return response;
}

/** Execute the contents of a file. Note the special case valerie_response returned.
*/

valerie_response valerie_parser_run( valerie_parser parser, char *filename )
{
	valerie_response response = valerie_response_init( );
	if ( response != NULL )
	{
		FILE *file = fopen( filename, "r" );
		if ( file != NULL )
		{
			char command[ 1024 ];
			valerie_response_set_error( response, 201, "OK" );
			while ( valerie_response_get_error_code( response ) == 201 && fgets( command, 1024, file ) )
			{
				valerie_util_trim( valerie_util_chomp( command ) );
				if ( strcmp( command, "" ) && command[ 0 ] != '#' )
				{
					valerie_response temp = NULL;
					valerie_response_printf( response, 1024, "%s\n", command );
					temp = valerie_parser_execute( parser, command );
					if ( temp != NULL )
					{
						int index = 0;
						for ( index = 0; index < valerie_response_count( temp ); index ++ )
							valerie_response_printf( response, 10240, "%s\n", valerie_response_get_line( temp, index ) );
						valerie_response_close( temp );
					}
					else
					{
						valerie_response_set_error( response, 500, "Batch execution failed" );
					}
				}
			}
			fclose( file );
		}
		else
		{
			valerie_response_set_error( response, 404, "File not found." );
		}
	}
	return response;
}

/** Get the notifier associated to the parser.
*/

valerie_notifier valerie_parser_get_notifier( valerie_parser parser )
{
	if ( parser->notifier == NULL )
		parser->notifier = valerie_notifier_init( );
	return parser->notifier;
}

/** Close the parser.
*/

void valerie_parser_close( valerie_parser parser )
{
	if ( parser != NULL )
	{
		parser->close( parser->real );
		valerie_notifier_close( parser->notifier );
		free( parser );
	}
}
