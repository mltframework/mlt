/**
 * MltMultitrack.h - Multitrack wrapper
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

#ifndef _MLTPP_MULTITRACK_H_
#define _MLTPP_MULTITRACK_H_

#include <framework/mlt.h>

#include "MltProducer.h"

namespace Mlt
{
	class Producer;

	class Multitrack : public Producer
	{
		private:
			mlt_multitrack instance;
		public:
			Multitrack( mlt_multitrack multitrack );
			Multitrack( Multitrack &multitrack );
			virtual ~Multitrack( );
			mlt_multitrack get_multitrack( );
			mlt_producer get_producer( );
			int connect( Producer &producer, int index );
			int clip( mlt_whence whence, int index );
			int count( );
			Producer *track( int index );
			void refresh( );
	};
}

#endif

