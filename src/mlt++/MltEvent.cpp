/**
 * MltEvent.cpp - MLT Wrapper
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

#include "MltEvent.h"
using namespace Mlt;


Event::Event( mlt_event event ) :
	instance( event )
{
	mlt_event_inc_ref( instance );
}

Event::Event( Event &event ) :
	instance( event.get_event( ) )
{
	mlt_event_inc_ref( instance );
}

Event::~Event( )
{
	mlt_event_close( instance );
}

mlt_event Event::get_event( )
{
	return instance;
}

bool Event::is_valid( )
{
	return instance != NULL;
}

void Event::block( )
{
	mlt_event_block( get_event( ) );
}

void Event::unblock( )
{
	mlt_event_unblock( get_event( ) );
}

