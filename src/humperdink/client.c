/*
 * client.c -- dv1394d client demo
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

/* Application header files */
#include "client.h"
#include "io.h"

/** Clip navigation enumeration.
*/

typedef enum
{
	absolute,
	relative
}
dv_demo_whence;

/** Function prototype for menu handling. 
*/

typedef valerie_error_code (*demo_function)( dv_demo );

/** The menu structure. 
*/

typedef struct
{
	char *description;
	struct menu_item
	{
		char *option;
		demo_function function;
	}
	array[ 50 ];
}
*dv_demo_menu, dv_demo_menu_t;

/** Forward reference to menu runner.
*/

extern valerie_error_code dv_demo_run_menu( dv_demo, dv_demo_menu );

/** Foward references. 
*/

extern valerie_error_code dv_demo_list_nodes( dv_demo );
extern valerie_error_code dv_demo_add_unit( dv_demo );
extern valerie_error_code dv_demo_select_unit( dv_demo );
extern valerie_error_code dv_demo_execute( dv_demo );
extern valerie_error_code dv_demo_load( dv_demo );
extern valerie_error_code dv_demo_transport( dv_demo );
static void *dv_demo_status_thread( void * );

/** Connected menu definition. 
*/

dv_demo_menu_t connected_menu =
{
	"Connected Menu",
	{
		{ "Add Unit", dv_demo_add_unit },
		{ "Select Unit", dv_demo_select_unit },
		{ "Command Shell", dv_demo_execute },
		{ NULL, NULL }
	}
};

/** Initialise the demo structure.
*/

dv_demo dv_demo_init( valerie_parser parser )
{
	dv_demo this = malloc( sizeof( dv_demo_t ) );
	if ( this != NULL )
	{
		int index = 0;
		memset( this, 0, sizeof( dv_demo_t ) );
		strcpy( this->last_directory, "/" );
		for ( index = 0; index < 4; index ++ )
		{
			this->queues[ index ].unit = index;
			this->queues[ index ].position = -1;
		}
		this->parser = parser;
	}
	return this;
}

/** Display a status record.
*/

void dv_demo_show_status( dv_demo demo, valerie_status status )
{
	if ( status->unit == demo->selected_unit && demo->showing )
	{
		char temp[ 1024 ] = "";

		sprintf( temp, "U%d ", demo->selected_unit );

		switch( status->status )
		{
			case unit_offline:
				strcat( temp, "offline   " );
				break;
			case unit_undefined:
				strcat( temp, "undefined " );
				break;
			case unit_not_loaded:
				strcat( temp, "unloaded  " );
				break;
			case unit_stopped:
				strcat( temp, "stopped   " );
				break;
			case unit_playing:
				strcat( temp, "playing   " );
				break;
			case unit_paused:
				strcat( temp, "paused    " );
				break;
			case unit_disconnected:
				strcat( temp, "disconnect" );
				break;
			default:
				strcat( temp, "unknown   " );
				break;
		}

		sprintf( temp + strlen( temp ), " %9d %9d %9d ", status->in, status->position, status->out );
		strcat( temp, status->clip );

		printf( "%-80.80s\r", temp );
		fflush( stdout );
	}
}

/** Determine action to carry out as dictated by the client unit queue.
*/

void dv_demo_queue_action( dv_demo demo, valerie_status status )
{
	dv_demo_queue queue = &demo->queues[ status->unit ];

	/* SPECIAL CASE STATUS NOTIFICATIONS TO IGNORE */

	/* When we've issued a LOAD on the previous notification, then ignore this one. */
	if ( queue->ignore )
	{
		queue->ignore --;
		return;
	}

	if ( queue->mode && status->status != unit_offline && queue->head != queue->tail )
	{
		if ( ( status->position >= status->out && status->speed > 0 ) || status->status == unit_not_loaded )
		{
			queue->position = ( queue->position + 1 ) % 50;
			if ( queue->position == queue->tail )
				queue->position = queue->head;
			valerie_unit_load( demo->dv_status, status->unit, queue->list[ queue->position ] );
			if ( status->status == unit_not_loaded )
				valerie_unit_play( demo->dv, queue->unit );
			queue->ignore = 1;
		}
		else if ( ( status->position <= status->in && status->speed < 0 ) || status->status == unit_not_loaded )
		{
			if ( queue->position == -1 )
				queue->position = queue->head;
			valerie_unit_load( demo->dv_status, status->unit, queue->list[ queue->position ] );
			if ( status->status == unit_not_loaded )
				valerie_unit_play( demo->dv, queue->unit );
			queue->position = ( queue->position - 1 ) % 50;
			queue->ignore = 1;
		}
	}
}

/** Status thread.
*/

static void *dv_demo_status_thread( void *arg )
{
	dv_demo demo = arg;
	valerie_status_t status;
	valerie_notifier notifier = valerie_get_notifier( demo->dv_status );

	while ( !demo->terminated )
	{
		if ( valerie_notifier_wait( notifier, &status ) != -1 )
		{
			dv_demo_queue_action( demo, &status );
			dv_demo_show_status( demo, &status );
			if ( status.status == unit_disconnected )
				demo->disconnected = 1;
		}
	}

	return NULL;
}

/** Turn on/off status display.
*/

void dv_demo_change_status( dv_demo demo, int flag )
{
	if ( demo->disconnected && flag )
	{
		valerie_error_code error = valerie_connect( demo->dv );
		if ( error == valerie_ok )
			demo->disconnected = 0;
		else
			beep();
	}

	if ( flag )
	{
		valerie_status_t status;
		valerie_notifier notifier = valerie_get_notifier( demo->dv );
		valerie_notifier_get( notifier, &status, demo->selected_unit );
		demo->showing = 1;
		dv_demo_show_status( demo, &status );
	}
	else
	{
		demo->showing = 0;
		printf( "%-80.80s\r", " " );
		fflush( stdout );
	}
}

/** Add a unit.
*/

valerie_error_code dv_demo_add_unit( dv_demo demo )
{
	valerie_error_code error = valerie_ok;
	valerie_nodes nodes = valerie_nodes_init( demo->dv );
	valerie_units units = valerie_units_init( demo->dv );

	if ( valerie_nodes_count( nodes ) != -1 && valerie_units_count( units ) != -1 )
	{
		char pressed;
		valerie_node_entry_t node;
		valerie_unit_entry_t unit;
		int node_index = 0;
		int unit_index = 0;

		printf( "Select a Node\n\n" );

		for ( node_index = 0; node_index < valerie_nodes_count( nodes ); node_index ++ )
		{
			valerie_nodes_get( nodes, node_index, &node );
			printf( "%d: %s - %s ", node_index + 1, node.guid, node.name );
			for ( unit_index = 0; unit_index < valerie_units_count( units ); unit_index ++ )
			{
				valerie_units_get( units, unit_index, &unit );
				if ( !strcmp( unit.guid, node.guid ) )
					printf( "[U%d] ", unit.unit );
			}
			printf( "\n" );
		}

		printf( "0. Exit\n\n" );

		printf( "Node: " );

		while ( ( pressed = get_keypress( ) ) != '0' )
		{
			node_index = pressed - '1';
			if ( node_index >= 0 && node_index < valerie_nodes_count( nodes ) )
			{
				int unit;
				printf( "%c\n\n", pressed );
				valerie_nodes_get( nodes, node_index, &node );
				if ( valerie_unit_add( demo->dv, node.guid, &unit ) == valerie_ok )
				{
					printf( "Unit added as U%d\n", unit );
					demo->selected_unit = unit;
				}
				else
				{
					int index = 0;
					valerie_response response = valerie_get_last_response( demo->dv );
					printf( "Failed to add unit:\n\n" );
					for( index = 1; index < valerie_response_count( response ) - 1; index ++ )
						printf( "%s\n", valerie_response_get_line( response, index ) );
				}
				printf( "\n" );
				wait_for_any_key( NULL );
				break;
			}
			else
			{
				beep( );
			}
		}
	}
	else
	{
		printf( "Invalid response from the server.\n\n" );
		wait_for_any_key( NULL );
	}

	valerie_nodes_close( nodes );
	valerie_units_close( units );

	return error;
}

/** Select a unit.
*/

valerie_error_code dv_demo_select_unit( dv_demo demo )
{
	int terminated = 0;
	int refresh = 1;

	while ( !terminated )
	{
		valerie_units units = valerie_units_init( demo->dv );

		if ( valerie_units_count( units ) > 0 )
		{
			valerie_unit_entry_t unit;
			int index = 0;
			char key = '\0';

			if ( refresh )
			{
				printf( "Select a Unit\n\n" );

				for ( index = 0; index < valerie_units_count( units ); index ++ )
				{
					valerie_units_get( units, index, &unit );
					printf( "%d: U%d - %s [%s]\n", index + 1, 
												   unit.unit, 
												   unit.guid, 
												   unit.online ? "online" : "offline" );
				}
				printf( "0: Exit\n\n" );

				printf( "Unit [%d]: ", demo->selected_unit + 1 );
				refresh = 0;
			}

			key = get_keypress( );

			if ( key == '\r' )
				key = demo->selected_unit + '1';

			if ( key != '0' )
			{
				if ( key >= '1' && key < '1' + valerie_units_count( units ) )
				{
					demo->selected_unit = key - '1';
					printf( "%c\n\n", key );
					dv_demo_load( demo );
					refresh = 1;
				}
				else
				{
					beep( );
				}					
			}
			else
			{
				printf( "0\n\n" );
				terminated = 1;
			}
		}
		else if ( valerie_units_count( units ) == 0 )
		{
			printf( "No units added - add a unit first\n\n" );
			dv_demo_add_unit( demo );
		}
		else
		{
			printf( "Unable to obtain Unit List.\n" );
			terminated = 1;
		}

		valerie_units_close( units );
	}

	return valerie_ok;
}

/** Execute an arbitrary command.
*/

valerie_error_code dv_demo_execute( dv_demo demo )
{
	valerie_error_code error = valerie_ok;
	char command[ 10240 ];
	int terminated = 0;

	printf( "Miracle Shell\n" );
	printf( "Enter an empty command to exit.\n\n" );

	while ( !terminated )
	{
		terminated = 1;
		printf( "Command> " );

		if ( chomp( get_string( command, 10240, "" ) ) != NULL )
		{
			if ( strcmp( command, "" ) )
			{
				int index = 0;
				valerie_response response = NULL;
				error = valerie_execute( demo->dv, 10240, command );
				printf( "\n" );
				response = valerie_get_last_response( demo->dv );
				for ( index = 0; index < valerie_response_count( response ); index ++ )
				{
					char *line = valerie_response_get_line( response, index );
					printf( "%4d: %s\n", index, line );
				}
				printf( "\n" );
				terminated = 0;
			}
		}
	}

	printf( "\n" );

	return error;
}

/** Add a file to the queue.
*/

valerie_error_code dv_demo_queue_add( dv_demo demo, dv_demo_queue queue, char *file )
{
	valerie_status_t status;
	valerie_notifier notifier = valerie_get_notifier( demo->dv );

	if ( ( queue->tail + 1 ) % 50 == queue->head )
		queue->head = ( queue->head + 1 ) % 50;
	strcpy( queue->list[ queue->tail ], file );
	queue->tail = ( queue->tail + 1 ) % 50;

	valerie_notifier_get( notifier, &status, queue->unit );
	valerie_notifier_put( notifier, &status );

	return valerie_ok;
}

/** Basic queue maintenance and status reports.
*/

valerie_error_code dv_demo_queue_maintenance( dv_demo demo, dv_demo_queue queue )
{
	printf( "Queue Maintenance for Unit %d\n\n", queue->unit );

	if ( !queue->mode )
	{
		char ch;
		printf( "Activate queueing? [Y] " );
		ch = get_keypress( );
		if ( ch == 'y' || ch == 'Y' || ch == '\r' )
			queue->mode = 1;
		printf( "\n\n" );
	}

	if ( queue->mode )
	{
		int terminated = 0;
		int last_position = -2;

		term_init( );

		while ( !terminated )
		{
			int first = ( queue->position + 1 ) % 50;
			int index = first;

			if ( first == queue->tail )
				index = first = queue->head;

			if ( queue->head == queue->tail )
			{
				if ( last_position == -2 )
				{
					printf( "Queue is empty\n" );
					printf( "\n" );
					printf( "0 = exit, t = turn off queueing\n\n" );
					last_position = -1;
				}
			}
			else if ( last_position != queue->position )
			{
				printf( "Order of play\n\n" );

				do 
				{
					printf( "%c%02d: %s\n", index == first ? '*' : ' ', index, queue->list[ index ] + 1 );
					index = ( index + 1 ) % 50;
					if ( index == queue->tail )
						index = queue->head;
				}
				while( index != first );
	
				printf( "\n" );
				printf( "0 = exit, t = turn off queueing, c = clear queue\n\n" );
				last_position = queue->position;
			}

			dv_demo_change_status( demo, 1 );
			
			switch( term_read( ) )
			{
				case -1:
					break;
				case '0':
					terminated = 1;
					break;
				case 't':
					terminated = 1;
					queue->mode = 0;
					break;
				case 'c':
					queue->head = queue->tail = 0;
					queue->position = -1;
					last_position = -2;
					break;
			}

			dv_demo_change_status( demo, 0 );
		}

		term_exit( );
	}

	return valerie_ok;
}

/** Load a file to the selected unit. Horrible function - sorry :-/. Not a good
	demo....
*/

valerie_error_code dv_demo_load( dv_demo demo )
{
	valerie_error_code error = valerie_ok;
	int terminated = 0;
	int refresh = 1;
	int start = 0;

	strcpy( demo->current_directory, demo->last_directory );

	term_init( );

	while ( !terminated )
	{
		valerie_dir dir = valerie_dir_init( demo->dv, demo->current_directory );

		if ( valerie_dir_count( dir ) == -1 )
		{
			printf( "Invalid directory - retrying %s\n", demo->last_directory );
			valerie_dir_close( dir );
			dir = valerie_dir_init( demo->dv, demo->last_directory );
			if ( valerie_dir_count( dir ) == -1 )
			{
				printf( "Invalid directory - going back to /\n" );
				valerie_dir_close( dir );
				dir = valerie_dir_init( demo->dv, "/" );
				strcpy( demo->current_directory, "/" );
			}
			else
			{
				strcpy( demo->current_directory, demo->last_directory );
			}
		}

		terminated = valerie_dir_count( dir ) == -1;

		if ( !terminated )
		{
			int index = 0;
			int selected = 0;
			int max = 9;
			int end = 0;

			end = valerie_dir_count( dir );

			strcpy( demo->last_directory, demo->current_directory );

			while ( !selected && !terminated )
			{
				valerie_dir_entry_t entry;
				int pressed;

				if ( refresh )
				{
					char *action = "Load & Play";
					if ( demo->queues[ demo->selected_unit ].mode )
						action = "Queue";
					printf( "%s from %s\n\n", action, demo->current_directory );
					if ( strcmp( demo->current_directory, "/" ) )
						printf( "-: Parent directory\n" );
					for ( index = start; index < end && ( index - start ) < max; index ++ )
					{
						valerie_dir_get( dir, index, &entry );
						printf( "%d: %s\n", index - start + 1, entry.name );
					}
					while ( ( index ++ % 9 ) != 0 )
						printf( "\n" );
					printf( "\n" );
					if ( start + max < end )
						printf( "space = more files" );
					else if ( end > max )
						printf( "space = return to start of list" );
					if ( start > 0 )
						printf( ", b = previous files" );
					printf( "\n" );
					printf( "0 = abort, t = transport, x = execute command, q = queue maintenance\n\n" );
					refresh = 0;
				}

				dv_demo_change_status( demo, 1 );

				pressed = term_read( );
				switch( pressed )
				{
					case -1:
						break;
					case '0':
						terminated = 1;
						break;
					case 'b':
						refresh = start - max >= 0;
						if ( refresh )
							start = start - max;
						break;
					case ' ':
						refresh = start + max < end;
						if ( refresh )
						{
							start = start + max;
						}
						else if ( end > max )
						{
							start = 0;
							refresh = 1;
						}
						break;
					case '-':
						if ( strcmp( demo->current_directory, "/" ) )
						{
							selected = 1;
							( *strrchr( demo->current_directory, '/' ) ) = '\0';
							( *( strrchr( demo->current_directory, '/' ) + 1 ) ) = '\0';
						}
						break;
					case 't':
						dv_demo_change_status( demo, 0 );
						term_exit( );
						dv_demo_transport( demo );
						term_init( );
						selected = 1;
						break;
					case 'x':
						dv_demo_change_status( demo, 0 );
						term_exit( );
						dv_demo_execute( demo );
						term_init( );
						selected = 1;
						break;
					case 'q':
						dv_demo_change_status( demo, 0 );
						term_exit( );
						dv_demo_queue_maintenance( demo, &demo->queues[ demo->selected_unit ] );
						term_init( );
						selected = 1;
						break;
					default:
						if ( pressed >= '1' && pressed <= '9' )
						{
							if ( ( start + pressed - '1' ) < end )
							{
								valerie_dir_get( dir, start + pressed - '1', &entry );
								selected = 1;
								strcat( demo->current_directory, entry.name );
							}
						}
						break;
				}

				dv_demo_change_status( demo, 0 );
			}

			valerie_dir_close( dir );
		}

		if ( !terminated && demo->current_directory[ strlen( demo->current_directory ) - 1 ] != '/' )
		{
			if ( demo->queues[ demo->selected_unit ].mode == 0 )
			{
				error = valerie_unit_load( demo->dv, demo->selected_unit, demo->current_directory );
				valerie_unit_play( demo->dv, demo->selected_unit );
			}
			else
			{
				dv_demo_queue_add( demo, &demo->queues[ demo->selected_unit ], demo->current_directory );
				printf( "File %s added to queue.\n", demo->current_directory );
			}
			strcpy( demo->current_directory, demo->last_directory );
			refresh = 0;
		}
		else
		{
			refresh = 1;
			start = 0;
		}
	}

	term_exit( );

	return error;
}

/** Set the in point of the clip on the select unit.
*/

valerie_error_code dv_demo_set_in( dv_demo demo )
{
	int position = 0;
	valerie_status_t status;
	valerie_notifier notifier = valerie_parser_get_notifier( demo->parser );
	valerie_notifier_get( notifier, &status, demo->selected_unit );
	position = status.position;
	return valerie_unit_set_in( demo->dv, demo->selected_unit, position );
}

/** Set the out point of the clip on the selected unit.
*/

valerie_error_code dv_demo_set_out( dv_demo demo )
{
	int position = 0;
	valerie_status_t status;
	valerie_notifier notifier = valerie_parser_get_notifier( demo->parser );
	valerie_notifier_get( notifier, &status, demo->selected_unit );
	position = status.position;
	return valerie_unit_set_out( demo->dv, demo->selected_unit, position );
}

/** Clear the in and out points on the selected unit.
*/

valerie_error_code dv_demo_clear_in_out( dv_demo demo )
{
	return valerie_unit_clear_in_out( demo->dv, demo->selected_unit );
}

/** Goto a user specified frame on the selected unit.
*/

valerie_error_code dv_demo_goto( dv_demo demo )
{
	int frame = 0;
	printf( "Frame: " );
	if ( get_int( &frame, 0 ) )
		return valerie_unit_goto( demo->dv, demo->selected_unit, frame );
	return valerie_ok;
}

/** Manipulate playback on the selected unit.
*/

valerie_error_code dv_demo_transport( dv_demo demo )
{
	valerie_error_code error = valerie_ok;
	int refresh = 1;
	int terminated = 0;
	valerie_status_t status;
	valerie_notifier notifier = valerie_get_notifier( demo->dv );

	while ( !terminated )
	{
		if ( refresh )
		{
			printf( "  +----+ +------+ +----+ +------+ +---+ +-----+ +------+ +-----+ +---+  \n" );
			printf( "  |1=-5| |2=-2.5| |3=-1| |4=-0.5| |5=1| |6=0.5| |7=1.25| |8=2.5| |9=5|  \n" );
			printf( "  +----+ +------+ +----+ +------+ +---+ +-----+ +------+ +-----+ +---+  \n" );
			printf( "\n" );
			printf( "+----------------------------------------------------------------------+\n" );
			printf( "|              0 = quit, x = eXecute, 'space' = pause                  |\n" );
			printf( "|              g = goto a frame, q = queue maintenance                 |\n" );
			printf( "|     h = step -1, j = end of clip, k = start of clip, l = step 1      |\n" );
			printf( "|        eof handling: p = pause, r = repeat, t = terminate            |\n" );
			printf( "|       i = set in point, o = set out point, c = clear in/out          |\n" );
			printf( "|       u = use point settings, d = don't use point settings           |\n" );
			printf( "+----------------------------------------------------------------------+\n" );
			printf( "\n" );
			term_init( );
			refresh = 0;
		}

		dv_demo_change_status( demo, 1 );

		switch( term_read( ) )
		{
			case '0':
				terminated = 1;
				break;
			case -1:
				break;
			case ' ':
				error = valerie_unit_pause( demo->dv, demo->selected_unit );
				break;
			case '1':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, -5000 );
				break;
			case '2':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, -2500 );
				break;
			case '3':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, -1000 );
				break;
			case '4':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, -500 );
				break;
			case '5':
				error = valerie_unit_play( demo->dv, demo->selected_unit );
				break;
			case '6':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, 500 );
				break;
			case '7':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, 1250 );
				break;
			case '8':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, 2500 );
				break;
			case '9':
				error = valerie_unit_play_at_speed( demo->dv, demo->selected_unit, 5000 );
				break;
			case 's':
				error = valerie_unit_goto( demo->dv, demo->selected_unit, 0 );
				break;
			case 'h':
				error = valerie_unit_step( demo->dv, demo->selected_unit, -1 );
				break;
			case 'j':
				valerie_notifier_get( notifier, &status, demo->selected_unit );
				error = valerie_unit_goto( demo->dv, demo->selected_unit, status.tail_out );
				break;
			case 'k':
				valerie_notifier_get( notifier, &status, demo->selected_unit );
				error = valerie_unit_goto( demo->dv, demo->selected_unit, status.in );
				break;
			case 'l':
				error = valerie_unit_step( demo->dv, demo->selected_unit, 1 );
				break;
			case 'p':
				error = valerie_unit_set( demo->dv, demo->selected_unit, "eof", "pause" );
				break;
			case 'r':
				error = valerie_unit_set( demo->dv, demo->selected_unit, "eof", "loop" );
				break;
			case 't':
				error = valerie_unit_set( demo->dv, demo->selected_unit, "eof", "stop" );
				break;
			case 'i':
				error = dv_demo_set_in( demo );
				break;
			case 'o':
				error = dv_demo_set_out( demo );
				break;
			case 'g':
				dv_demo_change_status( demo, 0 );
				term_exit( );
				error = dv_demo_goto( demo );
				refresh = 1;
				break;
			case 'c':
				error = dv_demo_clear_in_out( demo );
				break;
			case 'u':
				error = valerie_unit_set( demo->dv, demo->selected_unit, "points", "use" );
				break;
			case 'd':
				error = valerie_unit_set( demo->dv, demo->selected_unit, "points", "ignore" );
				break;
			case 'x':
				dv_demo_change_status( demo, 0 );
				term_exit( );
				dv_demo_execute( demo );
				refresh = 1;
				break;
			case 'q':
				dv_demo_change_status( demo, 0 );
				term_exit( );
				dv_demo_queue_maintenance( demo, &demo->queues[ demo->selected_unit ] );
				refresh = 1;
				break;
		}

		dv_demo_change_status( demo, 0 );
	}

	term_exit( );

	return error;
}

/** Recursive menu execution.
*/

valerie_error_code dv_demo_run_menu( dv_demo demo, dv_demo_menu menu )
{
	char *items = "123456789abcdefghijklmnopqrstuvwxyz";
	int refresh_menu = 1;
	int terminated = 0;
	int item_count = 0;
	int item_selected = 0;
	int index = 0;
	char key;

	while( !terminated )
	{

		if ( refresh_menu )
		{
			printf( "%s\n\n", menu->description );
			for ( index = 0; menu->array[ index ].option != NULL; index ++ )
				printf( "%c: %s\n", items[ index ], menu->array[ index ].option );
			printf( "0: Exit\n\n" );
			printf( "Select Option: " );
			refresh_menu = 0;
			item_count = index;
		}

		key = get_keypress( );

		if ( demo->disconnected && key != '0' )
		{
			valerie_error_code error = valerie_connect( demo->dv );
			if ( error == valerie_ok )
				demo->disconnected = 0;
			else
				beep();
		}

		if ( !demo->disconnected || key == '0' )
		{
			item_selected = strchr( items, key ) - items;

			if ( key == '0' )
			{
				printf( "%c\n\n", key );
				terminated = 1;
			}
			else if ( item_selected >= 0 && item_selected < item_count )
			{
				printf( "%c\n\n", key );
				menu->array[ item_selected ].function( demo );
				refresh_menu = 1;
			}
			else
			{
				beep( );
			}
		}
	}

	return valerie_ok;
}

/** Entry point for main menu.
*/

void dv_demo_run( dv_demo this )
{
	this->dv = valerie_init( this->parser );
	this->dv_status = valerie_init( this->parser );
	if ( valerie_connect( this->dv ) == valerie_ok )
	{
		pthread_create( &this->thread, NULL, dv_demo_status_thread, this );
		dv_demo_run_menu( this, &connected_menu );
		this->terminated = 1;
		pthread_join( this->thread, NULL );
		this->terminated = 0;
	}
	else
	{
		printf( "Unable to connect." );
		wait_for_any_key( "" );
	}

	valerie_close( this->dv_status );
	valerie_close( this->dv );

	printf( "Demo Exit.\n" );
}

/** Close the demo structure.
*/

void dv_demo_close( dv_demo demo )
{
	free( demo );
}
