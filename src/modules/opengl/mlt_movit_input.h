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

class MltInput
{
public:
	MltInput();
	~MltInput();

	void useFlatInput(MovitPixelFormat pix_fmt, unsigned width, unsigned height);
	void useYCbCrInput(const ImageFormat& image_format, const YCbCrFormat& ycbcr_format, unsigned width, unsigned height);
	void set_pixel_data(const unsigned char* data);
	void invalidate_pixel_data();
	Input *get_input() { return input; }

private:
	unsigned m_width, m_height;
	// Note: Owned by the EffectChain, so should not be deleted by us.
	Input *input;
	bool isRGB;
	YCbCrFormat m_ycbcr_format;
};

#endif // MLT_MOVIT_INPUT_H
