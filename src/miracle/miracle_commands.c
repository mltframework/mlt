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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#include "dvunit.h"
#include "global_commands.h"
#include "raw1394util.h"
#include <libavc1394/rom1394.h>
#include "log.h"

static dv_unit g_units[MAX_UNITS];


/** Return the dv_unit given a numeric index.
*/

dv_unit dv1394d_get_unit( int n )
{
	if (n < MAX_UNITS)
		return g_units[n];
	else
		return NULL;
}

/** Destroy the dv_unit given its numeric index.
*/

void dv1394d_delete_unit( int n )
{
	if (n < MAX_UNITS)
	{
		dv_unit unit = dv1394d_get_unit(n);
		if (unit != NULL)
		{
			dv_unit_close( unit );
			g_units[ n ] = NULL;
			dv1394d_log( LOG_NOTICE, "Deleted unit U%d.", n ); 
		}
	}
}

/** Destroy all allocated units on the server.
*/

void dv1394d_delete_all_units( void )
{
	int i;
	for (i = 0; i < MAX_UNITS; i++)
		if ( dv1394d_get_unit(i) != NULL )
		{
			dv_unit_close( dv1394d_get_unit(i) );
			dv1394d_log( LOG_NOTICE, "Deleted unit U%d.", i ); 
		}
}

/** Add a DV virtual vtr to the server.
*/
response_codes dv1394d_add_unit( command_argument cmd_arg )
{
	int i;
	int channel = -1;
	char *guid_str = (char*) cmd_arg->argument;
	octlet_t guid;
	uint32_t guid_hi;
	uint32_t guid_lo;
	
	sscanf( guid_str, "%08x%08x", &guid_hi, &guid_lo );
	guid = (octlet_t)guid_hi << 32 | (octlet_t) guid_lo;

	if ( dv_tokeniser_count( cmd_arg->tokeniser ) == 3 )
		channel = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 2 ) );

	/* make sure unit does not already exit */
	for (i = 0; i < MAX_UNITS; i++)
	{
		if (g_units[i] != NULL)
			if ( dv_unit_get_guid( g_units[i] ) == guid )
			{
				dv_response_printf( cmd_arg->response, 1024, "a unit already exists for that node\n\n" );
				return RESPONSE_ERROR;
			}
	}
	
	for (i = 0; i < MAX_UNITS; i++)
	{
		if (g_units[i] == NULL)
		{
		
			g_units[ i ] = dv_unit_init( guid, channel );
			if ( g_units[ i ] == NULL )
			{
				dv_response_printf( cmd_arg->response, 1024, "failed to allocate unit\n" );
				return RESPONSE_ERROR;
			}
			g_units[ i ]->unit = i;
			dv_unit_set_notifier( g_units[ i ], dv_parser_get_notifier( cmd_arg->parser ), cmd_arg->root_dir );

			dv1394d_log( LOG_NOTICE, "added unit %d to send to node %d over channel %d", 
				i, dv_unit_get_nodeid( g_units[i] ), dv_unit_get_channel( g_units[i] ) );
			dv_response_printf( cmd_arg->response, 10, "U%1d\n\n", i );
			return RESPONSE_SUCCESS_N;
		}
	}
	
	dv_response_printf( cmd_arg->response, 1024, "no more units can be created\n\n" );

	return RESPONSE_ERROR;
}


/** List all AV/C nodes on the bus.
*/
response_codes dv1394d_list_nodes( command_argument cmd_arg )
{
	response_codes error = RESPONSE_SUCCESS_N;
	raw1394handle_t handle;
	int i, j;
	char line[1024];
	octlet_t guid;
	rom1394_directory dir;

	for ( j = 0; j < raw1394_get_num_ports(); j++ )
	{
		handle = raw1394_open(j);
		for ( i = 0; i < raw1394_get_nodecount(handle); ++i )
		{
			rom1394_get_directory( handle, i, &dir);
			if ( (rom1394_get_node_type(&dir) == ROM1394_NODE_TYPE_AVC) )
			{
				guid = rom1394_get_guid(handle, i);
				if (dir.label != NULL)
				{
					snprintf( line, 1023, "%02d %08x%08x \"%s\"\n", i, 
						(quadlet_t) (guid>>32), (quadlet_t) (guid & 0xffffffff), dir.label );
				} else {
					snprintf( line, 1023, "%02d %08x%08x \"Unlabeled Node %d\"\n", i, 
						(quadlet_t) (guid>>32), (quadlet_t) (guid & 0xffffffff), i );
				}
				dv_response_write( cmd_arg->response, line, strlen(line) );
				rom1394_free_directory( &dir);
			}
		}
		raw1394_close( handle );
	}
	dv_response_write( cmd_arg->response, "\n", 1 );
	return error;
}


/** List units already added to server.
*/
response_codes dv1394d_list_units( command_argument cmd_arg )
{
	response_codes error = RESPONSE_SUCCESS_N;
	char line[1024];
	int i;
	
	for (i = 0; i < MAX_UNITS; i++)
	{
		if (dv1394d_get_unit(i) != NULL)
		{
			snprintf( line, 1023, "U%d %02d %08x%08x %d\n", i, dv_unit_get_nodeid(g_units[i]),
			(quadlet_t) (dv_unit_get_guid(g_units[i]) >> 32), 
			(quadlet_t) (dv_unit_get_guid(g_units[i]) & 0xffffffff),
			!dv_unit_is_offline( g_units[i] ) );
			dv_response_write( cmd_arg->response, line, strlen(line) );
		}
	}
	dv_response_write( cmd_arg->response, "\n", 1 );

	return error;
}

static int
filter_files( const struct dirent *de )
{
	if ( de->d_name[ 0 ] != '.' )
		return 1;
	else
		return 0;
}

/** List clips in a directory.
*/
response_codes dv1394d_list_clips( command_argument cmd_arg )
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
				dv_response_printf( cmd_arg->response, 1024, "\"%s/\"\n", de[i]->d_name );
		}
		for (i = 0; i < n; i++ )
		{
			snprintf( fullname, 1023, "%s%s/%s", cmd_arg->root_dir, dir_name, de[i]->d_name );
			if ( lstat( fullname, &info ) == 0 && 
				 ( S_ISREG( info.st_mode ) || ( strstr( fullname, ".clip" ) && info.st_mode | S_IXUSR ) ) )
				dv_response_printf( cmd_arg->response, 1024, "\"%s\" %llu\n", de[i]->d_name, (unsigned long long) info.st_size );
			free( de[ i ] );
		}
		free( de );
		closedir( dir );
		dv_response_write( cmd_arg->response, "\n", 1 );
	}

	return error;
}

/** Set a server configuration property.
*/

response_codes dv1394d_set_global_property( command_argument cmd_arg )
{
	char *key = (char*) cmd_arg->argument;
	char *value = NULL;

	value = strchr( key, '=' );
	if (value == NULL)
		return RESPONSE_OUT_OF_RANGE;
	*value = 0;
	value++;
	dv1394d_log( LOG_DEBUG, "SET %s = %s", key, value );

	if ( strncasecmp( key, "root", 1024) == 0 )
	{
		int len = strlen(value);
		int i;
		
		/* stop all units and unload clips */
		for (i = 0; i < MAX_UNITS; i++)
		{
			if (g_units[i] != NULL)
				dv_unit_terminate( g_units[i] );
		}

		/* set the property */
		strncpy( cmd_arg->root_dir, value, 1023 );

		/* add a trailing slash if needed */
		if ( cmd_arg->root_dir[ len - 1 ] != '/')
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

response_codes dv1394d_get_global_property( command_argument cmd_arg )
{
	char *key = (char*) cmd_arg->argument;

	if ( strncasecmp( key, "root", 1024) == 0 )
	{
		dv_response_write( cmd_arg->response, cmd_arg->root_dir, strlen(cmd_arg->root_dir) );
		return RESPONSE_SUCCESS_1;
	}
	else
		return RESPONSE_OUT_OF_RANGE;
	
	return RESPONSE_SUCCESS;
}

/** IEEE 1394 Bus Reset handler 

    This is included here for now due to all the unit management involved.
*/

static int reset_handler( raw1394handle_t h, unsigned int generation )
{
	int i, j, count, retry = 3;
	int port = (int) raw1394_get_userdata( h );
	
	raw1394_update_generation( h, generation );
	dv1394d_log( LOG_NOTICE, "bus reset on port %d", port );

	while ( retry-- > 0 ) 
	{
		raw1394handle_t handle = raw1394_open( port );
		count = raw1394_get_nodecount( handle );
		
		if ( count > 0 )
		{
			dv1394d_log( LOG_DEBUG, "bus reset, checking units" );
			
			/* suspend all units on this port */
			for ( j = MAX_UNITS; j > 0; j-- )
			{
				if ( g_units[ j-1 ] != NULL && dv_unit_get_port( g_units[ j-1 ] ) == port )
					dv_unit_suspend( g_units[ j-1 ] );
			}
			dv1394d_log( LOG_DEBUG, "All units are now stopped" );
			
			/* restore units with known guid, take others offline */
			for ( j = 0; j < MAX_UNITS; j++ )
			{
				if ( g_units[j] != NULL && 
					( dv_unit_get_port( g_units[ j ] ) == port || dv_unit_get_port( g_units[ j ] ) == -1 ) )
				{
					int found = 0;
					for ( i = 0; i < count; i++ )
					{
						octlet_t guid;
						dv1394d_log( LOG_DEBUG, "attempting to get guid for node %d", i );
						guid = rom1394_get_guid( handle, i );
						if ( guid == g_units[ j ]->guid )
						{
							dv1394d_log( LOG_NOTICE, "unit with GUID %08x%08x found", 
								(quadlet_t) (g_units[j]->guid>>32), (quadlet_t) (g_units[j]->guid & 0xffffffff));
							if ( dv_unit_is_offline( g_units[ j ] ) )
								dv_unit_online( g_units[ j ] );
							else
								dv_unit_restore( g_units[ j ] );
							found = 1;
							break;
						}
					}
					if ( found == 0 )
						dv_unit_offline( g_units[ j ] );
				}
			}
			dv1394d_log( LOG_DEBUG, "completed bus reset handler");
			raw1394_close( handle );
			return 0;
		}
		raw1394_close( handle );
	}
	dv1394d_log( LOG_CRIT, "raw1394 reported zero nodes on the bus!" );
	return 0;
}


/** One pthread per IEEE 1394 port
*/

static pthread_t raw1394service_thread[4];

/** One raw1394 handle for each pthread/port
*/

static raw1394handle_t raw1394service_handle[4];

/** The service thread that polls raw1394 for new events.
*/

static void* raw1394_service( void *arg )
{
	raw1394handle_t handle = (raw1394handle_t) arg;
	struct pollfd raw1394_poll;
	raw1394_poll.fd = raw1394_get_fd( handle );
	raw1394_poll.events = POLLIN;
	raw1394_poll.revents = 0;
	while ( 1 )
	{
		if ( poll( &raw1394_poll, 1, 200) > 0 )
		{
			if ( (raw1394_poll.revents & POLLIN) 
					|| (raw1394_poll.revents & POLLPRI) )
				raw1394_loop_iterate( handle );
		}
		pthread_testcancel();
	}
	
}


/** Start the raw1394 service threads for handling bus reset.

    One thread is launched per port on the system.
*/

void raw1394_start_service_threads( void )
{
	int port;
	for ( port = 0; port < raw1394_get_num_ports(); port++ )
	{
		raw1394service_handle[port] = raw1394_open( port );
		raw1394_set_bus_reset_handler( raw1394service_handle[port], reset_handler );
		pthread_create( &(raw1394service_thread[port]), NULL, raw1394_service, raw1394service_handle[port] );
	}
	for ( ; port < 4; port++ )
		raw1394service_handle[port] = NULL;
}

/** Shutdown all the raw1394 service threads.
*/

void raw1394_stop_service_threads( void )
{
	int i;
	for ( i = 0; i < 4; i++ )
	{
		if ( raw1394service_handle[i] != NULL )
		{
			pthread_cancel( raw1394service_thread[i] );
			pthread_join( raw1394service_thread[i], NULL );
			raw1394_close( raw1394service_handle[i] );
		}
	}
}


