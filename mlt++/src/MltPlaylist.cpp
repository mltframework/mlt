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

#include "MltPlaylist.h"
using namespace Mlt;

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

int Playlist::append( Producer &producer, mlt_position in, mlt_position out )
{
	return mlt_playlist_append_io( get_playlist( ), producer.get_producer( ), in, out );
}

int Playlist::blank( mlt_position length )
{
	return mlt_playlist_blank( get_playlist( ), length );
}

mlt_playlist PlaylistInstance::get_playlist( )
{
	return instance;
}

PlaylistInstance::PlaylistInstance( ) :
	destroy( true ),
	instance( NULL )
{
	instance = mlt_playlist_init( );
}

PlaylistInstance::PlaylistInstance( Playlist &playlist ) :
	destroy( false ),
	instance( playlist.get_playlist( ) )
{
}

PlaylistInstance::PlaylistInstance( mlt_playlist playlist ) :
	destroy( false ),
	instance( playlist )
{
}

PlaylistInstance::~PlaylistInstance( )
{
	if ( destroy )
		mlt_playlist_close( instance );
}

