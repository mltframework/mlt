/**
 * MltPlaylist.cpp - MLT Wrapper
 * Copyright (C) 2004-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@gmail.com>
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

#include <string.h>
#include <stdlib.h>
#include "MltPlaylist.h"
#include "MltTransition.h"
#include "MltProfile.h"
using namespace Mlt;

ClipInfo::ClipInfo( ) :
	clip( 0 ),
	producer( NULL ),
	cut( NULL ),
	start( 0 ),
	resource( NULL ),
	frame_in( 0 ),
	frame_out( 0 ),
	frame_count( 0 ),
	length( 0 ),
	fps( 0 ),
	repeat( 0 )
{
}

ClipInfo::ClipInfo( mlt_playlist_clip_info *info ) :
	clip( info->clip ),
	producer( new Producer( info->producer ) ),
	cut( new Producer( info->cut ) ),
	start( info->start ),
	resource( info->resource? strdup( info->resource )  : 0 ),
	frame_in( info->frame_in ),
	frame_out( info->frame_out ),
	frame_count( info->frame_count ),
	length( info->length ),
	fps( info->fps ),
	repeat( info->repeat )
{
}

ClipInfo::~ClipInfo( )
{
	delete producer;
	delete cut;
	free( resource );
}

void ClipInfo::update( mlt_playlist_clip_info *info )
{
	delete producer;
	delete cut;
	free( resource );
	clip = info->clip;
	producer = new Producer( info->producer );
	cut = new Producer( info->cut );
	start = info->start;
	resource = info->resource ? strdup( info->resource ) : 0;
	frame_in = info->frame_in;
	frame_out = info->frame_out;
	frame_count = info->frame_count;
	length = info->length;
	fps = info->fps;
	repeat = info->repeat;
}

Playlist::Playlist( ) :
	instance( NULL )
{
	instance = mlt_playlist_init( );
}

Playlist::Playlist( Profile& profile ) :
	instance( NULL )
{
	instance = mlt_playlist_new( profile.get_profile() );
}

Playlist::Playlist( Service &producer ) :
	instance( NULL )
{
	if ( producer.type( ) == playlist_type )
	{
		instance = ( mlt_playlist )producer.get_service( );
		inc_ref( );
	}
}

Playlist::Playlist( Playlist &playlist ) :
	Mlt::Producer( playlist ),
	instance( playlist.get_playlist( ) )
{
	inc_ref( );
}

Playlist::Playlist( mlt_playlist playlist ) :
	instance( playlist )
{
	inc_ref( );
}

Playlist::~Playlist( )
{
	mlt_playlist_close( instance );
}

mlt_playlist Playlist::get_playlist( )
{
	return instance;
}

mlt_producer Playlist::get_producer( )
{
	return mlt_playlist_producer( get_playlist( ) );
}

int Playlist::count( )
{
	return mlt_playlist_count( get_playlist( ) );
}

int Playlist::clear( )
{
	return mlt_playlist_clear( get_playlist( ) );
}

int Playlist::append( Producer &producer, int in, int out )
{
	return mlt_playlist_append_io( get_playlist( ), producer.get_producer( ), in, out );
}

int Playlist::blank( int out )
{
	return mlt_playlist_blank( get_playlist( ), out );
}

int Playlist::blank( const char *length )
{
	return mlt_playlist_blank_time( get_playlist( ), length );
}

int Playlist::clip( mlt_whence whence, int index )
{
	return mlt_playlist_clip( get_playlist( ), whence, index );
}

int Playlist::current_clip( )
{
	return mlt_playlist_current_clip( get_playlist( ) );
}

Producer *Playlist::current( )
{
	return new Producer( mlt_playlist_current( get_playlist( ) ) );
}

ClipInfo *Playlist::clip_info( int index, ClipInfo *info )
{
	mlt_playlist_clip_info clip_info;
	if ( mlt_playlist_get_clip_info( get_playlist( ), &clip_info, index ) )
		return NULL;
	if ( info == NULL )
		return new ClipInfo( &clip_info );
	info->update( &clip_info );
	return info;
}

void Playlist::delete_clip_info( ClipInfo *info )
{
	delete info;
}

int Playlist::insert( Producer &producer, int where, int in, int out )
{
	return mlt_playlist_insert( get_playlist( ), producer.get_producer( ), where, in, out );
}

int Playlist::remove( int where )
{
	return mlt_playlist_remove( get_playlist( ), where );
}

int Playlist::move( int from, int to )
{
	return mlt_playlist_move( get_playlist( ), from, to );
}

int Playlist::resize_clip( int clip, int in, int out )
{
	return mlt_playlist_resize_clip( get_playlist( ), clip, in, out );
}

int Playlist::split( int clip, int position )
{
	return mlt_playlist_split( get_playlist( ), clip, position );
}

int Playlist::split_at( int position, bool left )
{
	return mlt_playlist_split_at( get_playlist( ), position, left );
}

int Playlist::join( int clip, int count, int merge )
{
	return mlt_playlist_join( get_playlist( ), clip, count, merge );
}

int Playlist::mix( int clip, int length, Transition *transition )
{
	return mlt_playlist_mix( get_playlist( ), clip, length, transition == NULL ? NULL : transition->get_transition( ) );
}

int Playlist::mix_in(int clip, int length)
{
	return mlt_playlist_mix_in( get_playlist( ), clip, length );
}

int Playlist::mix_out(int clip, int length)
{
	return mlt_playlist_mix_out( get_playlist( ), clip, length );
}

int Playlist::mix_add( int clip, Transition *transition )
{
	return mlt_playlist_mix_add( get_playlist( ), clip, transition == NULL ? NULL : transition->get_transition( ) );
}

int Playlist::repeat( int clip, int count )
{
	return mlt_playlist_repeat_clip( get_playlist( ), clip, count );
}

Producer *Playlist::get_clip( int clip )
{
	mlt_producer producer = mlt_playlist_get_clip( get_playlist( ), clip );
	return producer != NULL ? new Producer( producer ) : NULL;
}

Producer *Playlist::get_clip_at( int position )
{
	mlt_producer producer = mlt_playlist_get_clip_at( get_playlist( ), position );
	return producer != NULL ? new Producer( producer ) : NULL;
}

int Playlist::get_clip_index_at( int position )
{
	return mlt_playlist_get_clip_index_at( get_playlist( ), position );
}

bool Playlist::is_mix( int clip )
{
	return mlt_playlist_clip_is_mix( get_playlist( ), clip ) != 0;
}

bool Playlist::is_blank( int clip )
{
	return mlt_playlist_is_blank( get_playlist( ), clip ) != 0;
}

bool Playlist::is_blank_at( int position )
{
	return mlt_playlist_is_blank_at( get_playlist( ), position ) != 0;
}

Producer *Playlist::replace_with_blank( int clip )
{
	mlt_producer producer = mlt_playlist_replace_with_blank( get_playlist( ), clip );
	Producer *object = producer != NULL ? new Producer( producer ) : NULL;
	mlt_producer_close( producer );
	return object;
}

void Playlist::consolidate_blanks( int keep_length )
{
	return mlt_playlist_consolidate_blanks( get_playlist( ), keep_length );
}

void Playlist::insert_blank(int clip, int out )
{
	mlt_playlist_insert_blank( get_playlist( ), clip, out );
}

void Playlist::pad_blanks( int position, int length, int find )
{
	mlt_playlist_pad_blanks( get_playlist( ), position, length, find );
}

int Playlist::insert_at( int position, Producer *producer, int mode )
{
	return mlt_playlist_insert_at( get_playlist( ), position, producer->get_producer( ), mode );
}

int Playlist::insert_at( int position, Producer &producer, int mode )
{
	return mlt_playlist_insert_at( get_playlist( ), position, producer.get_producer( ), mode );
}

int Playlist::clip_start( int clip )
{
	return mlt_playlist_clip_start( get_playlist( ), clip );
}

int Playlist::blanks_from( int clip, int bounded )
{
	return mlt_playlist_blanks_from( get_playlist( ), clip, bounded );
}

int Playlist::clip_length( int clip )
{
	return mlt_playlist_clip_length( get_playlist( ), clip );
}

int Playlist::remove_region( int position, int length )
{
	return mlt_playlist_remove_region( get_playlist( ), position, length );
}

int Playlist::move_region( int position, int length, int new_position )
{
	return mlt_playlist_move_region( get_playlist( ), position, length, new_position );
}

