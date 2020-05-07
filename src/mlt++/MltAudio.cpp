/**
 * MltAudio.cpp - MLT Wrapper
 * Copyright (C) 2020 Meltytech, LLC
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

#include "MltAudio.h"

using namespace Mlt;

Audio::Audio( )
{
	instance = mlt_audio_new();
}

Audio::Audio( mlt_audio audio )
 : instance( audio )
{
}

Audio::~Audio( )
{
	mlt_audio_close( instance );
}

void* Audio::data()
{
	return instance->data;
}

void Audio::set_data( void* data )
{
	instance->data = data;
}

int Audio::frequency()
{
	return instance->frequency;
}

void Audio::set_frequency( int frequency )
{
	instance->frequency = frequency;
}

mlt_audio_format Audio::format()
{
	return instance->format;
}

void Audio::set_format( mlt_audio_format format )
{
	instance->format = format;
}

int Audio::samples()
{
	return instance->samples;
}

void Audio::set_samples( int samples )
{
	instance->samples = samples;
}

int Audio::channels()
{
	return instance->channels;
}

void Audio::set_channels( int channels )
{
	instance->channels = channels;
}

mlt_channel_layout Audio::layout()
{
	return instance->layout;
}

void Audio::set_layout( mlt_channel_layout layout )
{
	instance->layout = layout;
}
