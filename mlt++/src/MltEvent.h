/**
 * MltEvent.h - MLT Wrapper
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

#ifndef _MLTPP_EVENT_H_
#define _MLTPP_EVENT_H_

#include <framework/mlt.h>

namespace Mlt 
{
	class Event 
	{
		private:
			mlt_event instance;
		public:
			Event( mlt_event );
			Event( Event & );
			~Event( );
			mlt_event get_event( );
			bool is_valid( );
			void block( );
			void unblock( );
	};
}

#endif

