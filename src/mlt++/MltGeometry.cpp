/**
 * MltGeometry.cpp - MLT Wrapper
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

#include <stdlib.h>
#include "MltGeometry.h"
using namespace Mlt;

Geometry::Geometry( char *data, int length, int nw, int nh )
{
	geometry = mlt_geometry_init( );
	parse( data, length, nw, nh );
}

Geometry::~Geometry( )
{
	mlt_geometry_close( geometry );
}

int Geometry::parse( char *data, int length, int nw, int nh )
{
	return mlt_geometry_parse( geometry, data, length, nw, nh );
}

// Fetch a geometry item for an absolute position
int Geometry::fetch( GeometryItem &item, float position )
{
	return mlt_geometry_fetch( geometry, item.get_item( ), position );
}

int Geometry::fetch( GeometryItem *item, float position )
{
	return mlt_geometry_fetch( geometry, item->get_item( ), position );
}

// Specify a geometry item at an absolute position
int Geometry::insert( GeometryItem &item )
{
	return mlt_geometry_insert( geometry, item.get_item( ) );
}

int Geometry::insert( GeometryItem *item )
{
	return mlt_geometry_insert( geometry, item->get_item( ) );
}

// Remove the key at the specified position
int Geometry::remove( int position )
{
	return mlt_geometry_remove( geometry, position );
}

void Geometry::interpolate( )
{
	mlt_geometry_interpolate( geometry );
}

// Get the key at the position or the next following
int Geometry::next_key( GeometryItem &item, int position )
{
	return mlt_geometry_next_key( geometry, item.get_item( ), position );
}

int Geometry::next_key( GeometryItem *item, int position )
{
	return mlt_geometry_next_key( geometry, item->get_item( ), position );
}

int Geometry::prev_key( GeometryItem &item, int position )
{
	return mlt_geometry_prev_key( geometry, item.get_item( ), position );
}

int Geometry::prev_key( GeometryItem *item, int position )
{
	return mlt_geometry_prev_key( geometry, item->get_item( ), position );
}

// Serialise the current geometry
char *Geometry::serialise( int in, int out )
{
	return mlt_geometry_serialise_cut( geometry, in, out );
}

char *Geometry::serialise( )
{
	return mlt_geometry_serialise( geometry );
}

