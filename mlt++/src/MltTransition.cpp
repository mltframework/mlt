/**
 * MltTransition.cpp - MLT Wrapper
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

#include <stdlib.h>
#include <string.h>
#include "MltTransition.h"
#include "MltProfile.h"
using namespace Mlt;

Transition::Transition( Profile& profile, char *id, char *arg ) :
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
