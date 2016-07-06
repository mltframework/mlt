/**
 * MltMultitrack.h - Multitrack wrapper
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

#ifndef MLTPP_MULTITRACK_H
#define MLTPP_MULTITRACK_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltProducer.h"

namespace Mlt
{
	class Service;
	class Producer;

	class MLTPP_DECLSPEC Multitrack : public Producer
	{
		private:
			mlt_multitrack instance;
		public:
			Multitrack( mlt_multitrack multitrack );
			Multitrack( Service &multitrack );
			Multitrack( Multitrack &multitrack );
			virtual ~Multitrack( );
			mlt_multitrack get_multitrack( );
			mlt_producer get_producer( );
			int connect( Producer &producer, int index );
			int insert( Producer &producer, int index );
			int disconnect( int index );
			int clip( mlt_whence whence, int index );
			int count( );
			Producer *track( int index );
			void refresh( );
	};
}

#endif

