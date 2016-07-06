/**
 * \file mlt_playlist.h
 * \brief playlist service class
 * \see mlt_playlist_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MLT_PLAYLIST_H
#define MLT_PLAYLIST_H

#include "mlt_producer.h"

/** \brief structure for returning clip information from a playlist entry
 */

typedef struct
{
	int clip;                 /**< the index of the clip within the playlist */
	mlt_producer producer;    /**< the clip's producer (or parent producer of a cut) */
	mlt_producer cut;         /**< the clips' cut producer */
	mlt_position start;       /**< the time this begins relative to the beginning of the playlist */
	char *resource;           /**< the file name or address of the clip */
	mlt_position frame_in;    /**< the clip's in point */
	mlt_position frame_out;   /**< the clip's out point */
	mlt_position frame_count; /**< the duration of the clip */
	mlt_position length;      /**< the unedited duration of the clip */
	float fps;                /**< the frame rate of the clip */
	int repeat;               /**< the number of times the clip is repeated */
}
mlt_playlist_clip_info;

/** Playlist Entry
*/

typedef struct playlist_entry_s playlist_entry;

/** \brief Playlist class
 *
 * A playlist is a sequential container of producers and blank spaces. The class provides all
 * sorts of playlist assembly and manipulation routines. A playlist is also a producer within
 * the framework.
 *
 * \extends mlt_producer_s
 * \properties \em autoclose Set this true if you are doing sequential processing and want to
 * automatically close producers as they are finished being used to free resources.
 * \properties \em meta.fx_cut Set true on a producer to indicate that it is a "fx_cut,"
 * which is a way to add filters as a playlist entry - useful only in a multitrack. See FxCut on the wiki.
 * \properties \em mix_in
 * \properties \em mix_out
 * \properties \em hide Set to 1 to hide the video (make it an audio-only track),
 * 2 to hide the audio (make it a video-only track), or 3 to hide audio and video (hidden track).
 * This property only applies when using a multitrack or transition.
 * \event \em playlist-next The playlist fires this when it moves to the next item in the list.
 * The listener receives one argument that is the index of the entry that just completed.
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
extern mlt_playlist mlt_playlist_new( mlt_profile profile );
extern mlt_producer mlt_playlist_producer( mlt_playlist self );
extern mlt_service mlt_playlist_service( mlt_playlist self );
extern mlt_properties mlt_playlist_properties( mlt_playlist self );
extern int mlt_playlist_count( mlt_playlist self );
extern int mlt_playlist_clear( mlt_playlist self );
extern int mlt_playlist_append( mlt_playlist self, mlt_producer producer );
extern int mlt_playlist_append_io( mlt_playlist self, mlt_producer producer, mlt_position in, mlt_position out );
extern int mlt_playlist_blank( mlt_playlist self, mlt_position out );
extern int mlt_playlist_blank_time( mlt_playlist self, const char *length );
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
extern int mlt_playlist_mix_in( mlt_playlist self, int clip, int length );
extern int mlt_playlist_mix_out( mlt_playlist self, int clip, int length );
extern int mlt_playlist_mix_add( mlt_playlist self, int clip, mlt_transition transition );
extern mlt_producer mlt_playlist_get_clip( mlt_playlist self, int clip );
extern mlt_producer mlt_playlist_get_clip_at( mlt_playlist self, mlt_position position );
extern int mlt_playlist_get_clip_index_at( mlt_playlist self, mlt_position position );
extern int mlt_playlist_clip_is_mix( mlt_playlist self, int clip );
extern void mlt_playlist_consolidate_blanks( mlt_playlist self, int keep_length );
extern int mlt_playlist_is_blank( mlt_playlist self, int clip );
extern int mlt_playlist_is_blank_at( mlt_playlist self, mlt_position position );
extern void mlt_playlist_insert_blank( mlt_playlist self, int clip, int out );
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

