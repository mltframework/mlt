/**
 * MltTransition.cpp - MLT Wrapper
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
#include <string.h>
#include "MltTransition.h"
#include "MltProfile.h"
#include "MltProducer.h"
using namespace Mlt;

Transition::Transition( Profile& profile, const char *id, const char *arg ) :
	instance( NULL )
{
	if ( arg != NULL )
	{
		instance = mlt_factory_transition( profile.get_profile(), id, arg );
	}
	else
	{
		if ( strchr( id, ':' ) )
		{
			char *temp = strdup( id );
			char *arg = strchr( temp, ':' ) + 1;
			*( arg - 1 ) = '\0';
			instance = mlt_factory_transition( profile.get_profile(), temp, arg );
			free( temp );
		}
		else
		{
			instance = mlt_factory_transition( profile.get_profile(), id, NULL );
		}
	}
}

Transition::Transition( Service &transition ) :
	instance( NULL )
{
	if ( transition.type( ) == transition_type )
	{
		instance = ( mlt_transition )transition.get_service( );
		inc_ref( );
	}
}

Transition::Transition( Transition &transition ) :
	Mlt::Service( transition ),
	instance( transition.get_transition( ) )
{
	inc_ref( );
}

Transition::Transition( mlt_transition transition ) :
	instance( transition )
{
	inc_ref( );
}

Transition::~Transition( )
{
	mlt_transition_close( instance );
}

mlt_transition Transition::get_transition( )
{
	return instance;
}

mlt_service Transition::get_service( )
{
	return mlt_transition_service( get_transition( ) );
}

void Transition::set_in_and_out( int in, int out )
{
	mlt_transition_set_in_and_out( get_transition( ), in, out );
}

void Transition::set_tracks( int a_track, int b_track )
{
	mlt_transition_set_tracks( get_transition(), a_track, b_track );
}

int Transition::connect( Producer &producer, int a_track, int b_track )
{
	return mlt_transition_connect( get_transition(), producer.get_service(), a_track, b_track );
}

int Transition::get_a_track( )
{
	return mlt_transition_get_a_track( get_transition() );
}

int Transition::get_b_track( )
{
	return mlt_transition_get_b_track( get_transition() );
}

int Transition::get_in( )
{
	return mlt_transition_get_in( get_transition() );
}

int Transition::get_out( )
{
	return mlt_transition_get_out( get_transition() );
}

int Transition::get_length( )
{
	return mlt_transition_get_length( get_transition( ) );
}

int Transition::get_position( Frame &frame )
{
	return mlt_transition_get_position( get_transition( ), frame.get_frame( ) );
}

double Transition::get_progress( Frame &frame )
{
	return mlt_transition_get_progress( get_transition( ), frame.get_frame( ) );
}

double Transition::get_progress_delta( Frame &frame )
{
	return mlt_transition_get_progress_delta( get_transition( ), frame.get_frame( ) );
}
