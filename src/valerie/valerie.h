/*
 * valerie.h -- High Level Client API for miracle
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

#ifndef _VALERIE_H_
#define _VALERIE_H_

/* System header files */
#include <limits.h>

/* Application header files */
#include "valerie_parser.h"
#include "valerie_status.h"
#include "valerie_notifier.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Client error conditions
*/

typedef enum
{
	valerie_ok = 0,
	valerie_malloc_failed,
	valerie_unknown_error,
	valerie_no_response,
	valerie_invalid_command,
	valerie_server_timeout,
	valerie_missing_argument,
	valerie_server_unavailable,
	valerie_unit_creation_failed,
	valerie_unit_unavailable,
	valerie_invalid_file,
	valerie_invalid_position
}
valerie_error_code;

/** Clip index specification.
*/

typedef enum
{
	valerie_absolute = 0,
	valerie_relative
}
valerie_clip_offset;

/** Client structure.
*/

typedef struct
{
	valerie_parser parser;
	valerie_response last_response;
}
*valerie, valerie_t;

/** Client API.
*/

extern valerie valerie_init( valerie_parser );

/* Connect to the valerie parser instance */
extern valerie_error_code valerie_connect( valerie );

/* Global functions */
extern valerie_error_code valerie_set( valerie, char *, char * );
extern valerie_error_code valerie_get( valerie, char *, char *, int );
extern valerie_error_code valerie_run( valerie, char * );

/* Unit functions */
extern valerie_error_code valerie_unit_add( valerie, char *, int * );
extern valerie_error_code valerie_unit_load( valerie, int, char * );
extern valerie_error_code valerie_unit_load_clipped( valerie, int, char *, int32_t, int32_t );
extern valerie_error_code valerie_unit_load_back( valerie, int, char * );
extern valerie_error_code valerie_unit_load_back_clipped( valerie, int, char *, int32_t, int32_t );
extern valerie_error_code valerie_unit_append( valerie, int, char *, int32_t, int32_t );
extern valerie_error_code valerie_unit_clean( valerie, int );
extern valerie_error_code valerie_unit_clip_move( valerie, int, valerie_clip_offset, int, valerie_clip_offset, int );
extern valerie_error_code valerie_unit_clip_remove( valerie, int, valerie_clip_offset, int );
extern valerie_error_code valerie_unit_remove_current_clip( valerie, int );
extern valerie_error_code valerie_unit_clip_insert( valerie, int, valerie_clip_offset, int, char *, int32_t, int32_t );
extern valerie_error_code valerie_unit_play( valerie, int );
extern valerie_error_code valerie_unit_play_at_speed( valerie, int, int );
extern valerie_error_code valerie_unit_stop( valerie, int );
extern valerie_error_code valerie_unit_pause( valerie, int );
extern valerie_error_code valerie_unit_rewind( valerie, int );
extern valerie_error_code valerie_unit_fast_forward( valerie, int );
extern valerie_error_code valerie_unit_step( valerie, int, int32_t );
extern valerie_error_code valerie_unit_goto( valerie, int, int32_t );
extern valerie_error_code valerie_unit_clip_goto( valerie, int, valerie_clip_offset, int, int32_t );
extern valerie_error_code valerie_unit_clip_set_in( valerie, int, valerie_clip_offset, int, int32_t );
extern valerie_error_code valerie_unit_clip_set_out( valerie, int, valerie_clip_offset, int, int32_t );
extern valerie_error_code valerie_unit_set_in( valerie, int, int32_t );
extern valerie_error_code valerie_unit_set_out( valerie, int, int32_t );
extern valerie_error_code valerie_unit_clear_in( valerie, int );
extern valerie_error_code valerie_unit_clear_out( valerie, int );
extern valerie_error_code valerie_unit_clear_in_out( valerie, int );
extern valerie_error_code valerie_unit_set( valerie, int, char *, char * );
extern valerie_error_code valerie_unit_get( valerie, int, char * );
extern valerie_error_code valerie_unit_status( valerie, int, valerie_status );
extern valerie_error_code valerie_unit_transfer( valerie, int, int );

/* Notifier functionality. */
extern valerie_notifier valerie_get_notifier( valerie );

/** Structure for the directory.
*/

typedef struct
{
	char *directory;
	valerie_response response;
}
*valerie_dir, valerie_dir_t;

/** Directory entry structure.
*/

typedef struct
{
	int dir;
	char name[ NAME_MAX ];
	char full[ PATH_MAX + NAME_MAX ];
	unsigned long long size;
}
*valerie_dir_entry, valerie_dir_entry_t;

/* Directory reading. */
extern valerie_dir valerie_dir_init( valerie, char * );
extern valerie_error_code valerie_dir_get_error_code( valerie_dir );
extern valerie_error_code valerie_dir_get( valerie_dir, int, valerie_dir_entry );
extern int valerie_dir_count( valerie_dir );
extern void valerie_dir_close( valerie_dir );

/** Structure for the list.
*/

typedef struct
{
	int generation;
	valerie_response response;
}
*valerie_list, valerie_list_t;

/** List entry structure.
*/

typedef struct
{
	int clip;
	char full[ PATH_MAX + NAME_MAX ];
	int32_t in;
	int32_t out;
	int32_t max;
	int32_t size;
	int32_t fps;
}
*valerie_list_entry, valerie_list_entry_t;

/* List reading. */
extern valerie_list valerie_list_init( valerie, int );
extern valerie_error_code valerie_list_get_error_code( valerie_list );
extern valerie_error_code valerie_list_get( valerie_list, int, valerie_list_entry );
extern int valerie_list_count( valerie_list );
extern void valerie_list_close( valerie_list );

/** Structure for nodes.
*/

typedef struct
{
	valerie_response response;
}
*valerie_nodes, valerie_nodes_t;

/** Node entry structure.
*/

typedef struct
{
	int node;
	char guid[ 17 ];
	char name[ 1024 ];
}
*valerie_node_entry, valerie_node_entry_t;

/* Node reading. */
extern valerie_nodes valerie_nodes_init( valerie );
extern valerie_error_code valerie_nodes_get_error_code( valerie_nodes );
extern valerie_error_code valerie_nodes_get( valerie_nodes, int, valerie_node_entry );
extern int valerie_nodes_count( valerie_nodes );
extern void valerie_nodes_close( valerie_nodes );

/** Structure for units.
*/

typedef struct
{
	valerie_response response;
}
*valerie_units, valerie_units_t;

/** Unit entry structure.
*/

typedef struct
{
	int unit;
	int node;
	char guid[ 512 ];
	int online;
}
*valerie_unit_entry, valerie_unit_entry_t;

/* Unit reading. */
extern valerie_units valerie_units_init( valerie );
extern valerie_error_code valerie_units_get_error_code( valerie_units );
extern valerie_error_code valerie_units_get( valerie_units, int, valerie_unit_entry );
extern int valerie_units_count( valerie_units );
extern void valerie_units_close( valerie_units );

/* Miscellaenous functions */
extern valerie_response valerie_get_last_response( valerie );
extern char *valerie_error_description( valerie_error_code );

/* Courtesy functions. */
extern valerie_error_code valerie_execute( valerie, size_t, char *, ... );

/* Close function. */
extern void valerie_close( valerie );

#ifdef __cplusplus
}
#endif

#endif
