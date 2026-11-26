/*
 * composite_line_yuv_sse2_simple.c
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <emmintrin.h>
#include <immintrin.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

static inline __m128i sse2_div255_epu16(__m128i v, __m128i c128)
{
    __m128i t = _mm_srli_epi16(v, 8);
    t = _mm_add_epi16(t, v);
    t = _mm_add_epi16(t, c128);
    return _mm_srli_epi16(t, 8);
}

static inline void sse2_blend_dest_alpha(uint8_t *dest_a,
                                         __m128i src_a16,
                                         __m128i zero,
                                         __m128i c255,
                                         __m128i c128)
{
    __m128i da = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *) dest_a), zero);
    __m128i inv_da = _mm_sub_epi16(c255, da);
    __m128i prod = _mm_mullo_epi16(src_a16, inv_da);
    __m128i add = sse2_div255_epu16(prod, c128);
    da = _mm_add_epi16(da, add);
    _mm_storel_epi64((__m128i *) dest_a, _mm_packus_epi16(da, zero));
}

static inline void sse2_blend_pixels(
    uint8_t *dest, const uint8_t *src, __m128i a16, __m128i zero, __m128i c128)
{
    __m128i s = _mm_loadu_si128((const __m128i *) src);
    __m128i d = _mm_loadu_si128((const __m128i *) dest);

    __m128i s_lo = _mm_unpacklo_epi8(s, zero);
    __m128i d_lo = _mm_unpacklo_epi8(d, zero);
    __m128i s_hi = _mm_unpackhi_epi8(s, zero);
    __m128i d_hi = _mm_unpackhi_epi8(d, zero);

    __m128i a_lo = _mm_unpacklo_epi16(a16, a16);
    __m128i a_hi = _mm_unpackhi_epi16(a16, a16);

    __m128i diff_lo = _mm_sub_epi16(s_lo, d_lo);
    __m128i diff_hi = _mm_sub_epi16(s_hi, d_hi);

    __m128i prod_lo = _mm_mullo_epi16(diff_lo, a_lo);
    __m128i prod_hi = _mm_mullo_epi16(diff_hi, a_hi);

    /* dst * 255 = (dst << 8) - dst */
    __m128i d255_lo = _mm_sub_epi16(_mm_slli_epi16(d_lo, 8), d_lo);
    __m128i d255_hi = _mm_sub_epi16(_mm_slli_epi16(d_hi, 8), d_hi);

    __m128i res_lo = sse2_div255_epu16(_mm_add_epi16(d255_lo, prod_lo), c128);
    __m128i res_hi = sse2_div255_epu16(_mm_add_epi16(d255_hi, prod_hi), c128);

    _mm_storeu_si128((__m128i *) dest, _mm_packus_epi16(res_lo, res_hi));
}

static void blend_sse2_dispatch(uint8_t *restrict dest,
                                const uint8_t *restrict src,
                                int width,
                                const uint8_t *restrict src_a,
                                uint8_t *restrict dest_a,
                                int weight,
                                int cond)
{
    int w8 = width / 8;
    const __m128i zero = _mm_setzero_si128();
    const __m128i c255 = _mm_set1_epi16(0x00FF);
    const __m128i c128 = _mm_set1_epi16(0x0080);
    const __m128i wv = _mm_set1_epi16((short) weight);

    switch (cond) {
    case 1:
        for (; w8 > 0; w8--, src += 16, dest += 16) {
            sse2_blend_pixels(dest, src, wv, zero, c128);
        }
        break;
    case 2:
        for (; w8 > 0; w8--, src += 16, dest += 16, src_a += 8) {
            __m128i a16 = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *) src_a), zero);
            sse2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    case 3:
        for (; w8 > 0; w8--, src += 16, dest += 16, src_a += 8) {
            __m128i a16 = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *) src_a), zero);
            a16 = _mm_srli_epi16(_mm_mullo_epi16(a16, wv), 8);
            sse2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    case 5:
        for (; w8 > 0; w8--, src += 16, dest += 16, dest_a += 8) {
            sse2_blend_dest_alpha(dest_a, wv, zero, c255, c128);
            sse2_blend_pixels(dest, src, wv, zero, c128);
        }
        break;
    case 6:
        for (; w8 > 0; w8--, src += 16, dest += 16, src_a += 8, dest_a += 8) {
            __m128i a16 = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *) src_a), zero);
            sse2_blend_dest_alpha(dest_a, a16, zero, c255, c128);
            sse2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    case 7:
        for (; w8 > 0; w8--, src += 16, dest += 16, src_a += 8, dest_a += 8) {
            __m128i a16 = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *) src_a), zero);
            a16 = _mm_srli_epi16(_mm_mullo_epi16(a16, wv), 8);
            sse2_blend_dest_alpha(dest_a, a16, zero, c255, c128);
            sse2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    default:
        break;
    }
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2"))) static inline __m256i avx2_div255_epu16(__m256i v, __m256i c128)
{
    __m256i t = _mm256_srli_epi16(v, 8);
    t = _mm256_add_epi16(t, v);
    t = _mm256_add_epi16(t, c128);
    return _mm256_srli_epi16(t, 8);
}

__attribute__((target("avx2"))) static inline void avx2_blend_dest_alpha(
    uint8_t *dest_a, __m256i src_a16, __m256i c255, __m256i c128)
{
    __m256i da = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *) dest_a));
    __m256i inv_da = _mm256_sub_epi16(c255, da);
    __m256i prod = _mm256_mullo_epi16(src_a16, inv_da);
    __m256i add = avx2_div255_epu16(prod, c128);
    da = _mm256_add_epi16(da, add);
    __m256i packed = _mm256_packus_epi16(da, da);
    packed = _mm256_permute4x64_epi64(packed, 0xD8);
    _mm_storeu_si128((__m128i *) dest_a, _mm256_castsi256_si128(packed));
}

__attribute__((target("avx2"))) static inline void avx2_blend_pixels(
    uint8_t *dest, const uint8_t *src, __m256i a16, __m256i zero, __m256i c128)
{
    __m256i s = _mm256_loadu_si256((const __m256i *) src);
    __m256i d = _mm256_loadu_si256((const __m256i *) dest);

    /* a16 holds [a0..a3, a4..a7 | a8..a11, a12..a15], matching unpacklo/unpackhi_epi8(s, 0) directly */
    __m256i a_lo = _mm256_unpacklo_epi16(a16, a16);
    __m256i a_hi = _mm256_unpackhi_epi16(a16, a16);

    __m256i s_lo = _mm256_unpacklo_epi8(s, zero);
    __m256i d_lo = _mm256_unpacklo_epi8(d, zero);
    __m256i s_hi = _mm256_unpackhi_epi8(s, zero);
    __m256i d_hi = _mm256_unpackhi_epi8(d, zero);

    __m256i prod_lo = _mm256_mullo_epi16(_mm256_sub_epi16(s_lo, d_lo), a_lo);
    __m256i prod_hi = _mm256_mullo_epi16(_mm256_sub_epi16(s_hi, d_hi), a_hi);

    __m256i d255_lo = _mm256_sub_epi16(_mm256_slli_epi16(d_lo, 8), d_lo);
    __m256i d255_hi = _mm256_sub_epi16(_mm256_slli_epi16(d_hi, 8), d_hi);

    __m256i res_lo = avx2_div255_epu16(_mm256_add_epi16(d255_lo, prod_lo), c128);
    __m256i res_hi = avx2_div255_epu16(_mm256_add_epi16(d255_hi, prod_hi), c128);

    _mm256_storeu_si256((__m256i *) dest, _mm256_packus_epi16(res_lo, res_hi));
}

__attribute__((target("avx2"))) static void blend_avx2_dispatch(uint8_t *restrict dest,
                                                                const uint8_t *restrict src,
                                                                int width,
                                                                const uint8_t *restrict src_a,
                                                                uint8_t *restrict dest_a,
                                                                int weight,
                                                                int cond)
{
    int w16 = width / 16;
    const __m256i zero = _mm256_setzero_si256();
    const __m256i c255 = _mm256_set1_epi16(0x00FF);
    const __m256i c128 = _mm256_set1_epi16(0x0080);
    const __m256i wv = _mm256_set1_epi16((short) weight);

    switch (cond) {
    case 1:
        for (; w16 > 0; w16--, src += 32, dest += 32) {
            avx2_blend_pixels(dest, src, wv, zero, c128);
        }
        break;
    case 2:
        for (; w16 > 0; w16--, src += 32, dest += 32, src_a += 16) {
            __m256i a16 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *) src_a));
            avx2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    case 3:
        for (; w16 > 0; w16--, src += 32, dest += 32, src_a += 16) {
            __m256i a16 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *) src_a));
            a16 = _mm256_srli_epi16(_mm256_mullo_epi16(a16, wv), 8);
            avx2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    case 5:
        for (; w16 > 0; w16--, src += 32, dest += 32, dest_a += 16) {
            avx2_blend_dest_alpha(dest_a, wv, c255, c128);
            avx2_blend_pixels(dest, src, wv, zero, c128);
        }
        break;
    case 6:
        for (; w16 > 0; w16--, src += 32, dest += 32, src_a += 16, dest_a += 16) {
            __m256i a16 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *) src_a));
            avx2_blend_dest_alpha(dest_a, a16, c255, c128);
            avx2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    case 7:
        for (; w16 > 0; w16--, src += 32, dest += 32, src_a += 16, dest_a += 16) {
            __m256i a16 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *) src_a));
            a16 = _mm256_srli_epi16(_mm256_mullo_epi16(a16, wv), 8);
            avx2_blend_dest_alpha(dest_a, a16, c255, c128);
            avx2_blend_pixels(dest, src, a16, zero, c128);
        }
        break;
    default:
        break;
    }

    int rem = width & 15;
    if (rem >= 8) {
        blend_sse2_dispatch(dest, src, rem, src_a, dest_a, weight, cond);
    }
}

static int has_avx2(void)
{
    static int cached = -1;
    if (cached < 0) {
        __builtin_cpu_init();
        cached = __builtin_cpu_supports("avx2") ? 1 : 0;
    }
    return cached;
}
#endif

void composite_line_yuv_sse2_simple(
    uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, uint8_t *dest_a, int weight)
{
    weight >>= 8;

    /*
        | 0   | dest_a == NULL | src_a == NULL | weight == 256 | blit
        | 1   | dest_a == NULL | src_a == NULL | weight != 256 | blend: with given alpha
        | 2   | dest_a == NULL | src_a != NULL | weight == 256 | blend: only src alpha
        | 3   | dest_a == NULL | src_a != NULL | weight != 256 | blend: premultiply src alpha
        | 4   | dest_a != NULL | src_a == NULL | weight == 256 | blit: blit and set dst alpha to FF
        | 5   | dest_a != NULL | src_a == NULL | weight != 256 | blend: with given alpha
        | 6   | dest_a != NULL | src_a != NULL | weight == 256 | blend: full blend without src alpha premutiply
        | 7   | dest_a != NULL | src_a != NULL | weight != 256 | blend: full (origin version)
    */

    int cond = ((dest_a != NULL) ? 4 : 0) + ((src_a != NULL) ? 2 : 0) + ((weight != 256) ? 1 : 0);

    if (cond == 0) {
        memcpy(dest, src, 2 * width);
        return;
    }
    if (cond == 4) {
        memcpy(dest, src, 2 * width);
        memset(dest_a, 0xFF, width);
        return;
    }

#if defined(__GNUC__) || defined(__clang__)
    if (width >= 16 && has_avx2()) {
        blend_avx2_dispatch(dest, src, width, src_a, dest_a, weight, cond);
        return;
    }
#endif
    blend_sse2_dispatch(dest, src, width, src_a, dest_a, weight, cond);
}
