/**
 * MltFactory.h - MLT Wrapper
 * Copyright (C) 2004-2017 Meltytech, LLC
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

#ifndef MLTPP_FACTORY_H
#define MLTPP_FACTORY_H

#include "MltConfig.h"

#ifdef SWIG
#define MLTPP_DECLSPEC
#endif

#include <framework/mlt.h>

namespace Mlt
{
	class Properties;
	class Producer;
	class Filter;
	class Transition;
	class Consumer;
	class Profile;
	class Repository;

	class MLTPP_DECLSPEC Factory
	{
		public:
			static Repository *init( const char *directory = NULL );
			static Properties *event_object( );
			static Producer *producer( Profile& profile, char *id, char *arg = NULL );
			static Filter *filter( Profile& profile, char *id, char *arg = NULL );
			static Transition *transition( Profile& profile, char *id, char *arg = NULL );
			static Consumer *consumer( Profile& profile, char *id, char *arg = NULL );
			static void close( );
	};
}

#endif
