/**
 * \file mlt_types.c
 * \brief Mlt types helper functions
 *
 * Copyright (C) 2023-2025 Meltytech, LLC
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

#include "mlt_types.h"

#include "mlt_image.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *mlt_deinterlacer_name(mlt_deinterlacer method)
{
    switch (method) {
    case mlt_deinterlacer_none:
        return "none";
    case mlt_deinterlacer_onefield:
        return "onefield";
    case mlt_deinterlacer_linearblend:
        return "linearblend";
    case mlt_deinterlacer_bob:
        return "bob";
    case mlt_deinterlacer_weave:
        return "weave";
    case mlt_deinterlacer_greedy:
        return "greedy";
    case mlt_deinterlacer_yadif_nospatial:
        return "yadif-nospatial";
    case mlt_deinterlacer_yadif:
        return "yadif";
    case mlt_deinterlacer_bwdif:
        return "bwdif";
    case mlt_deinterlacer_estdif:
        return "estdif";
    case mlt_deinterlacer_invalid:
        return "invalid";
    }
    return "invalid";
}

mlt_deinterlacer mlt_deinterlacer_id(const char *name)
{
    mlt_deinterlacer m;

    for (m = mlt_deinterlacer_none; name && m < mlt_deinterlacer_invalid; m++) {
        if (!strcmp(mlt_deinterlacer_name(m), name))
            return m;
    }
    return mlt_deinterlacer_invalid;
}

uint8_t srgb_to_linear(uint8_t value)
{
    // Normalize the 8-bit value to the [0.0, 1.0] range
    double factor = (double) value / 255.0;
    if (factor < 0.04045) {
        factor *= 0.0773993808;
    } else {
        factor = pow(factor * 0.9478672986 + 0.0521327014, 2.4);
    }
    // Map back to 0-255 range
    double linear_value = round((double) 255 * factor);
    return (uint8_t) CLAMP(linear_value, 0.0, 255.0);
}

/** Convert a standard sRGB color value to the specified colorspace.
 *
 * \param color a standard sRGB color value
 * \param trc_name a string representing the TRC to convert to
 * \return the converted color
 */

mlt_color mlt_color_convert_trc(mlt_color color, const char *trc_name)
{
    mlt_color_trc trc = mlt_image_color_trc_id(trc_name);
    // Only linear is supported for now.
    if (trc == mlt_color_trc_linear) {
        color.r = srgb_to_linear(color.r);
        color.g = srgb_to_linear(color.g);
        color.b = srgb_to_linear(color.b);
    }
    return color;
}
