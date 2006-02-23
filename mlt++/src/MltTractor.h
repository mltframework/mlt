/**
 * MltTractor.h - Tractor wrapper
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

#ifndef _MLTPP_TRACTOR_H_
#define _MLTPP_TRACTOR_H_

#include "config.h"

#include <framework/mlt.h>

#include "MltProducer.h"

namespace Mlt
{
	class Producer;
	class Field;
	class Multitrack;
	class Transition;
	class Filter;

	class MLTPP_DECLSPEC Tractor : public Producer
	{
		private:
			mlt_tractor instance;
		public:
			Tractor( );
			Tractor( Service &tractor );
			Tractor( mlt_tractor tractor );
			Tractor( Tractor &tractor );
			Tractor( char *id, char *arg = NULL );
			virtual ~Tractor( );
			virtual mlt_tractor get_tractor( );
			mlt_producer get_producer( );
			Multitrack *multitrack( );
			Field *field( );
			void refresh( );
			int set_track( Producer &producer, int index );
			Producer *track( int index );
			int count( );
			void plant_transition( Transition &transition, int a_track = 0, int b_track = 1 );
			void plant_transition( Transition *transition, int a_track = 0, int b_track = 1 );
			void plant_filter( Filter &filter, int track = 0 );
			void plant_filter( Filter *filter, int track = 0 );
			bool locate_cut( Producer *producer, int &track, int &cut );
	};
}

#endif

