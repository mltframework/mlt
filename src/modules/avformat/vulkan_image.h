/*
 * vulkan_image.h -- mlt_image_vulkan from a decoded FFmpeg Vulkan frame
 * Copyright (C) 2026 Meltytech, LLC
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

#ifndef AVFORMAT_VULKAN_IMAGE_H
#define AVFORMAT_VULKAN_IMAGE_H

#include <framework/mlt.h>

struct AVFrame;
struct AVBufferRef;

/* Fill an mlt_image_vulkan from video_frame and install it on frame.
   Returns 0 when the frame holds the texture. The timeline value is pulled
   under FFmpeg's frame lock and pushed on unlock. The destructor does not
   write the semaphore back. The caller sets the output width and height. */
int avformat_export_vulkan_frame(mlt_producer producer,
                                 struct AVFrame *video_frame,
                                 mlt_frame frame,
                                 uint8_t **buffer,
                                 mlt_image_format *format);

/* Borrow the VkDevice published as "placebo.vulkan". The returned context
   does not destroy that device. need_encode requires a video encode queue.
   Returns NULL when the device is missing or unsuitable. */
struct AVBufferRef *avformat_vulkan_adopt_placebo(int need_encode);

/* GPU conversion of an mlt_vulkan_image into an encoder Vulkan frame.
   The filter graph is rebuilt when the size or format changes. */
typedef struct avformat_vulkan_encoder_s avformat_vulkan_encoder;

avformat_vulkan_encoder *avformat_vulkan_encoder_new(void);
void avformat_vulkan_encoder_close(avformat_vulkan_encoder *enc);

/* dst_sw_fmt is an AVPixelFormat. dst_colorspace is an AVColorSpace used as
   the RGB to YUV matrix. On success *out holds a new AVFrame. */
int avformat_vulkan_encoder_frame(avformat_vulkan_encoder *enc,
                                 struct AVBufferRef *device,
                                 int dst_w,
                                 int dst_h,
                                 int dst_sw_fmt,
                                 int dst_full_range,
                                 int dst_colorspace,
                                 mlt_vulkan_image *image,
                                 struct AVFrame **out);

#endif
