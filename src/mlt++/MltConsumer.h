/**
 * MltConsumer.h - MLT Wrapper
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

#ifndef MLTPP_CONSUMER_H
#define MLTPP_CONSUMER_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltService.h"

namespace Mlt
{
	class Service;
	class Profile;

	class MLTPP_DECLSPEC Consumer : public Service
	{
		private:
			mlt_consumer instance;
		public:
			Consumer( );
			Consumer( Profile& profile );
			Consumer( Profile& profile, const char *id , const char *service = NULL );
			Consumer( mlt_profile profile, const char *id , const char *service = NULL );
			Consumer( Service &consumer );
			Consumer( Consumer &consumer );
			Consumer( mlt_consumer consumer );
			virtual ~Consumer( );
			virtual mlt_consumer get_consumer( );
			mlt_service get_service( );
			virtual int connect( Service &service );
			int run( );
			int start( );
			void purge( );
			int stop( );
			bool is_stopped( );
			int position( );
	};
}

#endif
