/*
 * filter_chroma.c -- Maps a chroma key to the alpha channel
 * Copyright (C) 2008-2025 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_slices.h>
#include <stdlib.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    int u_lo;
    int u_hi;
    int v_lo;
    int v_hi;
    int u2_lo;
    int u2_hi;
    int v2_lo;
    int v2_hi;
    uint8_t boundary_u[64];
    uint8_t boundary_v[64];
} chroma_hold_desc;

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    chroma_hold_desc *d = (chroma_hold_desc *) data;
    int start_row, slice_h = mlt_slices_size_slice(jobs, index, d->height, &start_row);
    int pairs = (slice_h * d->width) / 2;
    if (pairs <= 0)
        return 0;

    uint8_t *p = d->image + (size_t) start_row * d->width * 2;
    const int u_lo = d->u_lo;
    const int u_hi = d->u_hi;
    const int v_lo = d->v_lo;
    const int v_hi = d->v_hi;
    const int u2_lo = d->u2_lo;
    const int u2_hi = d->u2_hi;
    const int v2_lo = d->v2_lo;
    const int v2_hi = d->v2_hi;

    int main_pairs = (index + 1 < jobs) ? (pairs - 1) : pairs;
    if (u_hi >= u_lo) {
        const unsigned u_span = (unsigned) (u_hi - u_lo);
        const unsigned v_span = (unsigned) (v_hi - v_lo);
        const unsigned u2_span = (unsigned) (u2_hi - u2_lo);
        const unsigned v2_span = (unsigned) (v2_hi - v2_lo);
        int i = 0;
#if defined(__SSE2__)
        const __m128i vu_lo = _mm_set1_epi16((short) (u_lo - 1));
        const __m128i vu_hi = _mm_set1_epi16((short) (u_hi + 1));
        const __m128i vv_lo = _mm_set1_epi16((short) (v_lo - 1));
        const __m128i vv_hi = _mm_set1_epi16((short) (v_hi + 1));
        const __m128i vu2_lo = _mm_set1_epi16((short) (u2_lo - 1));
        const __m128i vu2_hi = _mm_set1_epi16((short) (u2_hi + 1));
        const __m128i vv2_lo = _mm_set1_epi16((short) (v2_lo - 1));
        const __m128i vv2_hi = _mm_set1_epi16((short) (v2_hi + 1));
        const __m128i v128 = _mm_set1_epi16(128);
        const __m128i mask_y = _mm_set1_epi16(0x00ff);
        const __m128i mask_lo8 = _mm_set1_epi32(0x000000ff);
        for (; i + 4 < main_pairs; i += 4, p += 16) {
            __m128i cur = _mm_loadu_si128((const __m128i *) p);
            __m128i nxt = _mm_loadu_si128((const __m128i *) (p + 4));
            /* Extract 4 U (bytes 1,5,9,13) and 4 V (bytes 3,7,11,15) into epi32 then pack to epi16 */
            __m128i cu32 = _mm_and_si128(_mm_srli_epi32(cur, 8), mask_lo8);
            __m128i cv32 = _mm_srli_epi32(cur, 24);
            __m128i nu32 = _mm_and_si128(_mm_srli_epi32(nxt, 8), mask_lo8);
            __m128i nv32 = _mm_srli_epi32(nxt, 24);
            __m128i cu = _mm_packs_epi32(cu32, cu32);
            __m128i cv = _mm_packs_epi32(cv32, cv32);
            __m128i nu = _mm_packs_epi32(nu32, nu32);
            __m128i nv = _mm_packs_epi32(nv32, nv32);

            __m128i keep0 = _mm_and_si128(_mm_and_si128(_mm_cmpgt_epi16(cu, vu_lo),
                                                        _mm_cmpgt_epi16(vu_hi, cu)),
                                          _mm_and_si128(_mm_cmpgt_epi16(cv, vv_lo),
                                                        _mm_cmpgt_epi16(vv_hi, cv)));
            __m128i pu = _mm_or_si128(_mm_and_si128(keep0, cu), _mm_andnot_si128(keep0, v128));

            __m128i su = _mm_add_epi16(pu, nu);
            __m128i sv = _mm_add_epi16(cv, nv);
            __m128i keep1 = _mm_and_si128(_mm_and_si128(_mm_cmpgt_epi16(su, vu2_lo),
                                                        _mm_cmpgt_epi16(vu2_hi, su)),
                                          _mm_and_si128(_mm_cmpgt_epi16(sv, vv2_lo),
                                                        _mm_cmpgt_epi16(vv2_hi, sv)));
            __m128i pv = _mm_or_si128(_mm_and_si128(keep1, cv), _mm_andnot_si128(keep1, v128));

            /* Interleave pu (in epi16) and pv (in epi16) into chroma high bytes */
            __m128i uv16 = _mm_unpacklo_epi16(pu, pv);
            __m128i chroma = _mm_slli_epi16(uv16, 8);
            __m128i out = _mm_or_si128(_mm_and_si128(cur, mask_y), chroma);
            _mm_storeu_si128((__m128i *) p, out);
        }
#endif
        for (; i < main_pairs; i++) {
            int pu = p[1];
            int pv = p[3];
            int keep0 = ((unsigned) (pu - u_lo) <= u_span) & ((unsigned) (pv - v_lo) <= v_span);
            pu = keep0 ? pu : 128;
            p[1] = (uint8_t) pu;
            int keep1 = ((unsigned) (pu + p[5] - u2_lo) <= u2_span)
                        & ((unsigned) (pv + p[7] - v2_lo) <= v2_span);
            p[3] = (uint8_t) (keep1 ? pv : 128);
            p += 4;
        }
    } else {
        for (int i = 0; i < main_pairs; i++) {
            p[1] = 128;
            p[3] = 128;
            p += 4;
        }
    }

    if (index + 1 < jobs) {
        int pu = p[1];
        int pv = p[3];
        if (pu < u_lo || pu > u_hi || pv < v_lo || pv > v_hi) {
            pu = 128;
            p[1] = 128;
        }
        int su = pu + d->boundary_u[index + 1];
        int sv = pv + d->boundary_v[index + 1];
        if (su < u2_lo || su > u2_hi || sv < v2_lo || sv > v2_hi) {
            p[3] = 128;
        }
    }

    return 0;
}

/** Get the images and map the chroma to the alpha of the frame.
*/

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter this = mlt_frame_pop_service(frame);
    mlt_position position = mlt_filter_get_position(this, frame);
    mlt_position length = mlt_filter_get_length2(this, frame);
    int variance = 200
                   * mlt_properties_anim_get_double(MLT_FILTER_PROPERTIES(this),
                                                    "variance",
                                                    position,
                                                    length);
    mlt_color key_val
        = mlt_properties_anim_get_color(MLT_FILTER_PROPERTIES(this), "key", position, length);
    key_val = mlt_color_convert_trc(key_val,
                                    mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc"));
    uint8_t r = key_val.r;
    uint8_t g = key_val.g;
    uint8_t b = key_val.b;
    uint8_t u, v;

    RGB2UV_601_SCALED(r, g, b, u, v);

    *format = mlt_image_yuv422;
    if (mlt_frame_get_image(frame, image, format, width, height, writable) == 0) {
        int jobs = mlt_slices_count_normal();
        if (jobs > 64)
            jobs = 64;
        chroma_hold_desc desc;
        desc.image = *image;
        desc.width = *width;
        desc.height = *height;
        desc.u_lo = (int) u - variance;
        desc.u_hi = (int) u + variance;
        desc.v_lo = (int) v - variance;
        desc.v_hi = (int) v + variance;
        desc.u2_lo = desc.u_lo * 2;
        desc.u2_hi = desc.u_hi * 2 + 1;
        desc.v2_lo = desc.v_lo * 2;
        desc.v2_hi = desc.v_hi * 2 + 1;

        for (int idx = 1; idx < jobs; idx++) {
            int start_row;
            mlt_slices_size_slice(jobs, idx, *height, &start_row);
            uint8_t *row = *image + (size_t) start_row * (*width) * 2;
            desc.boundary_u[idx] = row[1];
            desc.boundary_v[idx] = row[3];
        }
        mlt_slices_run_normal(jobs, sliced_proc, &desc);
    }

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter this, mlt_frame frame)
{
    mlt_frame_push_service(frame, this);
    mlt_frame_push_service(frame, filter_get_image);
    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_chroma_hold_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg)
{
    mlt_filter this = mlt_filter_new();
    if (this != NULL) {
        mlt_properties_set(MLT_FILTER_PROPERTIES(this), "key", arg == NULL ? "#c00000" : arg);
        mlt_properties_set_double(MLT_FILTER_PROPERTIES(this), "variance", 0.15);
        this->process = filter_process;
    }
    return this;
}
