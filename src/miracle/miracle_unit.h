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

#include <framework/mlt_properties.h>
#include <valerie/valerie.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	mlt_properties properties;
} 
miracle_unit_t, *miracle_unit;

extern miracle_unit         miracle_unit_init( int index, char *arg );
extern void 				miracle_unit_report_list( miracle_unit unit, valerie_response response );
extern void                 miracle_unit_allow_stdin( miracle_unit unit, int flag );
extern valerie_error_code   miracle_unit_load( miracle_unit unit, char *clip, int32_t in, int32_t out, int flush );
extern valerie_error_code 	miracle_unit_insert( miracle_unit unit, char *clip, int index, int32_t in, int32_t out );
extern valerie_error_code   miracle_unit_append( miracle_unit unit, char *clip, int32_t in, int32_t out );
extern valerie_error_code   miracle_unit_append_service( miracle_unit unit, mlt_service service );
extern valerie_error_code 	miracle_unit_remove( miracle_unit unit, int index );
extern valerie_error_code 	miracle_unit_clean( miracle_unit unit );
extern valerie_error_code 	miracle_unit_wipe( miracle_unit unit );
extern valerie_error_code 	miracle_unit_clear( miracle_unit unit );
extern valerie_error_code 	miracle_unit_move( miracle_unit unit, int src, int dest );
extern int                  miracle_unit_transfer( miracle_unit dest_unit, miracle_unit src_unit );
extern void                 miracle_unit_play( miracle_unit_t *unit, int speed );
extern void                 miracle_unit_terminate( miracle_unit );
extern int                  miracle_unit_has_terminated( miracle_unit );
extern int                  miracle_unit_get_nodeid( miracle_unit unit );
extern int                  miracle_unit_get_channel( miracle_unit unit );
extern int                  miracle_unit_is_offline( miracle_unit unit );
extern void                 miracle_unit_set_notifier( miracle_unit, valerie_notifier, char * );
extern int                  miracle_unit_get_status( miracle_unit, valerie_status );
extern void                 miracle_unit_change_position( miracle_unit, int, int32_t position );
extern void                 miracle_unit_change_speed( miracle_unit unit, int speed );
extern int                  miracle_unit_set_clip_in( miracle_unit unit, int index, int32_t position );
extern int                  miracle_unit_set_clip_out( miracle_unit unit, int index, int32_t position );
//extern void                 miracle_unit_set_mode( miracle_unit unit, dv_player_clip_mode mode );
//extern dv_player_clip_mode  miracle_unit_get_mode( miracle_unit unit );
//extern void                 miracle_unit_set_eof_action( miracle_unit unit, dv_player_eof_action mode );
//extern dv_player_eof_action miracle_unit_get_eof_action( miracle_unit unit );
extern void                 miracle_unit_step( miracle_unit unit, int32_t offset );
extern void                 miracle_unit_close( miracle_unit unit );
extern void                 miracle_unit_suspend( miracle_unit );
extern void                 miracle_unit_restore( miracle_unit );
extern int					miracle_unit_set( miracle_unit, char *name_value );
extern char *				miracle_unit_get( miracle_unit, char *name );
extern int					miracle_unit_get_current_clip( miracle_unit );


#ifdef __cplusplus
}
#endif

#endif
