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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MLT_MOVIT_INPUT_H
#define MLT_MOVIT_INPUT_H

#include <framework/mlt_types.h>

#include <movit/flat_input.h>
#include <movit/ycbcr_input.h>
#include <movit/effect_chain.h>

class MltInput
{
public:
	MltInput( mlt_image_format format );
	~MltInput();

	void useFlatInput(movit::MovitPixelFormat pix_fmt, unsigned width, unsigned height);
	void useYCbCrInput(const movit::ImageFormat& image_format, const movit::YCbCrFormat& ycbcr_format, unsigned width, unsigned height);
	void set_pixel_data(const unsigned char* data);
	void invalidate_pixel_data();
	movit::Input *get_input() { return input; }

	// The original pixel format that was used to create this MltInput,
	// in case we change our mind later and want to convert on the CPU instead.
	mlt_image_format get_format() const { return m_format; }

private:
	mlt_image_format m_format;
	unsigned m_width, m_height;
	// Note: Owned by the EffectChain, so should not be deleted by us.
	movit::Input *input;
	bool isRGB;
	movit::YCbCrFormat m_ycbcr_format;
};

#endif // MLT_MOVIT_INPUT_H
