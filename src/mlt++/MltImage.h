/**
 * MltImage.h - MLT Wrapper
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

#ifndef MLTPP_IMAGE_H
#define MLTPP_IMAGE_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt {
class MLTPP_DECLSPEC Image
{
private:
    mlt_image instance;

public:
    Image();
    Image(mlt_image image);
    Image(int width, int height, mlt_image_format format);
    virtual ~Image();
    mlt_image_format format();
    int width();
    int height();
    void set_colorspace(int colorspace);
    int colorspace();
    void alloc(int width, int height, mlt_image_format format, bool alpha = false);
    void init_alpha();
    uint8_t *plane(int plane);
    int stride(int plane);
};
} // namespace Mlt

#endif
