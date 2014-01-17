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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "mlt_movit_input.h"

MltInput::MltInput(unsigned width, unsigned height)
	: m_width(width)
	, m_height(height)
	, output_linear_gamma(false)
	, needs_mipmaps(false)
	, input(0)
	, isRGB(true)
	, m_chain(NULL)
{
	register_int("output_linear_gamma", &output_linear_gamma);
	register_int("needs_mipmaps", &needs_mipmaps);
}

MltInput::~MltInput()
{
	// XXX: this is crashing when a producer is closed
	// on Windows when using melt with qglsl.
//	delete input;
}

std::string MltInput::output_fragment_shader()
{
	assert(input);
	return input->output_fragment_shader();
}

void MltInput::set_gl_state(GLuint glsl_program_num, const std::string& prefix, unsigned *sampler_num)
{
	assert(input);
	input->set_gl_state(glsl_program_num, prefix, sampler_num);
}

Effect::AlphaHandling MltInput::alpha_handling() const
{
	assert(input);
	return input->alpha_handling();
}

void MltInput::finalize()
{
	assert(input);
	bool ok = input->set_int("output_linear_gamma", output_linear_gamma);
	ok |= input->set_int("needs_mipmaps", needs_mipmaps);
	assert(ok);
	input->inform_added(m_chain);
	input->finalize();
}

bool MltInput::can_output_linear_gamma() const
{
	assert(input);
	return input->can_output_linear_gamma();
}

Colorspace MltInput::get_color_space() const
{
	assert(input);
	return input->get_color_space();
}
GammaCurve MltInput::get_gamma_curve() const
{
	assert(input);
	return input->get_gamma_curve();
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
