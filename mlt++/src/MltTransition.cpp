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

#include "MltTransition.h"
using namespace Mlt;

Transition::Transition( char *id, char *arg ) :
	destroy( true ),
	instance( NULL )
{
	instance = mlt_factory_transition( id, arg );
}

Transition::Transition( Transition &transition ) :
	destroy( false ),
	instance( transition.get_transition( ) )
{
}

Transition::Transition( mlt_transition transition ) :
	destroy( false ),
	instance( transition )
{
}

Transition::~Transition( )
{
	if ( destroy )
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


