/*
 * mlt_manager.h -- manager service class
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
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

#ifndef _MLT_MANAGER_H_
#define _MLT_MANAGER_H_

extern mlt_manager mlt_manager_init( );
extern mlt_producer mlt_manager_producer( mlt_manager this );
extern mlt_producer mlt_manager_properties( mlt_manager this );
extern int mlt_manager_track_count( mlt_manager this );
extern int mlt_manager_clip_count( mlt_manager this, int track );
extern int mlt_manager_append_clip( mlt_manager this, int track, mlt_producer clip );
extern int mlt_manager_append_clip_io( mlt_manager this, int track, mlt_producer clip, mlt_position in, mlt_position out );
extern int mlt_manager_append_blank( mlt_manager this, int track, int length );
extern int mlt_manager_insert_clip( mlt_manager this, int track, mlt_producer clip, mlt_position position );
extern int mlt_manager_insert_clip_io( mlt_manager this, int track, mlt_position position, mlt_producer clip, mlt_position in, mlt_position out );
extern int mlt_manager_insert_blank( mlt_manager this, int track, mlt_position position, int length );
extern int mlt_manager_remove_clip( mlt_manager this, int track, int index );
extern mlt_producer mlt_manager_get_clip( mlt_manager this, int track, int index, char *type, mlt_position *in, mlt_position *out );
extern int mlt_manager_service_count( mlt_manager this );
extern int mlt_manager_append_filter( mlt_manager this, mlt_filter that );
extern int mlt_manager_append_transition( mlt_manager this, int index, mlt_transition that );
extern int mlt_manager_insert_filter( mlt_manager this, int index, mlt_filter that );
extern int mlt_manager_insert_transition( mlt_manager this, int index, mlt_transition that );
extern int mlt_manager_remove_service( mlt_manager this, int index );
extern mlt_service mlt_manager_get_service( mlt_manager this, int index, char *type );
extern int mlt_manager_set_resource( mlt_manager this, char *resource );
extern int mlt_manager_set_type( mlt_manager this, char *type );

#endif
