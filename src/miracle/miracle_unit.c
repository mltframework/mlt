/*
 * dvunit.c -- DV Transmission Unit Implementation
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

#include <libdv/dv1394.h>
#include <libraw1394/raw1394.h>
#include <libavc1394/avc1394_vcr.h>
#include <sys/mman.h>

#include "dvunit.h"
#include "dvframe.h"
#include "dvframepool.h"
#include "dvqueue.h"
#include "dvpump.h"
#include "dverror.h"
#include "dvplayer.h"
#include "raw1394util.h"
#include "log.h"
#include "dvlocal.h"

/* Forward references */
static void dv_unit_status_communicate( dv_unit );

/** dv1394 device file names based upon devfs default names. */

static char *devices[4][4] = {
	{
	"/dev/ieee1394/dv/host0/NTSC/in",
	"/dev/ieee1394/dv/host0/NTSC/out",
	"/dev/ieee1394/dv/host0/PAL/in",
	"/dev/ieee1394/dv/host0/PAL/out",
	},{
	"/dev/ieee1394/dv/host1/NTSC/in",
	"/dev/ieee1394/dv/host1/NTSC/out",
	"/dev/ieee1394/dv/host1/PAL/in",
	"/dev/ieee1394/dv/host1/PAL/out"
	},{
	"/dev/ieee1394/dv/host2/NTSC/in",
	"/dev/ieee1394/dv/host2/NTSC/out",
	"/dev/ieee1394/dv/host2/PAL/in",
	"/dev/ieee1394/dv/host2/PAL/out"
	},{
	"/dev/ieee1394/dv/host3/NTSC/in",
	"/dev/ieee1394/dv/host3/NTSC/out",
	"/dev/ieee1394/dv/host3/PAL/in",
	"/dev/ieee1394/dv/host3/PAL/out"
	}
};

static int device_count[4] = {0,0,0,0};

/** Allocate a new DV transmission unit.

    \param dv1394d_fd The file descriptor of a dv1394 device file to 
                      use for transmission.
    \param guid The node GUID of the receiving device.
    \param channel The channel to use for transmission.
    \return A new dv_unit handle.
*/

dv_unit dv_unit_init( octlet_t guid, int channel )
{
	dv_unit unit = malloc( sizeof( dv_unit_t ) );
	if ( unit != NULL )
	{
		int node_id;
		
		memset( unit, 0, sizeof( dv_unit_t ) );
		unit->guid = guid;
		unit->buffer_size = 25;
		unit->is_terminated = 1;
		unit->channel = channel;
		unit->dv1394_fd = -1;
		unit->n_frames = DV1394_MAX_FRAMES / 2;
		unit->n_fill = 1;

		/* get a raw1394 handle for plug control */
		if ( ( node_id = raw1394_find_node( &(unit->raw1394), guid ) ) != -1 )
		{
			if ( dv_unit_online( unit ) == 1 )
				dv1394d_log( LOG_DEBUG, "Added online unit with GUID %08x%08x", 
					(quadlet_t) (unit->guid>>32), (quadlet_t) (unit->guid & 0xffffffff) );
			else
			{
				dv_unit_close( unit );
				unit = NULL;
			}
		}
		else
		{
			dv1394d_log( LOG_DEBUG, "Added offline unit with GUID %08x%08x", 
				(quadlet_t) (unit->guid>>32), (quadlet_t) (unit->guid & 0xffffffff) );
		}
	}
	return unit;
}

/** Allow stdin to feed the unit (redundant now that senddv has been dropped).
*/

void dv_unit_allow_stdin( dv_unit unit, int flag )
{
	unit->allow_stdin = flag;
}

/** Override the default buffer/pump size - this must be done prior to the pumps
	creation.
*/

void dv_unit_set_buffer_size( dv_unit unit, int size )
{
	if ( size > 0 )
	{
		if ( unit->pump == NULL )
			unit->buffer_size = size;
		else
			unit->buffer_size = dv_pump_resize( unit->pump, size );
	}
}

int dv_unit_get_buffer_size( dv_unit unit )
{
	return unit->buffer_size;
}

void dv_unit_set_n_frames( dv_unit unit, int size )
{
	if ( size > 0 && size <= DV1394_MAX_FRAMES / 2 )
		unit->n_frames = size;
}

int dv_unit_get_n_frames( dv_unit unit )
{
	return unit->n_frames;
}

void dv_unit_set_n_fill( dv_unit unit, int size )
{
	unit->n_fill = size;
}

int dv_unit_get_n_fill( dv_unit unit )
{
	return unit->n_fill;
}

/** Set the notifier info
*/

void dv_unit_set_notifier( dv_unit this, dv1394_notifier notifier, char *root_dir )
{
	this->notifier = notifier;
	this->root_dir = root_dir;
	dv_unit_status_communicate( this );
}

/** Communicate the current status to all threads waiting on the notifier.
*/

static void dv_unit_status_communicate( dv_unit unit )
{
	if ( unit != NULL && unit->notifier != NULL && unit->root_dir != NULL )
	{
		dv1394_status_t status;
		if ( dv_unit_get_status( unit, &status ) == 0 )
			if ( !( ( status.status == unit_playing || status.status == unit_paused ) &&
					strcmp( status.clip, "" ) && 
				    !strcmp( status.tail_clip, "" ) && 
					status.position == 0 && 
					status.in == 0 && 
					status.out == 0 ) )
				dv1394_notifier_put( unit->notifier, &status );
	}
}

/** Load a clip into the unit clearing existing play list.

    \todo error handling
    \param unit A dv_unit handle.
    \param clip The absolute file name of the clip to load.
    \param in   The starting frame (-1 for 0)
	\param out  The ending frame (-1 for maximum)
*/

dv_error_code dv_unit_load( dv_unit unit, const char *clip, long in, long out, int flush )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_error_code error = dv_player_get_error( player );
	if ( error == dv_pump_ok )
	{
		error = dv_player_replace_file( player, (char*) clip, in, out, flush );
		dv1394d_log( LOG_DEBUG, "loaded clip %s", clip );
		if ( unit->is_terminated )
			dv_unit_status_communicate( unit );
	}
	return error;
}

dv_error_code dv_unit_insert( dv_unit unit, const char *clip, int index, long in, long out )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_error_code error = dv_player_get_error( player );
	if ( error == dv_pump_ok )
	{
		error = dv_player_insert_file( player, (char*) clip, index, in, out );
		dv1394d_log( LOG_DEBUG, "inserted clip %s", clip );
		if ( unit->is_terminated )
			dv_unit_status_communicate( unit );
	}
	return error;
}

dv_error_code dv_unit_remove( dv_unit unit, int index )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_error_code error = dv_player_get_error( player );
	if ( error == dv_pump_ok )
	{
		error = dv_player_remove_clip( player, index );
		dv1394d_log( LOG_DEBUG, "removed clip %d", index );
		if ( unit->is_terminated )
			dv_unit_status_communicate( unit );
	}
	return error;
}

dv_error_code dv_unit_clean( dv_unit unit )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_error_code error = dv_player_get_error( player );
	if ( error == dv_pump_ok )
	{
		error = dv_player_clean( player );
		dv1394d_log( LOG_DEBUG, "Cleaned playlist" );
		if ( unit->is_terminated )
			dv_unit_status_communicate( unit );
	}
	return error;
}

dv_error_code dv_unit_move( dv_unit unit, int src, int dest )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_error_code error = dv_player_get_error( player );
	if ( error == dv_pump_ok )
	{
		error = dv_player_move_clip( player, src, dest );
		dv1394d_log( LOG_DEBUG, "moved clip %d to %d", src, dest );
		if ( unit->is_terminated )
			dv_unit_status_communicate( unit );
	}
	return error;
}

/** Add a clip to the unit play list.

    \todo error handling
    \param unit A dv_unit handle.
    \param clip The absolute file name of the clip to load.
    \param in   The starting frame (-1 for 0)
	\param out  The ending frame (-1 for maximum)
*/

dv_error_code dv_unit_append( dv_unit unit, const char *clip, long in, long out )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_error_code error = dv_player_add_file( player, (char*) clip, in, out );
	dv_unit_status_communicate( unit );
	return error;
}

void *output_cleanup( void *arg )
{
	dv_unit unit = arg;
	if ( unit != NULL && unit->mmap != NULL )
	{
		unit->is_terminated = 1;
		dv_unit_status_communicate( unit );
		munmap( unit->mmap, unit->mmap_length );
		/* this actually stops transmission as opposed to allowing the 
		   last frame to loop in the OHCI DMA context. */
		ioctl( unit->dv1394_fd, DV1394_SHUTDOWN, NULL );
	}

	return NULL;
}

/** The dv1394 transmission thread.

    \param arg A dv_unit handle.
*/

static void *output( void *arg )
{
	dv_unit unit = arg;
	dv_frame frames[ DV1394_MAX_FRAMES ];
	int frames_dropped = 0; /* count of total frames dropped (repeated) */
	struct dv1394_status status;
	char errstr[64];
	int n_fill = unit->n_fill;
	int n_frames = unit->n_frames;
	
	/* Determine the number of frames to wait for/fill on each iteration */
	if ( n_fill < 1 )
		n_fill = 1;
	else if ( n_fill > unit->n_frames )
		n_fill = n_frames / 2;

	unit->mmap = mmap( NULL,unit->mmap_length,PROT_WRITE,MAP_SHARED,unit->dv1394_fd,0 );
	if ( unit->mmap == MAP_FAILED || unit->mmap == NULL )
	{
		perror( "mmap" );
		return NULL;
	}

	pthread_cleanup_push( output_cleanup, (void *)arg );

	while ( dv_pump_get_available_output_count( unit->pump ) || 
		!( dv_unit_has_terminated( unit ) || dv_pump_has_terminated( unit->pump) ) )
	{
		int available = 0;

		if ( ioctl( unit->dv1394_fd, DV1394_WAIT_FRAMES, n_fill ) < 0)
			perror( "DV1394_WAIT_FRAMES" );

		pthread_testcancel();

		/* update the status for the next iteration and detect dropped frames */
		if ( ioctl( unit->dv1394_fd, DV1394_GET_STATUS, &status ) >= 0)
		{
			pthread_testcancel();

			/*
			printf( "dv1394 status: active=%02d, #clear=%02d, first clear=%02d\n", 
				status.active_frame, status.n_clear_frames, status.first_clear_frame);
			*/
		
			/* report dropped frames */
			if( status.dropped_frames > 0 )
			{
				frames_dropped += status.dropped_frames;
				dv1394d_log( LOG_WARNING, "dv1394 repeated %d frames with %d available.", 
							 status.dropped_frames, dv_pump_get_available_output_count( unit->pump ) );
			}

			available = dv_pump_get_output_block( unit->pump, (void **)frames, n_fill );

			dv_unit_status_communicate( unit );
			
			/* The only time we get 0 frames is when the unit is being stopped. */
			if ( available != 0 )
			{
				int size = dv_frame_size( frames[ 0 ] );
				int pos = status.first_clear_frame;
				int index = 0;

				for ( index = 0; index < available; index ++ )
					memcpy( unit->mmap + ( ( pos + index ) % n_frames ) * size, dv_frame_data( frames[ index ] ), size );

				if ( ioctl( unit->dv1394_fd, DV1394_SUBMIT_FRAMES, available ) >= 0)
				{
					for ( index = 0; index < available - 1; index ++ )
					{
						dv_frame_clear_error( frames[ index ] );
						dv_frame_id_clear( dv_frame_get_id( frames[ index ] ) );
					}			
					dv_pump_return_output_block( unit->pump );
					pthread_testcancel();
				}
				else
				{
					dv1394d_log( LOG_ERR, "failed to write frames to dv1394: %s.", strerror_r( errno, errstr, 63 ) );
					dv_pump_terminate( unit->pump );
					dv_pump_flush( unit->pump );
					pthread_testcancel();
				}
			}
		}
		else
		{
			dv1394d_log( LOG_ERR, "failed to get dv1394 status: %s.", strerror_r( errno, errstr, 63 ) );
			dv_pump_return_used_output( unit->pump );
		}
	}

	if ( frames_dropped > 0 )
		dv1394d_log( LOG_WARNING, "dv1394 repeated %d frames total during this transmission.", frames_dropped );

	pthread_cleanup_pop( 1 );

	return NULL;
}

/** Start playing the clip.

    Start a dv-pump and commence dv1394 transmission.

    \todo error handling
    \param unit A dv_unit handle.
    \param speed An integer that specifies the playback rate as a
                 percentage multiplied by 100.
*/

void dv_unit_play( dv_unit_t *unit, int speed )
{
	dv_player player = dv_unit_get_dv_player( unit );

	if ( unit->is_terminated == 1 && ( dv_player_get_total_frames( player ) > 0 || unit->allow_stdin ) )
	{
		int retval;
		dv_frame frame = NULL;
		struct dv1394_init setup =
		{
			api_version: DV1394_API_VERSION,
			channel: unit->channel,
			/* this only sets the *requested* size of the ringbuffer,
			   in frames */ 
			n_frames: unit->n_frames,
			/* we set the format later */
			cip_n: unit->dv1394_cip_n,
			cip_d: unit->dv1394_cip_d,
			syt_offset: unit->dv1394_syt_offset
		};
		pthread_attr_t attr;

		if ( unit->in == NULL )
		{
			if ( !unit->allow_stdin || dv_player_get_total_frames( player ) != 0 )
				unit->in = dv_player_get_dv_input( player );
			else
				unit->in = dv_input_init( unit->pump );
		}
		else
		{
			dv_input_join_thread( unit->in );
			pthread_join( unit->out, NULL );
		}

		unit->is_terminated = 0;
		dv_pump_restart( unit->pump );
		dv_input_start_thread( unit->in );
		dv_player_set_speed( player, (double) speed/1000.0 );

		/* first we read a little data to see if this is PAL or NTSC
		   so we can initialize dv1394 properly */
		frame = dv_pump_get_available_output( unit->pump );
	
		/* initialize dv1394 */
		setup.format = dv_frame_is_pal(frame) ? DV1394_PAL : DV1394_NTSC;
		
		retval = ioctl( unit->dv1394_fd, DV1394_INIT, &setup );
		if (retval < 0)
		{
			perror( "DV1394_INIT" );
			return;
		}

		unit->mmap_length = unit->n_frames * dv_frame_size( frame );

		pthread_attr_init( &attr );
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
		pthread_attr_setinheritsched( &attr, PTHREAD_INHERIT_SCHED );
		pthread_create( &unit->out, &attr, output, unit );
	}
	else 
	{
		dv_player_set_speed( player, (double) speed/1000.0 );
	}
	dv_unit_status_communicate( unit );
}

/** Stop playback.

    Terminates the dv_pump and halts dv1394 transmission.

    \param unit A dv_unit handle.
*/

void dv_unit_terminate( dv_unit unit )
{
	unit->is_terminated = 1;
	if ( unit->pump != NULL )
	{
		dv_pump_terminate( unit->pump );
		dv_pump_flush( unit->pump );
	}
}

/** Query the status of unit playback.

    \param unit A dv_unit handle.
    \return 1 if the unit is not playing, 0 if playing.
*/

int dv_unit_has_terminated( dv_unit unit )
{
	return unit->is_terminated;
}

/** Get the dv_player from the dv_unit.

    \param unit A dv_unit handle.
    \return A dv_player handle.
*/

dv_player dv_unit_get_dv_player( dv_unit unit )
{
	if ( unit != NULL )
	{
		if ( unit->pump == NULL )
		{
			unit->pump = dv_pump_init( unit->buffer_size );
			if ( unit->pump != NULL )
				unit->player = dv_player_init( unit->pump );
		}
		return unit->player;
	}
	return NULL;
}


/** Transfer the currently loaded clip to another unit
*/

int dv_unit_transfer( dv_unit dest_unit, dv_unit src_unit )
{
	dv_player src_player = dv_unit_get_dv_player( src_unit );
	dv_player dest_player = dv_unit_get_dv_player( dest_unit );

	if( dest_player != NULL && src_player != NULL )
		dv_player_replace_player( dest_player, src_player );

	return 0;
}

/** Get the guid associated to this unit.
*/

octlet_t dv_unit_get_guid( dv_unit unit )
{
	return unit->guid;
}

/** Get the node id associated to this unit.
*/

int dv_unit_get_nodeid( dv_unit unit )
{
	return (unit->node_id & 0x3f);
}

/** Get the channel associated to this unit.
*/

int dv_unit_get_channel( dv_unit unit )
{
	return (unit->channel);
}

/** Turn unit online.
*/

int dv_unit_online( dv_unit unit )
{
	int result = 0;
	int port, node_id;
	
	if ( unit->raw1394 != NULL )
		raw1394_close( unit->raw1394 );
	
	node_id = raw1394_find_node( &(unit->raw1394), unit->guid );
	if ( node_id != -1 )
	{
		unit->node_id = 0xffc0 | node_id;	
		port = dv_unit_get_port( unit );
	
		unit->dv1394_fd = open( devices[ port ][ device_count[port] ], O_RDWR );
		if ( unit->dv1394_fd < 0 )
		{
			dv1394d_log( LOG_ERR, "failed to open dv1394 device - %s\n", devices[ port ][ device_count[port] ] );
			dv_unit_close( unit );
		}
		else
		{
			device_count[ port ] ++;
			if ( establish_p2p_connection( unit->raw1394, unit->node_id, (unsigned int *) &(unit->channel) ) )
			{
				avc1394_vcr_record( unit->raw1394, unit->node_id );
				unit->online = 1;
				dv_unit_status_communicate( unit );
				result = 1;
			}
		}
	}
				
	return result;
}

/** Turn unit offline.
*/

void dv_unit_offline( dv_unit unit )
{
	if ( unit->online == 1 )
	{
		if ( unit->is_terminated == 0 )
			dv_unit_terminate( unit );
		unit->online = 0;
		if ( unit->raw1394 != NULL )
		{
			avc1394_vcr_stop( unit->raw1394, unit->node_id );
			break_p2p_connection( unit->raw1394, unit->node_id, unit->channel );
		}
		if ( unit->dv1394_fd > -1 )
		{
			close( unit->dv1394_fd );
			device_count[ dv_unit_get_port( unit ) ] --;
		}
		dv_unit_status_communicate( unit );
		dv1394d_log( LOG_DEBUG, "Unit with GUID %08x%08x is now offline.",
			(quadlet_t) (unit->guid>>32), (quadlet_t) (unit->guid & 0xffffffff) );
	}
}

/** Determine if unit is offline.
*/

int dv_unit_is_offline( dv_unit unit )
{
	return (unit->online == 0);
}

/** Obtain the status for a given unit
*/

int dv_unit_get_status( dv_unit unit, dv1394_status status )
{
	int error = -1;

	memset( status, 0, sizeof( dv1394_status_t ) );

	if ( unit != NULL )
	{
		dv_player player = dv_unit_get_dv_player( unit );

		error = 0;

		if ( player != NULL )
		{
			dv_frame head = dv_pump_get_head( player->pump );
			dv_frame tail = dv_pump_get_tail( player->pump );

			status->speed = (int)( dv_player_get_speed( player ) * 1000.0 );
			status->fps = dv_player_frames_per_second( player, 0 );

			if ( head != NULL )
			{
				dv_frame_id id = dv_frame_get_id( head );
				if ( id->resource != NULL )
				{
					const char *resource = id->resource;
					if ( resource != NULL && unit->root_dir != NULL )
						resource += strlen( unit->root_dir ) - ( unit->root_dir[ strlen( unit->root_dir ) - 1 ] == '/' );
					strncpy( status->clip, resource, sizeof( status->clip ) );
				}
				else
				{
					char *title = dv_player_get_name( player, dv_player_get_clip_containing( player, 0 ), unit->root_dir );
					if ( title != NULL )
						strncpy( status->clip, title, sizeof( status->clip ) );
				}

				status->position = id->relative;
				status->in = id->in;
				status->out = id->out;
				status->length = id->length;
				status->seek_flag = id->seek_flag;
			}
			else
			{
				char *title = dv_player_get_name( player, dv_player_get_clip_containing( player, 0 ), unit->root_dir );
				if ( title != NULL )
					strncpy( status->clip, title, sizeof( status->clip ) );
			}

			if ( tail != NULL )
			{
				dv_frame_id id = dv_frame_get_id( tail );
				const char *resource = id->resource;
				if ( resource != NULL && unit->root_dir != NULL )
					resource += strlen( unit->root_dir ) - ( unit->root_dir[ strlen( unit->root_dir ) - 1 ] == '/' );
				if ( resource != NULL )
					strncpy( status->tail_clip, resource, sizeof( status->clip ) );
				status->tail_position = id->relative;
				status->tail_in = id->in;
				status->tail_out = id->out;
				status->tail_length = id->length;
			}
			
			status->generation = player->generation;
			status->clip_index = dv_unit_get_current_clip( unit );
		}

		if ( dv_unit_is_offline( unit ) )
			status->status = unit_offline;
		else if ( !strcmp( status->clip, "" ) )
			status->status = unit_not_loaded;
		else if ( dv_unit_has_terminated( unit ) )
			status->status = unit_stopped;
		else if ( status->speed == 0 )
			status->status = unit_paused;
		else
			status->status = unit_playing;
	}
	else
	{
		status->status = unit_undefined;
	}

	status->unit = unit->unit;

	return error;
}

/** Change position in the playlist.
*/

void dv_unit_change_position( dv_unit unit, int clip, long position )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_player_set_clip_position( player, clip, position );
	dv_unit_status_communicate( unit );
}

/** Change speed.
*/

void dv_unit_change_speed( dv_unit unit, int speed )
{
	if ( dv_unit_has_terminated( unit ) )
		dv_unit_change_position( unit, 0, 0 );
	else
		dv_unit_play( unit, speed );
}

int	dv_unit_get_current_clip( dv_unit unit )
{
	dv_player player = dv_unit_get_dv_player( unit );
	unsigned long position = dv_player_get_position( player );
	return dv_player_get_clip_containing( player, position );
}

/** Set a clip's in point
*/

int dv_unit_set_clip_in( dv_unit unit, int index, long position )
{
	int error = 0;
	dv_player player = dv_unit_get_dv_player( unit );

	if ( player != NULL )
	{
		dv_unit_change_speed( unit, 0 );
		if ( dv_player_set_in_point( player, index, (unsigned long) position ) == position )
			dv_player_set_clip_position( player, index, position );
		else
			error = -2;
	}
	else
	{
		error = -1;
	}

	dv_unit_status_communicate( unit );

	return error;

}

/** Set a clip's out point.
*/

int dv_unit_set_clip_out( dv_unit unit, int index, long position )
{
	int error = 0;
	dv_player player = dv_unit_get_dv_player( unit );

	if ( player != NULL )
	{
		dv_unit_change_speed( unit, 0 );
		if ( dv_player_set_out_point( player, index, position ) == position )
			dv_player_set_clip_position( player, index, position );
		else
			error = -2;
	}
	else
	{
		error = -1;
	}

	dv_unit_status_communicate( unit );

	return error;
}

/** Step by specified position.
*/

void dv_unit_step( dv_unit unit, int offset )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_player_change_position( player, dv_seek_relative, offset );
}

/** Set the unit's clip mode regarding in and out points.
*/

void dv_unit_set_mode( dv_unit unit, dv_player_clip_mode mode )
{
	dv_player player = dv_unit_get_dv_player( unit );
	if ( player != NULL )
		dv_player_set_clip_mode( player, mode );
	dv_unit_status_communicate( unit );
}

/** Get the unit's clip mode regarding in and out points.
*/

dv_player_clip_mode dv_unit_get_mode( dv_unit unit )
{
	dv_player player = dv_unit_get_dv_player( unit );
	return dv_player_get_clip_mode( player );
}

/** Set the unit's clip mode regarding eof handling.
*/

void dv_unit_set_eof_action( dv_unit unit, dv_player_eof_action action )
{
	dv_player player = dv_unit_get_dv_player( unit );
	dv_player_set_eof_action( player, action );
	dv_unit_status_communicate( unit );
}

/** Get the unit's clip mode regarding eof handling.
*/

dv_player_eof_action dv_unit_get_eof_action( dv_unit unit )
{
	dv_player player = dv_unit_get_dv_player( unit );
	return dv_player_get_eof_action( player );
}

/** Release the unit

    \todo error handling
    \param unit A dv_unit handle.
*/

void dv_unit_close( dv_unit unit )
{
	if ( unit != NULL )
	{
		dv1394d_log( LOG_DEBUG, "closing unit..." );
		dv_unit_offline( unit );
		if ( unit->pump != NULL )
		{
			dv_pump_terminate( unit->pump );
			dv_pump_flush( unit->pump );
			dv_pump_return_used_output( unit->pump );
			dv_input_join_thread( unit->in );
			if ( !unit->is_terminated )
				pthread_join( unit->out, NULL );
			dv_pump_close( unit->pump );
			unit->pump = NULL;
		}
		raw1394_close( unit->raw1394 );
		free( unit );
		dv1394d_log( LOG_DEBUG, "... unit closed." );
	}
}

/** Get the raw1394 port associated to this unit.
*/

int dv_unit_get_port( dv_unit unit )
{
	if ( unit->raw1394 != NULL )
		return (int) raw1394_get_userdata( unit->raw1394 );
	else
		return -1;
}

/** Set the dv1394 file descriptor for the unit.
*/

void dv_unit_set_dv1394_fd( dv_unit unit, int fd )
{
	unit->dv1394_fd = fd;
}

/** Get the dv1394 syt_offset (timestamp latency) property.
*/

unsigned int dv_unit_get_syt_offset( dv_unit unit )
{
	return unit->dv1394_syt_offset;
}

/** Get the dv1394 cip_n (timing numerator) property.
*/

unsigned int dv_unit_get_cip_n( dv_unit unit )
{
	return unit->dv1394_cip_n;
}

/** Get the dv1394 cip_d (timing denominator) property.
*/

unsigned int dv_unit_get_cip_d( dv_unit unit )
{
	return unit->dv1394_cip_d;
}

/** Set the dv1394 syt_offset (timestamp latency) property.

    Stops and restarts the unit if playing.
*/

void dv_unit_set_syt_offset( dv_unit unit, unsigned int syt_offset )
{
	int restart = !unit->is_terminated;
	int speed = (int)( dv_player_get_speed( dv_unit_get_dv_player(unit) ) * 1000.0 );
	
	dv_unit_terminate( unit );
	unit->dv1394_syt_offset = syt_offset;
	if ( restart )
		dv_unit_play( unit, speed );
}
	
/** Set the dv1394 cip_n (timing numerator) property.

    Stops and restarts the unit if playing.
*/

void dv_unit_set_cip_n( dv_unit unit, unsigned int cip_n )
{
	int restart = !unit->is_terminated;
	int speed = (int)( dv_player_get_speed( dv_unit_get_dv_player(unit) ) * 1000.0 );
	
	dv_unit_terminate( unit );
	unit->dv1394_cip_n = cip_n;
	if ( restart )
		dv_unit_play( unit, speed );
}

/** Set the dv1394 cip_d (timing denominator) property.

    Stops and restarts the unit if playing.
*/

void dv_unit_set_cip_d( dv_unit unit, unsigned int cip_d )
{
	int restart = !unit->is_terminated;
	int speed = (int)( dv_player_get_speed( dv_unit_get_dv_player(unit) ) * 1000.0 );
	
	dv_unit_terminate( unit );
	unit->dv1394_cip_d = cip_d;
	if ( restart )
		dv_unit_play( unit, speed );
}

/** Terminate, but only the output thread and close dv1394.
*/

void dv_unit_suspend( dv_unit unit )
{
	if ( unit->is_terminated == 0 )
	{
		unit->is_terminated = 1;
		unit->is_suspended = 1;
		dv_pump_terminate( unit->pump );
		dv_pump_flush( unit->pump );
		pthread_cancel( unit->out );
	}
	if ( unit->dv1394_fd > -1 )
	{
		close( unit->dv1394_fd );
		device_count[ dv_unit_get_port( unit ) ] --;
	}
	unit->dv1394_fd = -1;
	dv_unit_status_communicate( unit );
}


/** Restore unit on the bus, re-open dv1394, start playback if pump is running.
*/

void dv_unit_restore( dv_unit unit )
{
	int result = 0;
	int port, node_id;
	
	if ( unit->raw1394 != NULL )
		raw1394_close( unit->raw1394 );
	
	node_id = raw1394_find_node( &(unit->raw1394), unit->guid );
	if ( node_id != -1 )
	{
		unit->node_id = 0xffc0 | node_id;	
		port = dv_unit_get_port( unit );
	
		unit->dv1394_fd = open( devices[ port ][ device_count[port] ], O_RDWR );
		if ( unit->dv1394_fd < 0 )
		{
			dv1394d_log( LOG_ERR, "failed to open dv1394 device - %s\n", devices[ port ][ device_count[port] ] );
			dv_unit_close( unit );
		}
		else
		{
			device_count[ port ] ++;
			break_p2p_connection( unit->raw1394, unit->node_id, unit->channel );
			if ( establish_p2p_connection( unit->raw1394, unit->node_id, (unsigned int *) &(unit->channel) ) )
			{
				avc1394_vcr_record( unit->raw1394, unit->node_id );
				unit->online = 1;
				result = 1;
			}
		}
	}
	if ( unit->is_suspended == 1 )
	{
		int retval;
		dv_frame frame = dv_pump_get_available_output( unit->pump );
		struct dv1394_init setup =
		{
			api_version: DV1394_API_VERSION,
			channel: unit->channel,
			/* this only sets the *requested* size of the ringbuffer,
			   in frames */ 
			n_frames: unit->n_frames,
			format: dv_frame_is_pal(frame) ? DV1394_PAL : DV1394_NTSC,
			cip_n: unit->dv1394_cip_n,
			cip_d: unit->dv1394_cip_d,
			syt_offset: unit->dv1394_syt_offset
		};
		pthread_attr_t attr;

		dv_input_join_thread( unit->in );
		unit->is_terminated = 0;
		unit->is_suspended = 0;
		dv_pump_restart( unit->pump );
		dv_input_start_thread( unit->in );
		
		/* initialize dv1394 */
		retval = ioctl( unit->dv1394_fd, DV1394_INIT, &setup );
		if ( retval < 0 )
			return;
		
		pthread_attr_init( &attr );
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
		pthread_attr_setinheritsched( &attr, PTHREAD_INHERIT_SCHED );
		/* pthread_attr_setschedpolicy( &attr, SCHED_RR ); */
		pthread_create( &unit->out, &attr, output, unit );
	}
	dv_unit_status_communicate( unit );
}
