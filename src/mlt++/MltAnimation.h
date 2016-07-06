/**
 * MltAnimation.h - MLT Wrapper
 * Copyright (C) 2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef MLTPP_ANIMATION_H
#define MLTPP_ANIMATION_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt
{
	class MLTPP_DECLSPEC Animation
	{
		private:
			mlt_animation instance;
		public:
			Animation();
			Animation( mlt_animation animation );
			Animation( const Animation& );
			~Animation();

			bool is_valid() const;
			mlt_animation get_animation() const;
			Animation& operator=( const Animation& );

			int length();
			int get_item( int position, bool& is_key, mlt_keyframe_type& );
			bool is_key( int position );
			mlt_keyframe_type keyframe_type( int position );
			int next_key( int position );
			int previous_key( int position );
			int key_count();
			int key_get( int index, int& frame, mlt_keyframe_type& );
			int key_get_frame( int index );
			mlt_keyframe_type key_get_type( int index );

			void set_length( int length );
			int remove( int position );
			void interpolate();
			char* serialize_cut( int in = -1, int out = -1 );
	};
}

#endif
