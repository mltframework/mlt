/**
 * MltDeque.cpp - MLT Wrapper
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

#include "MltDeque.h"
using namespace Mlt;

Deque::Deque( )
{
	deque = mlt_deque_init( );
}

Deque::~Deque( )
{
	mlt_deque_close( deque );
}

int Deque::count( )
{
	return mlt_deque_count( deque );
}

int Deque::push_back( void *item )
{
	return mlt_deque_push_back( deque, item );
}

void *Deque::pop_back( )
{
	return mlt_deque_pop_back( deque );
}

int Deque::push_front( void *item )
{
	return mlt_deque_push_front( deque, item );
}

void *Deque::pop_front( )
{
	return mlt_deque_pop_front( deque );
}

void *Deque::peek_back( )
{
	return mlt_deque_peek_back( deque );
}

void *Deque::peek_front( )
{
	return mlt_deque_peek_front( deque );
}

void *Deque::peek( int index )
{
	return mlt_deque_peek( deque, index );
}

