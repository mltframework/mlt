/*
 * fbo_input.h
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

#ifndef FBO_INPUT_H
#define FBO_INPUT_H

#include <movit/input.h>

class FBOInput : public Input
{
public:
	FBOInput(unsigned width, unsigned height);

	virtual std::string effect_type_id() const { return "FBOInput"; }
	void finalize() {}
	bool can_output_linear_gamma() const { return false; }
	AlphaHandling alpha_handling() const { return OUTPUT_POSTMULTIPLIED_ALPHA; }
	std::string output_fragment_shader();
	void set_gl_state(GLuint glsl_program_num, const std::string& prefix, unsigned *sampler_num);
	unsigned get_width() const { return width; }
	unsigned get_height() const { return height; }
	Colorspace get_color_space() const { return COLORSPACE_sRGB; }
	GammaCurve get_gamma_curve() const { return GAMMA_sRGB; }
	void set_texture(GLuint texture) {
		texture_num = texture;
	}

private:
	GLuint texture_num;
	int needs_mipmaps;
	unsigned width, height;
};

#endif // FBO_INPUT_H
