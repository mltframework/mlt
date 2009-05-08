/**
 * MltProfile.cpp - MLT Wrapper
 * Copyright (C) 2008 Dan Dennedy <dan@dennedy.org>
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

#include "MltProfile.h"
#include "MltProperties.h"
using namespace Mlt;

Profile::Profile( ) :
	instance( NULL )
{
	instance = mlt_profile_init( NULL );
}

Profile::Profile( const char* name ) :
	instance( NULL )
{
	instance = mlt_profile_init( name );
}

Profile::Profile( Properties& properties ) :
	instance( NULL )
{
	instance = mlt_profile_load_properties( properties.get_properties() );
}

Profile::Profile( mlt_profile profile ) :
	instance( profile )
{
}

Profile::~Profile( )
{
	if ( instance )
		mlt_profile_close( instance );
	instance = NULL;
}

mlt_profile Profile::get_profile( ) const
{
	return instance;
}

char* Profile::description() const
{
	return instance->description;
}

int Profile::frame_rate_num() const
{
	return instance->frame_rate_num;
}

int Profile::frame_rate_den() const
{
	return instance->frame_rate_den;
}

double Profile::fps() const
{
	return mlt_profile_fps( instance );
}

int Profile::width() const
{
	return instance->width;
}

int Profile::height() const
{
	return instance->height;
}

bool Profile::progressive() const
{
	return instance->progressive;
}

int Profile::sample_aspect_num() const
{
	return instance->sample_aspect_num;
}

int Profile::sample_aspect_den() const
{
	return instance->sample_aspect_den;
}

double Profile::sar() const
{
	return mlt_profile_sar( instance );
}

int Profile::display_aspect_num() const
{
	return instance->display_aspect_num;
}

int Profile::display_aspect_den() const
{
	return instance->display_aspect_den;
}

double Profile::dar() const
{
	return mlt_profile_dar( instance );
}
