/*
 * producer_libdv.c -- a libxml2 parser of mlt service networks
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include "producer_westley.h"
#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/parser.h>

static mlt_producer parse_westley( char *file )
{
	return NULL;
}

mlt_producer producer_westley_init( char *filename )
{
	int i;
	int track = 0;
	mlt_producer producer = NULL;
	mlt_playlist playlist = mlt_playlist_init( );
	mlt_properties group = mlt_properties_new( );
	mlt_properties properties = group;
	mlt_field field = mlt_field_init( );
	mlt_properties field_properties = mlt_field_properties( field );
	mlt_multitrack multitrack = mlt_field_multitrack( field );

	// We need to track the number of registered filters
	mlt_properties_set_int( field_properties, "registered", 0 );

    // Parse
    producer = parse_westley( filename );
    
    // TODO
	
	// Connect producer to playlist
	if ( producer != NULL )
		mlt_playlist_append( playlist, producer );

	// We must have a producer at this point
	if ( mlt_playlist_count( playlist ) > 0 )
	{
		// Connect multitrack to producer
		mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist ), track );
	}

	mlt_tractor tractor = mlt_field_tractor( field );
	mlt_producer prod = mlt_tractor_producer( tractor );
	mlt_properties props = mlt_tractor_properties( tractor );
	mlt_properties_set( mlt_producer_properties( prod ), "resource", filename );
	mlt_properties_set_data( props, "multitrack", multitrack, 0, NULL, NULL );
	mlt_properties_set_data( props, "field", field, 0, NULL, NULL );
	mlt_properties_set_data( props, "group", group, 0, NULL, NULL );
	mlt_properties_set_position( props, "length", mlt_producer_get_out( mlt_multitrack_producer( multitrack ) ) + 1 );
	mlt_producer_set_in_and_out( prod, 0, mlt_producer_get_out( mlt_multitrack_producer( multitrack ) ) );
	mlt_properties_set_double( props, "fps", mlt_producer_get_fps( mlt_multitrack_producer( multitrack ) ) );

	return mlt_tractor_producer( tractor );
}

