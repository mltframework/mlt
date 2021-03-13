/**
 * MltEvent.cpp - MLT Wrapper
 * Copyright (C) 2004-2021 Meltytech, LLC
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
#include "MltFrame.h"
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


EventData::EventData(mlt_event_data* data)
    : instance(data)
{
}

EventData::EventData(EventData& data)
    : instance(data.get_event_data())
{
}

EventData::EventData(const EventData& data)
	: instance(data.get_event_data())
{
}

EventData& EventData::operator=(const EventData& data)
{
	instance = data.get_event_data();
	return *this;
}

mlt_event_data* EventData::get_event_data() const
{
	return instance;
}

bool EventData::is_valid() const
{
	return instance != nullptr;
}

int EventData::to_int() const
{
	return mlt_event_data_to_int(instance);
}

const char* EventData::to_string() const
{
	return mlt_event_data_to_string(instance);
}

Frame EventData::to_frame() const
{
	return Frame(mlt_event_data_to_frame(instance));
}

void* EventData::to_object() const
{
	return mlt_event_data_to_object(instance);
}
