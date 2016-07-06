/**
 * MltParser.h - MLT Wrapper
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

#ifndef MLTPP_PARSER_H
#define MLTPP_PARSER_H

#include "MltConfig.h"

#include <framework/mlt.h>
#include "MltProperties.h"

namespace Mlt
{
	class Properties;
	class Service;
	class Producer;
	class Playlist;
	class Tractor;
	class Multitrack;
	class Filter;
	class Transition;

	class MLTPP_DECLSPEC Parser : public Properties
	{
		private:
			mlt_parser parser;
		public:
			Parser( );
			~Parser( );
			int start( Service &service );
			virtual mlt_properties get_properties( );	
			virtual int on_invalid( Service *object );
			virtual int on_unknown( Service *object );
			virtual int on_start_producer( Producer *object );
			virtual int on_end_producer( Producer *object );
			virtual int on_start_playlist( Playlist *object );
			virtual int on_end_playlist( Playlist *object );
			virtual int on_start_tractor( Tractor *object );
			virtual int on_end_tractor( Tractor *object );
			virtual int on_start_multitrack( Multitrack *object );
			virtual int on_end_multitrack( Multitrack *object );
			virtual int on_start_track( );
			virtual int on_end_track( );
			virtual int on_start_filter( Filter *object );
			virtual int on_end_filter( Filter *object );
			virtual int on_start_transition( Transition *object );
			virtual int on_end_transition( Transition *object );
	};
}

#endif
