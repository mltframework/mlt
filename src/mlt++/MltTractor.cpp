/**
 * MltTractor.cpp - Tractor wrapper
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

#include "MltTractor.h"
#include "MltMultitrack.h"
#include "MltField.h"
#include "MltTransition.h"
#include "MltFilter.h"
#include "MltPlaylist.h"
using namespace Mlt;

Tractor::Tractor( ) :
	instance( mlt_tractor_new( ) )
{
}

Tractor::Tractor( Profile& profile ) :
	instance( mlt_tractor_new( ) )
{
	set_profile( profile );
}

Tractor::Tractor( Service &tractor ) :
	instance( NULL )
{
	if ( tractor.type( ) == tractor_type )
	{
		instance = ( mlt_tractor )tractor.get_service( );
		inc_ref( );
	}
}

Tractor::Tractor( mlt_tractor tractor ) :
	instance( tractor )
{
	inc_ref( );
}

Tractor::Tractor( Tractor &tractor ) :
	Mlt::Producer( tractor ),
	instance( tractor.get_tractor( ) )
{
	inc_ref( );
}

Tractor::Tractor( Profile& profile, char *id, char *resource ) :
	instance( NULL )
{
	Producer producer( profile, id, resource );
	if ( producer.is_valid( ) && producer.type( ) == tractor_type )
	{
		instance = ( mlt_tractor )producer.get_producer( );
		inc_ref( );
	}
	else if ( producer.is_valid( ) )
	{
		instance = mlt_tractor_new( );
		set_profile( profile );
		set_track( producer, 0 );
	}
}

Tractor::~Tractor( )
{
	mlt_tractor_close( instance );
}

mlt_tractor Tractor::get_tractor( )
{
	return instance;
}

mlt_producer Tractor::get_producer( )
{
	return mlt_tractor_producer( get_tractor( ) );
}

Multitrack *Tractor::multitrack( )
{
	return new Multitrack( mlt_tractor_multitrack( get_tractor( ) ) );
}

Field *Tractor::field( )
{
	return new Field( mlt_tractor_field( get_tractor( ) ) );
}

void Tractor::refresh( )
{
	return mlt_tractor_refresh( get_tractor( ) );
}

int Tractor::set_track( Producer &producer, int index )
{
	return mlt_tractor_set_track( get_tractor( ), producer.get_producer( ), index );
}

int Tractor::insert_track( Producer &producer, int index )
{
	return mlt_tractor_insert_track( get_tractor( ), producer.get_producer( ), index );
}

int Tractor::remove_track( int index )
{
	return mlt_tractor_remove_track( get_tractor(), index );
}

Producer *Tractor::track( int index )
{
	mlt_producer producer = mlt_tractor_get_track( get_tractor( ), index );
	return producer != NULL ? new Producer( producer ) : NULL;
}

int Tractor::count( )
{
	return mlt_multitrack_count( mlt_tractor_multitrack( get_tractor( ) ) );
}

void Tractor::plant_transition( Transition &transition, int a_track, int b_track )
{
	mlt_field_plant_transition( mlt_tractor_field( get_tractor( ) ), transition.get_transition( ), a_track, b_track );
}

void Tractor::plant_transition( Transition *transition, int a_track, int b_track )
{
	if ( transition != NULL )
		mlt_field_plant_transition( mlt_tractor_field( get_tractor( ) ), transition->get_transition( ), a_track, b_track );
}

void Tractor::plant_filter( Filter &filter, int track )
{
	mlt_field_plant_filter( mlt_tractor_field( get_tractor( ) ), filter.get_filter( ), track );
}

void Tractor::plant_filter( Filter *filter, int track )
{
	mlt_field_plant_filter( mlt_tractor_field( get_tractor( ) ), filter->get_filter( ), track );
}

bool Tractor::locate_cut( Producer *producer, int &track, int &cut )
{
	bool found = false;

	for ( track = 0; producer != NULL && !found && track < count( ); track ++ )
	{
		Playlist playlist( ( mlt_playlist )mlt_tractor_get_track( get_tractor( ), track ) );
		for ( cut = 0; !found && cut < playlist.count( ); cut ++ )
		{
			Producer *clip = playlist.get_clip( cut );
			found = producer->get_producer( ) == clip->get_producer( );
			delete clip;
		}
	}

	track --;
	cut --;

	return found;
}

int Tractor::connect( Producer &producer )
{
	return mlt_tractor_connect( get_tractor( ), producer.get_service( ) );
}
