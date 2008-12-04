/**
 * \file mlt_playlist.h
 * \brief playlist service class
 *
 * Copyright (C) 2003-2008 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MLT_PLAYLIST_H_
#define _MLT_PLAYLIST_H_

#include "mlt_producer.h"

/** \brief structure for returning clip information
 */

typedef struct
{
	int clip;
	mlt_producer producer;
	mlt_producer cut;
	mlt_position start;
	char *resource;
	mlt_position frame_in;
	mlt_position frame_out;
	mlt_position frame_count;
	mlt_position length;
	float fps;
	int repeat;
}
mlt_playlist_clip_info;

/** Private definition.
*/

typedef struct playlist_entry_s playlist_entry;

/** \brief Playlist class
 *
 * \extends mlt_producer_s
 */

struct mlt_playlist_s
{
	struct mlt_producer_s parent;
	struct mlt_producer_s blank;

	int size;
	int count;
	playlist_entry **list;
};

#define MLT_PLAYLIST_PRODUCER( playlist )	( &( playlist )->parent )
#define MLT_PLAYLIST_SERVICE( playlist )	MLT_PRODUCER_SERVICE( MLT_PLAYLIST_PRODUCER( playlist ) )
#define MLT_PLAYLIST_PROPERTIES( playlist )	MLT_SERVICE_PROPERTIES( MLT_PLAYLIST_SERVICE( playlist ) )

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
extern int mlt_playlist_repeat_clip( mlt_playlist self, int clip, int repeat );
extern int mlt_playlist_split( mlt_playlist self, int clip, mlt_position position );
extern int mlt_playlist_split_at( mlt_playlist self, mlt_position position, int left );
extern int mlt_playlist_join( mlt_playlist self, int clip, int count, int merge );
extern int mlt_playlist_mix( mlt_playlist self, int clip, int length, mlt_transition transition );
extern int mlt_playlist_mix_add( mlt_playlist self, int clip, mlt_transition transition );
extern mlt_producer mlt_playlist_get_clip( mlt_playlist self, int clip );
extern mlt_producer mlt_playlist_get_clip_at( mlt_playlist self, mlt_position position );
extern int mlt_playlist_get_clip_index_at( mlt_playlist self, mlt_position position );
extern int mlt_playlist_clip_is_mix( mlt_playlist self, int clip );
extern void mlt_playlist_consolidate_blanks( mlt_playlist self, int keep_length );
extern int mlt_playlist_is_blank( mlt_playlist self, int clip );
extern int mlt_playlist_is_blank_at( mlt_playlist self, mlt_position position );
extern void mlt_playlist_insert_blank( mlt_playlist self, int clip, int length );
extern void mlt_playlist_pad_blanks( mlt_playlist self, mlt_position position, int length, int find );
extern mlt_producer mlt_playlist_replace_with_blank( mlt_playlist self, int clip );
extern int mlt_playlist_insert_at( mlt_playlist self, mlt_position position, mlt_producer producer, int mode );
extern int mlt_playlist_clip_start( mlt_playlist self, int clip );
extern int mlt_playlist_clip_length( mlt_playlist self, int clip );
extern int mlt_playlist_blanks_from( mlt_playlist self, int clip, int bounded );
extern int mlt_playlist_remove_region( mlt_playlist self, mlt_position position, int length );
extern int mlt_playlist_move_region( mlt_playlist self, mlt_position position, int length, int new_position );
extern void mlt_playlist_close( mlt_playlist self );

#endif

