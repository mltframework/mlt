/**
 * MltFilteredProducer.h - MLT Wrapper
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

#ifndef MLTPP_FILTERED_PRODUCER_H
#define MLTPP_FILTERED_PRODUCER_H

#include "MltConfig.h"

#include "MltProducer.h"
#include "MltFilter.h"
#include "MltService.h"

namespace Mlt
{
	class Producer;
	class Service;
	class Filter;
	class Profile;

	class MLTPP_DECLSPEC FilteredProducer : public Producer
	{
		private:
			Service *last;
		public:
			FilteredProducer( Profile& profile, const char *id, const char *arg = NULL );
			virtual ~FilteredProducer( );
			int attach( Filter &filter );
			int detach( Filter &filter );
	};
}

#endif

