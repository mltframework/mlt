/*
 * mlt_playlist.h -- playlist service class
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

#ifndef _MLT_PLAYLIST_H_
#define _MLT_PLAYLIST_H_

#include "mlt_producer.h"

/** Structure for returning clip information.
*/

typedef struct
{
	int clip;
	mlt_producer producer;
	mlt_service service;
	mlt_position start;
	char *resource;
	mlt_position frame_in;
	mlt_position frame_out;
	mlt_position frame_count;
	mlt_position length;
	float fps;
}
mlt_playlist_clip_info;

/** Public final methods
*/

extern mlt_playlist mlt_playlist_init( );
extern mlt_producer mlt_playlist_producer( mlt_playlist self );
extern mlt_service mlt_playlist_service( mlt_playlist self );
extern mlt_properties mlt_playlist_properties( mlt_playlist self );
extern int mlt_playlist_count( mlt_playlist self );
extern int mlt_playlist_clear( mlt_playlist self );
extern int mlt_playlist_append( mlt_playlist self, mlt_producer producer );
extern int mlt_playlist_append_io( mlt_playlist self, mlt_producer producer, mlt_position in, mlt_position out );
extern int mlt_playlist_blank( mlt_playlist self, mlt_position length );
extern mlt_position mlt_playlist_clip( mlt_playlist self, mlt_whence whence, int index );
extern int mlt_playlist_current_clip( mlt_playlist self );
extern mlt_producer mlt_playlist_current( mlt_playlist self );
extern int mlt_playlist_get_clip_info( mlt_playlist self, mlt_playlist_clip_info *info, int index );
extern int mlt_playlist_insert( mlt_playlist self, mlt_producer producer, int where, mlt_position in, mlt_position out );
extern int mlt_playlist_remove( mlt_playlist self, int where );
extern int mlt_playlist_move( mlt_playlist self, int from, int to );
extern int mlt_playlist_resize_clip( mlt_playlist self, int clip, mlt_position in, mlt_position out );
extern int mlt_playlist_split( mlt_playlist self, int clip, mlt_position position );
extern int mlt_playlist_join( mlt_playlist self, int clip, int count, int merge );
extern void mlt_playlist_close( mlt_playlist self );

#endif

