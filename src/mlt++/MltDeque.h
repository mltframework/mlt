/**
 * MltDeque.h - MLT Wrapper
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

#ifndef MLTPP_DEQUE_H
#define MLTPP_DEQUE_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt
{
	class MLTPP_DECLSPEC Deque
	{
		private:
			mlt_deque deque;
		public:
			Deque( );
			~Deque( );
			int count( );
			int push_back( void *item );
			void *pop_back( );
			int push_front( void *item );
			void *pop_front( );
			void *peek_back( );
			void *peek_front( );
			void *peek( int index );
	};
}

#endif
