/*
 * mlt_flip_effect.h
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

#ifndef MLT_FLIP_EFFECT_H
#define MLT_FLIP_EFFECT_H

namespace Mlt
{

class VerticalFlip : public movit::Effect {
public:
	VerticalFlip() {}
	virtual std::string effect_type_id() const { return "MltVerticalFlip"; }
	std::string output_fragment_shader() {
		return "vec4 FUNCNAME(vec2 tc) { tc.y = 1.0 - tc.y; return INPUT(tc); }\n";
	}
	virtual bool needs_linear_light() const { return false; }
	virtual bool needs_srgb_primaries() const { return false; }
	AlphaHandling alpha_handling() const { return DONT_CARE_ALPHA_TYPE; }
};

} // namespace Mlt

#endif // MLT_FLIP_EFFECT_H
