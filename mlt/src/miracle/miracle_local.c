/*
 * dvlocal.c -- Local dv1394d Parser
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Library header files */
#include <dvutil.h>

/* Application header files */
#include <dvclipfactory.h>
#include <dvframepool.h>
#include "dvlocal.h"
#include "dvconnection.h"
#include "global_commands.h"
#include "unit_commands.h"
#include "log.h"
#include "raw1394util.h"

/** Private dv_local structure.
*/

typedef struct
{
	dv_parser parser;
	char root_dir[1024];
}
*dv_local, dv_local_t;

/** Forward declarations.
*/

static dv_response dv_local_connect( dv_local );
static dv_response dv_local_execute( dv_local, char * );
static void dv_local_close( dv_local );
response_codes print_help( command_argument arg );
response_codes dv1394d_run( command_argument arg );
response_codes dv1394d_shutdown( command_argument arg );

/** DV Parser constructor.
*/

dv_parser dv_parser_init_local( )
{
	dv_parser parser = malloc( sizeof( dv_parser_t ) );
	dv_local local = malloc( sizeof( dv_local_t ) );

	if ( parser != NULL )
	{
		memset( parser, 0, sizeof( dv_parser_t ) );

		parser->connect = (parser_connect)dv_local_connect;
		parser->execute = (parser_execute)dv_local_execute;
		parser->close = (parser_close)dv_local_close;
		parser->real = local;

		if ( local != NULL )
		{
			memset( local, 0, sizeof( dv_local_t ) );
			local->parser = parser;
			local->root_dir[0] = '/';
		}
	}
	return parser;
}

/** response status code/message pair 
*/

typedef struct 
{
	int code;
	char *message;
} 
responses_t;

/** response messages 
*/

static responses_t responses [] = 
{
	{RESPONSE_SUCCESS, "OK"},
	{RESPONSE_SUCCESS_N, "OK"},
	{RESPONSE_SUCCESS_1, "OK"},
	{RESPONSE_UNKNOWN_COMMAND, "Unknown command"},
	{RESPONSE_TIMEOUT, "Operation timed out"},
	{RESPONSE_MISSING_ARG, "Argument missing"},
	{RESPONSE_INVALID_UNIT, "Unit not found"},
	{RESPONSE_BAD_FILE, "Failed to locate or open clip"},
	{RESPONSE_OUT_OF_RANGE, "Argument value out of range"},
	{RESPONSE_TOO_MANY_FILES, "Too many files open"},
	{RESPONSE_ERROR, "Server Error"}
};

/** Argument types.
*/

typedef enum 
{
	ATYPE_NONE,
	ATYPE_FLOAT,
	ATYPE_STRING,
	ATYPE_INT
} 
arguments_types;

/** A command definition.
*/

typedef struct 
{
/* The command string corresponding to this operation (e.g. "play") */
	char *command;
/* The function associated with it */
	response_codes (*operation) ( command_argument );
/* a boolean to indicate if this is a unit or global command
   unit commands require a unit identifier as first argument */
	int is_unit;
/* What type is the argument (RTTI :-) ATYPE_whatever */
	int type;
/* online help information */
	char *help;
} 
command_t;

/* The following define the queue of commands available to the user. The
   first entry is the name of the command (the string which must be typed),
   the second command is the function associated with it, the third argument
   is for the type of the argument, and the last argument specifies whether
   this is something which should be handled immediately or whether it
   should be queued (only robot motion commands need to be queued). */

static command_t vocabulary[] = 
{
	{"BYE", NULL, 0, ATYPE_NONE, "Terminates the session. Units are not removed and task queue is not flushed."},
	{"HELP", print_help, 0, ATYPE_NONE, "Display this information!"},
	{"NLS", dv1394d_list_nodes, 0, ATYPE_NONE, "List the AV/C nodes on the 1394 bus."},
	{"UADD", dv1394d_add_unit, 0, ATYPE_STRING, "Create a new DV unit (virtual VTR) to transmit to receiver specified in GUID argument."},
	{"ULS", dv1394d_list_units, 0, ATYPE_NONE, "Lists the units that have already been added to the server."},
	{"CLS", dv1394d_list_clips, 0, ATYPE_STRING, "Lists the clips at directory name argument."},
	{"SET", dv1394d_set_global_property, 0, ATYPE_STRING, "Set a server configuration property."},
	{"GET", dv1394d_get_global_property, 0, ATYPE_STRING, "Get a server configuration property."},
	{"RUN", dv1394d_run, 0, ATYPE_STRING, "Run a batch file." },
	{"LIST", dv1394d_list, 1, ATYPE_NONE, "List the playlist associated to a unit."},
	{"LOAD", dv1394d_load, 1, ATYPE_STRING, "Load clip specified in absolute filename argument."},
	{"INSERT", dv1394d_insert, 1, ATYPE_STRING, "Insert a clip at the given clip index."},
	{"REMOVE", dv1394d_remove, 1, ATYPE_NONE, "Remove a clip at the given clip index."},
	{"CLEAN", dv1394d_clean, 1, ATYPE_NONE, "Clean a unit by removing all but the currently playing clip."},
	{"MOVE", dv1394d_move, 1, ATYPE_INT, "Move a clip to another clip index."},
	{"APND", dv1394d_append, 1, ATYPE_STRING, "Append a clip specified in absolute filename argument."},
	{"PLAY", dv1394d_play, 1, ATYPE_NONE, "Play a loaded clip at speed -2000 to 2000 where 1000 = normal forward speed."},
	{"STOP", dv1394d_stop, 1, ATYPE_NONE, "Stop a loaded and playing clip."},
	{"PAUSE", dv1394d_pause, 1, ATYPE_NONE, "Pause a playing clip."},
	{"REW", dv1394d_rewind, 1, ATYPE_NONE, "Rewind a unit. If stopped, seek to beginning of clip. If playing, play fast backwards."},
	{"FF", dv1394d_ff, 1, ATYPE_NONE, "Fast forward a unit. If stopped, seek to beginning of clip. If playing, play fast forwards."},
	{"STEP", dv1394d_step, 1, ATYPE_INT, "Step argument number of frames forward or backward."},
	{"GOTO", dv1394d_goto, 1, ATYPE_INT, "Jump to frame number supplied as argument."},
	{"SIN", dv1394d_set_in_point, 1, ATYPE_INT, "Set the IN point of the loaded clip to frame number argument. -1 = reset in point to 0"},
	{"SOUT", dv1394d_set_out_point, 1, ATYPE_INT, "Set the OUT point of the loaded clip to frame number argument. -1 = reset out point to maximum."},
	{"USTA", dv1394d_get_unit_status, 1, ATYPE_NONE, "Report information about the unit."},
	{"USET", dv1394d_set_unit_property, 1, ATYPE_STRING, "Set a unit configuration property."},
	{"UGET", dv1394d_get_unit_property, 1, ATYPE_STRING, "Get a unit configuration property."},
	{"XFER", dv1394d_transfer, 1, ATYPE_STRING, "Transfer the unit's clip to another unit specified as argument."},
	{"SHUTDOWN", dv1394d_shutdown, 0, ATYPE_NONE, "Shutdown the server."},
	{NULL, NULL, 0, ATYPE_NONE, NULL}
};

/** Usage message 
*/

static char helpstr [] = 
	"dv1394d -- A DV over IEEE 1394 TCP Server\n" 
	"	Copyright (C) 2002-2003 Ushodaya Enterprises Limited\n"
	"	Authors:\n"
	"		Dan Dennedy <dan@dennedy.org>\n"
	"		Charles Yates <charles.yates@pandora.be>\n"
	"Available commands:\n";

/** Lookup the response message for a status code.
*/

inline char *get_response_msg( int code )
{
	int i = 0;
	for ( i = 0; responses[ i ].message != NULL && code != responses[ i ].code; i ++ ) ;
	return responses[ i ].message;
}

/** Tell the user the dv1394d command set
*/

response_codes print_help( command_argument cmd_arg )
{
	int i = 0;
	
	dv_response_printf( cmd_arg->response, 10240, "%s", helpstr );
	
	for ( i = 0; vocabulary[ i ].command != NULL; i ++ )
		dv_response_printf( cmd_arg->response, 1024,
							"%-10.10s%s\n", 
							vocabulary[ i ].command, 
							vocabulary[ i ].help );

	dv_response_printf( cmd_arg->response, 2, "\n" );

	return RESPONSE_SUCCESS_N;
}

/** Execute a batch file.
*/

response_codes dv1394d_run( command_argument cmd_arg )
{
	dv_response temp = dv_parser_run( cmd_arg->parser, (char *)cmd_arg->argument );

	if ( temp != NULL )
	{
		int index = 0;

		dv_response_set_error( cmd_arg->response, 
							   dv_response_get_error_code( temp ),
							   dv_response_get_error_string( temp ) );

		for ( index = 1; index < dv_response_count( temp ); index ++ )
			dv_response_printf( cmd_arg->response, 10240, "%s\n", dv_response_get_line( temp, index ) );

		dv_response_close( temp );
	}

	return dv_response_get_error_code( cmd_arg->response );
}

response_codes dv1394d_shutdown( command_argument cmd_arg )
{
	exit( 0 );
	return RESPONSE_SUCCESS;
}

/** Processes 'thread' id
*/

static pthread_t self;

/* Signal handler to deal with various shutdown signals. Basically this
   should clean up and power down the motor. Note that the death of any
   child thread will kill all thrads. */

void signal_handler( int sig )
{
	if ( pthread_equal( self, pthread_self( ) ) )
	{

#ifdef _GNU_SOURCE
		dv1394d_log( LOG_DEBUG, "Received %s - shutting down.", strsignal(sig) );
#else
		dv1394d_log( LOG_DEBUG, "Received signal %i - shutting down.", sig );
#endif

		exit(EXIT_SUCCESS);
	}
}

/** Local 'connect' function.
*/

static dv_response dv_local_connect( dv_local local )
{
	dv_response response = dv_response_init( );

	self = pthread_self( );

	dv_response_set_error( response, 100, "VTR Ready" );

	signal( SIGHUP, signal_handler );
	signal( SIGINT, signal_handler );
	signal( SIGTERM, signal_handler );
	signal( SIGSTOP, signal_handler );
	signal( SIGCHLD, SIG_IGN );
	
	raw1394_reconcile_bus();
	/* Start the raw1394 service threads for handling bus resets */
	raw1394_start_service_threads();

	return response;
}

/** Set the error and determine the message associated to this command.
*/

void dv_command_set_error( command_argument cmd, response_codes code )
{
	dv_response_set_error( cmd->response, code, get_response_msg( code ) );
}

/** Parse the unit argument.
*/

int dv_command_parse_unit( command_argument cmd, int argument )
{
	int unit = -1;
	char *string = dv_tokeniser_get_string( cmd->tokeniser, argument );
	if ( string != NULL && ( string[ 0 ] == 'U' || string[ 0 ] == 'u' ) && strlen( string ) > 1 )
		unit = atoi( string + 1 );
	return unit;
}

/** Parse a normal argument.
*/

void *dv_command_parse_argument( command_argument cmd, int argument, arguments_types type )
{
	void *ret = NULL;
	char *value = dv_tokeniser_get_string( cmd->tokeniser, argument );

	if ( value != NULL )
	{
		switch( type )
		{
			case ATYPE_NONE:
				break;

			case ATYPE_FLOAT:
				ret = malloc( sizeof( float ) );
				if ( ret != NULL )
					*( float * )ret = atof( value );
				break;

			case ATYPE_STRING:
				ret = strdup( value );
				break;
					
			case ATYPE_INT:
				ret = malloc( sizeof( int ) );
				if ( ret != NULL )
					*( int * )ret = atoi( value );
				break;
		}
	}

	return ret;
}

/** Get the error code - note that we simply the success return.
*/

response_codes dv_command_get_error( command_argument cmd )
{
	response_codes ret = dv_response_get_error_code( cmd->response );
	if ( ret == RESPONSE_SUCCESS_N || ret == RESPONSE_SUCCESS_1 )
		ret = RESPONSE_SUCCESS;
	return ret;
}

/** Execute the command.
*/

static dv_response dv_local_execute( dv_local local, char *command )
{
	command_argument_t cmd;
	cmd.parser = local->parser;
	cmd.response = dv_response_init( );
	cmd.tokeniser = dv_tokeniser_init( );
	cmd.command = command;
	cmd.unit = -1;
	cmd.argument = NULL;
	cmd.root_dir = local->root_dir;

	/* Set the default error */
	dv_command_set_error( &cmd, RESPONSE_UNKNOWN_COMMAND );

	/* Parse the command */
	if ( dv_tokeniser_parse_new( cmd.tokeniser, command, " " ) > 0 )
	{
		int index = 0;
		char *value = dv_tokeniser_get_string( cmd.tokeniser, 0 );
		int found = 0;

		/* Strip quotes from all tokens */
		for ( index = 0; index < dv_tokeniser_count( cmd.tokeniser ); index ++ )
			dv_util_strip( dv_tokeniser_get_string( cmd.tokeniser, index ), '\"' );

		/* Search the vocabulary array for value */
		for ( index = 1; !found && vocabulary[ index ].command != NULL; index ++ )
			if ( ( found = !strcasecmp( vocabulary[ index ].command, value ) ) )
				break;

		/* If we found something, the handle the args and call the handler. */
		if ( found )
		{
			int position = 1;

			dv_command_set_error( &cmd, RESPONSE_SUCCESS );

			if ( vocabulary[ index ].is_unit )
			{
				cmd.unit = dv_command_parse_unit( &cmd, position );
				if ( cmd.unit == -1 )
					dv_command_set_error( &cmd, RESPONSE_MISSING_ARG );
				position ++;
			}

			if ( dv_command_get_error( &cmd ) == RESPONSE_SUCCESS )
			{
				cmd.argument = dv_command_parse_argument( &cmd, position, vocabulary[ index ].type );
				if ( cmd.argument == NULL && vocabulary[ index ].type != ATYPE_NONE )
					dv_command_set_error( &cmd, RESPONSE_MISSING_ARG );
				position ++;
			}

			if ( dv_command_get_error( &cmd ) == RESPONSE_SUCCESS )
			{
				response_codes error = vocabulary[ index ].operation( &cmd );
				dv_command_set_error( &cmd, error );
			}

			free( cmd.argument );
		}
	}

	dv_tokeniser_close( cmd.tokeniser );

	return cmd.response;
}

/** Close the parser.
*/

static void dv_local_close( dv_local local )
{
	raw1394_stop_service_threads();
	dv1394d_delete_all_units();
	pthread_kill_other_threads_np();
	dv1394d_log( LOG_DEBUG, "Clean shutdown." );
	free( local );
	dv_clip_factory_close( );
	dv_frame_pool_close( );
}
