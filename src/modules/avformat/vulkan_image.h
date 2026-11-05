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

/* Fill an mlt_image_vulkan from video_frame and install it on frame.
   Returns 0 when the frame holds the texture. The timeline value is pulled
   under FFmpeg's frame lock and pushed on unlock. The destructor does not
   write the semaphore back. The caller sets the output width and height. */
int avformat_export_vulkan_frame(mlt_producer producer,
                                 struct AVFrame *video_frame,
                                 mlt_frame frame,
                                 uint8_t **buffer,
                                 mlt_image_format *format);

#endif
