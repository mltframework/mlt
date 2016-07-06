/**
 * MltService.h - MLT Wrapper
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

#ifndef MLTPP_SERVICE_H
#define MLTPP_SERVICE_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltProperties.h"
#include "MltFrame.h"

namespace Mlt
{
	class Properties;
	class Filter;
	class Frame;
	class Profile;

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
			int insert_producer( Service &producer, int index = 0 );
			int disconnect_producer( int index = 0 );
			Service *consumer( );
			Service *producer( );
			Profile *profile( );
			mlt_profile get_profile( );
			Frame *get_frame( int index = 0 );
			mlt_service_type type( );
			int attach( Filter &filter );
			int detach( Filter &filter );
			int filter_count( );
			int move_filter( int from, int to );
			Filter *filter( int index );
			void set_profile( Profile &profile );
	};
}

#endif
