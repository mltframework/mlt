/**
 * MltGeometry.h - MLT Wrapper
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

#ifndef MLTPP_GEOMETRY_H
#define MLTPP_GEOMETRY_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt
{
	// Just for consistent naming purposes
	class MLTPP_DECLSPEC GeometryItem 
	{
		private:
			struct mlt_geometry_item_s item;
		public:
			mlt_geometry_item get_item( ) { return &item; }
			bool key( ) { return item.key != 0; }
			int frame( ) { return item.frame; }
			void frame( int value ) { item.frame = value; }
			float x( ) { return item.x; }
			void x( float value ) { item.f[0] = 1; item.x = value; }
			float y( ) { return item.y; }
			void y( float value ) { item.f[1] = 1; item.y = value; }
			float w( ) { return item.w; }
			void w( float value ) { item.f[2] = 1; item.w = value; }
			float h( ) { return item.h; }
			void h( float value ) { item.f[3] = 1; item.h = value; }
			float mix( ) { return item.mix; }
			void mix( float value ) { item.f[4] = 1; item.mix = value; }
	};

	class MLTPP_DECLSPEC Geometry
	{
		private:
			mlt_geometry geometry;
		public:
			Geometry( char *data = NULL, int length = 0, int nw = -1, int nh = -1 );
			~Geometry( );
			int parse( char *data, int length, int nw = -1, int nh = -1 );
			// Fetch a geometry item for an absolute position
			int fetch( GeometryItem &item, float position );
			int fetch( GeometryItem *item, float position );
			// Specify a geometry item at an absolute position
			int insert( GeometryItem &item );
			int insert( GeometryItem *item );
			// Remove the key at the specified position
			int remove( int position );
			void interpolate( );
			// Get the key at the position or the next following
			int next_key( GeometryItem &item, int position );
			int next_key( GeometryItem *item, int position );
			int prev_key( GeometryItem &item, int position );
			int prev_key( GeometryItem *item, int position );
			// Serialise the current geometry
			char *serialise( int in, int out );
			char *serialise( );
	};
}

#endif

