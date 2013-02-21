/*
 * fbo_input.cpp
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

#include "fbo_input.h"
#include <movit/util.h>

// comes from unexported effect_util.h
extern void set_uniform_int(GLuint glsl_program_num, const std::string &prefix, const std::string &key, int value);

FBOInput::FBOInput(unsigned width, unsigned height)
	: texture_num(0)
	, needs_mipmaps(false)
	, width(width)
	, height(height)
{
	register_int("needs_mipmaps", &needs_mipmaps);
}

void FBOInput::set_gl_state(GLuint glsl_program_num, const std::string& prefix, unsigned *sampler_num)
{
	glActiveTexture(GL_TEXTURE0 + *sampler_num);
	check_error();
	glBindTexture(GL_TEXTURE_2D, texture_num);
	check_error();

	// Bind it to a sampler.
	set_uniform_int(glsl_program_num, prefix, "tex", *sampler_num);
	++*sampler_num;
}

std::string FBOInput::output_fragment_shader()
{
	return read_file("flat_input.frag");
//	return "uniform sampler2D PREFIX(tex); vec4 FUNCNAME(vec2 tc) { return texture2D(PREFIX(tex), tc); }\n";
}
