/**
 * MltFilter.h - MLT Wrapper
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

#ifndef _MLTPP_FILTER_H_
#define _MLTPP_FILTER_H_

#include <framework/mlt.h>

#include "MltService.h"

namespace Mlt
{
	class Service;

	class Filter : public Service
	{
		private:
			bool destroy;
			mlt_filter instance;
		public:
			Filter( char *id, char *service = NULL );
			Filter( Filter &filter );
			Filter( mlt_filter filter );
			virtual ~Filter( );
			virtual mlt_filter get_filter( );
			mlt_service get_service( );
			int connect( Service &service, int index = 0 );
			void set_in_and_out( int in, int out );
			int get_in( );
			int get_out( );
			int get_track( );
	};
}

#endif
