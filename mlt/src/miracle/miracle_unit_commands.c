/*
 * unit_commands.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "dvunit.h"
#include "global_commands.h"
#include "dverror.h"
#include "dvframepool.h"
#include "log.h"

int dv1394d_load( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	char *filename = (char*) cmd_arg->argument;
	char fullname[1024];
	int flush = 1;

	if ( filename[0] == '!' )
	{
		flush = 0;
		filename ++;
	}

	if ( filename[0] == '/' )
		filename++;

	snprintf( fullname, 1023, "%s%s", cmd_arg->root_dir, filename );
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		long in = -1, out = -1;
		if ( dv_tokeniser_count( cmd_arg->tokeniser ) == 5 )
		{
			in = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 3 ) );
			out = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 4 ) );
		}
		if ( dv_unit_load( unit, fullname, in, out, flush ) != dv_pump_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int dv1394d_list( command_argument cmd_arg )
{
	int i = 0;
	dv_unit unit = dv1394d_get_unit( cmd_arg->unit );
	dv_player player = dv_unit_get_dv_player( unit );
	
	if ( player != NULL )
	{
		dv_response_printf( cmd_arg->response, 1024, "%d\n", player->generation );
		
		for ( i = 0; i < dv_player_get_clip_count( player ); i ++ )
		{
			dv_clip clip = dv_player_get_clip( player, i );
			
			dv_response_printf( cmd_arg->response, 10240,
								"%d \"%s\" %d %d %d %d %.2f\n", 
								i,
								dv_clip_get_resource( clip, cmd_arg->root_dir ),
								dv_clip_get_in( clip ),
								( !dv_clip_is_seekable( clip ) && clip->out_frame == -1 ? -1 : dv_clip_get_out( clip ) ),
								dv_clip_get_max_frames( clip ),
								( !dv_clip_is_seekable( clip ) && clip->out_frame == -1 ? -1 : dv_player_get_length_of_clip( player, i ) ),
								dv_clip_frames_per_second( clip ) );
		}
	
		dv_response_printf( cmd_arg->response, 2, "\n" );
		
		return RESPONSE_SUCCESS;
	}
	return RESPONSE_INVALID_UNIT;
}

static int parse_clip( command_argument cmd_arg, int arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	int clip = dv_unit_get_current_clip( unit );
	
	if ( dv_tokeniser_count( cmd_arg->tokeniser ) > arg )
	{
		dv_player player = dv_unit_get_dv_player( unit );
		char *token = dv_tokeniser_get_string( cmd_arg->tokeniser, arg );
		if ( token[ 0 ] == '+' )
			clip += atoi( token + 1 );
		else if ( token[ 0 ] == '-' )
			clip -= atoi( token + 1 );
		else
			clip = atoi( token );
		if ( clip < 0 )
			clip = 0;
		if ( clip >= player->size )
			clip = player->size - 1;
	}
	
	return clip;
}

int dv1394d_insert( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	char *filename = (char*) cmd_arg->argument;
	char fullname[1024];

	if ( filename[0] == '/' )
		filename++;

	snprintf( fullname, 1023, "%s%s", cmd_arg->root_dir, filename );
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		long in = -1, out = -1;
		int index = parse_clip( cmd_arg, 3 );
		
		if ( dv_tokeniser_count( cmd_arg->tokeniser ) == 6 )
		{
			in = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 4 ) );
			out = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 5 ) );
		}
		
		switch( dv_unit_insert( unit, fullname, index, in, out ) )
		{
			case dv_pump_ok:
				return RESPONSE_SUCCESS;
			case dv_pump_too_many_files_open:
				return RESPONSE_TOO_MANY_FILES;
			default:
				return RESPONSE_BAD_FILE;
		}
	}
	return RESPONSE_SUCCESS;
}

int dv1394d_remove( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		int index = parse_clip( cmd_arg, 2 );
			
		if ( dv_unit_remove( unit, index ) != dv_pump_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int dv1394d_clean( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		if ( dv_unit_clean( unit ) != dv_pump_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int dv1394d_move( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if ( unit != NULL )
	{
		if ( dv_tokeniser_count( cmd_arg->tokeniser ) > 2 )
		{
			int src = parse_clip( cmd_arg, 2 );
			int dest = parse_clip( cmd_arg, 3 );
			
			if ( dv_unit_move( unit, src, dest ) != dv_pump_ok )
				return RESPONSE_BAD_FILE;
		}
		else
		{
			return RESPONSE_MISSING_ARG;
		}
	}
	else
	{
		return RESPONSE_INVALID_UNIT;
	}
	
	return RESPONSE_SUCCESS;
}

int dv1394d_append( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	char *filename = (char*) cmd_arg->argument;
	char fullname[1024];

	if ( filename[0] == '/' )
		filename++;
	snprintf( fullname, 1023, "%s%s", cmd_arg->root_dir, filename );
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		long in = -1, out = -1;
		if ( dv_tokeniser_count( cmd_arg->tokeniser ) == 5 )
		{
			in = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 3 ) );
			out = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 4 ) );
		}
		switch ( dv_unit_append( unit, fullname, in, out ) )
		{
			case dv_pump_ok:
				return RESPONSE_SUCCESS;
			case dv_pump_too_many_files_open:
				return RESPONSE_TOO_MANY_FILES;
			default:
				return RESPONSE_BAD_FILE;
		}
	}
	return RESPONSE_SUCCESS;
}

int dv1394d_play( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
	{
		int speed = 1000;
		if ( dv_tokeniser_count( cmd_arg->tokeniser ) == 3 )
			speed = atoi( dv_tokeniser_get_string( cmd_arg->tokeniser, 2 ) );
		dv_unit_play( unit, speed );
	}
	
	return RESPONSE_SUCCESS;
}

int dv1394d_stop( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
		dv_unit_terminate( unit );
	
	return RESPONSE_SUCCESS;
}

int dv1394d_pause( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else 
		dv_unit_play( unit, 0 );
	
	return RESPONSE_SUCCESS;
}

int dv1394d_rewind( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
		dv_unit_change_speed( unit, -2000 );
	
	return RESPONSE_SUCCESS;
}

int dv1394d_step( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
	{
		dv_unit_play( unit, 0 );
		dv_unit_step( unit, *(int*) cmd_arg->argument );
	}
	
	return RESPONSE_SUCCESS;
}

int dv1394d_goto( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	int clip = parse_clip( cmd_arg, 3 );
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
		dv_unit_change_position( unit, clip, *(int*) cmd_arg->argument );
	
	return RESPONSE_SUCCESS;
}

int dv1394d_ff( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
		dv_unit_change_speed( unit, 2000 );
	
	return RESPONSE_SUCCESS;
}

int dv1394d_set_in_point( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	int clip = parse_clip( cmd_arg, 3 );
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
	{
		int position = *(int *) cmd_arg->argument;

		switch( dv_unit_set_clip_in( unit, clip, position ) )
		{
			case -1:
				return RESPONSE_BAD_FILE;
			case -2:
				return RESPONSE_OUT_OF_RANGE;
		}
	}
	
	return RESPONSE_SUCCESS;
}

int dv1394d_set_out_point( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	int clip = parse_clip( cmd_arg, 3 );
	
	if (unit == NULL || dv_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
	{
		int position = *(int *) cmd_arg->argument;

		switch( dv_unit_set_clip_out( unit, clip, position ) )
		{
			case -1:
				return RESPONSE_BAD_FILE;
			case -2:
				return RESPONSE_OUT_OF_RANGE;
		}
	}
	
	return RESPONSE_SUCCESS;
}

int dv1394d_get_unit_status( command_argument cmd_arg )
{
	dv1394_status_t status;
	int error = dv_unit_get_status( dv1394d_get_unit( cmd_arg->unit ), &status );

	if ( error == -1 )
		return RESPONSE_INVALID_UNIT;
	else
	{
		char text[ 10240 ];

		dv_response_printf( cmd_arg->response, 
							sizeof( text ), 
							dv1394_status_serialise( &status, text, sizeof( text ) ) );

		return RESPONSE_SUCCESS_1;
	}
	
	return 0;
}


int dv1394d_set_unit_property( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		char *key = (char*) cmd_arg->argument;
		char *value = NULL;

		value = strchr( key, '=' );
		if (value == NULL)
			return RESPONSE_OUT_OF_RANGE;
		value[0] = 0;
		value++;
		dv1394d_log( LOG_DEBUG, "USET %s = %s", key, value );
		if ( strncasecmp( key, "eof", 1024) == 0 )
		{
			if ( strncasecmp( value, "pause", 1024) == 0)
				dv_unit_set_eof_action( unit, dv_player_pause );
			else if ( strncasecmp( value, "loop", 1024) == 0)
				dv_unit_set_eof_action( unit, dv_player_loop );
			else if ( strncasecmp( value, "stop", 1024) == 0)
				dv_unit_set_eof_action( unit, dv_player_terminate );
			else if ( strncasecmp( value, "clean", 1024) == 0)
				dv_unit_set_eof_action( unit, dv_player_clean_loop );
			else
				return RESPONSE_OUT_OF_RANGE;
		}
		else if ( strncasecmp( key, "points", 1024) == 0 )
		{
			if ( strncasecmp( value, "use", 1024) == 0)
				dv_unit_set_mode( unit, dv_clip_mode_restricted );
			else if ( strncasecmp( value, "ignore", 1024) == 0)
				dv_unit_set_mode( unit, dv_clip_mode_unrestricted );
			else
				return RESPONSE_OUT_OF_RANGE;
		}
		else if ( strncasecmp( key, "syt_offset", 1024) == 0 )
		{
			dv_unit_set_syt_offset( unit, atoi( value ) );
		}
		else if ( strncasecmp( key, "cip_n", 1024) == 0 )
		{
			dv_unit_set_cip_n( unit, atoi( value ) );
		}
		else if ( strncasecmp( key, "cip_d", 1024) == 0 )
		{
			dv_unit_set_cip_d( unit, atoi( value ) );
		}
		else if ( strncasecmp( key, "size", 1024) == 0 )
		{
			dv_unit_set_buffer_size( unit, atoi( value ) );
		}
		else if ( strncasecmp( key, "n_frames", 1024) == 0 )
		{
			dv_unit_set_n_frames( unit, atoi( value ) );
		}		
		else if ( strncasecmp( key, "n_fill", 1024) == 0 )
		{
			dv_unit_set_n_fill( unit, atoi( value ) );
		}		
		else
			return RESPONSE_OUT_OF_RANGE;
	}
	
	return RESPONSE_SUCCESS;
}

int dv1394d_get_unit_property( command_argument cmd_arg )
{
	dv_unit unit = dv1394d_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		char *key = (char*) cmd_arg->argument;

		if ( strncasecmp( key, "eof", 1024) == 0 )
		{
			switch ( dv_unit_get_eof_action( unit ) )
			{
				case dv_player_pause:
					dv_response_write( cmd_arg->response, "pause", strlen("pause") );
					break;
				case dv_player_loop:
					dv_response_write( cmd_arg->response, "loop", strlen("loop") );
					break;
				case dv_player_terminate:
					dv_response_write( cmd_arg->response, "stop", strlen("stop") );
					break;
				case dv_player_clean_loop:
					dv_response_write( cmd_arg->response, "clean", strlen("clean") );
					break;
			}
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "points", 1024) == 0 )
		{
			if ( dv_unit_get_mode( unit ) == dv_clip_mode_restricted )
				dv_response_write( cmd_arg->response, "use", strlen("use") );
			else
				dv_response_write( cmd_arg->response, "ignore", strlen("ignore") );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "syt_offset", 1024) == 0 )
		{
			dv_response_printf( cmd_arg->response, 1024, "%d\n",
				dv_unit_get_syt_offset( unit ) );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "cip_n", 1024) == 0 )
		{
			dv_response_printf( cmd_arg->response, 1024, "%d\n",
				dv_unit_get_cip_n( unit ) );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "cip_d", 1024) == 0 )
		{
			dv_response_printf( cmd_arg->response, 1024, "%d\n",
				dv_unit_get_cip_d( unit ) );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "size", 1024) == 0 )
		{
			dv_response_printf( cmd_arg->response, 1024, "%d\n",
				dv_unit_get_buffer_size( unit ) );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "n_frames", 1024) == 0 )
		{
			dv_response_printf( cmd_arg->response, 1024, "%d\n",
				dv_unit_get_n_frames( unit ) );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "n_fill", 1024) == 0 )
		{
			dv_response_printf( cmd_arg->response, 1024, "%d\n",
				dv_unit_get_n_fill( unit ) );
			return RESPONSE_SUCCESS_1;
		}
		else if ( strncasecmp( key, "all", 1024 ) == 0 )
		{
			switch ( dv_unit_get_eof_action( unit ) )
			{
				case dv_player_pause:
					dv_response_write( cmd_arg->response, "eof=pause\n", strlen("pause") );
					break;
				case dv_player_loop:
					dv_response_write( cmd_arg->response, "eof=loop\n", strlen("loop") );
					break;
				case dv_player_terminate:
					dv_response_write( cmd_arg->response, "eof=stop\n", strlen("stop") );
					break;
				case dv_player_clean_loop:
					dv_response_write( cmd_arg->response, "eof=clean\n", strlen("clean") );
					break;
			}
			if ( dv_unit_get_mode( unit ) == dv_clip_mode_restricted )
				dv_response_write( cmd_arg->response, "points=use\n", strlen("use") );
			else
				dv_response_write( cmd_arg->response, "points=ignore\n", strlen("ignore") );
			dv_response_printf( cmd_arg->response, 1024, "syt_offset=%d\n", dv_unit_get_syt_offset( unit ) );
			dv_response_printf( cmd_arg->response, 1024, "cip_n=%d\n", dv_unit_get_cip_n( unit ) );
			dv_response_printf( cmd_arg->response, 1024, "cip_d=%d\n", dv_unit_get_cip_d( unit ) );
			dv_response_printf( cmd_arg->response, 1024, "size=%d\n", dv_unit_get_buffer_size( unit ) );
			dv_response_printf( cmd_arg->response, 1024, "n_frames=%d\n", dv_unit_get_n_frames( unit ) );
			dv_response_printf( cmd_arg->response, 1024, "n_fill=%d\n", dv_unit_get_n_fill( unit ) );
		}
	}
	
	return RESPONSE_SUCCESS;
}


int dv1394d_transfer( command_argument cmd_arg )
{
	dv_unit src_unit = dv1394d_get_unit(cmd_arg->unit);
	int dest_unit_id = -1;
	char *string = (char*) cmd_arg->argument;
	if ( string != NULL && ( string[ 0 ] == 'U' || string[ 0 ] == 'u' ) && strlen( string ) > 1 )
		dest_unit_id = atoi( string + 1 );
	
	if ( src_unit != NULL && dest_unit_id != -1 )
	{
		dv_unit dest_unit = dv1394d_get_unit( dest_unit_id );
		if ( dest_unit != NULL && !dv_unit_is_offline(dest_unit) && dest_unit != src_unit )
		{
			dv_unit_transfer( dest_unit, src_unit );
			return RESPONSE_SUCCESS;
		}
	}
	
	return RESPONSE_INVALID_UNIT;
}
