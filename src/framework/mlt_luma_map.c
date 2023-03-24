/**
 * \file mlt_luma_map.c
 * \brief functions to generate and read luma-wipe transition maps
 *
 * Copyright (C) 2003-2019 Meltytech, LLC
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

#include "mlt_luma_map.h"
#include "mlt_pool.h"
#include "mlt_types.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define HALF_USHRT_MAX (1 << 15)

void mlt_luma_map_init(mlt_luma_map self)
{
    memset(self, 0, sizeof(struct mlt_luma_map_s));
    self->type = 0;
    self->w = 720;
    self->h = 576;
    self->bands = 1;
    self->rband = 0;
    self->vmirror = 0;
    self->hmirror = 0;
    self->dmirror = 0;
    self->invert = 0;
    self->offset = 0;
    self->flip = 0;
    self->flop = 0;
    self->quart = 0;
    self->pflop = 0;
    self->pflip = 0;
}

static inline int sqrti(int n)
{
    int p = 0;
    int q = 1;
    int r = n;
    int h = 0;

    while (q <= n)
        q = 4 * q;

    while (q != 1) {
        q = q / 4;
        h = p + q;
        p = p / 2;
        if (r >= h) {
            p = p + q;
            r = r - h;
        }
    }

    return p;
}

uint16_t *mlt_luma_map_render(mlt_luma_map self)
{
    int i = 0;
    int j = 0;
    int k = 0;

    if (self->quart) {
        self->w *= 2;
        self->h *= 2;
    }

    if (self->rotate) {
        int t = self->w;
        self->w = self->h;
        self->h = t;
    }

    if (self->bands < 0)
        self->bands = self->h;

    int max = (1 << 16) - 1;
    uint16_t *image = mlt_pool_alloc(self->w * self->h * sizeof(uint16_t));
    uint16_t *end = image + self->w * self->h;
    uint16_t *p = image;
    uint16_t *r = image;
    int lower = 0;
    int lpb = self->h / self->bands;
    int rpb = max / self->bands;
    int direction = 1;

    int half_w = self->w / 2;
    int half_h = self->h / 2;

    if (!self->dmirror && (self->hmirror || self->vmirror))
        rpb *= 2;

    for (i = 0; i < self->bands; i++) {
        lower = i * rpb;
        direction = 1;

        if (self->rband && i % 2 == 1) {
            direction = -1;
            lower += rpb;
        }

        switch (self->type) {
        case 1: {
            int length = sqrti(half_w * half_w + lpb * lpb / 4);
            int value;
            int x = 0;
            int y = 0;
            for (j = 0; j < lpb; j++) {
                y = j - lpb / 2;
                for (k = 0; k < self->w; k++) {
                    x = k - half_w;
                    value = sqrti(x * x + y * y);
                    *p++ = lower + (direction * rpb * ((max * value) / length) / max)
                           + (j * self->offset * 2 / lpb) + (j * self->offset / lpb);
                }
            }
        } break;

        case 2: {
            for (j = 0; j < lpb; j++) {
                int value = ((j * self->w) / lpb) - half_w;
                if (value > 0)
                    value = -value;
                for (k = -half_w; k < value; k++)
                    *p++ = lower + (direction * rpb * ((max * abs(k)) / half_w) / max);
                for (k = value; k < abs(value); k++)
                    *p++ = lower + (direction * rpb * ((max * abs(value)) / half_w) / max)
                           + (j * self->offset * 2 / lpb) + (j * self->offset / lpb);
                for (k = abs(value); k < half_w; k++)
                    *p++ = lower + (direction * rpb * ((max * abs(k)) / half_w) / max);
            }
        } break;

        case 3: {
            int length;
            for (j = -half_h; j < half_h; j++) {
                if (j < 0) {
                    for (k = -half_w; k < half_w; k++) {
                        length = sqrti(k * k + j * j);
                        *p++ = (max / 4 * k) / (length + 1);
                    }
                } else {
                    for (k = half_w; k > -half_w; k--) {
                        length = sqrti(k * k + j * j);
                        *p++ = (max / 2) + (max / 4 * k) / (length + 1);
                    }
                }
            }
        } break;

        default:
            for (j = 0; j < lpb; j++)
                for (k = 0; k < self->w; k++)
                    *p++ = lower + (direction * (rpb * ((k * max) / self->w) / max))
                           + (j * self->offset * 2 / lpb);
            break;
        }
    }

    if (self->quart) {
        self->w /= 2;
        self->h /= 2;
        for (i = 1; i < self->h; i++) {
            p = image + i * self->w;
            r = image + i * 2 * self->w;
            j = self->w;
            while (j-- > 0)
                *p++ = *r++;
        }
    }

    if (self->dmirror) {
        for (i = 0; i < self->h; i++) {
            p = image + i * self->w;
            r = end - i * self->w;
            j = (self->w * (self->h - i)) / self->h;
            while (j--)
                *(--r) = *p++;
        }
    }

    if (self->flip) {
        uint16_t t;
        for (i = 0; i < self->h; i++) {
            p = image + i * self->w;
            r = p + self->w;
            while (p != r) {
                t = *p;
                *p++ = *(--r);
                *r = t;
            }
        }
    }

    if (self->flop) {
        uint16_t t;
        r = end;
        for (i = 1; i < self->h / 2; i++) {
            p = image + i * self->w;
            j = self->w;
            while (j--) {
                t = *(--p);
                *p = *(--r);
                *r = t;
            }
        }
    }

    if (self->hmirror) {
        p = image;
        while (p < end) {
            r = p + self->w;
            while (p != r)
                *(--r) = *p++;
            p += self->w / 2;
        }
    }

    if (self->vmirror) {
        p = image;
        r = end;
        while (p != r)
            *(--r) = *p++;
    }

    if (self->invert) {
        p = image;
        r = image;
        while (p < end)
            *p++ = max - *r++;
    }

    if (self->pflip) {
        uint16_t t;
        for (i = 0; i < self->h; i++) {
            p = image + i * self->w;
            r = p + self->w;
            while (p != r) {
                t = *p;
                *p++ = *(--r);
                *r = t;
            }
        }
    }

    if (self->pflop) {
        uint16_t t;
        end = image + self->w * self->h;
        r = end;
        for (i = 1; i < self->h / 2; i++) {
            p = image + i * self->w;
            j = self->w;
            while (j--) {
                t = *(--p);
                *p = *(--r);
                *r = t;
            }
        }
    }

    if (self->rotate) {
        uint16_t *image2 = mlt_pool_alloc(self->w * self->h * sizeof(uint16_t));
        for (i = 0; i < self->h; i++) {
            p = image + i * self->w;
            r = image2 + self->h - i - 1;
            for (j = 0; j < self->w; j++) {
                *r = *(p++);
                r += self->h;
            }
        }
        i = self->w;
        self->w = self->h;
        self->h = i;
        mlt_pool_release(image);
        image = image2;
    }

    return image;
}

/** Load the luma map from PGM stream.
*/

int mlt_luma_map_from_pgm(const char *filename, uint16_t **map, int *width, int *height)
{
    uint8_t *data = NULL;
    FILE *f = mlt_fopen(filename, "rb");
    int error = f == NULL;

    while (!error) {
        char line[128];
        char comment[128];
        int i = 2;
        int maxval;
        int bpp;
        uint16_t *p;

        line[127] = '\0';

        // get the magic code
        if (fgets(line, 127, f) == NULL)
            break;

        // skip comments
        while (sscanf(line, " #%s", comment) > 0)
            if (fgets(line, 127, f) == NULL)
                break;

        if (line[0] != 'P' || line[1] != '5')
            break;

        // skip white space and see if a new line must be fetched
        for (i = 2; i < 127 && line[i] != '\0' && isspace(line[i]); i++)
            ;
        if ((line[i] == '\0' || line[i] == '#') && fgets(line, 127, f) == NULL)
            break;

        // skip comments
        while (sscanf(line, " #%s", comment) > 0)
            if (fgets(line, 127, f) == NULL)
                break;

        // get the dimensions
        if (line[0] == 'P')
            i = sscanf(line, "P5 %d %d %d", width, height, &maxval);
        else
            i = sscanf(line, "%d %d %d", width, height, &maxval);

        // get the height value, if not yet
        if (i < 2) {
            if (fgets(line, 127, f) == NULL)
                break;

            // skip comments
            while (sscanf(line, " #%s", comment) > 0)
                if (fgets(line, 127, f) == NULL)
                    break;

            i = sscanf(line, "%d", height);
            if (i == 0)
                break;
            else
                i = 2;
        }

        // get the maximum gray value, if not yet
        if (i < 3) {
            if (fgets(line, 127, f) == NULL)
                break;

            // skip comments
            while (sscanf(line, " #%s", comment) > 0)
                if (fgets(line, 127, f) == NULL)
                    break;

            i = sscanf(line, "%d", &maxval);
            if (i == 0)
                break;
        }

        // determine if this is one or two bytes per pixel
        bpp = maxval > 255 ? 2 : 1;

        // allocate temporary storage for the raw data
        data = mlt_pool_alloc(*width * *height * bpp);
        if (!data) {
            error = 1;
            break;
        }

        // read the raw data
        if (fread(data, *width * *height * bpp, 1, f) != 1)
            break;

        // allocate the luma bitmap
        *map = p = (uint16_t *) mlt_pool_alloc(*width * *height * sizeof(uint16_t));
        if (!*map) {
            error = 1;
            break;
        }

        // process the raw data into the luma bitmap
        for (i = 0; i < *width * *height * bpp; i += bpp) {
            if (bpp == 1)
                *p++ = data[i] << 8;
            else
                *p++ = (data[i] << 8) + data[i + 1];
        }

        break;
    }
    if (f)
        fclose(f);

    mlt_pool_release(data);
    return error;
}

mlt_luma_map mlt_luma_map_new(const char *path)
{
    mlt_luma_map self = malloc(sizeof(struct mlt_luma_map_s));
    if (self) {
        mlt_luma_map_init(self);
        if (strstr(path, "luma02.pgm")) {
            self->bands = -1; // use height
        } else if (strstr(path, "luma03.pgm")) {
            self->hmirror = 1;
        } else if (strstr(path, "luma04.pgm")) {
            self->bands = -1; // use height
            self->vmirror = 1;
        } else if (strstr(path, "luma05.pgm")) {
            self->offset = HALF_USHRT_MAX;
            self->dmirror = 1;
        } else if (strstr(path, "luma06.pgm")) {
            self->offset = HALF_USHRT_MAX;
            self->dmirror = 1;
            self->flip = 1;
        } else if (strstr(path, "luma07.pgm")) {
            self->offset = HALF_USHRT_MAX;
            self->dmirror = 1;
            self->quart = 1;
        } else if (strstr(path, "luma08.pgm")) {
            self->offset = HALF_USHRT_MAX;
            self->dmirror = 1;
            self->quart = 1;
            self->flip = 1;
        } else if (strstr(path, "luma09.pgm")) {
            self->bands = 12;
        } else if (strstr(path, "luma10.pgm")) {
            self->bands = 12;
            self->rotate = 1;
        } else if (strstr(path, "luma11.pgm")) {
            self->bands = 12;
            self->rband = 1;
        } else if (strstr(path, "luma12.pgm")) {
            self->bands = 12;
            self->rband = 1;
            self->vmirror = 1;
        } else if (strstr(path, "luma13.pgm")) {
            self->bands = 12;
            self->rband = 1;
            self->rotate = 1;
            self->flop = 1;
        } else if (strstr(path, "luma14.pgm")) {
            self->bands = 12;
            self->rband = 1;
            self->rotate = 1;
            self->vmirror = 1;
        } else if (strstr(path, "luma15.pgm")) {
            self->offset = HALF_USHRT_MAX;
            self->dmirror = 1;
            self->hmirror = 1;
        } else if (strstr(path, "luma16.pgm")) {
            self->type = 1;
        } else if (strstr(path, "luma17.pgm")) {
            self->type = 1;
            self->bands = 2;
            self->rband = 1;
        } else if (strstr(path, "luma18.pgm")) {
            self->type = 2;
        } else if (strstr(path, "luma19.pgm")) {
            self->type = 2;
            self->quart = 1;
        } else if (strstr(path, "luma20.pgm")) {
            self->type = 2;
            self->quart = 1;
            self->flip = 1;
        } else if (strstr(path, "luma21.pgm")) {
            self->type = 2;
            self->quart = 1;
            self->bands = 2;
        } else if (strstr(path, "luma22.pgm")) {
            self->type = 3;
        }
    }
    return self;
}

/** Generate a 16-bit luma map from an 8-bit image.
*/

void mlt_luma_map_from_yuv422(uint8_t *image, uint16_t **map, int width, int height)
{
    int i;
    int size = width * height * 2;

    // allocate the luma bitmap
    uint16_t *p = *map = (uint16_t *) mlt_pool_alloc(width * height * sizeof(uint16_t));
    if (*map == NULL)
        return;

    // process the image data into the luma bitmap
    for (i = 0; i < size; i += 2)
        *p++ = (image[i] - 16) * 299; // 299 = 65535 / 219
}
