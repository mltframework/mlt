/**
 * \file mlt_parser.c
 * \brief service parsing functionality
 * \see mlt_parser_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include "mlt.h"
#include <stdlib.h>

static int on_invalid( mlt_parser self, mlt_service object )
{
	return 0;
}

static int on_unknown( mlt_parser self, mlt_service object )
{
	return 0;
}

static int on_start_producer( mlt_parser self, mlt_producer object )
{
	return 0;
}

static int on_end_producer( mlt_parser self, mlt_producer object )
{
	return 0;
}

static int on_start_playlist( mlt_parser self, mlt_playlist object )
{
	return 0;
}

static int on_end_playlist( mlt_parser self, mlt_playlist object )
{
	return 0;
}

static int on_start_tractor( mlt_parser self, mlt_tractor object )
{
	return 0;
}

static int on_end_tractor( mlt_parser self, mlt_tractor object )
{
	return 0;
}

static int on_start_multitrack( mlt_parser self, mlt_multitrack object )
{
	return 0;
}

static int on_end_multitrack( mlt_parser self, mlt_multitrack object )
{
	return 0;
}

static int on_start_track( mlt_parser self )
{
	return 0;
}

static int on_end_track( mlt_parser self )
{
	return 0;
}

static int on_start_filter( mlt_parser self, mlt_filter object )
{
	return 0;
}

static int on_end_filter( mlt_parser self, mlt_filter object )
{
	return 0;
}

static int on_start_transition( mlt_parser self, mlt_transition object )
{
	return 0;
}

static int on_end_transition( mlt_parser self, mlt_transition object )
{
	return 0;
}

mlt_parser mlt_parser_new( )
{
	mlt_parser self = calloc( 1, sizeof( struct mlt_parser_s ) );
	if ( self != NULL && mlt_properties_init( &self->parent, self ) == 0 )
	{
		self->on_invalid = on_invalid;
		self->on_unknown = on_unknown;
		self->on_start_producer = on_start_producer;
		self->on_end_producer = on_end_producer;
		self->on_start_playlist = on_start_playlist;
		self->on_end_playlist = on_end_playlist;
		self->on_start_tractor = on_start_tractor;
		self->on_end_tractor = on_end_tractor;
		self->on_start_multitrack = on_start_multitrack;
		self->on_end_multitrack = on_end_multitrack;
		self->on_start_track = on_start_track;
		self->on_end_track = on_end_track;
		self->on_start_filter = on_start_filter;
		self->on_end_filter = on_end_filter;
		self->on_start_transition = on_start_transition;
		self->on_end_transition = on_end_transition;
	}
	return self;
}

mlt_properties mlt_parser_properties( mlt_parser self )
{
	return &self->parent;
}

int mlt_parser_start( mlt_parser self, mlt_service object )
{
	int error = 0;
	mlt_service_type type = mlt_service_identify( object );
	switch( type )
	{
		case invalid_type:
			error = self->on_invalid( self, object );
			break;
		case unknown_type:
			error = self->on_unknown( self, object );
			break;
		case producer_type:
			if ( mlt_producer_is_cut( ( mlt_producer )object ) )
				error = mlt_parser_start( self, ( mlt_service )mlt_producer_cut_parent( ( mlt_producer )object ) );
			error = self->on_start_producer( self, ( mlt_producer )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( self, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = self->on_end_producer( self, ( mlt_producer )object );
			break;
		case playlist_type:
			error = self->on_start_playlist( self, ( mlt_playlist )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && i < mlt_playlist_count( ( mlt_playlist )object ) )
					mlt_parser_start( self, ( mlt_service )mlt_playlist_get_clip( ( mlt_playlist )object, i ++ ) );
				i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( self, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = self->on_end_playlist( self, ( mlt_playlist )object );
			break;
		case tractor_type:
			error = self->on_start_tractor( self, ( mlt_tractor )object );
			if ( error == 0 )
			{
				int i = 0;
				mlt_service next = mlt_service_producer( object );
				mlt_parser_start( self, ( mlt_service )mlt_tractor_multitrack( ( mlt_tractor )object ) );
				while ( next != ( mlt_service )mlt_tractor_multitrack( ( mlt_tractor )object ) )
				{
					mlt_parser_start( self, next );
					next = mlt_service_producer( next );
				}
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( self, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = self->on_end_tractor( self, ( mlt_tractor )object );
			break;
		case multitrack_type:
			error = self->on_start_multitrack( self, ( mlt_multitrack )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( i < mlt_multitrack_count( ( mlt_multitrack )object ) )
				{
					self->on_start_track( self );
					mlt_parser_start( self, ( mlt_service )mlt_multitrack_track( ( mlt_multitrack )object , i ++ ) );
					self->on_end_track( self );
				}
				i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( self, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = self->on_end_multitrack( self, ( mlt_multitrack )object );
			break;
		case filter_type:
			error = self->on_start_filter( self, ( mlt_filter )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( self, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = self->on_end_filter( self, ( mlt_filter )object );
			break;
		case transition_type:
			error = self->on_start_transition( self, ( mlt_transition )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( self, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = self->on_end_transition( self, ( mlt_transition )object );
			break;
		case field_type:
			break;
		case consumer_type:
			break;
	}
	return error;
}

void mlt_parser_close( mlt_parser self )
{
	if ( self != NULL )
	{
		mlt_properties_close( &self->parent );
		free( self );
	}
}


