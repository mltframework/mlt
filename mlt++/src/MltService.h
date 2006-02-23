/**
 * MltService.h - MLT Wrapper
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

#ifndef _MLTPP_SERVICE_H_
#define _MLTPP_SERVICE_H_

#include "config.h"

#include <framework/mlt.h>

#include "MltProperties.h"
#include "MltFrame.h"

namespace Mlt
{
	class Properties;
	class Filter;
	class Frame;

	class MLTPP_DECLSPEC Service : public Properties
	{
		private:
			mlt_service instance;
		public:
			Service( );
			Service( Service &service );
			Service( mlt_service service );
			virtual ~Service( );
			virtual mlt_service get_service( );
			void lock( );
			void unlock( );
			virtual mlt_properties get_properties( );
			int connect_producer( Service &producer, int index = 0 );
			Service *consumer( );
			Service *producer( );
			Frame *get_frame( int index = 0 );
			mlt_service_type type( );
			int attach( Filter &filter );
			int detach( Filter &filter );
			Filter *filter( int index );
	};
}

#endif
