/**
 * MltPlaylist.h - MLT Wrapper
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

#ifndef _MLTPP_PLAYLIST_H_
#define _MLTPP_PLAYLIST_H_

#include <framework/mlt.h>

#include "MltProducer.h"

namespace Mlt
{
	class Producer;
	class Service;
	class Playlist;

	class ClipInfo
	{
		public:
			ClipInfo( mlt_playlist_clip_info *info );
			~ClipInfo( );
			int clip;
			Producer *producer;
			Service *service;
			int start;
			char *resource;
			int frame_in;
			int frame_out;
			int frame_count;
			int length;
			float fps;
	};

	class Playlist : public Producer
	{
		private:
			bool destroy;
			mlt_playlist instance;
		public:
			Playlist( );
			Playlist( Playlist &playlist );
			Playlist( mlt_playlist playlist );
			virtual ~Playlist( );
			virtual mlt_playlist get_playlist( );
			mlt_producer get_producer( );
			int count( );
			int clear( );
			int append( Producer &producer, int in = -1, int out = -1 );
			int blank( int length );
			int clip( mlt_whence whence, int index );
			int current_clip( );
			Producer *current( );
			ClipInfo *clip_info( int index );
			int insert( Producer &producer, int where, int in = -1, int out = -1 );
			int remove( int where );
			int move( int from, int to );
			int resize_clip( int clip, int in, int out );
	};
}

#endif
