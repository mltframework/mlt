/*
 * filter_mirror.c -- mirror filter
 * Copyright (C) 2003-2021 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_image.h>
#include <framework/mlt_slices.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MIRROR_NONE = 0,
    MIRROR_HORIZONTAL,
    MIRROR_VERTICAL,
    MIRROR_DIAGONAL,
    MIRROR_XDIAGONAL,
    MIRROR_FLIP,
    MIRROR_FLOP
} mirror_mode;

typedef struct
{
    mlt_image image;
    mirror_mode mode;
    int reverse;
} slice_desc;

static inline uint32_t swap_yuyv_luma(uint32_t v)
{
    /* [Y0, U, Y1, V] -> [Y1, U, Y0, V] */
    return (v & 0xFF00FF00u) | ((v >> 16) & 0x000000FFu) | ((v << 16) & 0x00FF0000u);
}

static inline void swap_row_bytes(uint8_t *restrict a, uint8_t *restrict b, size_t nbytes)
{
    uint8_t tmp[512];
    size_t off = 0;
    while (off + sizeof(tmp) <= nbytes) {
        memcpy(tmp, a + off, sizeof(tmp));
        memcpy(a + off, b + off, sizeof(tmp));
        memcpy(b + off, tmp, sizeof(tmp));
        off += sizeof(tmp);
    }
    if (off < nbytes) {
        size_t rem = nbytes - off;
        memcpy(tmp, a + off, rem);
        memcpy(a + off, b + off, rem);
        memcpy(b + off, tmp, rem);
    }
}

static int do_slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    const slice_desc *desc = (const slice_desc *) data;
    const mlt_image img = desc->image;
    const int width = img->width;
    const int height = img->height;
    const int stride0 = img->strides[0];
    const int stride3 = img->strides[3];
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    int slice_line_start_half,
        slice_height_half = mlt_slices_size_slice(jobs, index, height / 2, &slice_line_start_half);
    int slice_line_end_half = slice_line_start_half + slice_height_half;
    const int uneven_w = (width % 2) * 2;
    int i;

    if (desc->mode == MIRROR_HORIZONTAL) {
        for (i = slice_line_start; i < slice_line_end; i++) {
            uint8_t *p = img->planes[0] + (size_t) stride0 * i;
            uint8_t *q = p + width * 2;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            if (uneven_w == 0) {
                if (!desc->reverse) {
                    while (p < q) {
                        q -= 4;
                        uint32_t v;
                        memcpy(&v, q, 4);
                        v = swap_yuyv_luma(v);
                        memcpy(p, &v, 4);
                        p += 4;
                    }
                } else {
                    while (p < q) {
                        uint32_t v;
                        memcpy(&v, p, 4);
                        v = swap_yuyv_luma(v);
                        p += 4;
                        q -= 4;
                        memcpy(q, &v, 4);
                    }
                }
                continue;
            }
#endif
            if (!desc->reverse) {
                while (p < q) {
                    *p++ = *(q - 2);
                    *p++ = *(q - 3 - uneven_w);
                    *p++ = *(q - 4);
                    *p++ = *(q - 1 - uneven_w);
                    q -= 4;
                }
            } else {
                while (p < q) {
                    *(q - 2) = *p++;
                    *(q - 3 - uneven_w) = *p++;
                    *(q - 4) = *p++;
                    *(q - 1 - uneven_w) = *p++;
                    q -= 4;
                }
            }
        }
        if (img->planes[3]) {
            for (i = slice_line_start; i < slice_line_end; i++) {
                uint8_t *a = img->planes[3] + (size_t) stride3 * i;
                uint8_t *b = a + width - 1;
                if (!desc->reverse) {
                    while (a + 3 < b - 3) {
                        uint32_t v;
                        memcpy(&v, b - 3, 4);
                        v = __builtin_bswap32(v);
                        memcpy(a, &v, 4);
                        a += 4;
                        b -= 4;
                    }
                    while (a < b) {
                        *a++ = *b--;
                        *a++ = *b--;
                    }
                } else {
                    while (a + 3 < b - 3) {
                        uint32_t v;
                        memcpy(&v, a, 4);
                        v = __builtin_bswap32(v);
                        memcpy(b - 3, &v, 4);
                        a += 4;
                        b -= 4;
                    }
                    while (a < b) {
                        *b-- = *a++;
                        *b-- = *a++;
                    }
                }
            }
        }
    } else if (desc->mode == MIRROR_VERTICAL) {
        const size_t row_bytes = (size_t) width * 2;
        for (i = slice_line_start_half; i < slice_line_end_half; i++) {
            uint8_t *p = img->planes[0] + (size_t) stride0 * i;
            uint8_t *q = img->planes[0] + (size_t) stride0 * (height - i - 1);
            if (!desc->reverse) {
                memcpy(p, q, row_bytes);
            } else {
                memcpy(q, p, row_bytes);
            }
        }
        if (img->planes[3]) {
            for (i = slice_line_start_half; i < slice_line_end_half; i++) {
                uint8_t *a = img->planes[3] + (size_t) stride3 * i;
                uint8_t *b = img->planes[3] + (size_t) stride3 * (height - i - 1);
                if (!desc->reverse)
                    memcpy(a, b, (size_t) width);
                else
                    memcpy(b, a, (size_t) width);
            }
        }
    } else if (desc->mode == MIRROR_DIAGONAL) {
        for (i = slice_line_start; i < slice_line_end; i++) {
            uint8_t *p = img->planes[0] + (size_t) stride0 * i;
            uint8_t *q = img->planes[0] + (size_t) stride0 * (height - i - 1);
            int j = ((width * (height - i)) / height) / 2;
            if (!desc->reverse) {
                while (j--) {
                    *p++ = *(q - 2);
                    *p++ = *(q - 3 - uneven_w);
                    *p++ = *(q - 4);
                    *p++ = *(q - 1 - uneven_w);
                    q -= 4;
                }
            } else {
                while (j--) {
                    *(q - 2) = *p++;
                    *(q - 3 - uneven_w) = *p++;
                    *(q - 4) = *p++;
                    *(q - 1 - uneven_w) = *p++;
                    q -= 4;
                }
            }
        }
        if (img->planes[3]) {
            for (i = slice_line_start; i < slice_line_end; i++) {
                int j = (width * (height - i)) / height;
                uint8_t *a = img->planes[3] + (size_t) stride3 * i;
                uint8_t *b = img->planes[3] + (size_t) stride3 * (height - i - 1);
                if (!desc->reverse)
                    while (j--)
                        *a++ = *b--;
                else
                    while (j--)
                        *b-- = *a++;
            }
        }
    } else if (desc->mode == MIRROR_XDIAGONAL) {
        for (i = slice_line_start; i < slice_line_end; i++) {
            uint8_t *p = img->planes[0] + (size_t) stride0 * (i + 1);
            uint8_t *q = img->planes[0] + (size_t) stride0 * (height - i);
            int j = ((width * (height - i)) / height) / 2;
            if (!desc->reverse) {
                while (j--) {
                    *q++ = *(p - 2);
                    *q++ = *(p - 3 - uneven_w);
                    *q++ = *(p - 4);
                    *q++ = *(p - 1 - uneven_w);
                    p -= 4;
                }
            } else {
                while (j--) {
                    *(p - 2) = *q++;
                    *(p - 3 - uneven_w) = *q++;
                    *(p - 4) = *q++;
                    *(p - 1 - uneven_w) = *q++;
                    p -= 4;
                }
            }
        }
        if (img->planes[3]) {
            for (i = slice_line_start; i < slice_line_end; i++) {
                int j = ((width * (height - i)) / height);
                uint8_t *a = img->planes[3] + (size_t) stride3 * i + width - 1;
                uint8_t *b = img->planes[3] + (size_t) stride3 * (height - i - 1);
                if (!desc->reverse)
                    while (j--)
                        *b++ = *a--;
                else
                    while (j--)
                        *a-- = *b++;
            }
        }
    } else if (desc->mode == MIRROR_FLIP) {
        uint8_t t[4];
        for (i = slice_line_start; i < slice_line_end; i++) {
            uint8_t *p = img->planes[0] + (size_t) stride0 * i;
            uint8_t *q = p + width * 2;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            if (uneven_w == 0) {
                while (p < q) {
                    q -= 4;
                    uint32_t vp, vq;
                    memcpy(&vp, p, 4);
                    memcpy(&vq, q, 4);
                    vp = swap_yuyv_luma(vp);
                    vq = swap_yuyv_luma(vq);
                    memcpy(p, &vq, 4);
                    memcpy(q, &vp, 4);
                    p += 4;
                }
                continue;
            }
#endif
            while (p < q) {
                t[0] = p[0];
                t[1] = p[1 + uneven_w];
                t[2] = p[2];
                t[3] = p[3 + uneven_w];
                *p++ = *(q - 2);
                *p++ = *(q - 3 - uneven_w);
                *p++ = *(q - 4);
                *p++ = *(q - 1 - uneven_w);
                *(--q) = t[3];
                *(--q) = t[0];
                *(--q) = t[1];
                *(--q) = t[2];
            }
        }
        if (img->planes[3]) {
            uint8_t c;
            for (i = slice_line_start; i < slice_line_end; i++) {
                uint8_t *a = img->planes[3] + (size_t) stride3 * i;
                uint8_t *b = a + width - 1;
                while (a + 3 < b - 3) {
                    uint32_t va, vb;
                    memcpy(&va, a, 4);
                    memcpy(&vb, b - 3, 4);
                    va = __builtin_bswap32(va);
                    vb = __builtin_bswap32(vb);
                    memcpy(a, &vb, 4);
                    memcpy(b - 3, &va, 4);
                    a += 4;
                    b -= 4;
                }
                while (a < b) {
                    c = *a;
                    *a++ = *b;
                    *b-- = c;
                }
            }
        }
    } else if (desc->mode == MIRROR_FLOP) {
        const size_t row_bytes = (size_t) width * 2;
        for (i = slice_line_start_half; i < slice_line_end_half; i++) {
            uint8_t *p = img->planes[0] + (size_t) stride0 * i;
            uint8_t *q = img->planes[0] + (size_t) stride0 * (height - i - 1);
            swap_row_bytes(p, q, row_bytes);
        }
        if (img->planes[3]) {
            uint8_t c;
            for (i = slice_line_start_half; i < slice_line_end_half; i++) {
                uint8_t *a = img->planes[3] + (size_t) stride3 * i;
                uint8_t *b = img->planes[3] + (size_t) stride3 * (height - i - 1);
                while (a < b) {
                    c = *a;
                    *a++ = *b;
                    *b-- = c;
                }
            }
        }
    }
    return 0;
}

static mirror_mode parse_mirror_mode(const char *mirror)
{
    if (!mirror)
        return MIRROR_NONE;
    if (!strcmp(mirror, "horizontal"))
        return MIRROR_HORIZONTAL;
    if (!strcmp(mirror, "vertical"))
        return MIRROR_VERTICAL;
    if (!strcmp(mirror, "diagonal"))
        return MIRROR_DIAGONAL;
    if (!strcmp(mirror, "xdiagonal"))
        return MIRROR_XDIAGONAL;
    if (!strcmp(mirror, "flip"))
        return MIRROR_FLIP;
    if (!strcmp(mirror, "flop"))
        return MIRROR_FLOP;
    return MIRROR_NONE;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    // Pop the mirror filter from the stack
    mlt_filter filter = mlt_frame_pop_service(frame);

    // Get the image
    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    // If we have an image of the right colour space
    if (error == 0 && *format == mlt_image_yuv422) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
        mirror_mode mode = parse_mirror_mode(mlt_properties_get(properties, "mirror"));
        if (mode != MIRROR_NONE) {
            slice_desc desc;
            struct mlt_image_s img;
            mlt_image_set_values(&img, *image, *format, *width, *height);
            if (mlt_frame_get_alpha(frame)) {
                img.planes[3] = mlt_frame_get_alpha(frame);
                img.strides[3] = img.width;
            }
            desc.image = &img;
            desc.mode = mode;
            desc.reverse = mlt_properties_get_int(properties, "reverse");
            mlt_slices_run_normal(0, do_slice_proc, &desc);
        }
    }

    // Return the error
    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Push the service on to the stack
    mlt_frame_push_service(frame, filter);

    // Push the filter method on to the stack
    mlt_frame_push_service(frame, filter_get_image);

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mirror_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    // Construct a new filter
    mlt_filter filter = mlt_filter_new();

    // If we have a filter, initialise it
    if (filter != NULL) {
        // Get the properties
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

        // Set the default mirror type
        mlt_properties_set_or_default(properties, "mirror", arg, "horizontal");

        // Assign the process method
        filter->process = filter_process;
    }

    // Return the filter
    return filter;
}
