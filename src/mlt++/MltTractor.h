/**
 * MltTractor.h - Tractor wrapper
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

#ifndef MLTPP_TRACTOR_H
#define MLTPP_TRACTOR_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltProducer.h"

namespace Mlt
{
	class Producer;
	class Field;
	class Multitrack;
	class Transition;
	class Filter;
	class Profile;

	class MLTPP_DECLSPEC Tractor : public Producer
	{
		private:
			mlt_tractor instance;
		public:
			Tractor( );
			Tractor( Profile& profile );
			Tractor( Service &tractor );
			Tractor( mlt_tractor tractor );
			Tractor( Tractor &tractor );
			Tractor( Profile& profile, char *id, char *arg = NULL );
			virtual ~Tractor( );
			virtual mlt_tractor get_tractor( );
			mlt_producer get_producer( );
			Multitrack *multitrack( );
			Field *field( );
			void refresh( );
			int set_track( Producer &producer, int index );
			int insert_track( Producer &producer, int index );
			int remove_track( int index );
			Producer *track( int index );
			int count( );
			void plant_transition( Transition &transition, int a_track = 0, int b_track = 1 );
			void plant_transition( Transition *transition, int a_track = 0, int b_track = 1 );
			void plant_filter( Filter &filter, int track = 0 );
			void plant_filter( Filter *filter, int track = 0 );
			bool locate_cut( Producer *producer, int &track, int &cut );
			int connect( Producer &producer );
	};
}

#endif

