/**
 * \file mlt_image.h
 * \brief Image class
 * \see mlt_image_s
 *
 * Copyright (C) 2022-2025 Meltytech, LLC
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

#ifndef MLT_IMAGE_H
#define MLT_IMAGE_H

#include "mlt_export.h"
#include "mlt_types.h"

/** \brief Image class
 *
 * Image is the data object that represents image for a period of time.
 */
#define MLT_IMAGE_MAX_PLANES 4

/** Maximum VkImages in one mlt_image_vulkan payload. */
#define MLT_VULKAN_MAX_PLANES 4

/** One VkImage inside an mlt_vulkan_image.
 *
 * image is a VkImage cast to uint64_t. Zero means this entry is a subplane of
 * planes[0] (one multi-planar Vulkan image). format is the VkFormat used to
 * interpret it. layout is the VkImageLayout. semaphore is a timeline
 * VkSemaphore, or zero when the image needs no wait. sem_value is the value to
 * wait on; the user signals sem_value + 1 and stores that new value.
 * component_mapping[n] is the color channel for that component, or -1.
 */
typedef struct
{
    uint64_t image;
    int format;
    int width;
    int height;
    int layout;
    uint64_t semaphore;
    uint64_t sem_value;
    int components;
    int component_mapping[4];
} mlt_vulkan_plane;

/** mlt_image_vulkan payload. The frame image pointer is this struct.
 *
 * The VkImages belong to the shared device published on mlt_global_properties()
 * as "placebo.vulkan". They stay on the GPU. A caller waits on each plane's
 * timeline semaphore, submits its work, then signals the next value and updates
 * layout and sem_value. Call lock before reading or writing those fields and
 * unlock afterwards, when the callbacks are set. Do not free the VkImage; the
 * image destructor does that.
 *
 * chroma_location uses the same order as AVChromaLocation: 0 unknown, 1 left,
 * 2 center, 3 top-left, 4 top, 5 bottom-left, 6 bottom. colorspace, color_trc,
 * and color_primaries are mlt_colorspace, mlt_color_trc, and mlt_color_primaries.
 * rgb is non-zero for RGB and zero for YCbCr. bit_shift is the number of unused
 * low bits when a 10-bit or 12-bit sample is stored in 16 bits.
 *
 * module_data is private to the module that created the image. Other modules
 * use the planes and the semaphore only.
 */
typedef struct mlt_vulkan_image_s
{
    int width;
    int height;
    int crop_left;
    int crop_top;
    int crop_right;
    int crop_bottom;
    int nb_planes;
    uint32_t usage;
    int rgb;
    int full_range;
    int alpha;
    int bit_depth;
    int bit_shift;
    int colorspace;
    int color_trc;
    int color_primaries;
    int chroma_location;
    int log2_chroma_w;
    int log2_chroma_h;
    mlt_vulkan_plane planes[MLT_VULKAN_MAX_PLANES];
    void (*lock)(void *opaque);
    void (*unlock)(void *opaque);
    void *sync_opaque;
    void *module_data;
    void (*module_close)(void *module_data);
} mlt_vulkan_image;

struct mlt_image_s
{
    mlt_image_format format;
    int width;
    int height;
    int colorspace;
    uint8_t *planes[MLT_IMAGE_MAX_PLANES];
    int strides[MLT_IMAGE_MAX_PLANES];
    void *data;
    mlt_destructor release_data;
    void *alpha;
    mlt_destructor release_alpha;
    mlt_destructor close;
};

MLT_EXPORT mlt_image mlt_image_new();
MLT_EXPORT void mlt_image_close(mlt_image self);
MLT_EXPORT void mlt_image_set_values(
    mlt_image self, void *data, mlt_image_format format, int width, int height);
MLT_EXPORT void mlt_image_get_values(
    mlt_image self, void **data, mlt_image_format *format, int *width, int *height);
MLT_EXPORT void mlt_image_alloc_data(mlt_image self);
MLT_EXPORT void mlt_image_alloc_alpha(mlt_image self);
MLT_EXPORT int mlt_image_calculate_size(mlt_image self);
MLT_EXPORT void mlt_image_fill_black(mlt_image self);
MLT_EXPORT void mlt_image_fill_checkerboard(mlt_image self, double sample_aspect_ratio);
MLT_EXPORT void mlt_image_fill_white(mlt_image self, int full_range);
MLT_EXPORT void mlt_image_fill_opaque(mlt_image self);
MLT_EXPORT int mlt_image_is_opaque(mlt_image self);
MLT_EXPORT const char *mlt_image_format_name(mlt_image_format format);
MLT_EXPORT mlt_image_format mlt_image_format_id(const char *name);
MLT_EXPORT const char *mlt_image_color_trc_name(mlt_color_trc trc);
MLT_EXPORT mlt_color_trc mlt_image_color_trc_id(const char *name);
MLT_EXPORT const char *mlt_image_colorspace_name(mlt_colorspace colorspace);
MLT_EXPORT mlt_colorspace mlt_image_colorspace_id(const char *name);
MLT_EXPORT const char *mlt_image_color_pri_name(mlt_color_primaries primaries);
MLT_EXPORT mlt_color_primaries mlt_image_color_pri_id(const char *name);
MLT_EXPORT mlt_colorspace mlt_image_default_colorspace(mlt_image_format format, int height);
MLT_EXPORT mlt_color_trc mlt_image_default_trc(mlt_colorspace colorspace);
MLT_EXPORT mlt_color_primaries mlt_image_default_primaries(mlt_colorspace colorspace, int height);
MLT_EXPORT int mlt_image_rgba_opaque(uint8_t *image, int width, int height);
MLT_EXPORT int mlt_image_full_range(const char *color_range);

// Deprecated functions
MLT_DEPRECATED_EXPORT int mlt_image_format_size(mlt_image_format format,
                                                int width,
                                                int height,
                                                int *bpp);
MLT_DEPRECATED_EXPORT void mlt_image_format_planes(
    mlt_image_format format, int width, int height, void *data, uint8_t *planes[4], int strides[4]);

#endif
