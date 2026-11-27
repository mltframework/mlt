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
#include <string.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

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

static inline int fast_rintf_to_int(float x)
{
#if defined(__SSE2__)
    return _mm_cvtss_si32(_mm_set_ss(x));
#else
    return (int) rintf(x);
#endif
}

static inline int fast_floorf_to_int(float x)
{
    int i = (int) x;
    return i - ((float) i > x);
}

static inline int fast_ceilf_to_int(float x)
{
    int i = (int) x;
    return i + ((float) i < x);
}

#if defined(__SSE2__)
static inline __m128 load_rgba8_ps(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    __m128i zero = _mm_setzero_si128();
    __m128i v8 = _mm_cvtsi32_si128((int) v);
    __m128i v16 = _mm_unpacklo_epi8(v8, zero);
    __m128i v32 = _mm_unpacklo_epi16(v16, zero);
    return _mm_cvtepi32_ps(v32);
}

static inline void blend_rgb8_sse2(uint8_t *d, __m128 src_ps, float alpha)
{
    __m128 vd = load_rgba8_ps(d);
    __m128 res = _mm_add_ps(_mm_mul_ps(vd, _mm_set1_ps(1.0f - alpha)),
                            _mm_mul_ps(src_ps, _mm_set1_ps(alpha)));
    __m128i r32 = _mm_cvttps_epi32(res);
    __m128i r16 = _mm_packs_epi32(r32, r32);
    __m128i r8 = _mm_packus_epi16(r16, r16);
    uint32_t packed = (uint32_t) _mm_cvtsi128_si32(r8);
    d[0] = (uint8_t) (packed);
    d[1] = (uint8_t) (packed >> 8);
    d[2] = (uint8_t) (packed >> 16);
}
#endif

// nearest neighbor

static inline int interpNN_b32(
    uint8_t *s, int w, int h, float x, float y, float o, uint8_t *d, int is_atop)
{
#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif
    (void) h;
    int p = (fast_rintf_to_int(x) + fast_rintf_to_int(y) * w) * 4;
    const uint8_t *sp = s + p;
    float alpha_s = (float) sp[3] / 255.0f * o;
    float alpha_d = (float) d[3] / 255.0f;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    d[3] = is_atop ? sp[3] : (uint8_t) (255 * alpha);
    alpha = alpha_s / alpha;
    float inv_alpha = 1.0f - alpha;
    d[0] = d[0] * inv_alpha + sp[0] * alpha;
    d[1] = d[1] * inv_alpha + sp[1] * alpha;
    d[2] = d[2] * inv_alpha + sp[2] * alpha;

    return 0;
}

static inline int interpNN_b64(
    uint16_t *s, int w, int h, float x, float y, float o, uint16_t *d, int is_atop)
{
#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif
    (void) h;
    int p = (fast_rintf_to_int(x) + fast_rintf_to_int(y) * w) * 4;
    const uint16_t *sp = s + p;
    float alpha_s = (float) sp[3] / 65535.0f * o;
    float alpha_d = (float) d[3] / 65535.0f;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    d[3] = is_atop ? sp[3] : (uint16_t) (65535 * alpha);
    alpha = alpha_s / alpha;
    float inv_alpha = 1.0f - alpha;
    d[0] = d[0] * inv_alpha + sp[0] * alpha;
    d[1] = d[1] * inv_alpha + sp[1] * alpha;
    d[2] = d[2] * inv_alpha + sp[2] * alpha;

    return 0;
}

//  bilinear

static inline int interpBL_b32(
    uint8_t *s, int w, int h, float x, float y, float o, uint8_t *d, int is_atop)
{
#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    int m = fast_floorf_to_int(x);
    if (m + 2 > w)
        m = w - 2;
    int n = fast_floorf_to_int(y);
    if (n + 2 > h)
        n = h - 2;

    float fx = x - (float) m;
    float fy = y - (float) n;
    int w4 = w * 4;
    const uint8_t *p00 = s + (n * w + m) * 4;
    const uint8_t *p01 = p00 + w4;

#if defined(__SSE2__)
    __m128 v00 = load_rgba8_ps(p00);
    __m128 v10 = load_rgba8_ps(p00 + 4);
    __m128 v01 = load_rgba8_ps(p01);
    __m128 v11 = load_rgba8_ps(p01 + 4);
    __m128 vfx = _mm_set1_ps(fx);
    __m128 vfy = _mm_set1_ps(fy);
    __m128 va = _mm_add_ps(v00, _mm_mul_ps(_mm_sub_ps(v10, v00), vfx));
    __m128 vb = _mm_add_ps(v01, _mm_mul_ps(_mm_sub_ps(v11, v01), vfx));
    __m128 vres = _mm_add_ps(va, _mm_mul_ps(_mm_sub_ps(vb, va), vfy));

    float alpha_s = _mm_cvtss_f32(_mm_shuffle_ps(vres, vres, _MM_SHUFFLE(3, 3, 3, 3)));
    float alpha_d = (float) d[3] / 255.0f;
    if (is_atop)
        d[3] = (uint8_t) alpha_s;
    alpha_s = alpha_s / 255.0f * o;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    if (!is_atop)
        d[3] = (uint8_t) (255 * alpha);
    alpha = alpha_s / alpha;

    blend_rgb8_sse2(d, vres, alpha);
#else
    float a = p00[3] + (p00[7] - p00[3]) * fx;
    float b = p01[3] + (p01[7] - p01[3]) * fx;

    float alpha_s = a + (b - a) * fy;
    float alpha_d = (float) d[3] / 255.0f;
    if (is_atop)
        d[3] = (uint8_t) alpha_s;
    alpha_s = alpha_s / 255.0f * o;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    if (!is_atop)
        d[3] = (uint8_t) (255 * alpha);
    alpha = alpha_s / alpha;
    float inv_alpha = 1.0f - alpha;

    a = p00[0] + (p00[4] - p00[0]) * fx;
    b = p01[0] + (p01[4] - p01[0]) * fx;
    d[0] = d[0] * inv_alpha + (a + (b - a) * fy) * alpha;

    a = p00[1] + (p00[5] - p00[1]) * fx;
    b = p01[1] + (p01[5] - p01[1]) * fx;
    d[1] = d[1] * inv_alpha + (a + (b - a) * fy) * alpha;

    a = p00[2] + (p00[6] - p00[2]) * fx;
    b = p01[2] + (p01[6] - p01[2]) * fx;
    d[2] = d[2] * inv_alpha + (a + (b - a) * fy) * alpha;
#endif

    return 0;
}

static inline int interpBL_b64(
    uint16_t *s, int w, int h, float x, float y, float o, uint16_t *d, int is_atop)
{
#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    int m = fast_floorf_to_int(x);
    if (m + 2 > w)
        m = w - 2;
    int n = fast_floorf_to_int(y);
    if (n + 2 > h)
        n = h - 2;

    float fx = x - (float) m;
    float fy = y - (float) n;
    int w4 = w * 4;
    const uint16_t *p00 = s + (n * w + m) * 4;
    const uint16_t *p01 = p00 + w4;

    float a = p00[3] + (p00[7] - p00[3]) * fx;
    float b = p01[3] + (p01[7] - p01[3]) * fx;

    float alpha_s = a + (b - a) * fy;
    float alpha_d = (float) d[3] / 65535.0f;
    if (is_atop)
        d[3] = (uint16_t) alpha_s;
    alpha_s = alpha_s / 65535.0f * o;
    float alpha = alpha_s + alpha_d - alpha_s * alpha_d;
    if (!is_atop)
        d[3] = (uint16_t) (65535 * alpha);
    alpha = alpha_s / alpha;
    float inv_alpha = 1.0f - alpha;

    a = p00[0] + (p00[4] - p00[0]) * fx;
    b = p01[0] + (p01[4] - p01[0]) * fx;
    d[0] = d[0] * inv_alpha + (a + (b - a) * fy) * alpha;

    a = p00[1] + (p00[5] - p00[1]) * fx;
    b = p01[1] + (p01[5] - p01[1]) * fx;
    d[1] = d[1] * inv_alpha + (a + (b - a) * fy) * alpha;

    a = p00[2] + (p00[6] - p00[2]) * fx;
    b = p01[2] + (p01[6] - p01[2]) * fx;
    d[2] = d[2] * inv_alpha + (a + (b - a) * fy) * alpha;

    return 0;
}

// bicubic

static inline int interpBC_b32(
    uint8_t *s, int w, int h, float x, float y, float o, uint8_t *d, int is_atop)
{
    int i, j, b, m, n;
    float p[4], p1[4], p2[4], p3[4], p4[4];
    float alpha = 1.0f;
    float inv_alpha = 0.0f;

#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    m = fast_ceilf_to_int(x) - 2;
    if (m < 0)
        m = 0;
    if ((m + 5) > w)
        m = w - 4;
    n = fast_ceilf_to_int(y) - 2;
    if (n < 0)
        n = 0;
    if ((n + 5) > h)
        n = h - 4;

    int w4 = w * 4;
    const uint8_t *row0 = s + (m + n * w) * 4;
    const uint8_t *row1 = row0 + w4;
    const uint8_t *row2 = row1 + w4;
    const uint8_t *row3 = row2 + w4;

    for (b = 3; b > -1; b--) {
        p1[0] = row0[b];
        p2[0] = row0[4 + b];
        p3[0] = row0[8 + b];
        p4[0] = row0[12 + b];
        p1[1] = row1[b];
        p2[1] = row1[4 + b];
        p3[1] = row1[8 + b];
        p4[1] = row1[12 + b];
        p1[2] = row2[b];
        p2[2] = row2[4 + b];
        p3[2] = row2[8 + b];
        p4[2] = row2[12 + b];
        p1[3] = row3[b];
        p2[3] = row3[4 + b];
        p3[3] = row3[8 + b];
        p4[3] = row3[12 + b];

        for (j = 1; j < 4; j++)
            for (i = 3; i >= j; i--) {
                float k = (y - i - n) / j;
                p1[i] = p1[i] + k * (p1[i] - p1[i - 1]);
                p2[i] = p2[i] + k * (p2[i] - p2[i - 1]);
                p3[i] = p3[i] + k * (p3[i] - p3[i - 1]);
                p4[i] = p4[i] + k * (p4[i] - p4[i - 1]);
            }

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
            inv_alpha = 1.0f - alpha;
        } else {
            d[b] = d[b] * inv_alpha + p[3] * alpha;
        }
    }

    return 0;
}

static inline int interpBC_b64(
    uint16_t *s, int w, int h, float x, float y, float o, uint16_t *d, int is_atop)
{
    int i, j, b, m, n;
    float p[4], p1[4], p2[4], p3[4], p4[4];
    float alpha = 1.0f;
    float inv_alpha = 0.0f;

#ifdef TEST_XY_LIMITS
    if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
        return -1;
#endif

    m = fast_ceilf_to_int(x) - 2;
    if (m < 0)
        m = 0;
    if ((m + 5) > w)
        m = w - 4;
    n = fast_ceilf_to_int(y) - 2;
    if (n < 0)
        n = 0;
    if ((n + 5) > h)
        n = h - 4;

    int w4 = w * 4;
    const uint16_t *row0 = s + (m + n * w) * 4;
    const uint16_t *row1 = row0 + w4;
    const uint16_t *row2 = row1 + w4;
    const uint16_t *row3 = row2 + w4;

    for (b = 3; b > -1; b--) {
        p1[0] = row0[b];
        p2[0] = row0[4 + b];
        p3[0] = row0[8 + b];
        p4[0] = row0[12 + b];
        p1[1] = row1[b];
        p2[1] = row1[4 + b];
        p3[1] = row1[8 + b];
        p4[1] = row1[12 + b];
        p1[2] = row2[b];
        p2[2] = row2[4 + b];
        p3[2] = row2[8 + b];
        p4[2] = row2[12 + b];
        p1[3] = row3[b];
        p2[3] = row3[4 + b];
        p3[3] = row3[8 + b];
        p4[3] = row3[12 + b];

        for (j = 1; j < 4; j++)
            for (i = 3; i >= j; i--) {
                float k = (y - i - n) / j;
                p1[i] = p1[i] + k * (p1[i] - p1[i - 1]);
                p2[i] = p2[i] + k * (p2[i] - p2[i - 1]);
                p3[i] = p3[i] + k * (p3[i] - p3[i - 1]);
                p4[i] = p4[i] + k * (p4[i] - p4[i - 1]);
            }

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
            inv_alpha = 1.0f - alpha;
        } else {
            d[b] = d[b] * inv_alpha + p[3] * alpha;
        }
    }

    return 0;
}
