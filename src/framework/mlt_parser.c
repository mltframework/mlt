/*
 * mlt_parser.c -- service parsing functionality
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "config.h"
#include "mlt.h"
#include <stdlib.h>

static int on_invalid( mlt_parser this, mlt_service object )
{
	return 0;
}

static int on_unknown( mlt_parser this, mlt_service object )
{
	return 0;
}

static int on_start_producer( mlt_parser this, mlt_producer object )
{
	return 0;
}

static int on_end_producer( mlt_parser this, mlt_producer object )
{
	return 0;
}

static int on_start_playlist( mlt_parser this, mlt_playlist object )
{
	return 0;
}

static int on_end_playlist( mlt_parser this, mlt_playlist object )
{
	return 0;
}

static int on_start_tractor( mlt_parser this, mlt_tractor object )
{
	return 0;
}

static int on_end_tractor( mlt_parser this, mlt_tractor object )
{
	return 0;
}

static int on_start_multitrack( mlt_parser this, mlt_multitrack object )
{
	return 0;
}

static int on_end_multitrack( mlt_parser this, mlt_multitrack object )
{
	return 0;
}

static int on_start_track( mlt_parser this )
{
	return 0;
}

static int on_end_track( mlt_parser this )
{
	return 0;
}

static int on_start_filter( mlt_parser this, mlt_filter object )
{
	return 0;
}

static int on_end_filter( mlt_parser this, mlt_filter object )
{
	return 0;
}

static int on_start_transition( mlt_parser this, mlt_transition object )
{
	return 0;
}

static int on_end_transition( mlt_parser this, mlt_transition object )
{
	return 0;
}

mlt_parser mlt_parser_new( )
{
	mlt_parser this = calloc( 1, sizeof( struct mlt_parser_s ) );
	if ( this != NULL && mlt_properties_init( &this->parent, this ) == 0 )
	{
		this->on_invalid = on_invalid;
		this->on_unknown = on_unknown;
		this->on_start_producer = on_start_producer;
		this->on_end_producer = on_end_producer;
		this->on_start_playlist = on_start_playlist;
		this->on_end_playlist = on_end_playlist;
		this->on_start_tractor = on_start_tractor;
		this->on_end_tractor = on_end_tractor;
		this->on_start_multitrack = on_start_multitrack;
		this->on_end_multitrack = on_end_multitrack;
		this->on_start_track = on_start_track;
		this->on_end_track = on_end_track;
		this->on_start_filter = on_start_filter;
		this->on_end_filter = on_end_filter;
		this->on_start_transition = on_start_transition;
		this->on_end_transition = on_end_transition;
	}
	return this;
}

mlt_properties mlt_parser_properties( mlt_parser this )
{
	return &this->parent;
}

int mlt_parser_start( mlt_parser this, mlt_service object )
{
	int error = 0;
	mlt_service_type type = mlt_service_identify( object );
	switch( type )
	{
		case invalid_type:
			error = this->on_invalid( this, object );
			break;
		case unknown_type:
			error = this->on_unknown( this, object );
			break;
		case producer_type:
			if ( mlt_producer_is_cut( ( mlt_producer )object ) )
				error = mlt_parser_start( this, ( mlt_service )mlt_producer_cut_parent( ( mlt_producer )object ) );
			error = this->on_start_producer( this, ( mlt_producer )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( this, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = this->on_end_producer( this, ( mlt_producer )object );
			break;
		case playlist_type:
			error = this->on_start_playlist( this, ( mlt_playlist )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && i < mlt_playlist_count( ( mlt_playlist )object ) )
					mlt_parser_start( this, ( mlt_service )mlt_playlist_get_clip( ( mlt_playlist )object, i ++ ) );
				i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( this, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = this->on_end_playlist( this, ( mlt_playlist )object );
			break;
		case tractor_type:
			error = this->on_start_tractor( this, ( mlt_tractor )object );
			if ( error == 0 )
			{
				int i = 0;
				mlt_service next = mlt_service_producer( object );
				mlt_parser_start( this, ( mlt_service )mlt_tractor_multitrack( ( mlt_tractor )object ) );
				while ( next != ( mlt_service )mlt_tractor_multitrack( ( mlt_tractor )object ) )
				{
					mlt_parser_start( this, next );
					next = mlt_service_producer( next );
				}
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( this, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = this->on_end_tractor( this, ( mlt_tractor )object );
			break;
		case multitrack_type:
			error = this->on_start_multitrack( this, ( mlt_multitrack )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( i < mlt_multitrack_count( ( mlt_multitrack )object ) )
				{
					this->on_start_track( this );
					mlt_parser_start( this, ( mlt_service )mlt_multitrack_track( ( mlt_multitrack )object , i ++ ) );
					this->on_end_track( this );
				}
				i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( this, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = this->on_end_multitrack( this, ( mlt_multitrack )object );
			break;
		case filter_type:
			error = this->on_start_filter( this, ( mlt_filter )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( this, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = this->on_end_filter( this, ( mlt_filter )object );
			break;
		case transition_type:
			error = this->on_start_transition( this, ( mlt_transition )object );
			if ( error == 0 )
			{
				int i = 0;
				while ( error == 0 && mlt_producer_filter( ( mlt_producer )object, i ) != NULL )
					error = mlt_parser_start( this, ( mlt_service )mlt_producer_filter( ( mlt_producer )object, i ++ ) );
			}
			error = this->on_end_transition( this, ( mlt_transition )object );
			break;
		case field_type:
			break;
		case consumer_type:
			break;
	}
	return error;
}

void mlt_parser_close( mlt_parser this )
{
	if ( this != NULL )
	{
		mlt_properties_close( &this->parent );
		free( this );
	}
}


