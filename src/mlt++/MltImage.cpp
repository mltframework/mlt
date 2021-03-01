/**
 * MltImage.cpp - MLT Wrapper
 * Copyright (C) 2021 Meltytech, LLC
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

#include "MltImage.h"

using namespace Mlt;

Image::Image( )
{
	instance = mlt_image_new();
}

Image::Image( mlt_image image )
 : instance( image )
{
}

Image::Image( int width, int height, mlt_image_format format )
{
	instance = mlt_image_new();
	alloc( width, height, format );
}

Image::~Image( )
{
	mlt_image_close( instance );
}

mlt_image_format Image::format()
{
	return instance->format;
}

int Image::width()
{
	return instance->width;
}

int Image::height()
{
	return instance->height;
}

void Image::set_colorspace( int colorspace )
{
	instance->colorspace = colorspace;
}

int Image::colorspace()
{
	return instance->colorspace;
}

void Image::alloc( int width, int height, mlt_image_format format, bool alpha )
{
	instance->width = width;
	instance->height = height;
	instance->format = format;
	mlt_image_alloc_data( instance );
	if ( alpha )
		mlt_image_alloc_alpha( instance );
}

void Image::init_alpha()
{
	mlt_image_alloc_alpha( instance );
}

uint8_t* Image::plane( int plane )
{
	return instance->planes[plane];
}

int Image::stride( int plane )
{
	return instance->strides[plane];
}
