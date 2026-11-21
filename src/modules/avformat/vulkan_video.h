/*
 * vulkan_video.h -- FFmpeg Vulkan device context for FFmpeg 7.1+
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

#ifndef AVFORMAT_VULKAN_VIDEO_H
#define AVFORMAT_VULKAN_VIDEO_H

/* Include this before any other use of hwcontext_vulkan.h.
   Older Vulkan headers hide the video-queue enumerators behind the beta
   switch. Those enumerators are not macros. Do not #define them: a zero
   substitute makes FFmpeg reject a codec the device supports. Callers name
   VK_QUEUE_VIDEO_*_BIT_KHR only inside VK_KHR_video_decode_queue and
   VK_KHR_video_encode_queue. FFmpeg older than 7.1 does not include this. */

#ifndef VK_ENABLE_BETA_EXTENSIONS
#define VK_ENABLE_BETA_EXTENSIONS
#endif
#include <libavutil/hwcontext_vulkan.h>

#ifdef VK_KHR_video_decode_queue
_Static_assert(VK_QUEUE_VIDEO_DECODE_BIT_KHR != 0, "Vulkan decode queue flag must not be zero");
#endif
#ifdef VK_KHR_video_encode_queue
_Static_assert(VK_QUEUE_VIDEO_ENCODE_BIT_KHR != 0, "Vulkan encode queue flag must not be zero");
_Static_assert(VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR != 0,
               "Vulkan encode source usage must not be zero");
#endif

#endif
