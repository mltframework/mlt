/**
 * MltPlaylist.cpp - MLT Wrapper
 * Copyright (C) 2004-2005 Charles Yates
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
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

#include <string.h>
#include <stdlib.h>
#include "MltPlaylist.h"
#include "MltTransition.h"
using namespace Mlt;

ClipInfo::ClipInfo( mlt_playlist_clip_info *info ) :
	clip( info->clip ),
	producer( new Producer( info->producer ) ),
	cut( new Producer( info->cut ) ),
	start( info->start ),
	resource( strdup( info->resource ) ),
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

Playlist::Playlist( ) :
	instance( NULL )
{
	instance = mlt_playlist_init( );
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

int Playlist::blank( int length )
{
	return mlt_playlist_blank( get_playlist( ), length );
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

ClipInfo *Playlist::clip_info( int index )
{
	mlt_playlist_clip_info info;
	mlt_playlist_get_clip_info( get_playlist( ), &info, index );
	return new ClipInfo( &info );
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

int Playlist::join( int clip, int count, int merge )
{
	return mlt_playlist_join( get_playlist( ), clip, count, merge );
}

int Playlist::mix( int clip, int length, Transition *transition )
{
	return mlt_playlist_mix( get_playlist( ), clip, length, transition == NULL ? NULL : transition->get_transition( ) );
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
	return new Producer( mlt_playlist_get_clip( get_playlist( ), clip ) );
}

bool Playlist::is_mix( int clip )
{
	return mlt_playlist_clip_is_mix( get_playlist( ), clip ) != 0;
}

