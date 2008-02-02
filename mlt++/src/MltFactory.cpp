/**
 * MltFactory.cpp - MLT Wrapper
 * Copyright (C) 2004-2005 Charles Yates
 * Copyright (C) 2008 Dan Dennedy <dan@dennedy.org>
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

#include "MltFactory.h"
#include "MltProducer.h"
#include "MltFilter.h"
#include "MltTransition.h"
#include "MltConsumer.h"
using namespace Mlt;

int Factory::init( char *arg )
{
	return mlt_factory_init( arg );
}

Properties *Factory::event_object( )
{
	return new Properties( mlt_factory_event_object( ) );
}

Producer *Factory::producer( Profile& profile, char *id, char *arg )
{
	return new Producer( profile, id, arg );
}

Filter *Factory::filter( Profile& profile, char *id, char *arg )
{
	return new Filter( profile, id, arg );
}

Transition *Factory::transition( Profile& profile, char *id, char *arg )
{
	return new Transition( profile, id, arg );
}

Consumer *Factory::consumer( Profile& profile, char *id, char *arg )
{
	return new Consumer( profile, id, arg );
}

#ifdef WIN32
char *Factory::getenv( const char *name )
{
	return mlt_getenv( name );
}

int Factory::setenv( const char *name, const char *value )
{
	return mlt_setenv( name, value );
}
#endif

void Factory::close( )
{
	mlt_factory_close( );
}


