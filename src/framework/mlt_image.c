/**
 * \file mlt_image.c
 * \brief Image class
 * \see mlt_mlt_image_s
 *
 * Copyright (C) 2020-2025 Meltytech, LLC
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

#include "mlt_image.h"

#include <stdlib.h>
#include <string.h>

/** Allocate a new Image object.
 *
 * \return a new image object with default values set
 */

mlt_image mlt_image_new()
{
    mlt_image self = calloc(1, sizeof(struct mlt_image_s));
    self->close = free;
    return self;
}

/** Destroy an image object created by mlt_image_new().
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 */

void mlt_image_close(mlt_image self)
{
    if (self) {
        if (self->release_data) {
            self->release_data(self->data);
        }
        if (self->release_alpha) {
            self->release_alpha(self->alpha);
        }
        if (self->close) {
            self->close(self);
        }
    }
}

/** Set the most common values for the image.
 *
 * Less common values will be set to reasonable defaults.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 * \param data the buffer that contains the image data
 * \param format the image format
 * \param width the width of the image
 * \param height the height of the image
 */

void mlt_image_set_values(mlt_image self, void *data, mlt_image_format format, int width, int height)
{
    self->data = data;
    self->format = format;
    self->width = width;
    self->height = height;
    self->colorspace = mlt_colorspace_unspecified;
    self->release_data = NULL;
    self->release_alpha = NULL;
    self->close = NULL;
    mlt_image_format_planes(self->format,
                            self->width,
                            self->height,
                            self->data,
                            self->planes,
                            self->strides);
}

/** Get the most common values for the image.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 * \param[out] data the buffer that contains the image data
 * \param[out] format the image format
 * \param[out] width the width of the image
 * \param[out] height the height of the image
 */

void mlt_image_get_values(
    mlt_image self, void **data, mlt_image_format *format, int *width, int *height)
{
    *data = self->data;
    *format = self->format;
    *width = self->width;
    *height = self->height;
}

/** Allocate the data field based on the other properties of the Image.
 *
 * If the data field is already set, and a destructor function exists, the data
 * will be released. Else, the data pointer will be overwritten without being
 * released.
 *
 * After this function call, the release_data field will be set and can be used
 * to release the data when necessary.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 */

void mlt_image_alloc_data(mlt_image self)
{
    if (!self)
        return;

    if (self->release_data) {
        self->release_data(self->data);
    }

    int size = mlt_image_calculate_size(self);
    self->data = mlt_pool_alloc(size);
    self->release_data = mlt_pool_release;
    mlt_image_format_planes(self->format,
                            self->width,
                            self->height,
                            self->data,
                            self->planes,
                            self->strides);
}

/** Allocate the alpha field based on the other properties of the Image.
 *
 * If the alpha field is already set, and a destructor function exists, the data
 * will be released. Else, the data pointer will be overwritten without being
 * released.
 *
 * After this function call, the release_data field will be set and can be used
 * to release the data when necessary.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 */

void mlt_image_alloc_alpha(mlt_image self)
{
    if (!self)
        return;

    if (self->release_alpha) {
        self->release_alpha(self->alpha);
    }

    self->alpha = mlt_pool_alloc(self->width * self->height);
    self->release_alpha = mlt_pool_release;
    self->strides[3] = self->width;
    self->planes[3] = self->alpha;
}

/** Calculate the number of bytes needed for the Image data.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 * \return the number of bytes
 */

int mlt_image_calculate_size(mlt_image self)
{
    switch (self->format) {
    case mlt_image_rgb:
        return self->width * self->height * 3;
    case mlt_image_rgba:
        return self->width * self->height * 4;
    case mlt_image_yuv422:
        return self->width * self->height * 2;
    case mlt_image_yuv420p:
        return self->width * self->height * 3 / 2;
    case mlt_image_movit:
    case mlt_image_opengl_texture:
        return 4;
    case mlt_image_yuv422p16:
        return self->width * self->height * 4;
    case mlt_image_yuv420p10:
        return self->width * self->height * 3;
    case mlt_image_yuv444p10:
        return self->width * self->height * 6;
    case mlt_image_rgba64:
        return self->width * self->height * 4 * 2;
    case mlt_image_none:
    case mlt_image_invalid:
        return 0;
    }
    return 0;
}

/** Get the short name for an image format.
 *
 * \public \memberof mlt_image_s
 * \param format the image format
 * \return a string
 */

const char *mlt_image_format_name(mlt_image_format format)
{
    switch (format) {
    case mlt_image_none:
        return "none";
    case mlt_image_rgb:
        return "rgb";
    case mlt_image_rgba:
        return "rgba";
    case mlt_image_yuv422:
        return "yuv422";
    case mlt_image_yuv420p:
        return "yuv420p";
    case mlt_image_movit:
        return "glsl";
    case mlt_image_opengl_texture:
        return "opengl_texture";
    case mlt_image_yuv422p16:
        return "yuv422p16";
    case mlt_image_yuv420p10:
        return "yuv420p10";
    case mlt_image_yuv444p10:
        return "yuv444p10";
    case mlt_image_rgba64:
        return "rgba64";
    case mlt_image_invalid:
        return "invalid";
    }
    return "invalid";
}

/** Get the id of image format from short name.
 *
 * \public \memberof mlt_image_s
 * \param name the image format short name
 * \return a image format
 */

mlt_image_format mlt_image_format_id(const char *name)
{
    mlt_image_format f;

    for (f = mlt_image_none; name && f < mlt_image_invalid; f++) {
        const char *v = mlt_image_format_name(f);
        if (!strcmp(v, name))
            return f;
    }

    return mlt_image_invalid;
}

/** Get the short name for a color transfer characteristics.
 *
 * \public \memberof mlt_image_s
 * \param trc the color transfer characteristics
 * \return a string
 */

const char *mlt_image_color_trc_name(mlt_color_trc trc)
{
    switch (trc) {
    case mlt_color_trc_none:
    case mlt_color_trc_unspecified:
    case mlt_color_trc_reserved:
        return "none";
    case mlt_color_trc_bt709:
        return "bt709";
    case mlt_color_trc_gamma22:
        return "bt470m";
    case mlt_color_trc_gamma28:
        return "bt470bg";
    case mlt_color_trc_smpte170m:
        return "smpte170m";
    case mlt_color_trc_smpte240m:
        return "smpte240m";
    case mlt_color_trc_linear:
        return "linear";
    case mlt_color_trc_log:
        return "log100";
    case mlt_color_trc_log_sqrt:
        return "log316";
    case mlt_color_trc_iec61966_2_4:
        return "iec61966-2-4";
    case mlt_color_trc_bt1361_ecg:
        return "bt1361e";
    case mlt_color_trc_iec61966_2_1:
        return "iec61966-2-1";
    case mlt_color_trc_bt2020_10:
        return "bt2020-10";
    case mlt_color_trc_bt2020_12:
        return "bt2020-12";
    case mlt_color_trc_smpte2084:
        return "smpte2084";
    case mlt_color_trc_smpte428:
        return "smpte428";
    case mlt_color_trc_arib_std_b67:
        return "arib-std-b67";
    case mlt_color_trc_invalid:
        return "invalid";
    }
    return "none";
}

/** Get the id of color transfer characteristics from short name.
 *
 * \public \memberof mlt_image_s
 * \param name the color trc short name
 * \return a color trc
 */

mlt_color_trc mlt_image_color_trc_id(const char *name)
{
    mlt_color_trc c;

    for (c = mlt_color_trc_none; name && c <= mlt_color_trc_invalid; c++) {
        const char *s = mlt_image_color_trc_name(c);
        if (!strcmp(s, name))
            return c;
    }

    // Fall back to see if it was specified as a number
    int value = strtol(name, NULL, 10);
    if (value && value < mlt_color_trc_invalid)
        return value;

    return mlt_color_trc_none;
}

/** Fill an image with black.
  *
  * \bug This does not respect full range YUV if needed.
  * \public \memberof mlt_image_s
  * \param self a mlt_image
  */
void mlt_image_fill_black(mlt_image self)
{
    if (!self->data)
        return;

    switch (self->format) {
    case mlt_image_invalid:
    case mlt_image_none:
    case mlt_image_movit:
    case mlt_image_opengl_texture:
        return;
    case mlt_image_rgb:
    case mlt_image_rgba:
    case mlt_image_rgba64: {
        int size = mlt_image_calculate_size(self);
        memset(self->planes[0], 0, size);
        break;
    }
    case mlt_image_yuv422: {
        int size = mlt_image_calculate_size(self);
        register uint8_t *p = self->planes[0];
        register uint8_t *q = p + size;
        while (p != NULL && p != q) {
            *p++ = 16;
            *p++ = 128;
        }
    } break;
    case mlt_image_yuv422p16: {
        for (int plane = 0; plane < 3; plane++) {
            uint16_t value = 16 << 8;
            size_t width = self->width;
            if (plane > 0) {
                value = 128 << 8;
                width /= 2;
            }
            uint16_t *p = (uint16_t *) self->planes[plane];
            for (int i = 0; i < width * self->height; i++) {
                p[i] = value;
            }
        }
    } break;
    case mlt_image_yuv420p10:
    case mlt_image_yuv444p10: {
        for (int plane = 0; plane < 3; plane++) {
            uint16_t value = 16 << 2;
            size_t width = self->width;
            size_t height = self->height;
            if (plane > 0) {
                value = 128 << 2;
                if (self->format == mlt_image_yuv420p10) {
                    width /= 2;
                    height /= 2;
                }
            }
            uint16_t *p = (uint16_t *) self->planes[plane];
            for (int i = 0; i < width * height; i++) {
                p[i] = value;
            }
        }
    } break;
    case mlt_image_yuv420p: {
        memset(self->planes[0], 16, self->height * self->strides[0]);
        memset(self->planes[1], 128, self->height * self->strides[1] / 2);
        memset(self->planes[2], 128, self->height * self->strides[2] / 2);
    } break;
    }
}

/** Fill an image with a checkerboard pattern.
  *
  * \public \memberof mlt_image_s
  * \param self a mlt_image
  * \param sample_aspect_ratio the pixel aspect ratio
  */
void mlt_image_fill_checkerboard(mlt_image self, double sample_aspect_ratio)
{
    if (!self->data)
        return;

    if (sample_aspect_ratio == 0)
        sample_aspect_ratio = 1.0;
    int h = 0.025 * MAX(self->width * sample_aspect_ratio, self->height);
    int w = h / sample_aspect_ratio;

    if (w <= 0 || h <= 0)
        return;

    // compute center offsets
    int ox = w * 2 - (self->width / 2) % (w * 2);
    int oy = h * 2 - (self->height / 2) % (h * 2);
    int bpp = self->strides[0] / self->width;
    uint8_t color, gray1 = 0x7F, gray2 = 0xB2;

    switch (self->format) {
    case mlt_image_invalid:
    case mlt_image_none:
    case mlt_image_movit:
    case mlt_image_opengl_texture:
        return;
    case mlt_image_rgb:
    case mlt_image_rgba: {
        uint8_t *p = self->planes[0];
        for (int i = 0; i < self->height; i++) {
            for (int j = 0; j < self->width; j++) {
                color = ((((i + oy) / h) % 2) ^ (((j + ox) / w) % 2)) ? gray1 : gray2;
                memset(&p[i * self->strides[0] + j * bpp], color, bpp);
            }
        }
        break;
    }
    case mlt_image_yuv422: {
        uint8_t *p = self->planes[0];
        for (int i = 0; i < self->height; i++) {
            for (int j = 0; j < self->width; j++) {
                color = ((((i + oy) / h) % 2) ^ (((j + ox) / w) % 2)) ? gray1 : gray2;
                p[i * self->strides[0] + j * bpp] = color;
                p[i * self->strides[0] + j * bpp + 1] = 128;
            }
        }
    } break;
    case mlt_image_yuv422p16: {
        for (int plane = 0; plane < 3; plane++) {
            int width = plane > 0 ? self->width / 2 : self->width;
            uint16_t *p = (uint16_t *) self->planes[plane];
            uint16_t color;

            for (int i = 0; i < self->height; i++) {
                for (int j = 0; j < width; j++) {
                    color = plane > 0                                       ? 128
                            : ((((i + oy) / h) % 2) ^ (((j + ox) / w) % 2)) ? gray1
                                                                            : gray2;
                    p[i * width + j] = color << 8;
                }
            }
        }
    } break;
    case mlt_image_yuv420p10:
    case mlt_image_yuv444p10: {
        for (int plane = 0; plane < 3; plane++) {
            uint16_t *p = (uint16_t *) self->planes[plane];
            uint16_t color;
            int width = self->width;
            int height = self->height;

            if (plane > 0 && self->format == mlt_image_yuv420p10) {
                width /= 2;
                height /= 2;
            }
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    color = plane > 0                                       ? 128
                            : ((((i + oy) / h) % 2) ^ (((j + ox) / w) % 2)) ? gray1
                                                                            : gray2;
                    p[i * width + j] = color << 2;
                }
            }
        }
    } break;
    case mlt_image_yuv420p: {
        uint8_t *p = self->planes[0];
        for (int i = 0; i < self->height; i++) {
            for (int j = 0; j < self->width; j++) {
                color = ((((i + oy) / h) % 2) ^ (((j + ox) / w) % 2)) ? gray1 : gray2;
                p[i * self->width + j] = color;
            }
        }
        memset(self->planes[1], 128, self->height * self->strides[1] / 2);
        memset(self->planes[2], 128, self->height * self->strides[2] / 2);
    } break;
    case mlt_image_rgba64: {
        uint16_t *p = (uint16_t *) self->planes[0];
        for (int i = 0; i < self->height; i++) {
            for (int j = 0; j < self->width; j++) {
                uint16_t color = ((((i + oy) / h) % 2) ^ (((j + ox) / w) % 2)) ? gray1 : gray2;
                color *= 256;
                p[0] = color;
                p[1] = color;
                p[2] = color;
                p[3] = 0xffff;
                p += 4;
            }
        }
    } break;
    }
}

/** Fill an image with white.
  *
  * \public \memberof mlt_image_s
  * \param self a mlt_image
  * \param full_range whether to use full color range
  */
void mlt_image_fill_white(mlt_image self, int full_range)
{
    if (!self->data)
        return;

    uint8_t white = full_range ? 255 : 235;
    switch (self->format) {
    case mlt_image_invalid:
    case mlt_image_none:
    case mlt_image_movit:
    case mlt_image_opengl_texture:
        return;
    case mlt_image_rgb:
    case mlt_image_rgba:
    case mlt_image_rgba64: {
        int size = mlt_image_calculate_size(self);
        memset(self->planes[0], 255, size);
        break;
    }
    case mlt_image_yuv422: {
        int size = mlt_image_calculate_size(self);
        register uint8_t *p = self->planes[0];
        register uint8_t *q = p + size;
        while (p != NULL && p != q) {
            *p++ = white;
            *p++ = 128;
        }
    } break;
    case mlt_image_yuv422p16: {
        for (int plane = 0; plane < 3; plane++) {
            uint16_t value = white << 8;
            size_t width = self->width;
            if (plane > 0) {
                value = 128 << 8;
                width /= 2;
            }
            uint16_t *p = (uint16_t *) self->planes[plane];
            for (int i = 0; i < width * self->height; i++) {
                p[i] = value;
            }
        }
    } break;
    case mlt_image_yuv420p10:
    case mlt_image_yuv444p10:
        for (int plane = 0; plane < 3; plane++) {
            uint16_t value = white << 2;
            size_t width = self->width;
            size_t height = self->height;
            if (plane > 0) {
                value = 128 << 2;
                if (self->format == mlt_image_yuv420p10) {
                    width /= 2;
                    height /= 2;
                }
            }
            uint16_t *p = (uint16_t *) self->planes[plane];
            for (int i = 0; i < width * height; i++) {
                p[i] = value;
            }
        }
        break;
    case mlt_image_yuv420p: {
        memset(self->planes[0], white, self->height * self->strides[0]);
        memset(self->planes[1], 128, self->height * self->strides[1] / 2);
        memset(self->planes[2], 128, self->height * self->strides[2] / 2);
    } break;
    }
}

/** Fill an image alpha channel with opaque if it exists.
  *
  * \public \memberof mlt_image_s
  */
void mlt_image_fill_opaque(mlt_image self)
{
    if (!self->data)
        return;

    if (self->format == mlt_image_rgba && self->planes[0] != NULL) {
        for (int line = 0; line < self->height; line++) {
            uint8_t *pLine = self->planes[0] + (self->strides[0] * line) + 3;
            for (int pixel = 0; pixel < self->width; pixel++) {
                *pLine = 0xff;
                *pLine += 4;
            }
        }
    } else if (self->format == mlt_image_rgba64 && self->planes[0] != NULL) {
        for (int line = 0; line < self->height; line++) {
            uint16_t *pLine = (uint16_t *) self->planes[0] + (self->strides[0] * line) + 3;
            for (int pixel = 0; pixel < self->width; pixel++) {
                *pLine = 0xffff;
                *pLine += 4;
            }
        }
    } else if (self->planes[3] != NULL) {
        memset(self->planes[3], 255, self->height * self->strides[3]);
    }
}

/** Check if the alpha channel of an image is opaque
 *
 * \public \memberof mlt_image_s
 * \param self a mlt_image
 * \return true (1) or false (0) if the image is opaque
 */
extern int mlt_image_is_opaque(mlt_image self)
{
    if (!self->data)
        return 0;

    int pixels = self->width * self->height;

    if (self->format == mlt_image_rgba && self->planes[0] != NULL) {
        int samples = pixels * 4;
        uint8_t *img = self->planes[0];
        for (int i = 0; i < samples; i += 4) {
            if (img[i + 3] != 0xff)
                return 0;
        }
    } else if (self->format == mlt_image_rgba64 && self->planes[0] != NULL) {
        int samples = pixels * 4;
        uint16_t *img = (uint16_t *) self->planes[0];
        for (int i = 0; i < samples; i += 4) {
            if (img[i + 3] != 0xffff)
                return 0;
        }
    } else if (self->planes[3] != NULL) {
        for (int i = 0; i < pixels; i++) {
            if (self->planes[3][i] != 0xff)
                return 0;
        }
    }
    return 1;
}

/** Get the number of bytes needed for an image.
  *
  * \public \memberof mlt_image_s
  * \param format the image format
  * \param width width of the image in pixels
  * \param height height of the image in pixels
  * \param[out] bpp the number of bytes per pixel (optional)
  * \return the number of bytes
  */
int mlt_image_format_size(mlt_image_format format, int width, int height, int *bpp)
{
    switch (format) {
    case mlt_image_rgb:
        if (bpp)
            *bpp = 3;
        return width * height * 3;
    case mlt_image_rgba:
        if (bpp)
            *bpp = 4;
        return width * height * 4;
    case mlt_image_yuv422:
        if (bpp)
            *bpp = 2;
        return width * height * 2;
    case mlt_image_yuv420p:
        if (bpp)
            *bpp = 3 / 2;
        return width * height * 3 / 2;
    case mlt_image_movit:
    case mlt_image_opengl_texture:
        if (bpp)
            *bpp = 0;
        return 4;
    case mlt_image_yuv422p16:
        if (bpp)
            *bpp = 4;
        return 4 * height * width;
    case mlt_image_yuv420p10:
        if (bpp)
            *bpp = 3;
        return 3 * height * width;
    case mlt_image_yuv444p10:
        if (bpp)
            *bpp = 6;
        return 6 * height * width;
    case mlt_image_rgba64:
        if (bpp)
            *bpp = 8;
        return 8 * height * width;
    default:
        if (bpp)
            *bpp = 0;
        return 0;
    }
    return 0;
}

/** Build a planes pointers of image mapping
 *
 * For proper and unified planar image processing, planes sizes and planes pointers should
 * be provides to processing code.
 *
 * \public \memberof mlt_image_s
 * \param format the image format
 * \param width width of the image in pixels
 * \param height height of the image in pixels
 * \param[in] data pointer to allocated image
 * \param[out] planes pointers to plane's pointers will be set
 * \param[out] strides pointers to plane's strides will be set
 */
void mlt_image_format_planes(
    mlt_image_format format, int width, int height, void *data, uint8_t *planes[4], int strides[4])
{
    switch (format) {
    case mlt_image_yuv422p16:
        strides[0] = width * 2;
        strides[1] = width;
        strides[2] = width;
        strides[3] = 0;

        planes[0] = (unsigned char *) data;
        planes[1] = planes[0] + height * strides[0];
        planes[2] = planes[1] + height * strides[1];
        planes[3] = 0;
        break;
    case mlt_image_yuv420p10:
        strides[0] = width * 2;
        strides[1] = width;
        strides[2] = width;
        strides[3] = 0;

        planes[0] = (unsigned char *) data;
        planes[1] = planes[0] + height * strides[0];
        planes[2] = planes[1] + (height >> 1) * strides[1];
        planes[3] = 0;
        break;
    case mlt_image_yuv444p10:
        strides[0] = width * 2;
        strides[1] = width * 2;
        strides[2] = width * 2;
        strides[3] = 0;

        planes[0] = (unsigned char *) data;
        planes[1] = planes[0] + height * strides[0];
        planes[2] = planes[1] + height * strides[1];
        planes[3] = 0;
        break;
    case mlt_image_yuv420p:
        strides[0] = width;
        strides[1] = width >> 1;
        strides[2] = width >> 1;
        strides[3] = 0;

        planes[0] = (unsigned char *) data;
        planes[1] = planes[0] + width * height;
        planes[2] = planes[1] + (width >> 1) * (height >> 1);
        planes[3] = 0;
        break;
    default:
        planes[0] = data;
        planes[1] = 0;
        planes[2] = 0;
        planes[3] = 0;

        strides[0] = mlt_image_format_size(format, width, 1, NULL);
        strides[1] = 0;
        strides[2] = 0;
        strides[3] = 0;
        break;
    };
}

/** Check if the alpha channel of an rgba image is opaque
 *
 * \public \memberof mlt_image_s
 * \param image the image buffer
 * \param width width of the image in pixels
 * \param height height of the image in pixels
 * \return true (1) or false (0) if the image is opaque
 */
int mlt_image_rgba_opaque(uint8_t *image, int width, int height)
{
    for (int i = 0; i < width * height; ++i) {
        if (image[4 * i + 3] != 0xff)
            return 0;
    }
    return 1;
}

/** Check if the string indicates full range
 *
 * \public \memberof mlt_image_s
 * \param color_range the string whose values to test
 * \return true (1) or false (0) if the color range is full
 */
int mlt_image_full_range(const char *color_range)
{
    return color_range
           && (!strcmp("pc", color_range) || !strcmp("full", color_range)
               || !strcmp("jpeg", color_range));
}
