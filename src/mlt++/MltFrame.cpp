/**
 * MltFrame.cpp - MLT Wrapper
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

#include "MltFrame.h"
#include "MltProducer.h"
using namespace Mlt;

Frame::Frame() :
	Mlt::Properties( (mlt_properties)NULL ),
	instance( NULL )
{
}

Frame::Frame( mlt_frame frame ) :
	Mlt::Properties( (mlt_properties)NULL ),
	instance( frame )
{
	inc_ref( );
}

Frame::Frame( Frame &frame ) :
	Mlt::Properties( (mlt_properties)NULL ),
	instance( frame.instance )
{
	inc_ref( );
}

Frame::Frame( const Frame &frame ) :
	Mlt::Properties( (mlt_properties)NULL ),
	instance( frame.instance )
{
	inc_ref( );
}

Frame::~Frame( )
{
	mlt_frame_close( instance );
}

Frame& Frame::operator=( const Frame &frame )
{
	if (this != &frame)
	{
		mlt_frame_close( instance );
		instance = frame.instance;
		inc_ref( );
	}
	return *this;
}

mlt_frame Frame::get_frame( )
{
	return instance;
}

mlt_properties Frame::get_properties( )
{
	return mlt_frame_properties( get_frame( ) );
}

uint8_t *Frame::get_image( mlt_image_format &format, int &w, int &h, int writable )
{
	uint8_t *image = NULL;
	if ( get_double( "consumer_aspect_ratio" ) == 0.0 )
		set( "consumer_aspect_ratio", 1.0 );
	mlt_frame_get_image( get_frame( ), &image, &format, &w, &h, writable );
	set( "format", format );
	set( "writable", writable );
	return image;
}

unsigned char *Frame::fetch_image( mlt_image_format f, int w, int h, int writable )
{
	uint8_t *image = NULL;
	if ( get_double( "consumer_aspect_ratio" ) == 0.0 )
		set( "consumer_aspect_ratio", 1.0 );
	mlt_frame_get_image( get_frame( ), &image, &f, &w, &h, writable );
	set( "format", f );
	set( "writable", writable );
	return image;
}

void *Frame::get_audio( mlt_audio_format &format, int &frequency, int &channels, int &samples )
{
	void *audio = NULL;
	mlt_frame_get_audio( get_frame( ), &audio, &format, &frequency, &channels, &samples );
	return audio;
}

unsigned char *Frame::get_waveform( int w, int h )
{
	return mlt_frame_get_waveform( get_frame( ), w, h );
}

Producer *Frame::get_original_producer( )
{
	return new Producer( mlt_frame_get_original_producer( get_frame( ) ) );
}

mlt_properties Frame::get_unique_properties( Service &service )
{
	return mlt_frame_unique_properties( get_frame(), service.get_service() );
}

int Frame::get_position( )
{
	return mlt_frame_get_position( get_frame() );
}

int Frame::set_image( uint8_t *image, int size, mlt_destructor destroy )
{
	return mlt_frame_set_image( get_frame(), image, size, destroy );
}

int Frame::set_alpha( uint8_t *alpha, int size, mlt_destructor destroy )
{
	return mlt_frame_set_alpha( get_frame(), alpha, size, destroy );
}
