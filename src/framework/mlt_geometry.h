/**
 * \file mlt_geometry.h
 * \brief geometry animation API (deprecated)
 * \deprecated use mlt_animation_s instead
 *
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#ifndef MLT_GEOMETRY_H
#define MLT_GEOMETRY_H

#include "mlt_types.h"

/** geometry animation item (deprecated)
 * \deprecated use mlt_animation_s instead
 */

struct mlt_geometry_item_s
{
	/* Will be 1 when this is a key frame */
	int key;
	/* The actual frame this corresponds to */
	int frame;
	/* Distort */
	int distort;
	/* x,y are upper left */
	float x, y, w, h, mix;
	/* Indicates which values are fixed */
	int f[ 5 ];
};

/** geometry object (deprecated)
 * \deprecated use mlt_animation_s instead
 */

struct mlt_geometry_s
{
	void *local;
};

/* Create a new geometry structure */
extern mlt_geometry mlt_geometry_init( );
/* Parse the geometry specification for a given length and normalised width/height (-1 for default) */
extern int mlt_geometry_parse( mlt_geometry self, char *data, int length, int nw, int nh );
/* Conditionally refresh the geometry if it's modified */
extern int mlt_geometry_refresh( mlt_geometry self, char *data, int length, int nw, int nh );
/* Get and set the length */
extern int mlt_geometry_get_length( mlt_geometry self );
extern void mlt_geometry_set_length( mlt_geometry self, int length );
/* Parse an item - doesn't affect the geometry itself but uses current information for evaluation */
/* (item->frame should be specified if not included in the data itself) */
extern int mlt_geometry_parse_item( mlt_geometry self, mlt_geometry_item item, char *data );
/* Fetch a geometry item for an absolute position */
extern int mlt_geometry_fetch( mlt_geometry self, mlt_geometry_item item, float position );
/* Specify a geometry item at an absolute position */
extern int mlt_geometry_insert( mlt_geometry self, mlt_geometry_item item );
/* Remove the key at the specified position */
extern int mlt_geometry_remove( mlt_geometry self, int position );
/* Typically, re-interpolate after a series of insertions or removals. */
extern void mlt_geometry_interpolate( mlt_geometry self );
/* Get the key at the position or the next following */
extern int mlt_geometry_next_key( mlt_geometry self, mlt_geometry_item item, int position );
extern int mlt_geometry_prev_key( mlt_geometry self, mlt_geometry_item item, int position );
/* Serialise the current geometry */
extern char *mlt_geometry_serialise_cut( mlt_geometry self, int in, int out );
extern char *mlt_geometry_serialise( mlt_geometry self );
/* Close the geometry */
extern void mlt_geometry_close( mlt_geometry self );

#endif

