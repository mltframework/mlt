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

mlt_properties Frame::get_properties( )
{
	return mlt_frame_properties( get_frame( ) );
}

uint8_t *Frame::get_image( mlt_image_format &format, int &w, int &h, int writable )
{
	uint8_t *image = NULL;
	mlt_frame_get_image( get_frame( ), &image, &format, &w, &h, writable );
	return image;
}

int16_t *Frame::get_audio( mlt_audio_format &format, int &frequency, int &channels, int &samples )
{
	int16_t *audio = NULL;
	mlt_frame_get_audio( get_frame( ), &audio, &format, &frequency, &channels, &samples );
	return audio;
}

mlt_frame FrameInstance::get_frame( )
{
	return instance;
}

FrameInstance::FrameInstance( mlt_frame frame ) :
	destroy( true ),
	instance( frame )
{
}

FrameInstance::FrameInstance( Frame &frame ) :
	destroy( false ),
	instance( frame.get_frame( ) )
{
}

FrameInstance::~FrameInstance( )
{
	if ( destroy )
		mlt_frame_close( instance );
}


