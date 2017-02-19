/**
 * MltFactory.cpp - MLT Wrapper
 * Copyright (C) 2004-2017 Meltytech, LLC
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

#include "MltFactory.h"
#include "MltProducer.h"
#include "MltFilter.h"
#include "MltTransition.h"
#include "MltConsumer.h"
#include "MltRepository.h"
using namespace Mlt;

Repository *Factory::init( const char *directory )
{
	return new Repository( mlt_factory_init( directory ) );
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

void Factory::close( )
{
	mlt_factory_close( );
}


