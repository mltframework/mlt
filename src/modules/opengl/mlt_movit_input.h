/*
 * mlt_movit_input.h
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

#ifndef MLT_MOVIT_INPUT_H
#define MLT_MOVIT_INPUT_H

#include <movit/flat_input.h>
#include <movit/ycbcr_input.h>
#include <movit/effect_chain.h>

class MltInput : public Input
{
public:
	MltInput(unsigned width, unsigned height);
	~MltInput();

	// Effect overrides
	std::string effect_type_id() const { return "MltInput"; }
	Effect::AlphaHandling alpha_handling() const;
	std::string output_fragment_shader();
	void set_gl_state(GLuint glsl_program_num, const std::string& prefix, unsigned *sampler_num);

	// Input ovverrides
	void finalize();
	bool can_output_linear_gamma() const;
	unsigned get_width() const { return m_width; }
	unsigned get_height() const { return m_height; }
	Colorspace get_color_space() const;
	GammaCurve get_gamma_curve() const;

	// Custom methods
	void useFlatInput(MovitPixelFormat pix_fmt, unsigned width, unsigned height);
	void useYCbCrInput(const ImageFormat& image_format, const YCbCrFormat& ycbcr_format, unsigned width, unsigned height);
	void set_pixel_data(const unsigned char* data);

private:
	unsigned m_width, m_height;
	int output_linear_gamma, needs_mipmaps;
	Input *input;
	bool isRGB;
	YCbCrFormat m_ycbcr_format;
};

#endif // MLT_MOVIT_INPUT_H
