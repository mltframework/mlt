/*
 * Copyright (C) 2024 Meltytech, LLC
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <framework/mlt_types.h>

static void rgbToHsl(float r, float g, float b, float *h, float *s, float *l)
{
    float max = MAX(MAX(r, g), b);
    float min = MIN(MIN(r, g), b);
    *l = (max + min) / 2;
    if (max == min) {
        // No color
        *h = *s = 0;
    } else {
        float d = max - min;
        *s = (*l > 0.5) ? d / (2 - max - min) : d / (max + min);
        if (max == r)
            *h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g)
            *h = (b - r) / d + 2;
        else // if (max == b)
            *h = (r - g) / d + 4;
        *h /= 6;
    }
}

static float hueToRgb(float p, float q, float t)
{
    if (t < 0)
        t += 1;
    if (t > 1)
        t -= 1;
    if (t < 1. / 6)
        return p + (q - p) * 6 * t;
    if (t < 1. / 2)
        return q;
    if (t < 2. / 3)
        return p + (q - p) * (2. / 3 - t) * 6;
    return p;
}

static void hslToRgb(float h, float s, float l, float *r, float *g, float *b)
{
    if (0 == s) {
        // No color
        *r = *g = *b = l;
    } else {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        *r = hueToRgb(p, q, h + 1.0 / 3.0);
        *g = hueToRgb(p, q, h);
        *b = hueToRgb(p, q, h - 1.0 / 3.0);
    }
}
