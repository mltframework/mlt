/**
 * MltRepository.cpp - MLT Wrapper
 * Copyright (C) 2008-2017 Meltytech, LLC
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

#include "MltRepository.h"
#include "MltProfile.h"
#include "MltProperties.h"
using namespace Mlt;

Repository::Repository( const char* directory ) :
	instance( NULL )
{
	instance = mlt_repository_init( directory );
}

Repository::Repository( mlt_repository repository ) :
	instance( repository )
{
}

Repository::~Repository( )
{
	instance = NULL;
}

void Repository::register_service( mlt_service_type service_type, const char *service, mlt_register_callback symbol )
{
	mlt_repository_register( instance, service_type, service, symbol );
}

void *Repository::create( Profile& profile, mlt_service_type type, const char *service, void *arg )
{
	return mlt_repository_create( instance, profile.get_profile(), type, service, arg );
}

Properties *Repository::consumers( ) const
{
	return new Properties( mlt_repository_consumers( instance ) );
}

Properties *Repository::filters( ) const
{
	return new Properties( mlt_repository_filters( instance ) );
}

Properties *Repository::producers( ) const
{
	return new Properties( mlt_repository_producers( instance ) );
}

Properties *Repository::transitions( ) const
{
	return new Properties( mlt_repository_transitions( instance ) );
}

void Repository::register_metadata( mlt_service_type type, const char *service, mlt_metadata_callback callback, void *callback_data )
{
	mlt_repository_register_metadata( instance, type, service, callback, callback_data );
}

Properties *Repository::metadata( mlt_service_type type, const char *service ) const
{
	return new Properties( mlt_repository_metadata( instance, type, service ) );
}

Properties *Repository::languages( ) const
{
	return new Properties( mlt_repository_languages( instance ) );
}

Properties *Repository::presets( )
{
	return new Properties( mlt_repository_presets( ) );
}
