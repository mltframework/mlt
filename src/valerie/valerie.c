/*
 * valerie.c -- High Level Client API for miracle
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
#include <stdarg.h>

/* Application header files */
#include "valerie.h"
#include "valerie_tokeniser.h"
#include "valerie_util.h"

/** Initialise the valerie structure.
*/

valerie valerie_init( valerie_parser parser )
{
	valerie this = malloc( sizeof( valerie_t ) );
	if ( this != NULL )
	{
		memset( this, 0, sizeof( valerie_t ) );
		this->parser = parser;
	}
	return this;
}

/** Set the response structure associated to the last command.
*/

static void valerie_set_last_response( valerie this, valerie_response response )
{
	if ( this != NULL )
	{
		if ( this->last_response != NULL )
			valerie_response_close( this->last_response );
		this->last_response = response;
	}
}

/** Connect to the parser.
*/

valerie_error_code valerie_connect( valerie this )
{
	valerie_error_code error = valerie_server_unavailable;
	valerie_response response = valerie_parser_connect( this->parser );
	if ( response != NULL )
	{
		valerie_set_last_response( this, response );
		if ( valerie_response_get_error_code( response ) == 100 )
			error = valerie_ok;
	}
	return error;
}

/** Interpret a non-context sensitive error code.
*/

static valerie_error_code valerie_get_error_code( valerie this, valerie_response response )
{
	valerie_error_code error = valerie_server_unavailable;
	switch( valerie_response_get_error_code( response ) )
	{
		case -1:
			error = valerie_server_unavailable;
			break;
		case -2:
			error = valerie_no_response;
			break;
		case 200:
		case 201:
		case 202:
			error = valerie_ok;
			break;
		case 400:
			error = valerie_invalid_command;
			break;
		case 401:
			error = valerie_server_timeout;
			break;
		case 402:
			error = valerie_missing_argument;
			break;
		case 403:
			error = valerie_unit_unavailable;
			break;
		case 404:
			error = valerie_invalid_file;
			break;
		default:
		case 500:
			error = valerie_unknown_error;
			break;
	}
	return error;
}

/** Execute a command.
*/

valerie_error_code valerie_execute( valerie this, size_t size, char *format, ... )
{
	valerie_error_code error = valerie_server_unavailable;
	char *command = malloc( size );
	if ( this != NULL && command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, size, format, list ) != 0 )
		{
			valerie_response response = valerie_parser_execute( this->parser, command );
			valerie_set_last_response( this, response );
			error = valerie_get_error_code( this, response );
		}
		else
		{
			error = valerie_invalid_command;
		}
		va_end( list );
	}
	else
	{
		error = valerie_malloc_failed;
	}
	free( command );
	return error;
}

/** Execute a command.
*/

valerie_error_code valerie_receive( valerie this, char *doc, size_t size, char *format, ... )
{
	valerie_error_code error = valerie_server_unavailable;
	char *command = malloc( size );
	if ( this != NULL && command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, size, format, list ) != 0 )
		{
			valerie_response response = valerie_parser_received( this->parser, command, doc );
			valerie_set_last_response( this, response );
			error = valerie_get_error_code( this, response );
		}
		else
		{
			error = valerie_invalid_command;
		}
		va_end( list );
	}
	else
	{
		error = valerie_malloc_failed;
	}
	free( command );
	return error;
}

/** Execute a command.
*/

valerie_error_code valerie_push( valerie this, mlt_service service, size_t size, char *format, ... )
{
	valerie_error_code error = valerie_server_unavailable;
	char *command = malloc( size );
	if ( this != NULL && command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, size, format, list ) != 0 )
		{
			valerie_response response = valerie_parser_push( this->parser, command, service );
			valerie_set_last_response( this, response );
			error = valerie_get_error_code( this, response );
		}
		else
		{
			error = valerie_invalid_command;
		}
		va_end( list );
	}
	else
	{
		error = valerie_malloc_failed;
	}
	free( command );
	return error;
}

/** Set a global property.
*/

valerie_error_code valerie_set( valerie this, char *property, char *value )
{
	return valerie_execute( this, 1024, "SET %s=%s", property, value );
}

/** Get a global property.
*/

valerie_error_code valerie_get( valerie this, char *property, char *value, int length )
{
	valerie_error_code error = valerie_execute( this, 1024, "GET %s", property );
	if ( error == valerie_ok )
	{
		valerie_response response = valerie_get_last_response( this );
		strncpy( value, valerie_response_get_line( response, 1 ), length );
	}
	return error;
}

/** Run a script.
*/

valerie_error_code valerie_run( valerie this, char *file )
{
	return valerie_execute( this, 10240, "RUN \"%s\"", file );
}

/** Add a unit.
*/

valerie_error_code valerie_unit_add( valerie this, char *guid, int *unit )
{
	valerie_error_code error = valerie_execute( this, 1024, "UADD %s", guid );
	if ( error == valerie_ok )
	{
		int length = valerie_response_count( this->last_response );
		char *line = valerie_response_get_line( this->last_response, length - 1 );
		if ( line == NULL || sscanf( line, "U%d", unit ) != 1 )
			error = valerie_unit_creation_failed;
	}
	else
	{
		if ( error == valerie_unknown_error )
			error = valerie_unit_creation_failed;
	}
	return error;
}

/** Load a file on the specified unit.
*/

valerie_error_code valerie_unit_load( valerie this, int unit, char *file )
{
	return valerie_execute( this, 10240, "LOAD U%d \"%s\"", unit, file );
}

static void valerie_interpret_clip_offset( char *output, valerie_clip_offset offset, int clip )
{
	switch( offset )
	{
		case valerie_absolute:
			sprintf( output, "%d", clip );
			break;
		case valerie_relative:
			if ( clip < 0 )
				sprintf( output, "%d", clip );
			else
				sprintf( output, "+%d", clip );
			break;
	}
}

/** Load a file on the specified unit with the specified in/out points.
*/

valerie_error_code valerie_unit_load_clipped( valerie this, int unit, char *file, int32_t in, int32_t out )
{
	return valerie_execute( this, 10240, "LOAD U%d \"%s\" %d %d", unit, file, in, out );
}

/** Load a file on the specified unit at the end of the current pump.
*/

valerie_error_code valerie_unit_load_back( valerie this, int unit, char *file )
{
	return valerie_execute( this, 10240, "LOAD U%d \"!%s\"", unit, file );
}

/** Load a file on the specified unit at the end of the pump with the specified in/out points.
*/

valerie_error_code valerie_unit_load_back_clipped( valerie this, int unit, char *file, int32_t in, int32_t out )
{
	return valerie_execute( this, 10240, "LOAD U%d \"!%s\" %d %d", unit, file, in, out );
}

/** Append a file on the specified unit.
*/

valerie_error_code valerie_unit_append( valerie this, int unit, char *file, int32_t in, int32_t out )
{
	return valerie_execute( this, 10240, "APND U%d \"%s\" %d %d", unit, file, in, out );
}

/** Push a service on to a unit.
*/

valerie_error_code valerie_unit_receive( valerie this, int unit, char *command, char *doc )
{
	return valerie_receive( this, doc, 10240, "PUSH U%d %s", unit, command );
}

/** Push a service on to a unit.
*/

valerie_error_code valerie_unit_push( valerie this, int unit, char *command, mlt_service service )
{
	return valerie_push( this, service, 10240, "PUSH U%d %s", unit, command );
}

/** Clean the unit - this function removes all but the currently playing clip.
*/

valerie_error_code valerie_unit_clean( valerie this, int unit )
{
	return valerie_execute( this, 1024, "CLEAN U%d", unit );
}

/** Clear the unit - this function removes all clips.
*/

valerie_error_code valerie_unit_clear( valerie this, int unit )
{
	return valerie_execute( this, 1024, "CLEAR U%d", unit );
}

/** Move clips on the units playlist.
*/

valerie_error_code valerie_unit_clip_move( valerie this, int unit, valerie_clip_offset src_offset, int src, valerie_clip_offset dest_offset, int dest )
{
	char temp1[ 100 ];
	char temp2[ 100 ];
	valerie_interpret_clip_offset( temp1, src_offset, src );
	valerie_interpret_clip_offset( temp2, dest_offset, dest );
	return valerie_execute( this, 1024, "MOVE U%d %s %s", unit, temp1, temp2 );
}

/** Remove clip at the specified position.
*/

valerie_error_code valerie_unit_clip_remove( valerie this, int unit, valerie_clip_offset offset, int clip )
{
	char temp[ 100 ];
	valerie_interpret_clip_offset( temp, offset, clip );
	return valerie_execute( this, 1024, "REMOVE U%d %s", unit, temp );
}

/** Remove the currently playing clip.
*/

valerie_error_code valerie_unit_remove_current_clip( valerie this, int unit )
{
	return valerie_execute( this, 1024, "REMOVE U%d", unit );
}

/** Insert clip at the specified position.
*/

valerie_error_code valerie_unit_clip_insert( valerie this, int unit, valerie_clip_offset offset, int clip, char *file, int32_t in, int32_t out )
{
	char temp[ 100 ];
	valerie_interpret_clip_offset( temp, offset, clip );
	return valerie_execute( this, 1024, "INSERT U%d \"%s\" %s %d %d", unit, file, temp, in, out );
}

/** Play the unit at normal speed.
*/

valerie_error_code valerie_unit_play( valerie this, int unit )
{
	return valerie_execute( this, 1024, "PLAY U%d 1000", unit );
}

/** Play the unit at specified speed.
*/

valerie_error_code valerie_unit_play_at_speed( valerie this, int unit, int speed )
{
	return valerie_execute( this, 10240, "PLAY U%d %d", unit, speed );
}

/** Stop playback on the specified unit.
*/

valerie_error_code valerie_unit_stop( valerie this, int unit )
{
	return valerie_execute( this, 1024, "STOP U%d", unit );
}

/** Pause playback on the specified unit.
*/

valerie_error_code valerie_unit_pause( valerie this, int unit )
{
	return valerie_execute( this, 1024, "PAUSE U%d", unit );
}

/** Rewind the specified unit.
*/

valerie_error_code valerie_unit_rewind( valerie this, int unit )
{
	return valerie_execute( this, 1024, "REW U%d", unit );
}

/** Fast forward the specified unit.
*/

valerie_error_code valerie_unit_fast_forward( valerie this, int unit )
{
	return valerie_execute( this, 1024, "FF U%d", unit );
}

/** Step by the number of frames on the specified unit.
*/

valerie_error_code valerie_unit_step( valerie this, int unit, int32_t step )
{
	return valerie_execute( this, 1024, "STEP U%d %d", unit, step );
}

/** Goto the specified frame on the specified unit.
*/

valerie_error_code valerie_unit_goto( valerie this, int unit, int32_t position )
{
	return valerie_execute( this, 1024, "GOTO U%d %d", unit, position );
}

/** Goto the specified frame in the clip on the specified unit.
*/

valerie_error_code valerie_unit_clip_goto( valerie this, int unit, valerie_clip_offset offset, int clip, int32_t position )
{
	char temp[ 100 ];
	valerie_interpret_clip_offset( temp, offset, clip );
	return valerie_execute( this, 1024, "GOTO U%d %d %s", unit, position, temp );
}

/** Set the in point of the loaded file on the specified unit.
*/

valerie_error_code valerie_unit_set_in( valerie this, int unit, int32_t in )
{
	return valerie_execute( this, 1024, "SIN U%d %d", unit, in );
}

/** Set the in point of the clip on the specified unit.
*/

valerie_error_code valerie_unit_clip_set_in( valerie this, int unit, valerie_clip_offset offset, int clip, int32_t in )
{
	char temp[ 100 ];
	valerie_interpret_clip_offset( temp, offset, clip );
	return valerie_execute( this, 1024, "SIN U%d %d %s", unit, in, temp );
}

/** Set the out point of the loaded file on the specified unit.
*/

valerie_error_code valerie_unit_set_out( valerie this, int unit, int32_t out )
{
	return valerie_execute( this, 1024, "SOUT U%d %d", unit, out );
}

/** Set the out point of the clip on the specified unit.
*/

valerie_error_code valerie_unit_clip_set_out( valerie this, int unit, valerie_clip_offset offset, int clip, int32_t in )
{
	char temp[ 100 ];
	valerie_interpret_clip_offset( temp, offset, clip );
	return valerie_execute( this, 1024, "SOUT U%d %d %s", unit, in, temp );
}

/** Clear the in point of the loaded file on the specified unit.
*/

valerie_error_code valerie_unit_clear_in( valerie this, int unit )
{
	return valerie_execute( this, 1024, "SIN U%d -1", unit );
}

/** Clear the out point of the loaded file on the specified unit.
*/

valerie_error_code valerie_unit_clear_out( valerie this, int unit )
{
	return valerie_execute( this, 1024, "SOUT U%d -1", unit );
}

/** Clear the in and out points on the loaded file on the specified unit.
*/

valerie_error_code valerie_unit_clear_in_out( valerie this, int unit )
{
	valerie_error_code error = valerie_unit_clear_out( this, unit );
	if ( error == valerie_ok )
		error = valerie_unit_clear_in( this, unit );
	return error;
}

/** Set a unit configuration property.
*/

valerie_error_code valerie_unit_set( valerie this, int unit, char *name, char *value )
{
	return valerie_execute( this, 1024, "USET U%d %s=%s", unit, name, value );
}

/** Get a unit configuration property.
*/

valerie_error_code valerie_unit_get( valerie this, int unit, char *name )
{
	return valerie_execute( this, 1024, "UGET U%d %s", unit, name );
}

/** Get a units status.
*/

valerie_error_code valerie_unit_status( valerie this, int unit, valerie_status status )
{
	valerie_error_code error = valerie_execute( this, 1024, "USTA U%d", unit );
	int error_code = valerie_response_get_error_code( this->last_response );

	memset( status, 0, sizeof( valerie_status_t ) );
	status->unit = unit;
	if ( error_code == 202 && valerie_response_count( this->last_response ) == 2 )
		valerie_status_parse( status, valerie_response_get_line( this->last_response, 1 ) );
	else if ( error_code == 403 )
		status->status = unit_undefined;

	return error;
}

/** Transfer the current settings of unit src to unit dest.
*/

valerie_error_code valerie_unit_transfer( valerie this, int src, int dest )
{
	return valerie_execute( this, 1024, "XFER U%d U%d", src, dest );
}

/** Obtain the parsers notifier.
*/

valerie_notifier valerie_get_notifier( valerie this )
{
	if ( this != NULL )
		return valerie_parser_get_notifier( this->parser );
	else
		return NULL;
}

/** List the contents of the specified directory.
*/

valerie_dir valerie_dir_init( valerie this, char *directory )
{
	valerie_dir dir = malloc( sizeof( valerie_dir_t ) );
	if ( dir != NULL )
	{
		memset( dir, 0, sizeof( valerie_dir_t ) );
		dir->directory = strdup( directory );
		dir->response = valerie_parser_executef( this->parser, "CLS \"%s\"", directory );
	}
	return dir;
}

/** Return the error code associated to the dir.
*/

valerie_error_code valerie_dir_get_error_code( valerie_dir dir )
{
	if ( dir != NULL )
		return valerie_get_error_code( NULL, dir->response );
	else
		return valerie_malloc_failed;
}

/** Get a particular file entry in the directory.
*/

valerie_error_code valerie_dir_get( valerie_dir dir, int index, valerie_dir_entry entry )
{
	valerie_error_code error = valerie_ok;
	memset( entry, 0, sizeof( valerie_dir_entry_t ) );
	if ( index < valerie_dir_count( dir ) )
	{
		char *line = valerie_response_get_line( dir->response, index + 1 );
		valerie_tokeniser tokeniser = valerie_tokeniser_init( );
		valerie_tokeniser_parse_new( tokeniser, line, " " );

		if ( valerie_tokeniser_count( tokeniser ) > 0 )
		{
			valerie_util_strip( valerie_tokeniser_get_string( tokeniser, 0 ), '\"' );
			strcpy( entry->full, dir->directory );
			if ( entry->full[ strlen( entry->full ) - 1 ] != '/' )
				strcat( entry->full, "/" );
			strcpy( entry->name, valerie_tokeniser_get_string( tokeniser, 0 ) );
			strcat( entry->full, entry->name );

			switch ( valerie_tokeniser_count( tokeniser ) )
			{
				case 1:
					entry->dir = 1;
					break;
				case 2:
					entry->size = strtoull( valerie_tokeniser_get_string( tokeniser, 1 ), NULL, 10 );
					break;
				default:
					error = valerie_invalid_file;
					break;
			}
		}
		valerie_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of entries in the directory
*/

int valerie_dir_count( valerie_dir dir )
{
	if ( dir != NULL && valerie_response_count( dir->response ) >= 2 )
		return valerie_response_count( dir->response ) - 2;
	else
		return -1;
}

/** Close the directory structure.
*/

void valerie_dir_close( valerie_dir dir )
{
	if ( dir != NULL )
	{
		free( dir->directory );
		valerie_response_close( dir->response );
		free( dir );
	}
}

/** List the playlist of the specified unit.
*/

valerie_list valerie_list_init( valerie this, int unit )
{
	valerie_list list = calloc( 1, sizeof( valerie_list_t ) );
	if ( list != NULL )
	{
		list->response = valerie_parser_executef( this->parser, "LIST U%d", unit );
		if ( valerie_response_count( list->response ) >= 2 )
			list->generation = atoi( valerie_response_get_line( list->response, 1 ) );
	}
	return list;
}

/** Return the error code associated to the list.
*/

valerie_error_code valerie_list_get_error_code( valerie_list list )
{
	if ( list != NULL )
		return valerie_get_error_code( NULL, list->response );
	else
		return valerie_malloc_failed;
}

/** Get a particular file entry in the list.
*/

valerie_error_code valerie_list_get( valerie_list list, int index, valerie_list_entry entry )
{
	valerie_error_code error = valerie_ok;
	memset( entry, 0, sizeof( valerie_list_entry_t ) );
	if ( index < valerie_list_count( list ) )
	{
		char *line = valerie_response_get_line( list->response, index + 2 );
		valerie_tokeniser tokeniser = valerie_tokeniser_init( );
		valerie_tokeniser_parse_new( tokeniser, line, " " );

		if ( valerie_tokeniser_count( tokeniser ) > 0 )
		{
			entry->clip = atoi( valerie_tokeniser_get_string( tokeniser, 0 ) );
			valerie_util_strip( valerie_tokeniser_get_string( tokeniser, 1 ), '\"' );
			strcpy( entry->full, valerie_tokeniser_get_string( tokeniser, 1 ) );
			entry->in = atol( valerie_tokeniser_get_string( tokeniser, 2 ) );
			entry->out = atol( valerie_tokeniser_get_string( tokeniser, 3 ) );
			entry->max = atol( valerie_tokeniser_get_string( tokeniser, 4 ) );
			entry->size = atol( valerie_tokeniser_get_string( tokeniser, 5 ) );
			entry->fps = atof( valerie_tokeniser_get_string( tokeniser, 6 ) );
		}
		valerie_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of entries in the list
*/

int valerie_list_count( valerie_list list )
{
	if ( list != NULL && valerie_response_count( list->response ) >= 3 )
		return valerie_response_count( list->response ) - 3;
	else
		return -1;
}

/** Close the list structure.
*/

void valerie_list_close( valerie_list list )
{
	if ( list != NULL )
	{
		valerie_response_close( list->response );
		free( list );
	}
}

/** List the currently connected nodes.
*/

valerie_nodes valerie_nodes_init( valerie this )
{
	valerie_nodes nodes = malloc( sizeof( valerie_nodes_t ) );
	if ( nodes != NULL )
	{
		memset( nodes, 0, sizeof( valerie_nodes_t ) );
		nodes->response = valerie_parser_executef( this->parser, "NLS" );
	}
	return nodes;
}

/** Return the error code associated to the nodes list.
*/

valerie_error_code valerie_nodes_get_error_code( valerie_nodes nodes )
{
	if ( nodes != NULL )
		return valerie_get_error_code( NULL, nodes->response );
	else
		return valerie_malloc_failed;
}

/** Get a particular node entry.
*/

valerie_error_code valerie_nodes_get( valerie_nodes nodes, int index, valerie_node_entry entry )
{
	valerie_error_code error = valerie_ok;
	memset( entry, 0, sizeof( valerie_node_entry_t ) );
	if ( index < valerie_nodes_count( nodes ) )
	{
		char *line = valerie_response_get_line( nodes->response, index + 1 );
		valerie_tokeniser tokeniser = valerie_tokeniser_init( );
		valerie_tokeniser_parse_new( tokeniser, line, " " );

		if ( valerie_tokeniser_count( tokeniser ) == 3 )
		{
			entry->node = atoi( valerie_tokeniser_get_string( tokeniser, 0 ) );
			strncpy( entry->guid, valerie_tokeniser_get_string( tokeniser, 1 ), sizeof( entry->guid ) );
			valerie_util_strip( valerie_tokeniser_get_string( tokeniser, 2 ), '\"' );
			strncpy( entry->name, valerie_tokeniser_get_string( tokeniser, 2 ), sizeof( entry->name ) );
		}

		valerie_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of nodes
*/

int valerie_nodes_count( valerie_nodes nodes )
{
	if ( nodes != NULL && valerie_response_count( nodes->response ) >= 2 )
		return valerie_response_count( nodes->response ) - 2;
	else
		return -1;
}

/** Close the nodes structure.
*/

void valerie_nodes_close( valerie_nodes nodes )
{
	if ( nodes != NULL )
	{
		valerie_response_close( nodes->response );
		free( nodes );
	}
}

/** List the currently defined units.
*/

valerie_units valerie_units_init( valerie this )
{
	valerie_units units = malloc( sizeof( valerie_units_t ) );
	if ( units != NULL )
	{
		memset( units, 0, sizeof( valerie_units_t ) );
		units->response = valerie_parser_executef( this->parser, "ULS" );
	}
	return units;
}

/** Return the error code associated to the nodes list.
*/

valerie_error_code valerie_units_get_error_code( valerie_units units )
{
	if ( units != NULL )
		return valerie_get_error_code( NULL, units->response );
	else
		return valerie_malloc_failed;
}

/** Get a particular unit entry.
*/

valerie_error_code valerie_units_get( valerie_units units, int index, valerie_unit_entry entry )
{
	valerie_error_code error = valerie_ok;
	memset( entry, 0, sizeof( valerie_unit_entry_t ) );
	if ( index < valerie_units_count( units ) )
	{
		char *line = valerie_response_get_line( units->response, index + 1 );
		valerie_tokeniser tokeniser = valerie_tokeniser_init( );
		valerie_tokeniser_parse_new( tokeniser, line, " " );

		if ( valerie_tokeniser_count( tokeniser ) == 4 )
		{
			entry->unit = atoi( valerie_tokeniser_get_string( tokeniser, 0 ) + 1 );
			entry->node = atoi( valerie_tokeniser_get_string( tokeniser, 1 ) );
			strncpy( entry->guid, valerie_tokeniser_get_string( tokeniser, 2 ), sizeof( entry->guid ) );
			entry->online = atoi( valerie_tokeniser_get_string( tokeniser, 3 ) );
		}

		valerie_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of units
*/

int valerie_units_count( valerie_units units )
{
	if ( units != NULL && valerie_response_count( units->response ) >= 2 )
		return valerie_response_count( units->response ) - 2;
	else
		return -1;
}

/** Close the units structure.
*/

void valerie_units_close( valerie_units units )
{
	if ( units != NULL )
	{
		valerie_response_close( units->response );
		free( units );
	}
}

/** Get the response of the last command executed.
*/

valerie_response valerie_get_last_response( valerie this )
{
	return this->last_response;
}

/** Obtain a printable message associated to the error code provided.
*/

char *valerie_error_description( valerie_error_code error )
{
	char *msg = "Unrecognised error";
	switch( error )
	{
		case valerie_ok:
			msg = "OK";
			break;
		case valerie_malloc_failed:
			msg = "Memory allocation error";
			break;
		case valerie_unknown_error:
			msg = "Unknown error";
			break;
		case valerie_no_response:
			msg = "No response obtained";
			break;
		case valerie_invalid_command:
			msg = "Invalid command";
			break;
		case valerie_server_timeout:
			msg = "Communications with server timed out";
			break;
		case valerie_missing_argument:
			msg = "Missing argument";
			break;
		case valerie_server_unavailable:
			msg = "Unable to communicate with server";
			break;
		case valerie_unit_creation_failed:
			msg = "Unit creation failed";
			break;
		case valerie_unit_unavailable:
			msg = "Unit unavailable";
			break;
		case valerie_invalid_file:
			msg = "Invalid file";
			break;
		case valerie_invalid_position:
			msg = "Invalid position";
			break;
	}
	return msg;
}

/** Close the valerie structure.
*/

void valerie_close( valerie this )
{
	if ( this != NULL )
	{
		valerie_set_last_response( this, NULL );
		free( this );
	}
}
