/*
 * miracle_unit.c -- DV Transmission Unit Implementation
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
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include <sys/mman.h>

#include "miracle_unit.h"
#include "miracle_log.h"
#include "miracle_local.h"

#include <framework/mlt.h>

/* Forward references */
static void miracle_unit_status_communicate( miracle_unit );

/** Allocate a new DV transmission unit.

    \return A new miracle_unit handle.
*/

miracle_unit miracle_unit_init( int index, char *constructor )
{
	miracle_unit this = NULL;
	mlt_consumer consumer = NULL;

	char *id = strdup( constructor );
	char *arg = strchr( id, ':' );

	if ( arg != NULL )
		*arg ++ = '\0';

	consumer = mlt_factory_consumer( id, arg );

	if ( consumer != NULL )
	{
		mlt_playlist playlist = mlt_playlist_init( );
		this = calloc( sizeof( miracle_unit_t ), 1 );
		this->properties = mlt_properties_new( );
		this->producers = mlt_properties_new( );
		mlt_properties_init( this->properties, this );
		mlt_properties_set_int( this->properties, "unit", index );
		mlt_properties_set_int( this->properties, "generation", 0 );
		mlt_properties_set( this->properties, "constructor", constructor );
		mlt_properties_set( this->properties, "id", id );
		mlt_properties_set( this->properties, "arg", arg );
		mlt_properties_set_data( this->properties, "consumer", consumer, 0, ( mlt_destructor )mlt_consumer_close, NULL );
		mlt_properties_set_data( this->properties, "playlist", playlist, 0, ( mlt_destructor )mlt_playlist_close, NULL );
		mlt_consumer_connect( consumer, mlt_playlist_service( playlist ) );
	}

	return this;
}

/** Communicate the current status to all threads waiting on the notifier.
*/

static void miracle_unit_status_communicate( miracle_unit unit )
{
	if ( unit != NULL )
	{
		mlt_properties properties = unit->properties;
		char *root_dir = mlt_properties_get( properties, "root" );
		valerie_notifier notifier = mlt_properties_get_data( properties, "notifier", NULL );
		valerie_status_t status;

		if ( root_dir != NULL && notifier != NULL )
		{
			if ( miracle_unit_get_status( unit, &status ) == 0 )
				/* if ( !( ( status.status == unit_playing || status.status == unit_paused ) &&
						strcmp( status.clip, "" ) && 
				    	!strcmp( status.tail_clip, "" ) && 
						status.position == 0 && 
						status.in == 0 && 
						status.out == 0 ) ) */
					valerie_notifier_put( notifier, &status );
		}
	}
}

/** Set the notifier info
*/

void miracle_unit_set_notifier( miracle_unit this, valerie_notifier notifier, char *root_dir )
{
	mlt_properties properties = this->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_properties playlist_properties = mlt_playlist_properties( playlist );

	mlt_properties_set( properties, "root", root_dir );
	mlt_properties_set_data( properties, "notifier", notifier, 0, NULL, NULL );
	mlt_properties_set_data( playlist_properties, "notifier_arg", this, 0, NULL, NULL );
	mlt_properties_set_data( playlist_properties, "notifier", miracle_unit_status_communicate, 0, NULL, NULL );

	miracle_unit_status_communicate( this );
}

static mlt_producer create_producer( char *file )
{
	return mlt_factory_producer( "fezzik", file );
}

/** Create or locate a producer for the file specified.
*/

static mlt_producer locate_producer( miracle_unit unit, char *file )
{
	// Get the unit properties
	mlt_properties properties = unit->producers;

	// Check if we already have loaded this file
	mlt_producer result = mlt_properties_get_data( properties, file, NULL );

	if ( result == NULL )
	{
		// Create the producer
		result = create_producer( file );

		// Now store the result
		if ( result != NULL )
			mlt_properties_set_data( properties, file, result, 0, ( mlt_destructor )mlt_producer_close, NULL );
	}

	return result;
}

/** Update the generation count.
*/

static void update_generation( miracle_unit unit )
{
	mlt_properties properties = unit->properties;
	int generation = mlt_properties_get_int( properties, "generation" );
	mlt_properties_set_int( properties, "generation", ++ generation );
}

/** Wipe all clips on the playlist for this unit.
*/

static void clear_unit( miracle_unit unit )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_producer producer = mlt_playlist_producer( playlist );

	mlt_playlist_clear( playlist );
	mlt_producer_seek( producer, 0 );

	if ( unit->old_producers != NULL )
		mlt_properties_close( unit->old_producers );
	unit->old_producers = unit->producers;
	unit->producers = mlt_properties_new( );

	update_generation( unit );
}

/** Generate a report on all loaded clips.
*/

void miracle_unit_report_list( miracle_unit unit, valerie_response response )
{
	int i;
	mlt_properties properties = unit->properties;
	char *root_dir = mlt_properties_get( properties, "root" );
	int generation = mlt_properties_get_int( properties, "generation" );
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );

	valerie_response_printf( response, 1024, "%d\n", generation );
		
	for ( i = 0; i < mlt_playlist_count( playlist ); i ++ )
	{
		mlt_playlist_clip_info info;
		mlt_playlist_get_clip_info( playlist , &info, i );
		char *resource = info.resource;
		if ( root_dir != NULL && !strncmp( resource, root_dir, strlen( root_dir ) ) )
			resource += strlen( root_dir );
		valerie_response_printf( response, 10240, "%d \"%s\" %lld %lld %lld %lld %.2f\n", 
								 i, resource, 
								 info.frame_in, 
								 info.frame_out,
								 info.frame_count, 
								 info.length, 
								 info.fps );
	}
}

/** Load a clip into the unit clearing existing play list.

    \todo error handling
    \param unit A miracle_unit handle.
    \param clip The absolute file name of the clip to load.
    \param in   The starting frame (-1 for 0)
	\param out  The ending frame (-1 for maximum)
*/

valerie_error_code miracle_unit_load( miracle_unit unit, char *clip, int64_t in, int64_t out, int flush )
{
	// Now try to create an producer
	mlt_producer instance = create_producer( clip );

	if ( instance != NULL )
	{
		clear_unit( unit );
		mlt_properties properties = unit->properties;
		mlt_properties_set_data( unit->producers, clip, instance, 0, ( mlt_destructor )mlt_producer_close, NULL );
		mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
		mlt_playlist_append_io( playlist, instance, in, out );
		miracle_log( LOG_DEBUG, "loaded clip %s", clip );
		miracle_unit_status_communicate( unit );
		return valerie_ok;
	}

	return valerie_invalid_file;
}

valerie_error_code miracle_unit_insert( miracle_unit unit, char *clip, int index, int64_t in, int64_t out )
{
	mlt_producer instance = locate_producer( unit, clip );

	if ( instance != NULL )
	{
		mlt_properties properties = unit->properties;
		mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
		mlt_playlist_insert( playlist, instance, index, in, out );
		miracle_log( LOG_DEBUG, "inserted clip %s at %d", clip, index );
		update_generation( unit );
		miracle_unit_status_communicate( unit );
		return valerie_ok;
	}

	return valerie_invalid_file;
}

valerie_error_code miracle_unit_remove( miracle_unit unit, int index )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_playlist_remove( playlist, index );
	miracle_log( LOG_DEBUG, "removed clip at %d", index );
	update_generation( unit );
	miracle_unit_status_communicate( unit );
	return valerie_ok;
}

valerie_error_code miracle_unit_clean( miracle_unit unit )
{
	clear_unit( unit );
	miracle_log( LOG_DEBUG, "Cleaned playlist" );
	miracle_unit_status_communicate( unit );
	return valerie_ok;
}

valerie_error_code miracle_unit_move( miracle_unit unit, int src, int dest )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_playlist_move( playlist, src, dest );
	miracle_log( LOG_DEBUG, "moved clip %d to %d", src, dest );
	update_generation( unit );
	miracle_unit_status_communicate( unit );
	return valerie_ok;
}

/** Add a clip to the unit play list.

    \todo error handling
    \param unit A miracle_unit handle.
    \param clip The absolute file name of the clip to load.
    \param in   The starting frame (-1 for 0)
	\param out  The ending frame (-1 for maximum)
*/

valerie_error_code miracle_unit_append( miracle_unit unit, char *clip, int64_t in, int64_t out )
{
	mlt_producer instance = locate_producer( unit, clip );

	if ( instance != NULL )
	{
		mlt_properties properties = unit->properties;
		mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
		mlt_playlist_append_io( playlist, instance, in, out );
		miracle_log( LOG_DEBUG, "appended clip %s", clip );
		update_generation( unit );
		miracle_unit_status_communicate( unit );
		return valerie_ok;
	}

	return valerie_invalid_file;
}

/** Start playing the unit.

    \todo error handling
    \param unit A miracle_unit handle.
    \param speed An integer that specifies the playback rate as a
                 percentage multiplied by 100.
*/

void miracle_unit_play( miracle_unit_t *unit, int speed )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_producer producer = mlt_playlist_producer( playlist );
	mlt_consumer consumer = mlt_properties_get_data( unit->properties, "consumer", NULL );
	mlt_producer_set_speed( producer, ( double )speed / 1000 );
	mlt_consumer_start( consumer );
	miracle_unit_status_communicate( unit );
}

/** Stop playback.

    Terminates the dv_pump and halts dv1394 transmission.

    \param unit A miracle_unit handle.
*/

void miracle_unit_terminate( miracle_unit unit )
{
	mlt_consumer consumer = mlt_properties_get_data( unit->properties, "consumer", NULL );
	mlt_consumer_stop( consumer );
	miracle_unit_status_communicate( unit );
}

/** Query the status of unit playback.

    \param unit A miracle_unit handle.
    \return 1 if the unit is not playing, 0 if playing.
*/

int miracle_unit_has_terminated( miracle_unit unit )
{
	mlt_consumer consumer = mlt_properties_get_data( unit->properties, "consumer", NULL );
	return mlt_consumer_is_stopped( consumer );
}

/** Transfer the currently loaded clip to another unit
*/

int miracle_unit_transfer( miracle_unit dest_unit, miracle_unit src_unit )
{
	return 0;
}

/** Determine if unit is offline.
*/

int miracle_unit_is_offline( miracle_unit unit )
{
	return 0;
}

/** Obtain the status for a given unit
*/

int miracle_unit_get_status( miracle_unit unit, valerie_status status )
{
	int error = unit == NULL;

	memset( status, 0, sizeof( valerie_status_t ) );

	if ( !error )
	{
		mlt_properties properties = unit->properties;
		char *root_dir = mlt_properties_get( properties, "root" );		
		mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
		mlt_producer producer = mlt_playlist_producer( playlist );
		mlt_producer clip = mlt_playlist_current( playlist );

		mlt_playlist_clip_info info;
		int clip_index = mlt_playlist_current_clip( playlist );
		mlt_playlist_get_clip_info( playlist, &info, clip_index );

		if ( info.resource != NULL && strcmp( info.resource, "" ) )
		{
			if ( root_dir == NULL || strncmp( info.resource, root_dir, strlen( root_dir ) ) )
				strncpy( status->clip, info.resource, sizeof( status->clip ) );
			else
				strncpy( status->clip, info.resource + strlen( root_dir ), sizeof( status->clip ) );
			status->speed = (int)( mlt_producer_get_speed( producer ) * 1000.0 );
			status->fps = mlt_producer_get_fps( producer );
			status->in = info.frame_in;
			status->out = info.frame_out;
			status->position = mlt_producer_position( clip );
			status->length = mlt_producer_get_length( clip );
			if ( root_dir == NULL || strncmp( info.resource, root_dir, strlen( root_dir ) ) )
				strncpy( status->tail_clip, info.resource, sizeof( status->tail_clip ) );
			else
				strncpy( status->clip, info.resource + strlen( root_dir ), sizeof( status->clip ) );
			status->tail_in = info.frame_in;
			status->tail_out = info.frame_out;
			status->tail_position = mlt_producer_position( clip );
			status->tail_length = mlt_producer_get_length( clip );
			status->clip_index = mlt_playlist_current_clip( playlist );
			status->seek_flag = 1;
		}

		status->generation = mlt_properties_get_int( properties, "generation" );

		if ( miracle_unit_has_terminated( unit ) )
			status->status = unit_stopped;
		else if ( !strcmp( status->clip, "" ) )
			status->status = unit_not_loaded;
		else if ( status->speed == 0 )
			status->status = unit_paused;
		else
			status->status = unit_playing;
	}
	else
	{
		status->status = unit_undefined;
	}

	status->unit = mlt_properties_get_int( unit->properties, "unit" );

	return error;
}

/** Change position in the playlist.
*/

void miracle_unit_change_position( miracle_unit unit, int clip, int64_t position )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_producer producer = mlt_playlist_producer( playlist );
	mlt_playlist_clip_info info;

	if ( clip < 0 )
	{
		clip = 0;
		position = 0;
	}
	else if ( clip >= mlt_playlist_count( playlist ) )
	{
		clip = mlt_playlist_count( playlist ) - 1;
		position = LONG_MAX;
	}

	if ( mlt_playlist_get_clip_info( playlist, &info, clip ) == 0 )
	{
		int64_t frame_start = info.start;
		int64_t frame_offset = position;

		if ( frame_offset < 0 )
			frame_offset = info.frame_out;
		if ( frame_offset < info.frame_in )
			frame_offset = info.frame_in;
		if ( frame_offset >= info.frame_out )
			frame_offset = info.frame_out;
		
		mlt_producer_seek( producer, frame_start + frame_offset - info.frame_in );
	}

	miracle_unit_status_communicate( unit );
}

/** Get the index of the current clip.
*/

int	miracle_unit_get_current_clip( miracle_unit unit )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	int clip_index = mlt_playlist_current_clip( playlist );
	return clip_index;
}

/** Set a clip's in point
*/

int miracle_unit_set_clip_in( miracle_unit unit, int index, int64_t position )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_playlist_clip_info info;
	int error = mlt_playlist_get_clip_info( playlist, &info, index );

	if ( error == 0 )
	{
		miracle_unit_play( unit, 0 );
		error = mlt_playlist_resize_clip( playlist, index, position, info.frame_out );
		update_generation( unit );
		miracle_unit_change_position( unit, index, 0 );
	}

	return error;
}

/** Set a clip's out point.
*/

int miracle_unit_set_clip_out( miracle_unit unit, int index, int64_t position )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_playlist_clip_info info;
	int error = mlt_playlist_get_clip_info( playlist, &info, index );

	if ( error == 0 )
	{
		miracle_unit_play( unit, 0 );
		error = mlt_playlist_resize_clip( playlist, index, info.frame_in, position );
		update_generation( unit );
		miracle_unit_status_communicate( unit );
		miracle_unit_change_position( unit, index, -1 );
	}

	return error;
}

/** Step by specified position.
*/

void miracle_unit_step( miracle_unit unit, int64_t offset )
{
	mlt_properties properties = unit->properties;
	mlt_playlist playlist = mlt_properties_get_data( properties, "playlist", NULL );
	mlt_producer producer = mlt_playlist_producer( playlist );
	mlt_position position = mlt_producer_frame( producer );
	mlt_producer_seek( producer, position + offset );
}

/** Set the unit's clip mode regarding in and out points.
*/

//void miracle_unit_set_mode( miracle_unit unit, dv_player_clip_mode mode )
//{
	//dv_player player = miracle_unit_get_dv_player( unit );
	//if ( player != NULL )
		//dv_player_set_clip_mode( player, mode );
	//miracle_unit_status_communicate( unit );
//}

/** Get the unit's clip mode regarding in and out points.
*/

//dv_player_clip_mode miracle_unit_get_mode( miracle_unit unit )
//{
	//dv_player player = miracle_unit_get_dv_player( unit );
	//return dv_player_get_clip_mode( player );
//}

/** Set the unit's clip mode regarding eof handling.
*/

//void miracle_unit_set_eof_action( miracle_unit unit, dv_player_eof_action action )
//{
	//dv_player player = miracle_unit_get_dv_player( unit );
	//dv_player_set_eof_action( player, action );
	//miracle_unit_status_communicate( unit );
//}

/** Get the unit's clip mode regarding eof handling.
*/

//dv_player_eof_action miracle_unit_get_eof_action( miracle_unit unit )
//{
	//dv_player player = miracle_unit_get_dv_player( unit );
	//return dv_player_get_eof_action( player );
//}

int miracle_unit_set( miracle_unit unit, char *name_value )
{
	mlt_properties properties = NULL;

	if ( strncmp( name_value, "consumer.", 9 ) )
	{
		mlt_playlist playlist = mlt_properties_get_data( unit->properties, "playlist", NULL );
		properties = mlt_playlist_properties( playlist );
	}
	else
	{
		mlt_consumer consumer = mlt_properties_get_data( unit->properties, "consumer", NULL );
		properties = mlt_consumer_properties( consumer );
		name_value += 9;
	}

	return mlt_properties_parse( properties, name_value );
}

char *miracle_unit_get( miracle_unit unit, char *name )
{
	mlt_playlist playlist = mlt_properties_get_data( unit->properties, "playlist", NULL );
	mlt_properties properties = mlt_playlist_properties( playlist );
	return mlt_properties_get( properties, name );
}

/** Release the unit

    \todo error handling
    \param unit A miracle_unit handle.
*/

void miracle_unit_close( miracle_unit unit )
{
	if ( unit != NULL )
	{
		miracle_log( LOG_DEBUG, "closing unit..." );
		if ( unit->old_producers != NULL )
			mlt_properties_close( unit->old_producers );
		mlt_properties_close( unit->properties );
		mlt_properties_close( unit->producers );
		free( unit );
		miracle_log( LOG_DEBUG, "... unit closed." );
	}
}

