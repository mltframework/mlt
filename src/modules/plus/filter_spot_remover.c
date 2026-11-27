/*
 * filter_remover.c -- filter to interpolate pixels to cover an area
 * Copyright (c) 2018-2025 Meltytech, LLC
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

#include <framework/mlt.h>
#include <framework/mlt_log.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

/** Scale a rectangle by the specified factors. */
static mlt_rect scale_rect(mlt_rect rect, double x_scale, double y_scale)
{
    rect.x = rect.x / x_scale;
    rect.y = rect.y / y_scale;
    rect.w = rect.w / x_scale;
    rect.h = rect.h / y_scale;
    return rect;
}

/** Constrain a rect to be within the max dimensions with an additional 1 pixel
  * padding.
  */
static mlt_rect constrain_rect(mlt_rect rect, int max_x, int max_y)
{
    rect.x = round(rect.x);
    rect.y = round(rect.y);
    rect.w = round(rect.w);
    rect.h = round(rect.h);
    if (rect.x < 0) {
        rect.w = rect.w + rect.x - 1;
        rect.x = 1;
    }
    if (rect.y < 0) {
        rect.h = rect.h + rect.y - 1;
        rect.y = 1;
    }
    if (rect.x + rect.w < 0) {
        rect.w = 0;
    }
    if (rect.y + rect.h < 0) {
        rect.h = 0;
    }
    if (rect.x < 1) {
        rect.x = 1;
    }
    if (rect.y < 1) {
        rect.y = 1;
    }
    if (rect.x + rect.w > max_x - 1) {
        rect.w = max_x - rect.x - 1;
    }
    if (rect.y + rect.h > max_y - 1) {
        rect.h = max_y - rect.y - 1;
    }
    return rect;
}

typedef struct
{
    void *chan[4];    // pointer to the first value in the channel
    int rowCount[4];  // the number of values in each line (row)
    int step[4];      // the space between values in each line
    int word[4];      // the number of bits in a word (8 or 16)
    mlt_rect rect[4]; // rect the area to be removed
} slice_desc;

typedef struct
{
    void *base;
    int row_stride; // in elements (width * 4)
    int word;       // 8 or 16
    mlt_rect rect;
    int xStart;
    int yStart;
    int w;
    int h;
    const double *xRatioArr;
    const double *xInvArr;
    const double *topArr; // 4 * w doubles
    const double *botArr; // 4 * w doubles
} rgba_slice_desc;

static int remove_spot_rgba_slice_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    const rgba_slice_desc *d = (const rgba_slice_desc *) data;
    int slice_y_start, slice_h = mlt_slices_size_slice(jobs, index, d->h, &slice_y_start);
    const int xStart = d->xStart;
    const int yStart = d->yStart;
    const int w = d->w;
    const int rowSize = d->row_stride;
    const double rect_y = d->rect.y;
    const double rect_h = d->rect.h;
    const int rect_w = (int) d->rect.w;
    const double *xRatioArr = d->xRatioArr;
    const double *xInvArr = d->xInvArr;
    const double *topArr = d->topArr;
    const double *botArr = d->botArr;

    if (d->word == 8) {
        uint8_t *base = (uint8_t *) d->base;
        for (int dy = slice_y_start; dy < slice_y_start + slice_h; dy++) {
            int y = yStart + dy;
            uint8_t *xValueL = base + (y * rowSize) + ((xStart - 1) * 4);
            uint8_t *xValueR = xValueL + (rect_w * 4);
            uint8_t *p = base + (y * rowSize) + (xStart * 4);
            double yRatio = 1.0 - ((y - rect_y) / rect_h);
            double yInv = 1.0 - yRatio;
#ifdef __SSE2__
            __m128d vL01 = _mm_set_pd((double) xValueL[1], (double) xValueL[0]);
            __m128d vL23 = _mm_set_pd((double) xValueL[3], (double) xValueL[2]);
            __m128d vR01 = _mm_set_pd((double) xValueR[1], (double) xValueR[0]);
            __m128d vR23 = _mm_set_pd((double) xValueR[3], (double) xValueR[2]);
            __m128d vyR = _mm_set1_pd(yRatio);
            __m128d vyI = _mm_set1_pd(yInv);
            for (int i = 0; i < w; i++, p += 4) {
                __m128d xr = _mm_set1_pd(xRatioArr[i]);
                __m128d xi = _mm_set1_pd(xInvArr[i]);
                const double *tp = topArr + i * 4;
                const double *bp = botArr + i * 4;

                __m128i x01 = _mm_cvttpd_epi32(
                    _mm_add_pd(_mm_mul_pd(vL01, xr), _mm_mul_pd(vR01, xi)));
                __m128i x23 = _mm_cvttpd_epi32(
                    _mm_add_pd(_mm_mul_pd(vL23, xr), _mm_mul_pd(vR23, xi)));
                __m128i y01 = _mm_cvttpd_epi32(_mm_add_pd(_mm_mul_pd(_mm_loadu_pd(tp), vyR),
                                                          _mm_mul_pd(_mm_loadu_pd(bp), vyI)));
                __m128i y23 = _mm_cvttpd_epi32(_mm_add_pd(_mm_mul_pd(_mm_loadu_pd(tp + 2), vyR),
                                                          _mm_mul_pd(_mm_loadu_pd(bp + 2), vyI)));

                __m128i s01 = _mm_srli_epi32(_mm_add_epi32(x01, y01), 1);
                __m128i s23 = _mm_srli_epi32(_mm_add_epi32(x23, y23), 1);
                __m128i s0123 = _mm_unpacklo_epi64(s01, s23);
                __m128i p16 = _mm_packs_epi32(s0123, s0123);
                __m128i p8 = _mm_packus_epi16(p16, p16);
                uint32_t pix = (uint32_t) _mm_cvtsi128_si32(p8);
                memcpy(p, &pix, 4);
            }
#else
            double vL[4] = {xValueL[0], xValueL[1], xValueL[2], xValueL[3]};
            double vR[4] = {xValueR[0], xValueR[1], xValueR[2], xValueR[3]};
            for (int i = 0; i < w; i++, p += 4) {
                double xr = xRatioArr[i];
                double xi = xInvArr[i];
                const double *tp = topArr + i * 4;
                const double *bp = botArr + i * 4;
                for (int c = 0; c < 4; c++) {
                    unsigned int xInterp = (unsigned int) (vL[c] * xr + vR[c] * xi);
                    unsigned int yInterp = (unsigned int) (tp[c] * yRatio + bp[c] * yInv);
                    unsigned int value = (xInterp + yInterp) / 2;
                    p[c] = (uint8_t) (value > 255 ? 255 : value);
                }
            }
#endif
        }
    } else {
        uint16_t *base = (uint16_t *) d->base;
        for (int dy = slice_y_start; dy < slice_y_start + slice_h; dy++) {
            int y = yStart + dy;
            uint16_t *xValueL = base + (y * rowSize) + ((xStart - 1) * 4);
            uint16_t *xValueR = xValueL + (rect_w * 4);
            uint16_t *p = base + (y * rowSize) + (xStart * 4);
            double yRatio = 1.0 - ((y - rect_y) / rect_h);
            double yInv = 1.0 - yRatio;
#ifdef __SSE2__
            __m128d vL01 = _mm_set_pd((double) xValueL[1], (double) xValueL[0]);
            __m128d vL23 = _mm_set_pd((double) xValueL[3], (double) xValueL[2]);
            __m128d vR01 = _mm_set_pd((double) xValueR[1], (double) xValueR[0]);
            __m128d vR23 = _mm_set_pd((double) xValueR[3], (double) xValueR[2]);
            __m128d vyR = _mm_set1_pd(yRatio);
            __m128d vyI = _mm_set1_pd(yInv);
            for (int i = 0; i < w; i++, p += 4) {
                __m128d xr = _mm_set1_pd(xRatioArr[i]);
                __m128d xi = _mm_set1_pd(xInvArr[i]);
                const double *tp = topArr + i * 4;
                const double *bp = botArr + i * 4;

                __m128i x01 = _mm_cvttpd_epi32(
                    _mm_add_pd(_mm_mul_pd(vL01, xr), _mm_mul_pd(vR01, xi)));
                __m128i x23 = _mm_cvttpd_epi32(
                    _mm_add_pd(_mm_mul_pd(vL23, xr), _mm_mul_pd(vR23, xi)));
                __m128i y01 = _mm_cvttpd_epi32(_mm_add_pd(_mm_mul_pd(_mm_loadu_pd(tp), vyR),
                                                          _mm_mul_pd(_mm_loadu_pd(bp), vyI)));
                __m128i y23 = _mm_cvttpd_epi32(_mm_add_pd(_mm_mul_pd(_mm_loadu_pd(tp + 2), vyR),
                                                          _mm_mul_pd(_mm_loadu_pd(bp + 2), vyI)));

                __m128i s01 = _mm_srli_epi32(_mm_add_epi32(x01, y01), 1);
                __m128i s23 = _mm_srli_epi32(_mm_add_epi32(x23, y23), 1);

                uint32_t v0 = (uint32_t) _mm_cvtsi128_si32(s01);
                uint32_t v1 = (uint32_t) _mm_cvtsi128_si32(_mm_srli_si128(s01, 4));
                uint32_t v2 = (uint32_t) _mm_cvtsi128_si32(s23);
                uint32_t v3 = (uint32_t) _mm_cvtsi128_si32(_mm_srli_si128(s23, 4));
                p[0] = (uint16_t) (v0 > 65535 ? 65535 : v0);
                p[1] = (uint16_t) (v1 > 65535 ? 65535 : v1);
                p[2] = (uint16_t) (v2 > 65535 ? 65535 : v2);
                p[3] = (uint16_t) (v3 > 65535 ? 65535 : v3);
            }
#else
            double vL[4] = {xValueL[0], xValueL[1], xValueL[2], xValueL[3]};
            double vR[4] = {xValueR[0], xValueR[1], xValueR[2], xValueR[3]};
            for (int i = 0; i < w; i++, p += 4) {
                double xr = xRatioArr[i];
                double xi = xInvArr[i];
                const double *tp = topArr + i * 4;
                const double *bp = botArr + i * 4;
                for (int c = 0; c < 4; c++) {
                    unsigned int xInterp = (unsigned int) (vL[c] * xr + vR[c] * xi);
                    unsigned int yInterp = (unsigned int) (tp[c] * yRatio + bp[c] * yInv);
                    unsigned int value = (xInterp + yInterp) / 2;
                    p[c] = (uint16_t) (value > 65535 ? 65535 : value);
                }
            }
#endif
        }
    }
    return 0;
}

/** Perform spot removal on a channel.
  *
  * Values within the rectangle are replaced with interpolated values.
  * Each value is an interpolation of the first values outside of the rect on
  * the top, bottom, left and right of the value being interpolated.
  */
static int remove_spot_channel_proc(int id, int index, int jobs, void *data)
{
    (void) id;   // unused
    (void) jobs; // unused
    slice_desc *desc = ((slice_desc *) data);
    int rowCount = desc->rowCount[index];
    int step = desc->step[index];
    int word = desc->word[index];
    mlt_rect rect = desc->rect[index];
    int xStart = (int) rect.x;
    int yStart = (int) rect.y;
    int yStop = (int) (rect.y + rect.h);
    int xStop = (int) (rect.x + rect.w);
    int w = xStop - xStart;
    int rowSize = rowCount * step;
    if (w <= 0 || yStop <= yStart)
        return 0;

    // Precompute xRatio, (1 - xRatio), top border, and bottom border once per channel
    double stack_buf[4 * 256];
    double *buf = (w <= 256) ? stack_buf : (double *) malloc((size_t) w * 4 * sizeof(double));
    if (!buf)
        return 0;
    double *xRatioArr = buf;
    double *xInvArr = buf + w;
    double *topArr = buf + 2 * w;
    double *botArr = buf + 3 * w;

    for (int i = 0; i < w; i++) {
        int x = xStart + i;
        double xr = 1.0 - ((x - rect.x) / rect.w);
        xRatioArr[i] = xr;
        xInvArr[i] = 1.0 - xr;
    }

    if (word == 8) {
        uint8_t *chan = desc->chan[index];
        const uint8_t *topRow = chan + ((yStart - 1) * rowSize) + (xStart * step);
        const uint8_t *botRow = topRow + (int) rect.h * rowSize;
        for (int i = 0; i < w; i++) {
            topArr[i] = (double) topRow[i * step];
            botArr[i] = (double) botRow[i * step];
        }

        for (int y = yStart; y < yStop; y++) {
            uint8_t *xValueL = chan + (y * rowSize) + ((xStart - 1) * step);
            uint8_t *xValueR = xValueL + ((int) rect.w * step);
            uint8_t *p = chan + (y * rowSize) + (xStart * step);
            double valL = (double) (*xValueL);
            double valR = (double) (*xValueR);
            double yRatio = 1.0 - ((y - rect.y) / rect.h);
            double yInv = 1.0 - yRatio;
            int i = 0;
#ifdef __SSE2__
            __m128d vL = _mm_set1_pd(valL);
            __m128d vR = _mm_set1_pd(valR);
            __m128d vyR = _mm_set1_pd(yRatio);
            __m128d vyI = _mm_set1_pd(yInv);
            for (; i <= w - 2; i += 2) {
                __m128d xr = _mm_loadu_pd(xRatioArr + i);
                __m128d xi = _mm_loadu_pd(xInvArr + i);
                __m128d vt = _mm_loadu_pd(topArr + i);
                __m128d vb = _mm_loadu_pd(botArr + i);

                __m128d xInterp = _mm_add_pd(_mm_mul_pd(vL, xr), _mm_mul_pd(vR, xi));
                __m128d yInterp = _mm_add_pd(_mm_mul_pd(vt, vyR), _mm_mul_pd(vb, vyI));

                __m128i xi32 = _mm_cvttpd_epi32(xInterp);
                __m128i yi32 = _mm_cvttpd_epi32(yInterp);
                __m128i sum = _mm_srli_epi32(_mm_add_epi32(xi32, yi32), 1);

                uint32_t v0 = (uint32_t) _mm_cvtsi128_si32(sum);
                uint32_t v1 = (uint32_t) _mm_cvtsi128_si32(_mm_srli_si128(sum, 4));
                if (v0 > 255)
                    v0 = 255;
                if (v1 > 255)
                    v1 = 255;
                p[0] = (uint8_t) v0;
                p[step] = (uint8_t) v1;
                p += 2 * step;
            }
#endif
            for (; i < w; i++) {
                unsigned int xValueInterp = (unsigned int) (valL * xRatioArr[i] + valR * xInvArr[i]);
                unsigned int yValueInterp = (unsigned int) (topArr[i] * yRatio + botArr[i] * yInv);
                unsigned int value = (xValueInterp + yValueInterp) / 2;
                if (value > 255)
                    value = 255;
                *p = (uint8_t) value;
                p += step;
            }
        }
    } else if (word == 16) {
        uint16_t *chan = desc->chan[index];
        const uint16_t *topRow = chan + ((yStart - 1) * rowSize) + (xStart * step);
        const uint16_t *botRow = topRow + (int) rect.h * rowSize;
        for (int i = 0; i < w; i++) {
            topArr[i] = (double) topRow[i * step];
            botArr[i] = (double) botRow[i * step];
        }

        for (int y = yStart; y < yStop; y++) {
            uint16_t *xValueL = chan + (y * rowSize) + ((xStart - 1) * step);
            uint16_t *xValueR = xValueL + ((int) rect.w * step);
            uint16_t *p = chan + (y * rowSize) + (xStart * step);
            double valL = (double) (*xValueL);
            double valR = (double) (*xValueR);
            double yRatio = 1.0 - ((y - rect.y) / rect.h);
            double yInv = 1.0 - yRatio;
            int i = 0;
#ifdef __SSE2__
            __m128d vL = _mm_set1_pd(valL);
            __m128d vR = _mm_set1_pd(valR);
            __m128d vyR = _mm_set1_pd(yRatio);
            __m128d vyI = _mm_set1_pd(yInv);
            for (; i <= w - 2; i += 2) {
                __m128d xr = _mm_loadu_pd(xRatioArr + i);
                __m128d xi = _mm_loadu_pd(xInvArr + i);
                __m128d vt = _mm_loadu_pd(topArr + i);
                __m128d vb = _mm_loadu_pd(botArr + i);

                __m128d xInterp = _mm_add_pd(_mm_mul_pd(vL, xr), _mm_mul_pd(vR, xi));
                __m128d yInterp = _mm_add_pd(_mm_mul_pd(vt, vyR), _mm_mul_pd(vb, vyI));

                __m128i xi32 = _mm_cvttpd_epi32(xInterp);
                __m128i yi32 = _mm_cvttpd_epi32(yInterp);
                __m128i sum = _mm_srli_epi32(_mm_add_epi32(xi32, yi32), 1);

                uint32_t v0 = (uint32_t) _mm_cvtsi128_si32(sum);
                uint32_t v1 = (uint32_t) _mm_cvtsi128_si32(_mm_srli_si128(sum, 4));
                if (v0 > 65535)
                    v0 = 65535;
                if (v1 > 65535)
                    v1 = 65535;
                p[0] = (uint16_t) v0;
                p[step] = (uint16_t) v1;
                p += 2 * step;
            }
#endif
            for (; i < w; i++) {
                unsigned int xValueInterp = (unsigned int) (valL * xRatioArr[i] + valR * xInvArr[i]);
                unsigned int yValueInterp = (unsigned int) (topArr[i] * yRatio + botArr[i] * yInv);
                unsigned int value = (xValueInterp + yValueInterp) / 2;
                if (value > 65535)
                    value = 65535;
                *p = (uint16_t) value;
                p += step;
            }
        }
    }

    if (buf != stack_buf)
        free(buf);
    return 0;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    char *rect_str = mlt_properties_get(filter_properties, "rect");
    if (!rect_str) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "rect property not set\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    mlt_rect rect = mlt_properties_anim_get_rect(filter_properties, "rect", position, length);
    if (strchr(rect_str, '%')) {
        rect.x *= profile->width;
        rect.w *= profile->width;
        rect.y *= profile->height;
        rect.h *= profile->height;
    }
    double scale = mlt_profile_scale_width(profile, *width);
    rect.x *= scale;
    rect.w *= scale;
    scale = mlt_profile_scale_height(profile, *height);
    rect.y *= scale;
    rect.h *= scale;
    rect = constrain_rect(rect, profile->width * scale, profile->height * scale);
    if (rect.w < 1 || rect.h < 1) {
        mlt_log_info(MLT_FILTER_SERVICE(filter), "rect invalid\n");
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    switch (*format) {
    case mlt_image_rgba:
    case mlt_image_rgb:
    case mlt_image_yuv422:
    case mlt_image_yuv420p:
    case mlt_image_rgba64:
        // These formats are all supported
        break;
    default:
        *format = mlt_image_rgba;
        break;
    }
    error = mlt_frame_get_image(frame, image, format, width, height, 1);
    if (error)
        return error;

    struct mlt_image_s img;
    mlt_image_set_values(&img, *image, *format, *width, *height);

    // Row-sliced contiguous path for packed 4-channel RGBA / RGBA64 avoids false sharing
    if (*format == mlt_image_rgba || *format == mlt_image_rgba64) {
        int xStart = (int) rect.x;
        int yStart = (int) rect.y;
        int xStop = (int) (rect.x + rect.w);
        int yStop = (int) (rect.y + rect.h);
        int w = xStop - xStart;
        int h = yStop - yStart;
        if (w > 0 && h > 0) {
            double stack_buf[10 * 128];
            size_t need = (size_t) w * 10;
            double *buf = (w <= 128) ? stack_buf : (double *) malloc(need * sizeof(double));
            if (buf) {
                double *xRatioArr = buf;
                double *xInvArr = buf + w;
                double *topArr = buf + 2 * w;
                double *botArr = buf + 6 * w;
                for (int i = 0; i < w; i++) {
                    double xr = 1.0 - (((xStart + i) - rect.x) / rect.w);
                    xRatioArr[i] = xr;
                    xInvArr[i] = 1.0 - xr;
                }
                int rowSize = img.width * 4;
                if (*format == mlt_image_rgba) {
                    const uint8_t *topRow = img.planes[0] + ((yStart - 1) * rowSize) + (xStart * 4);
                    const uint8_t *botRow = topRow + (int) rect.h * rowSize;
                    for (int i = 0; i < w * 4; i++) {
                        topArr[i] = (double) topRow[i];
                        botArr[i] = (double) botRow[i];
                    }
                } else {
                    const uint16_t *topRow = ((const uint16_t *) img.planes[0])
                                             + ((yStart - 1) * rowSize) + (xStart * 4);
                    const uint16_t *botRow = topRow + (int) rect.h * rowSize;
                    for (int i = 0; i < w * 4; i++) {
                        topArr[i] = (double) topRow[i];
                        botArr[i] = (double) botRow[i];
                    }
                }
                rgba_slice_desc rdesc = {
                    .base = img.planes[0],
                    .row_stride = rowSize,
                    .word = (*format == mlt_image_rgba) ? 8 : 16,
                    .rect = rect,
                    .xStart = xStart,
                    .yStart = yStart,
                    .w = w,
                    .h = h,
                    .xRatioArr = xRatioArr,
                    .xInvArr = xInvArr,
                    .topArr = topArr,
                    .botArr = botArr,
                };
                mlt_slices_run_normal(0, remove_spot_rgba_slice_proc, &rdesc);
                if (buf != stack_buf)
                    free(buf);
            }
        }
        return error;
    }

    // Process each plane in a separate thread.
    int i;
    slice_desc desc;
    int jobs = 0;
    switch (*format) {
    case mlt_image_rgb:
        jobs = 3;
        for (i = 0; i < 3; i++) {
            desc.chan[i] = img.planes[0] + i;
            desc.rowCount[i] = img.width;
            desc.step[i] = 4;
            desc.word[i] = 8;
            desc.rect[i] = rect;
        }
        break;
    case mlt_image_yuv422:
        jobs = 3;
        // Y
        desc.chan[0] = img.planes[0];
        desc.rowCount[0] = img.width;
        desc.step[0] = 2;
        desc.word[0] = 8;
        desc.rect[0] = rect;
        // U
        desc.chan[1] = img.planes[0] + 1;
        desc.rowCount[1] = img.width / 2;
        desc.step[1] = 4;
        desc.word[1] = 8;
        desc.rect[1] = constrain_rect(scale_rect(rect, 2, 1), img.width / 2, img.height);
        // V
        desc.chan[2] = img.planes[0] + 3;
        desc.rowCount[2] = img.width / 2;
        desc.step[2] = 4;
        desc.word[2] = 8;
        desc.rect[2] = constrain_rect(scale_rect(rect, 2, 1), img.width / 2, img.height);
        break;
    case mlt_image_yuv420p:
        jobs = 3;
        // Y
        desc.chan[0] = img.planes[0];
        desc.rowCount[0] = img.width;
        desc.step[0] = 1;
        desc.word[0] = 8;
        desc.rect[0] = rect;
        // U
        desc.chan[1] = img.planes[1];
        desc.rowCount[1] = img.width / 2;
        desc.step[1] = 1;
        desc.word[1] = 8;
        desc.rect[1] = constrain_rect(scale_rect(rect, 2, 2), img.width / 2, img.height / 2);
        // V
        desc.chan[2] = img.planes[2];
        desc.rowCount[2] = img.width / 2;
        desc.step[2] = 1;
        desc.word[2] = 8;
        desc.rect[2] = constrain_rect(scale_rect(rect, 2, 2), img.width / 2, img.height / 2);
        break;
    default:
        return 1;
    }

    uint8_t *alpha = mlt_frame_get_alpha(frame);
    if (alpha && *format != mlt_image_rgba && *format != mlt_image_rgba64) {
        jobs++;
        desc.chan[3] = alpha;
        desc.rowCount[3] = img.width;
        desc.step[3] = 1;
        desc.word[3] = 8;
        desc.rect[3] = rect;
    }

    mlt_slices_run_normal(jobs, remove_spot_channel_proc, &desc);

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_spot_remover_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set(properties, "rect", "0% 0% 10% 10%");
        filter->process = filter_process;
    } else {
        mlt_log_error(NULL, "Filter spot_remover initialization failed\n");
    }
    return filter;
}
