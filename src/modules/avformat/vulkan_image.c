/*
 * vulkan_image.c -- mlt_image_vulkan from a decoded FFmpeg Vulkan frame
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

#include "vulkan_image.h"

#include "common.h"

#if HAVE_FFMPEG_VULKAN
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libavutil/pixdesc.h>

#include <stdlib.h>
#include <string.h>

typedef struct
{
    AVFrame *frame;
    AVHWFramesContext *hwfc;
    AVVulkanFramesContext *vkfc;
    AVVkFrame *vkf;
    mlt_vulkan_image *image;
} vulkan_export_sync;

/* av_frame_clone shares one AVVkFrame. Its timeline value has to move under
   the frame lock, or a second export waits on and signals a stale value. */
static void pull_vulkan_sync(mlt_vulkan_image *image)
{
    vulkan_export_sync *sync;
    int i;

    if (!image)
        return;
    sync = image->sync_opaque;
    if (!sync || !sync->vkf)
        return;
    for (i = 0; i < image->nb_planes && i < AV_NUM_DATA_POINTERS; i++) {
        if (!sync->vkf->img[i] && !sync->vkf->sem[i])
            continue;
        image->planes[i].layout = (int) sync->vkf->layout[i];
        image->planes[i].sem_value = sync->vkf->sem_value[i];
    }
}

static void push_vulkan_sync(mlt_vulkan_image *image)
{
    vulkan_export_sync *sync;
    int i;

    if (!image)
        return;
    sync = image->sync_opaque;
    if (!sync || !sync->vkf)
        return;
    for (i = 0; i < image->nb_planes && i < AV_NUM_DATA_POINTERS; i++) {
        if (!image->planes[i].image && !image->planes[i].semaphore)
            continue;
        sync->vkf->layout[i] = (VkImageLayout) image->planes[i].layout;
        sync->vkf->sem_value[i] = image->planes[i].sem_value;
        sync->vkf->access[i] = 0;
    }
}

static void lock_vulkan_image(void *opaque)
{
    vulkan_export_sync *sync = opaque;

    if (!sync || !sync->vkfc || !sync->vkfc->lock_frame)
        return;
    sync->vkfc->lock_frame(sync->hwfc, sync->vkf);
    pull_vulkan_sync(sync->image);
}

static void unlock_vulkan_image(void *opaque)
{
    vulkan_export_sync *sync = opaque;

    if (!sync || !sync->vkfc)
        return;
    push_vulkan_sync(sync->image);
    if (sync->vkfc->unlock_frame)
        sync->vkfc->unlock_frame(sync->hwfc, sync->vkf);
}

static void destroy_vulkan_image(void *p)
{
    mlt_vulkan_image *image = p;
    vulkan_export_sync *sync;

    if (!image)
        return;
    sync = image->sync_opaque;
    if (image->module_close)
        image->module_close(image->module_data);
    /* Unlock already published layout and sem_value. This copy can be older
       than a later export of the same AVVkFrame, so do not write it back. */
    if (sync) {
        av_frame_free(&sync->frame);
        free(sync);
    }
    free(image);
}

/* 10-bit and 12-bit samples stored in 16-bit containers. libplacebo samples
   the wider format and shifts. The image itself is unchanged. */
static VkFormat vulkan_view_format(const AVVulkanFramesContext *vkfc, VkFormat fmt, int *shift)
{
    *shift = 0;
    if (!(vkfc->img_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
        return fmt;
    switch (fmt) {
    case VK_FORMAT_R10X6_UNORM_PACK16:
        *shift = 6;
        return VK_FORMAT_R16_UNORM;
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        *shift = 6;
        return VK_FORMAT_R16G16_UNORM;
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        *shift = 6;
        return VK_FORMAT_R16G16B16A16_UNORM;
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
        *shift = 6;
        return VK_FORMAT_G16B16G16R16_422_UNORM;
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
        *shift = 6;
        return VK_FORMAT_B16G16R16G16_422_UNORM;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        *shift = 6;
        return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        *shift = 6;
        return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        *shift = 6;
        return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        *shift = 6;
        return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        *shift = 6;
        return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
        *shift = 6;
        return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM;
    case VK_FORMAT_R12X4_UNORM_PACK16:
        *shift = 4;
        return VK_FORMAT_R16_UNORM;
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        *shift = 4;
        return VK_FORMAT_R16G16_UNORM;
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        *shift = 4;
        return VK_FORMAT_R16G16B16A16_UNORM;
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
        *shift = 4;
        return VK_FORMAT_G16B16G16R16_422_UNORM;
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
        *shift = 4;
        return VK_FORMAT_B16G16R16G16_422_UNORM;
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        *shift = 4;
        return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        *shift = 4;
        return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        *shift = 4;
        return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        *shift = 4;
        return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        *shift = 4;
        return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
        *shift = 4;
        return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM;
    default:
        return fmt;
    }
}

static void fill_vulkan_plane(mlt_vulkan_plane *plane,
                              const AVVulkanFramesContext *vkfc,
                              const AVVkFrame *vkf,
                              const AVPixFmtDescriptor *desc,
                              int index,
                              int coded_w,
                              int coded_h,
                              int *bit_shift)
{
    int chroma = index == 1 || index == 2;
    int shift = 0;
    int c;

    memset(plane, 0, sizeof(*plane));
    plane->width = AV_CEIL_RSHIFT(coded_w, chroma ? desc->log2_chroma_w : 0);
    plane->height = AV_CEIL_RSHIFT(coded_h, chroma ? desc->log2_chroma_h : 0);
    plane->image = (uint64_t) (uintptr_t) vkf->img[index];
    plane->format = (int) vulkan_view_format(vkfc, vkfc->format[index], &shift);
    plane->layout = (int) vkf->layout[index];
    plane->semaphore = (uint64_t) (uintptr_t) vkf->sem[index];
    plane->sem_value = vkf->sem_value[index];
    if (index == 0)
        *bit_shift = shift;
    for (c = 0; c < desc->nb_components && plane->components < 4; c++) {
        if (desc->comp[c].plane == index)
            plane->component_mapping[plane->components++] = c;
    }
    for (c = plane->components; c < 4; c++)
        plane->component_mapping[c] = -1;
}
#endif

/* Copy the decoded VkImages into an mlt_image_vulkan. The AVFrame stays alive
   so the pool images and timeline semaphores do. This does not use
   allocate_buffer(): the payload is not a pixel buffer. */
int avformat_export_vulkan_frame(mlt_producer producer,
                                 struct AVFrame *video_frame,
                                 mlt_frame frame,
                                 uint8_t **buffer,
                                 mlt_image_format *format)
{
#if !HAVE_FFMPEG_VULKAN
    (void) producer;
    (void) video_frame;
    (void) frame;
    (void) buffer;
    (void) format;
    return 1;
#else
    AVFrame *cloned = av_frame_clone(video_frame);
    AVHWFramesContext *hwfc;
    AVVulkanFramesContext *vkfc;
    AVVkFrame *vkf;
    const AVPixFmtDescriptor *desc;
    vulkan_export_sync *sync;
    mlt_vulkan_image *image;
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int planes;
    int i;
    static int logged;

    if (!cloned || !cloned->hw_frames_ctx || !cloned->data[0]) {
        av_frame_free(&cloned);
        return 1;
    }
    hwfc = (AVHWFramesContext *) cloned->hw_frames_ctx->data;
    vkfc = hwfc->hwctx;
    vkf = (AVVkFrame *) cloned->data[0];
    desc = av_pix_fmt_desc_get(hwfc->sw_format);
    planes = desc ? av_pix_fmt_count_planes(hwfc->sw_format) : 0;
    if (!desc || !vkfc || planes < 1 || planes > MLT_VULKAN_MAX_PLANES) {
        av_frame_free(&cloned);
        return 1;
    }

    image = calloc(1, sizeof(*image));
    sync = calloc(1, sizeof(*sync));
    if (!image || !sync) {
        free(image);
        free(sync);
        av_frame_free(&cloned);
        return 1;
    }
    sync->frame = cloned;
    sync->hwfc = hwfc;
    sync->vkfc = vkfc;
    sync->vkf = vkf;

    image->width = cloned->width;
    image->height = cloned->height;
    image->crop_left = cloned->crop_left;
    image->crop_top = cloned->crop_top;
    image->crop_right = cloned->crop_right;
    image->crop_bottom = cloned->crop_bottom;
    image->nb_planes = planes;
    image->usage = (uint32_t) vkfc->usage;
    if (!image->usage) {
        image->usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
                       | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    image->rgb = (desc->flags & AV_PIX_FMT_FLAG_RGB) ? 1 : 0;
    image->alpha = (desc->flags & AV_PIX_FMT_FLAG_ALPHA) ? 1 : 0;
    image->bit_depth = desc->comp[0].depth;
    image->colorspace = av_to_mlt_colorspace(cloned->colorspace, cloned->width, cloned->height);
    image->color_trc = av_to_mlt_color_trc(cloned->color_trc);
    image->color_primaries = av_to_mlt_color_primaries(cloned->color_primaries);
    image->full_range = image->rgb ? 1 : av_to_mlt_full_range(cloned->color_range);
    image->chroma_location = (int) cloned->chroma_location;
    image->log2_chroma_w = desc->log2_chroma_w;
    image->log2_chroma_h = desc->log2_chroma_h;
    image->lock = lock_vulkan_image;
    image->unlock = unlock_vulkan_image;
    image->sync_opaque = sync;
    sync->image = image;
    /* Snapshot under the frame lock. Do not call lock_vulkan_image here:
       that pulls into planes this loop is about to fill. */
    if (vkfc->lock_frame && vkfc->unlock_frame)
        vkfc->lock_frame(hwfc, vkf);
    for (i = 0; i < planes; i++)
        fill_vulkan_plane(&image->planes[i],
                          vkfc,
                          vkf,
                          desc,
                          i,
                          hwfc->width,
                          hwfc->height,
                          &image->bit_shift);
    if (vkfc->lock_frame && vkfc->unlock_frame)
        vkfc->unlock_frame(hwfc, vkf);
    if (!image->planes[0].image) {
        destroy_vulkan_image(image);
        return 1;
    }

    *format = mlt_image_vulkan;
    *buffer = (uint8_t *) image;
    mlt_properties_set_int(props, "format", mlt_image_vulkan);
    mlt_properties_set(props, "mlt_image_private", NULL);
    mlt_properties_set_int(props, "colorspace", image->colorspace);
    mlt_properties_set_int(props, "color_trc", image->color_trc);
    mlt_properties_set_int(props, "color_primaries", image->color_primaries);
    mlt_properties_set_int(props, "full_range", image->full_range);
    if (!logged) {
        logged = 1;
        mlt_log_info(MLT_PRODUCER_SERVICE(producer),
                     "Vulkan texture %dx%d (%d planes)\n",
                     image->width,
                     image->height,
                     image->nb_planes);
    }
    if (mlt_frame_set_image(frame, (uint8_t *) image, 0, destroy_vulkan_image)) {
        destroy_vulkan_image(image);
        *buffer = NULL;
        return 1;
    }
    return 0;
#endif
}
