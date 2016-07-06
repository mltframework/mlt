/*
 * mlt_movit_input.cpp
 * Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mlt_movit_input.h"

using namespace movit;

MltInput::MltInput( mlt_image_format format )
	: m_format(format)
	, input(0)
	, isRGB(true)
{
}

MltInput::~MltInput()
{
}

void MltInput::useFlatInput(MovitPixelFormat pix_fmt, unsigned width, unsigned height)
{
	if (!input) {
		m_width = width;
		m_height = height;
		ImageFormat image_format;
		image_format.color_space = COLORSPACE_sRGB;
		image_format.gamma_curve = GAMMA_sRGB;
		input = new FlatInput(image_format, pix_fmt, GL_UNSIGNED_BYTE, width, height);
	}
}

void MltInput::useYCbCrInput(const ImageFormat& image_format, const YCbCrFormat& ycbcr_format, unsigned width, unsigned height)
{
	if (!input) {
		m_width = width;
		m_height = height;
		input = new YCbCrInput(image_format, ycbcr_format, width, height);
		isRGB = false;
		m_ycbcr_format = ycbcr_format;
	}
}

void MltInput::set_pixel_data(const unsigned char* data)
{
	assert(input);
	if (isRGB) {
		FlatInput* flat = (FlatInput*) input;
		flat->set_pixel_data(data);
	} else {
		YCbCrInput* ycbcr = (YCbCrInput*) input;
		ycbcr->set_pixel_data(0, data);
		ycbcr->set_pixel_data(1, &data[m_width * m_height]);
		ycbcr->set_pixel_data(2, &data[m_width * m_height + (m_width / m_ycbcr_format.chroma_subsampling_x * m_height / m_ycbcr_format.chroma_subsampling_y)]);
	}
}

void MltInput::invalidate_pixel_data()
{
	assert(input);
	if (isRGB) {
		FlatInput* flat = (FlatInput*) input;
		flat->invalidate_pixel_data();
	} else {
		YCbCrInput* ycbcr = (YCbCrInput*) input;
		ycbcr->invalidate_pixel_data();
	}
}
