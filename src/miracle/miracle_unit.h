/*
 * dvunit.h -- DV Transmission Unit Header
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

#ifndef _DV_UNIT_H_
#define _DV_UNIT_H_

#include <pthread.h>
#include <libraw1394/raw1394.h>

#include <dv1394notifier.h>
#include <dv1394status.h>
#include <dvpump.h>
#include <dvplayer.h>
#include <dvinput.h>
#include <dverror.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	int        unit;
	dv_pump    pump;
	dv_player  player;
	dv_input   in;
	int		   dv1394_fd;
	int        is_terminated;
	int        is_suspended;
	pthread_t  out;
	int		   channel;
	nodeid_t   node_id;
	octlet_t   guid;
	raw1394handle_t raw1394;
	int        allow_stdin;
	int        buffer_size;
	int        online;
	dv1394_notifier notifier;
	char      *root_dir;
	unsigned int dv1394_syt_offset;
	unsigned int dv1394_cip_n;
	unsigned int dv1394_cip_d;
	unsigned int n_frames;
	unsigned int n_fill;
	uint8_t *mmap;
	int mmap_pos;
	int mmap_length;
} dv_unit_t, *dv_unit;

extern dv_unit              dv_unit_init( octlet_t guid, int channel );
extern void                 dv_unit_allow_stdin( dv_unit unit, int flag );
extern void                 dv_unit_set_buffer_size( dv_unit unit, int size );
extern int                  dv_unit_get_buffer_size( dv_unit unit );
extern void                 dv_unit_set_n_frames( dv_unit unit, int size );
extern int 					dv_unit_get_n_frames( dv_unit unit );
extern void                 dv_unit_set_n_fill( dv_unit unit, int size );
extern int 					dv_unit_get_n_fill( dv_unit unit );
extern dv_error_code        dv_unit_load( dv_unit unit, const char *clip, long in, long out, int flush );
extern dv_error_code 		dv_unit_insert( dv_unit unit, const char *clip, int index, long in, long out );
extern dv_error_code        dv_unit_append( dv_unit unit, const char *clip, long in, long out );
extern dv_error_code 		dv_unit_remove( dv_unit unit, int index );
extern dv_error_code 		dv_unit_clean( dv_unit unit );
extern dv_error_code 		dv_unit_move( dv_unit unit, int src, int dest );
extern int                  dv_unit_transfer( dv_unit dest_unit, dv_unit src_unit );
extern void                 dv_unit_play( dv_unit_t *unit, int speed );
extern void                 dv_unit_terminate( dv_unit );
extern int                  dv_unit_has_terminated( dv_unit );
extern octlet_t             dv_unit_get_guid( dv_unit unit );
extern int                  dv_unit_get_nodeid( dv_unit unit );
extern int                  dv_unit_get_channel( dv_unit unit );
extern int                  dv_unit_online( dv_unit unit );
extern void                 dv_unit_offline( dv_unit unit );
extern int                  dv_unit_is_offline( dv_unit unit );
extern void                 dv_unit_set_notifier( dv_unit, dv1394_notifier, char * );
extern int                  dv_unit_get_status( dv_unit, dv1394_status );
extern void                 dv_unit_change_position( dv_unit, int, long position );
extern void                 dv_unit_change_speed( dv_unit unit, int speed );
extern int                  dv_unit_set_clip_in( dv_unit unit, int index, long position );
extern int                  dv_unit_set_clip_out( dv_unit unit, int index, long position );
extern void                 dv_unit_set_mode( dv_unit unit, dv_player_clip_mode mode );
extern dv_player_clip_mode  dv_unit_get_mode( dv_unit unit );
extern void                 dv_unit_set_eof_action( dv_unit unit, dv_player_eof_action mode );
extern dv_player_eof_action dv_unit_get_eof_action( dv_unit unit );
extern void                 dv_unit_step( dv_unit unit, int offset );
extern void                 dv_unit_close( dv_unit unit );
extern int                  dv_unit_get_port( dv_unit unit );
extern void                 dv_unit_set_dv1394_fd( dv_unit unit, int fd );
extern unsigned int         dv_unit_get_syt_offset( dv_unit unit );
extern unsigned int         dv_unit_get_cip_n( dv_unit unit );
extern unsigned int         dv_unit_get_cip_d( dv_unit unit );
extern void                 dv_unit_set_syt_offset( dv_unit unit, unsigned int );
extern void                 dv_unit_set_cip_n( dv_unit unit, unsigned int );
extern void                 dv_unit_set_cip_d( dv_unit unit, unsigned int );
extern void                 dv_unit_suspend( dv_unit );
extern void                 dv_unit_restore( dv_unit );
extern dv_player 			dv_unit_get_dv_player( dv_unit );
extern int					dv_unit_get_current_clip( dv_unit );


#ifdef __cplusplus
}
#endif

#endif
