/**
 * MltFrame.cpp - MLT Wrapper
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

#include "MltFrame.h"
using namespace Mlt;

Frame::Frame( mlt_frame frame ) :
	instance( frame )
{
	inc_ref( );
}

Frame::Frame( Frame &frame ) :
	instance( frame.get_frame( ) )
{
	inc_ref( );
}

Frame::~Frame( )
{
	mlt_frame_close( instance );
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
	if ( mlt_properties_get_int( get_properties( ), "consumer_aspect_ratio" ) == 0 )
		mlt_properties_set_int( get_properties( ), "consumer_aspect_ratio", 1 );
	mlt_frame_get_image( get_frame( ), &image, &format, &w, &h, writable );
	set( "format", format );
	set( "writable", writable );
	return image;
}

unsigned char *Frame::fetch_image( mlt_image_format f, int w, int h, int writable )
{
	uint8_t *image = NULL;
	if ( mlt_properties_get_int( get_properties( ), "consumer_aspect_ratio" ) == 0 )
		mlt_properties_set_int( get_properties( ), "consumer_aspect_ratio", 1 );
	mlt_frame_get_image( get_frame( ), &image, &f, &w, &h, writable );
	set( "format", f );
	set( "writable", writable );
	return image;
}

int16_t *Frame::get_audio( mlt_audio_format &format, int &frequency, int &channels, int &samples )
{
	int16_t *audio = NULL;
	mlt_frame_get_audio( get_frame( ), &audio, &format, &frequency, &channels, &samples );
	return audio;
}

unsigned char *Frame::get_waveform( int w, int h )
{
	return mlt_frame_get_waveform( get_frame( ), w, h );
}

