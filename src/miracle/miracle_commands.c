/*
 * global_commands.c
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>

#include "miracle_unit.h"
#include "miracle_commands.h"
#include "miracle_log.h"

static miracle_unit g_units[MAX_UNITS];


/** Return the miracle_unit given a numeric index.
*/

miracle_unit miracle_get_unit( int n )
{
	if (n < MAX_UNITS)
		return g_units[n];
	else
		return NULL;
}

/** Destroy the miracle_unit given its numeric index.
*/

void miracle_delete_unit( int n )
{
	if (n < MAX_UNITS)
	{
		miracle_unit unit = miracle_get_unit(n);
		if (unit != NULL)
		{
			miracle_unit_close( unit );
			g_units[ n ] = NULL;
			miracle_log( LOG_NOTICE, "Deleted unit U%d.", n ); 
		}
	}
}

/** Destroy all allocated units on the server.
*/

void miracle_delete_all_units( void )
{
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		if ( miracle_get_unit(i) != NULL )
		{
			miracle_unit_close( miracle_get_unit(i) );
			miracle_log( LOG_NOTICE, "Deleted unit U%d.", i ); 
		}
	}
}

/** Add a DV virtual vtr to the server.
*/
response_codes miracle_add_unit( command_argument cmd_arg )
{
	int i = 0;
	for ( i = 0; i < MAX_UNITS; i ++ )
		if ( g_units[ i ] == NULL )
			break;

	if ( i < MAX_UNITS )
	{
		char *arg = cmd_arg->argument;
		g_units[ i ] = miracle_unit_init( i, arg );
		if ( g_units[ i ] != NULL )
			miracle_unit_set_notifier( g_units[ i ], valerie_parser_get_notifier( cmd_arg->parser ), cmd_arg->root_dir );
		return g_units[ i ] != NULL ? RESPONSE_SUCCESS : RESPONSE_ERROR;
	}

	return RESPONSE_ERROR;
}


/** List all AV/C nodes on the bus.
*/
response_codes miracle_list_nodes( command_argument cmd_arg )
{
	response_codes error = RESPONSE_SUCCESS_N;
	return error;
}


/** List units already added to server.
*/
response_codes miracle_list_units( command_argument cmd_arg )
{
	response_codes error = RESPONSE_SUCCESS_N;
	int i = 0;

	for ( i = 0; i < MAX_UNITS; i ++ )
	{
		miracle_unit unit = miracle_get_unit( i );
		if ( unit != NULL )
		{
			mlt_properties properties = unit->properties;
			char *constructor = mlt_properties_get( properties, "constructor" );
			int node = mlt_properties_get_int( properties, "node" );
			int online = !mlt_properties_get_int( properties, "offline" );
			valerie_response_printf( cmd_arg->response, 1024, "U%d %02d %s %d\n", i, node, constructor, online );
		}
	}
	valerie_response_printf( cmd_arg->response, 1024, "\n" );

	return error;
}

static int filter_files( const struct dirent *de )
{
	return de->d_name[ 0 ] != '.';
}

/** List clips in a directory.
*/
response_codes miracle_list_clips( command_argument cmd_arg )
{
	response_codes error = RESPONSE_BAD_FILE;
	const char *dir_name = (const char*) cmd_arg->argument;
	DIR *dir;
	char fullname[1024];
	struct dirent **de = NULL;
	int i, n;
	
	snprintf( fullname, 1023, "%s%s", cmd_arg->root_dir, dir_name );
	dir = opendir( fullname );
	if (dir != NULL)
	{
		struct stat info;
		error = RESPONSE_SUCCESS_N;
		n = scandir( fullname, &de, filter_files, alphasort );
		for (i = 0; i < n; i++ )
		{
			snprintf( fullname, 1023, "%s%s/%s", cmd_arg->root_dir, dir_name, de[i]->d_name );
			if ( stat( fullname, &info ) == 0 && S_ISDIR( info.st_mode ) )
				valerie_response_printf( cmd_arg->response, 1024, "\"%s/\"\n", de[i]->d_name );
		}
		for (i = 0; i < n; i++ )
		{
			snprintf( fullname, 1023, "%s%s/%s", cmd_arg->root_dir, dir_name, de[i]->d_name );
			if ( lstat( fullname, &info ) == 0 && 
				 ( S_ISREG( info.st_mode ) || S_ISLNK( info.st_mode ) || ( strstr( fullname, ".clip" ) && info.st_mode | S_IXUSR ) ) )
				valerie_response_printf( cmd_arg->response, 1024, "\"%s\" %llu\n", de[i]->d_name, (unsigned long long) info.st_size );
			free( de[ i ] );
		}
		free( de );
		closedir( dir );
		valerie_response_write( cmd_arg->response, "\n", 1 );
	}

	return error;
}

/** Set a server configuration property.
*/

response_codes miracle_set_global_property( command_argument cmd_arg )
{
	char *key = (char*) cmd_arg->argument;
	char *value = NULL;

	value = strchr( key, '=' );
	if (value == NULL)
		return RESPONSE_OUT_OF_RANGE;
	*value = 0;
	value++;
	miracle_log( LOG_DEBUG, "SET %s = %s", key, value );

	if ( strncasecmp( key, "root", 1024) == 0 )
	{
		int len = strlen(value);
		int i;
		
		/* stop all units and unload clips */
		for (i = 0; i < MAX_UNITS; i++)
		{
			if (g_units[i] != NULL)
				miracle_unit_terminate( g_units[i] );
		}

		/* set the property */
		strncpy( cmd_arg->root_dir, value, 1023 );

		/* add a trailing slash if needed */
		if ( len && cmd_arg->root_dir[ len - 1 ] != '/')
		{
			cmd_arg->root_dir[ len ] = '/';
			cmd_arg->root_dir[ len + 1 ] = '\0';
		}
	}
	else
		return RESPONSE_OUT_OF_RANGE;
	
	return RESPONSE_SUCCESS;
}

/** Get a server configuration property.
*/

response_codes miracle_get_global_property( command_argument cmd_arg )
{
	char *key = (char*) cmd_arg->argument;

	if ( strncasecmp( key, "root", 1024) == 0 )
	{
		valerie_response_write( cmd_arg->response, cmd_arg->root_dir, strlen(cmd_arg->root_dir) );
		return RESPONSE_SUCCESS_1;
	}
	else
		return RESPONSE_OUT_OF_RANGE;
	
	return RESPONSE_SUCCESS;
}


