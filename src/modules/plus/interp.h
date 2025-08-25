//interp.c
/*
 * Copyright (C) 2010 Marko Cebokli   http://lea.hamradio.si/~s57uuu
 * Copyright (C) 2010-2025 Meltytech, LLC
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

#include <inttypes.h>
#include <math.h>
#include <stdio.h>

//#define TEST_XY_LIMITS

//--------------------------------------------------------
// pointer to an interpolating function
// parameters:
//  source image
//  source width
//  source height
//  X coordinate
//  Y coordinate
//  opacity
//  destination image
//  flag to overwrite alpha channel

typedef int (*interpp32)(uint8_t *, int, int, float, float, float, uint8_t *, int);
typedef int (*interpp64)(uint16_t *, int, int, float, float, float, uint16_t *, int);

typedef enum {
    interp_nn,
    interp_bl,
    interp_bc,
} interp_type;

// nearest neighbor

int interpNN_b32(uint8_t *s, int w, int h, float x, float y, float o, uint8_t *d, int is_atop)
{
#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif
    int p = (int) rintf(x) * 4 + (int) rintf(y) * 4 * w;
    float alpha_s = (float) s[p + 3] / 255.0f * o;
    float alpha_d = (float) d[3] / 255.0f;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    d[3] = is_atop ? s[p + 3] : (255 * alpha);
    alpha = alpha_s / alpha;
    d[0] = d[0] * (1.0f - alpha) + s[p] * alpha;
    d[1] = d[1] * (1.0f - alpha) + s[p + 1] * alpha;
    d[2] = d[2] * (1.0f - alpha) + s[p + 2] * alpha;

    return 0;
}

int interpNN_b64(uint16_t *s, int w, int h, float x, float y, float o, uint16_t *d, int is_atop)
{
#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif
    int p = (int) rintf(x) * 4 + (int) rintf(y) * 4 * w;
    float alpha_s = (float) s[p + 3] / 65535.0f * o;
    float alpha_d = (float) d[3] / 65535.0f;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    d[3] = is_atop ? s[p + 3] : (65535 * alpha);
    alpha = alpha_s / alpha;
    d[0] = d[0] * (1.0f - alpha) + s[p] * alpha;
    d[1] = d[1] * (1.0f - alpha) + s[p + 1] * alpha;
    d[2] = d[2] * (1.0f - alpha) + s[p + 2] * alpha;

    return 0;
}

//  bilinear

int interpBL_b32(uint8_t *s, int w, int h, float x, float y, float o, uint8_t *d, int is_atop)
{
    int m, n, k, l, n1, l1, k1;
    float a, b;

#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    m = (int) floorf(x);
    if (m + 2 > w)
        m = w - 2;
    n = (int) floorf(y);
    if (n + 2 > h)
        n = h - 2;

    k = n * w + m;
    l = (n + 1) * w + m;
    k1 = 4 * (k + 1);
    l1 = 4 * (l + 1);
    n1 = 4 * ((n + 1) * w + m);
    l = 4 * l;
    k = 4 * k;

    a = s[k + 3] + (s[k1 + 3] - s[k + 3]) * (x - (float) m);
    b = s[l + 3] + (s[l1 + 3] - s[n1 + 3]) * (x - (float) m);

    float alpha_s = a + (b - a) * (y - (float) n);
    float alpha_d = (float) d[3] / 255.0f;
    if (is_atop)
        d[3] = alpha_s;
    alpha_s = alpha_s / 255.0f * o;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    if (!is_atop)
        d[3] = 255 * alpha;
    alpha = alpha_s / alpha;

    a = s[k] + (s[k1] - s[k]) * (x - (float) m);
    b = s[l] + (s[l1] - s[n1]) * (x - (float) m);
    d[0] = d[0] * (1.0f - alpha) + (a + (b - a) * (y - (float) n)) * alpha;

    a = s[k + 1] + (s[k1 + 1] - s[k + 1]) * (x - (float) m);
    b = s[l + 1] + (s[l1 + 1] - s[n1 + 1]) * (x - (float) m);
    d[1] = d[1] * (1.0f - alpha) + (a + (b - a) * (y - (float) n)) * alpha;

    a = s[k + 2] + (s[k1 + 2] - s[k + 2]) * (x - (float) m);
    b = s[l + 2] + (s[l1 + 2] - s[n1 + 2]) * (x - (float) m);
    d[2] = d[2] * (1.0f - alpha) + (a + (b - a) * (y - (float) n)) * alpha;

    return 0;
}

int interpBL_b64(uint16_t *s, int w, int h, float x, float y, float o, uint16_t *d, int is_atop)
{
    int m, n, k, l, n1, l1, k1;
    float a, b;

#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    m = (int) floorf(x);
    if (m + 2 > w)
        m = w - 2;
    n = (int) floorf(y);
    if (n + 2 > h)
        n = h - 2;

    k = n * w + m;
    l = (n + 1) * w + m;
    k1 = 4 * (k + 1);
    l1 = 4 * (l + 1);
    n1 = 4 * ((n + 1) * w + m);
    l = 4 * l;
    k = 4 * k;

    a = s[k + 3] + (s[k1 + 3] - s[k + 3]) * (x - (float) m);
    b = s[l + 3] + (s[l1 + 3] - s[n1 + 3]) * (x - (float) m);

    float alpha_s = a + (b - a) * (y - (float) n);
    float alpha_d = (float) d[3] / 65535.0f;
    if (is_atop)
        d[3] = alpha_s;
    alpha_s = alpha_s / 65535.0f * o;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    if (!is_atop)
        d[3] = 65535 * alpha;
    alpha = alpha_s / alpha;

    a = s[k] + (s[k1] - s[k]) * (x - (float) m);
    b = s[l] + (s[l1] - s[n1]) * (x - (float) m);
    d[0] = d[0] * (1.0f - alpha) + (a + (b - a) * (y - (float) n)) * alpha;

    a = s[k + 1] + (s[k1 + 1] - s[k + 1]) * (x - (float) m);
    b = s[l + 1] + (s[l1 + 1] - s[n1 + 1]) * (x - (float) m);
    d[1] = d[1] * (1.0f - alpha) + (a + (b - a) * (y - (float) n)) * alpha;

    a = s[k + 2] + (s[k1 + 2] - s[k + 2]) * (x - (float) m);
    b = s[l + 2] + (s[l1 + 2] - s[n1 + 2]) * (x - (float) m);
    d[2] = d[2] * (1.0f - alpha) + (a + (b - a) * (y - (float) n)) * alpha;

    return 0;
}

// bicubic

int interpBC_b32(uint8_t *s, int w, int h, float x, float y, float o, uint8_t *d, int is_atop)
{
    int i, j, b, l, m, n;
    float k;
    float p[4], p1[4], p2[4], p3[4], p4[4];
    float alpha = 1.0;

#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    m = (int) ceilf(x) - 2;
    if (m < 0)
        m = 0;
    if ((m + 5) > w)
        m = w - 4;
    n = (int) ceilf(y) - 2;
    if (n < 0)
        n = 0;
    if ((n + 5) > h)
        n = h - 4;

    for (b = 3; b > -1; b--) {
        // first after y (four columns)
        for (i = 0; i < 4; i++) {
            l = m + (i + n) * w;
            p1[i] = s[4 * l + b];
            p2[i] = s[4 * (l + 1) + b];
            p3[i] = s[4 * (l + 2) + b];
            p4[i] = s[4 * (l + 3) + b];
        }
        for (j = 1; j < 4; j++)
            for (i = 3; i >= j; i--) {
                k = (y - i - n) / j;
                p1[i] = p1[i] + k * (p1[i] - p1[i - 1]);
                p2[i] = p2[i] + k * (p2[i] - p2[i - 1]);
                p3[i] = p3[i] + k * (p3[i] - p3[i - 1]);
                p4[i] = p4[i] + k * (p4[i] - p4[i - 1]);
            }

        // now after x
        p[0] = p1[3];
        p[1] = p2[3];
        p[2] = p3[3];
        p[3] = p4[3];
        for (j = 1; j < 4; j++)
            for (i = 3; i >= j; i--)
                p[i] = p[i] + (x - i - m) / j * (p[i] - p[i - 1]);

        if (p[3] < 0.0f)
            p[3] = 0.0f;
        if (p[3] > 255.0f)
            p[3] = 255.0f;

        if (b == 3) {
            float alpha_s = (float) p[3] / 255.0f * o;
            float alpha_d = (float) d[3] / 255.0f;
            alpha = alpha_s + alpha_d - alpha_s * alpha_d;
            d[3] = is_atop ? p[3] : (255 * alpha);
            alpha = alpha_s / alpha;
        } else {
            d[b] = d[b] * (1.0f - alpha) + p[3] * alpha;
        }
    }

    return 0;
}

int interpBC_b64(uint16_t *s, int w, int h, float x, float y, float o, uint16_t *d, int is_atop)
{
    int i, j, b, l, m, n;
    float k;
    float p[4], p1[4], p2[4], p3[4], p4[4];
    float alpha = 1.0;

#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    m = (int) ceilf(x) - 2;
    if (m < 0)
        m = 0;
    if ((m + 5) > w)
        m = w - 4;
    n = (int) ceilf(y) - 2;
    if (n < 0)
        n = 0;
    if ((n + 5) > h)
        n = h - 4;

    for (b = 3; b > -1; b--) {
        // first after y (four columns)
        for (i = 0; i < 4; i++) {
            l = m + (i + n) * w;
            p1[i] = s[4 * l + b];
            p2[i] = s[4 * (l + 1) + b];
            p3[i] = s[4 * (l + 2) + b];
            p4[i] = s[4 * (l + 3) + b];
        }
        for (j = 1; j < 4; j++)
            for (i = 3; i >= j; i--) {
                k = (y - i - n) / j;
                p1[i] = p1[i] + k * (p1[i] - p1[i - 1]);
                p2[i] = p2[i] + k * (p2[i] - p2[i - 1]);
                p3[i] = p3[i] + k * (p3[i] - p3[i - 1]);
                p4[i] = p4[i] + k * (p4[i] - p4[i - 1]);
            }

        // now after x
        p[0] = p1[3];
        p[1] = p2[3];
        p[2] = p3[3];
        p[3] = p4[3];
        for (j = 1; j < 4; j++)
            for (i = 3; i >= j; i--)
                p[i] = p[i] + (x - i - m) / j * (p[i] - p[i - 1]);

        if (p[3] < 0.0f)
            p[3] = 0.0f;
        if (p[3] > 65535.0f)
            p[3] = 65535.0f;

        if (b == 3) {
            float alpha_s = (float) p[3] / 65535.0f * o;
            float alpha_d = (float) d[3] / 65535.0f;
            alpha = alpha_s + alpha_d - alpha_s * alpha_d;
            d[3] = is_atop ? p[3] : (65535 * alpha);
            alpha = alpha_s / alpha;
        } else {
            d[b] = d[b] * (1.0f - alpha) + p[3] * alpha;
        }
    }

    return 0;
}
