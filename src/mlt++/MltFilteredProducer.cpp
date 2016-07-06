/**
 * MltFilteredProducer.cpp - MLT Wrapper
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

#include "MltFilteredProducer.h"
#include "MltProfile.h"
using namespace Mlt;

FilteredProducer::FilteredProducer( Profile& profile, const char *id, const char *arg ) :
	Producer( profile, id, arg )
{
	// Create a reference to the last service
	last = new Service( *this );
}

FilteredProducer::~FilteredProducer( )
{
	// Delete the reference to the last service
	delete last;
}

int FilteredProducer::attach( Filter &filter )
{
	int error = 0;
	if ( filter.is_valid( ) )
	{
		Service *consumer = last->consumer( );
		filter.connect_producer( *last );
		if ( consumer->is_valid( ) )
			consumer->connect_producer( filter );
		delete consumer;
		delete last;
		last = new Service( filter );
	}
	else
	{
		error = 1;
	}
	return error;
}

int FilteredProducer::detach( Filter &filter )
{
	if ( filter.is_valid( ) )
	{
		Service *it = new Service( *last );
		while ( it->is_valid( ) && it->get_service( ) != filter.get_service( ) )
		{
			Service *producer = it->producer( );
			delete it;
			it = producer;
		}
		if ( it->get_service( ) == filter.get_service( ) )
		{
			Service *producer = it->producer( );
			Service *consumer = it->consumer( );
			if ( consumer->is_valid( ) )
				consumer->connect_producer( *producer );
			Profile p( get_profile() );
			Producer dummy( p, "colour" );
			dummy.connect_producer( *it );
			if ( last->get_service( ) == it->get_service( ) )
			{
				delete last;
				last = new Service( *producer );
			}
		}
		delete it;
	}
	return 0;
}

