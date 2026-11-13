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
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#include <pthread.h>
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

/* FFmpeg allocates AVVkFrame.internal and puts a pthread mutex first.
   The rest is private and stays unused for a frame we never export. */
typedef struct
{
    pthread_mutex_t update_mutex;
} vulkan_frame_mutex;

typedef struct
{
    void *ctx;
    void (*lock_queue)(void *, uint32_t, uint32_t);
    void (*unlock_queue)(void *, uint32_t, uint32_t);
} vulkan_queue_lock;

typedef struct
{
    mlt_vulkan_image *image;
} vulkan_wrap_owner;

struct avformat_vulkan_encoder_s
{
    AVFilterGraph *graph;
    AVFilterContext *src;
    AVFilterContext *sink;
    AVBufferRef *in_frames;
    int src_w;
    int src_h;
    enum AVPixelFormat src_sw;
    int src_full;
    int dst_w;
    int dst_h;
    enum AVPixelFormat dst_sw;
    int dst_full;
    int logged;
};

static void load_fn_ptr(void *ptr, void *dest, size_t size)
{
    if (!ptr || size != sizeof(ptr))
        return;
    memcpy(dest, &ptr, size);
}

static void placebo_lock_queue(struct AVHWDeviceContext *dev_ctx,
                               uint32_t queue_family,
                               uint32_t index)
{
    vulkan_queue_lock *lock = dev_ctx->user_opaque;

    if (lock && lock->lock_queue && lock->ctx)
        lock->lock_queue(lock->ctx, queue_family, index);
}

static void placebo_unlock_queue(struct AVHWDeviceContext *dev_ctx,
                                 uint32_t queue_family,
                                 uint32_t index)
{
    vulkan_queue_lock *lock = dev_ctx->user_opaque;

    if (lock && lock->unlock_queue && lock->ctx)
        lock->unlock_queue(lock->ctx, queue_family, index);
}

static void free_adopted_lock(struct AVHWDeviceContext *ctx)
{
    free(ctx->user_opaque);
    ctx->user_opaque = NULL;
}

static void add_queue_family(AVVulkanDeviceContext *vk, int *nb, int idx, int num, VkQueueFlags flags)
{
    int i;

    if (idx < 0 || num <= 0 || !flags)
        return;
    for (i = 0; i < *nb; i++) {
        if (vk->qf[i].idx == idx) {
            vk->qf[i].flags |= (VkQueueFlagBits) flags;
            if (num > vk->qf[i].num)
                vk->qf[i].num = num;
            return;
        }
    }
    if (*nb >= (int) (sizeof(vk->qf) / sizeof(vk->qf[0])))
        return;
    vk->qf[*nb] = (AVVulkanDeviceQueueFamily){
        .idx = idx,
        .num = num,
        .flags = (VkQueueFlagBits) flags,
    };
    (*nb)++;
}

static void destroy_wrapped_vkframe(AVVkFrame *vkf)
{
    if (!vkf)
        return;
    if (vkf->internal) {
        vulkan_frame_mutex *internal = (vulkan_frame_mutex *) vkf->internal;
        pthread_mutex_destroy(&internal->update_mutex);
        av_freep(&vkf->internal);
    }
    av_free(vkf);
}

/* image->lock would copy the original semaphore over the value this submit
   just produced. Lock the decoded frame directly when there is one. */
static void write_sync_back(mlt_vulkan_image *image, AVVkFrame *vkf)
{
    vulkan_export_sync *sync;
    int i;

    if (!image || !vkf)
        return;
    for (i = 0; i < image->nb_planes && i < AV_NUM_DATA_POINTERS; i++) {
        if (!image->planes[i].image && !image->planes[i].semaphore)
            continue;
        image->planes[i].layout = (int) vkf->layout[i];
        image->planes[i].sem_value = vkf->sem_value[i];
    }
    sync = image->sync_opaque;
    if (!sync || !sync->vkf)
        return;
    if (sync->vkfc && sync->vkfc->lock_frame)
        sync->vkfc->lock_frame(sync->hwfc, sync->vkf);
    push_vulkan_sync(image);
    if (sync->vkfc && sync->vkfc->unlock_frame)
        sync->vkfc->unlock_frame(sync->hwfc, sync->vkf);
}

static void free_wrapped_vkframe(void *opaque, uint8_t *data)
{
    vulkan_wrap_owner *owner = opaque;

    if (owner && owner->image)
        write_sync_back(owner->image, (AVVkFrame *) data);
    destroy_wrapped_vkframe((AVVkFrame *) data);
    free(owner);
}

static int plane_format_matches(const mlt_vulkan_image *image, enum AVPixelFormat pix)
{
    const VkFormat *fmts = av_vkfmt_from_pixfmt(pix);
    int planes;
    int i;

    if (!fmts || !image)
        return 0;
    if (fmts[0] != (VkFormat) image->planes[0].format)
        return 0;
    planes = av_pix_fmt_count_planes(pix);
    for (i = 1; i < planes && i < image->nb_planes; i++) {
        if (!image->planes[i].format)
            continue;
        if (fmts[i] != VK_FORMAT_UNDEFINED && fmts[i] != (VkFormat) image->planes[i].format)
            return 0;
    }
    return 1;
}

/* An image exported from FFmpeg keeps its frames context. That names the exact
   software format and the VkFormats FFmpeg chose, which the plane format alone
   cannot: a multi-planar 16-bit view with bit_shift 6 is P010, not P016.
   lock_vulkan_image is installed only by the exporter, so it marks these
   images. Anything else, such as a placebo texture, has no such context. */
static const vulkan_export_sync *exported_sync(const mlt_vulkan_image *image)
{
    return image->lock == lock_vulkan_image ? image->sync_opaque : NULL;
}

/* The software format of an image. An exported image states it. For any other
   image it is the first candidate whose per-plane VkFormats match. */
static enum AVPixelFormat vulkan_sw_format(const mlt_vulkan_image *image)
{
    static const enum AVPixelFormat candidates[] = {
        AV_PIX_FMT_RGBA,      AV_PIX_FMT_BGRA,      AV_PIX_FMT_RGB0,      AV_PIX_FMT_BGR0,
        AV_PIX_FMT_RGBA64,    AV_PIX_FMT_BGRA64,    AV_PIX_FMT_RGBAF16,   AV_PIX_FMT_RGBAF32,
        AV_PIX_FMT_NV12,      AV_PIX_FMT_P010,      AV_PIX_FMT_P016,      AV_PIX_FMT_NV16,
        AV_PIX_FMT_NV24,      AV_PIX_FMT_YUV420P,   AV_PIX_FMT_YUV422P,   AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_YUV420P10, AV_PIX_FMT_YUV422P10, AV_PIX_FMT_YUV444P10, AV_PIX_FMT_NONE,
    };
    int bgra = image->planes[0].component_mapping[0] == 2;
    const vulkan_export_sync *sync = exported_sync(image);
    int i;

    if (sync)
        return sync->hwfc->sw_format;
    for (i = 0; candidates[i] != AV_PIX_FMT_NONE; i++) {
        int is_bgr = candidates[i] == AV_PIX_FMT_BGRA || candidates[i] == AV_PIX_FMT_BGR0
                     || candidates[i] == AV_PIX_FMT_BGRA64;
        if (bgra != is_bgr)
            continue;
        if (plane_format_matches(image, candidates[i]))
            return candidates[i];
    }
    return AV_PIX_FMT_NONE;
}

static void vulkan_coded_size(const mlt_vulkan_image *image, int *coded_w, int *coded_h)
{
    int w = image->planes[0].width > 0 ? image->planes[0].width
                                       : image->width + image->crop_left + image->crop_right;
    int h = image->planes[0].height > 0 ? image->planes[0].height
                                        : image->height + image->crop_top + image->crop_bottom;

    if (w < image->width)
        w = image->width;
    if (h < image->height)
        h = image->height;
    *coded_w = w;
    *coded_h = h;
}

static int colorspace_matrix(const mlt_vulkan_image *image, int dst_colorspace)
{
    int matrix = dst_colorspace;

    if (!image->rgb) {
        int from_image = mlt_to_av_colorspace(image->colorspace, image->height);
        if (from_image > AVCOL_SPC_RGB && from_image != AVCOL_SPC_UNSPECIFIED
            && from_image != AVCOL_SPC_RESERVED)
            matrix = from_image;
    }
    if (matrix <= AVCOL_SPC_RGB || matrix == AVCOL_SPC_UNSPECIFIED || matrix == AVCOL_SPC_RESERVED)
        matrix = AVCOL_SPC_BT709;
    return matrix;
}

static void release_input_frame(AVFrame *in, vulkan_wrap_owner *owner, mlt_vulkan_image *image)
{
    if (in && in->data[0] && image)
        write_sync_back(image, (AVVkFrame *) in->data[0]);
    if (owner)
        owner->image = NULL;
    av_frame_free(&in);
}

static int alloc_input_frames(
    AVBufferRef *device, const mlt_vulkan_image *image, int coded_w, int coded_h, AVBufferRef **out)
{
    AVBufferRef *ref = av_hwframe_ctx_alloc(device);
    AVHWFramesContext *hwfc;
    AVVulkanFramesContext *vkfc;
    const vulkan_export_sync *sync = exported_sync(image);
    int i;
    int err;

    if (!ref)
        return AVERROR(ENOMEM);
    hwfc = (AVHWFramesContext *) ref->data;
    vkfc = hwfc->hwctx;
    hwfc->format = AV_PIX_FMT_VULKAN;
    hwfc->sw_format = vulkan_sw_format(image);
    hwfc->width = coded_w;
    hwfc->height = coded_h;
    vkfc->tiling = VK_IMAGE_TILING_OPTIMAL;
    if (sync) {
        memcpy(vkfc->format, sync->vkfc->format, sizeof(vkfc->format));
    } else {
        for (i = 0; i < image->nb_planes && i < AV_NUM_DATA_POINTERS; i++) {
            if (image->planes[i].format)
                vkfc->format[i] = (VkFormat) image->planes[i].format;
        }
    }
    err = av_hwframe_ctx_init(ref);
    if (err < 0) {
        av_buffer_unref(&ref);
        return err;
    }
    /* The wrapped images were not allocated as encode sources. Clearing this
       stops the conversion filter from reusing the description for its output. */
    vkfc->usage &= ~(VkImageUsageFlags) VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
    *out = ref;
    return 0;
}

static int configure_encoder_graph(avformat_vulkan_encoder *enc,
                                   AVBufferRef *device,
                                   const mlt_vulkan_image *image,
                                   int src_w,
                                   int src_h,
                                   enum AVPixelFormat src_sw,
                                   int src_full,
                                   int dst_w,
                                   int dst_h,
                                   enum AVPixelFormat dst_sw,
                                   int dst_full,
                                   int colorspace)
{
    AVFilterGraph *graph;
    AVFilterContext *src;
    AVFilterContext *convert;
    AVFilterContext *sink;
    AVBufferSrcParameters *par;
    AVBufferRef *in_frames = NULL;
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_VULKAN, AV_PIX_FMT_NONE};
    char args[512];
    const char *dst_name;
    const char *matrix_name;
    const char *trc_name;
    const char *pri_name;
    int trc;
    int pri;
    int err;

    avfilter_graph_free(&enc->graph);
    av_buffer_unref(&enc->in_frames);
    enc->src = NULL;
    enc->sink = NULL;

    err = alloc_input_frames(device, image, src_w, src_h, &in_frames);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan input frames init failed: %d\n", err);
        return err;
    }

    graph = avfilter_graph_alloc();
    if (!graph) {
        av_buffer_unref(&in_frames);
        return AVERROR(ENOMEM);
    }

    /* parameters_set must run before init. A hardware pix_fmt in the
       create-filter args is rejected while hw_frames_ctx is still null. */
    src = avfilter_graph_alloc_filter(graph, avfilter_get_by_name("buffer"), "mlt_vulkan_buffer");
    if (!src) {
        err = AVERROR_FILTER_NOT_FOUND;
        goto fail;
    }
    par = av_buffersrc_parameters_alloc();
    if (!par) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
    par->format = AV_PIX_FMT_VULKAN;
    par->width = image->width;
    par->height = image->height;
    par->time_base = (AVRational){1, 1};
    par->sample_aspect_ratio = (AVRational){1, 1};
    par->hw_frames_ctx = in_frames;
    par->color_space = (enum AVColorSpace) colorspace;
    par->color_range = src_full ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
    err = av_buffersrc_parameters_set(src, par);
    av_free(par);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan buffer parameters failed: %d\n", err);
        goto fail;
    }
    err = avfilter_init_dict(src, NULL);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan buffer source failed: %d\n", err);
        goto fail;
    }

    /* scale_vulkan only converts RGB to YUV in its compute shader when both
       dimensions change. A same-size change goes through swscale, which cannot
       run this hardware conversion. libplacebo samples the Vulkan image and
       writes the encoder format on the same device. */
    dst_name = av_get_pix_fmt_name(dst_sw);
    matrix_name = av_color_space_name(colorspace);
    if (!matrix_name || !strcmp(matrix_name, "unknown") || !strcmp(matrix_name, "unspecified"))
        matrix_name = "bt709";
    trc = mlt_to_av_color_trc(image->color_trc);
    if (trc == AVCOL_TRC_UNSPECIFIED)
        trc = AVCOL_TRC_BT709;
    trc_name = av_color_transfer_name(trc);
    if (!trc_name || !strcmp(trc_name, "unknown") || !strcmp(trc_name, "unspecified"))
        trc_name = "bt709";
    pri = mlt_to_av_color_primaries(image->color_primaries);
    if (pri == AVCOL_PRI_UNSPECIFIED)
        pri = AVCOL_PRI_BT709;
    pri_name = av_color_primaries_name(pri);
    if (!pri_name || !strcmp(pri_name, "unknown") || !strcmp(pri_name, "unspecified"))
        pri_name = "bt709";
    snprintf(args,
             sizeof(args),
             "w=%d:h=%d:format=%s:colorspace=%s:color_trc=%s:color_primaries=%s:range=%s",
             dst_w,
             dst_h,
             dst_name ? dst_name : "nv12",
             matrix_name,
             trc_name,
             pri_name,
             dst_full ? "pc" : "tv");
    convert = avfilter_graph_alloc_filter(graph,
                                          avfilter_get_by_name("libplacebo"),
                                          "mlt_libplacebo");
    if (!convert) {
        mlt_log_error(NULL, "libplacebo filter is not available\n");
        err = AVERROR_FILTER_NOT_FOUND;
        goto fail;
    }
    convert->hw_device_ctx = av_buffer_ref(device);
    err = avfilter_init_str(convert, args);
    if (err < 0) {
        mlt_log_error(NULL, "libplacebo init failed: %d\n", err);
        goto fail;
    }

    sink = avfilter_graph_alloc_filter(graph, avfilter_get_by_name("buffersink"), "mlt_vulkan_sink");
    if (!sink) {
        err = AVERROR_FILTER_NOT_FOUND;
        goto fail;
    }
#if LIBAVFILTER_VERSION_INT >= ((10 << 16) + (6 << 8) + 100)
    err = av_opt_set_array(sink,
                           "pixel_formats",
                           AV_OPT_SEARCH_CHILDREN,
                           0,
                           1,
                           AV_OPT_TYPE_PIXEL_FMT,
                           pix_fmts);
#else
    err = av_opt_set_int_list(sink, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
#endif
    if (err < 0)
        goto fail;
    err = avfilter_init_dict(sink, NULL);
    if (err < 0)
        goto fail;

    err = avfilter_link(src, 0, convert, 0);
    if (err < 0)
        goto fail;
    err = avfilter_link(convert, 0, sink, 0);
    if (err < 0)
        goto fail;
    err = avfilter_graph_config(graph, NULL);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan conversion graph failed: %d\n", err);
        goto fail;
    }

    enc->graph = graph;
    enc->src = src;
    enc->sink = sink;
    enc->in_frames = in_frames;
    enc->src_w = src_w;
    enc->src_h = src_h;
    enc->src_sw = src_sw;
    enc->src_full = src_full;
    enc->dst_w = dst_w;
    enc->dst_h = dst_h;
    enc->dst_sw = dst_sw;
    enc->dst_full = dst_full;
    return 0;

fail:
    avfilter_graph_free(&graph);
    av_buffer_unref(&in_frames);
    return err;
}

static int wrap_vulkan_image(mlt_vulkan_image *image,
                             AVBufferRef *in_frames,
                             int colorspace,
                             int src_full,
                             AVFrame **frame_out,
                             vulkan_wrap_owner **owner_out)
{
    AVVkFrame *vkf;
    AVFrame *frame;
    vulkan_wrap_owner *owner;
    int i;
    static int shift_logged;

    *frame_out = NULL;
    *owner_out = NULL;
    if (image->bit_shift && !exported_sync(image) && !shift_logged) {
        shift_logged = 1;
        mlt_log_warning(NULL,
                        "Vulkan image bit_shift %d is not applied by the encoder\n",
                        image->bit_shift);
    }

    vkf = av_vk_frame_alloc();
    frame = av_frame_alloc();
    owner = calloc(1, sizeof(*owner));
    if (!vkf || !frame || !owner) {
        destroy_wrapped_vkframe(vkf);
        av_frame_free(&frame);
        free(owner);
        return AVERROR(ENOMEM);
    }
    owner->image = image;
    vkf->tiling = VK_IMAGE_TILING_OPTIMAL;
    for (i = 0; i < image->nb_planes && i < AV_NUM_DATA_POINTERS; i++) {
        if (!image->planes[i].image)
            continue;
        if (!image->planes[i].semaphore) {
            mlt_log_error(NULL, "Vulkan image plane %d has no timeline semaphore\n", i);
            destroy_wrapped_vkframe(vkf);
            av_frame_free(&frame);
            free(owner);
            return AVERROR(EINVAL);
        }
        vkf->img[i] = (VkImage) (uintptr_t) image->planes[i].image;
        vkf->layout[i] = (VkImageLayout) image->planes[i].layout;
        vkf->sem[i] = (VkSemaphore) (uintptr_t) image->planes[i].semaphore;
        vkf->sem_value[i] = image->planes[i].sem_value;
        vkf->access[i] = 0;
        vkf->queue_family[i] = VK_QUEUE_FAMILY_IGNORED;
    }

    frame->buf[0] = av_buffer_create((uint8_t *) vkf, sizeof(*vkf), free_wrapped_vkframe, owner, 0);
    if (!frame->buf[0]) {
        destroy_wrapped_vkframe(vkf);
        av_frame_free(&frame);
        free(owner);
        return AVERROR(ENOMEM);
    }
    frame->data[0] = (uint8_t *) vkf;
    frame->format = AV_PIX_FMT_VULKAN;
    frame->width = image->width;
    frame->height = image->height;
    frame->crop_left = image->crop_left;
    frame->crop_top = image->crop_top;
    frame->crop_right = image->crop_right;
    frame->crop_bottom = image->crop_bottom;
    frame->colorspace = colorspace;
    frame->color_range = src_full ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
    /* Unspecified primaries make swscale insert a hardware LUT it cannot run.
       The output frame copies these, so source and destination stay matched. */
    frame->color_primaries = mlt_to_av_color_primaries(image->color_primaries);
    if (frame->color_primaries == AVCOL_PRI_UNSPECIFIED)
        frame->color_primaries = AVCOL_PRI_BT709;
    frame->color_trc = mlt_to_av_color_trc(image->color_trc);
    if (frame->color_trc == AVCOL_TRC_UNSPECIFIED)
        frame->color_trc = AVCOL_TRC_BT709;
    if (image->chroma_location > 0)
        frame->chroma_location = image->chroma_location;
    frame->hw_frames_ctx = av_buffer_ref(in_frames);
    if (!frame->hw_frames_ctx) {
        av_frame_free(&frame);
        return AVERROR(ENOMEM);
    }
    *frame_out = frame;
    *owner_out = owner;
    return 0;
}

AVBufferRef *avformat_vulkan_adopt_placebo(int need_encode)
{
    mlt_properties published = mlt_properties_get_data(mlt_global_properties(),
                                                       "placebo.vulkan",
                                                       NULL);
    vulkan_queue_lock *lock;
    void *instance;
    void *phys_dev;
    void *device;
    void *features;
    void (*lock_queue)(void *, uint32_t, uint32_t) = NULL;
    void (*unlock_queue)(void *, uint32_t, uint32_t) = NULL;
    PFN_vkGetInstanceProcAddr get_proc_addr = NULL;
    AVBufferRef *ref;
    AVHWDeviceContext *device_ctx;
    AVVulkanDeviceContext *vk;
    int encode_index;
    int encode_count;
    int decode_index;
    int decode_count;
    int nb_qf = 0;
    int ret;

    if (!published)
        return NULL;

    encode_count = mlt_properties_get_int(published, "queue_encode_count");
    encode_index = mlt_properties_get_int(published, "queue_encode_index");
    decode_count = mlt_properties_get_int(published, "queue_decode_count");
    decode_index = mlt_properties_get_int(published, "queue_decode_index");
    if (need_encode && (encode_count <= 0 || encode_index < 0)) {
        mlt_log_error(NULL, "shared placebo Vulkan device has no encode queue\n");
        return NULL;
    }

    instance = mlt_properties_get_data(published, "instance", NULL);
    phys_dev = mlt_properties_get_data(published, "phys_dev", NULL);
    device = mlt_properties_get_data(published, "device", NULL);
    features = mlt_properties_get_data(published, "features", NULL);
    load_fn_ptr(mlt_properties_get_data(published, "lock_queue", NULL),
                &lock_queue,
                sizeof(lock_queue));
    load_fn_ptr(mlt_properties_get_data(published, "unlock_queue", NULL),
                &unlock_queue,
                sizeof(unlock_queue));
    load_fn_ptr(mlt_properties_get_data(published, "get_proc_addr", NULL),
                &get_proc_addr,
                sizeof(get_proc_addr));
    if (!instance || !phys_dev || !device || !features || !lock_queue || !unlock_queue) {
        mlt_log_error(NULL, "shared placebo Vulkan device is incomplete\n");
        return NULL;
    }

    lock = calloc(1, sizeof(*lock));
    ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VULKAN);
    if (!lock || !ref) {
        free(lock);
        av_buffer_unref(&ref);
        return NULL;
    }
    lock->ctx = mlt_properties_get_data(published, "lock_ctx", NULL);
    lock->lock_queue = lock_queue;
    lock->unlock_queue = unlock_queue;
    if (!lock->ctx) {
        mlt_log_error(NULL, "shared placebo Vulkan device is incomplete\n");
        free(lock);
        av_buffer_unref(&ref);
        return NULL;
    }

    device_ctx = (AVHWDeviceContext *) ref->data;
    device_ctx->user_opaque = lock;
    device_ctx->free = free_adopted_lock;
    vk = device_ctx->hwctx;
#if FF_API_VULKAN_SYNC_QUEUES
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    vk->lock_queue = placebo_lock_queue;
    vk->unlock_queue = placebo_unlock_queue;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
    vk->get_proc_addr = get_proc_addr;
    vk->inst = (VkInstance) instance;
    vk->phys_dev = (VkPhysicalDevice) phys_dev;
    vk->act_dev = (VkDevice) device;
    vk->device_features = *(const VkPhysicalDeviceFeatures2 *) features;
    vk->enabled_inst_extensions = mlt_properties_get_data(published, "inst_extensions", NULL);
    vk->nb_enabled_inst_extensions = mlt_properties_get_int(published, "nb_inst_extensions");
    vk->enabled_dev_extensions = mlt_properties_get_data(published, "dev_extensions", NULL);
    vk->nb_enabled_dev_extensions = mlt_properties_get_int(published, "nb_dev_extensions");

    add_queue_family(vk,
                     &nb_qf,
                     mlt_properties_get_int(published, "queue_graphics_index"),
                     mlt_properties_get_int(published, "queue_graphics_count"),
                     VK_QUEUE_GRAPHICS_BIT);
    add_queue_family(vk,
                     &nb_qf,
                     mlt_properties_get_int(published, "queue_transfer_index"),
                     mlt_properties_get_int(published, "queue_transfer_count"),
                     VK_QUEUE_TRANSFER_BIT);
    add_queue_family(vk,
                     &nb_qf,
                     mlt_properties_get_int(published, "queue_compute_index"),
                     mlt_properties_get_int(published, "queue_compute_count"),
                     VK_QUEUE_COMPUTE_BIT);
    if (decode_count > 0 && decode_index >= 0)
        add_queue_family(vk, &nb_qf, decode_index, decode_count, VK_QUEUE_VIDEO_DECODE_BIT_KHR);
    if (encode_count > 0 && encode_index >= 0)
        add_queue_family(vk, &nb_qf, encode_index, encode_count, VK_QUEUE_VIDEO_ENCODE_BIT_KHR);
    vk->nb_qf = nb_qf;

    ret = av_hwdevice_ctx_init(ref);
    if (ret < 0) {
        mlt_log_error(NULL, "shared placebo Vulkan device init failed: %d\n", ret);
        av_buffer_unref(&ref);
        return NULL;
    }
    mlt_log_info(NULL, "hwaccel vulkan using shared placebo device\n");
    return ref;
}

avformat_vulkan_encoder *avformat_vulkan_encoder_new(void)
{
    return calloc(1, sizeof(avformat_vulkan_encoder));
}

void avformat_vulkan_encoder_close(avformat_vulkan_encoder *enc)
{
    if (!enc)
        return;
    avfilter_graph_free(&enc->graph);
    av_buffer_unref(&enc->in_frames);
    free(enc);
}

int avformat_vulkan_encoder_frame(avformat_vulkan_encoder *enc,
                                  AVBufferRef *device,
                                  int dst_w,
                                  int dst_h,
                                  int dst_sw_fmt,
                                  int dst_full_range,
                                  int dst_colorspace,
                                  mlt_vulkan_image *image,
                                  AVFrame **out)
{
    enum AVPixelFormat src_sw;
    enum AVPixelFormat dst_sw = (enum AVPixelFormat) dst_sw_fmt;
    int src_full;
    int colorspace;
    int coded_w;
    int coded_h;
    int err;
    AVFrame *in = NULL;
    AVFrame *filtered = NULL;
    vulkan_wrap_owner *owner = NULL;

    if (out)
        *out = NULL;
    if (!enc || !device || !image || !out || image->nb_planes < 1 || !image->planes[0].image)
        return AVERROR(EINVAL);

    src_sw = vulkan_sw_format(image);
    if (src_sw == AV_PIX_FMT_NONE) {
        mlt_log_error(NULL,
                      "Vulkan image VkFormat %d has no software format\n",
                      image->planes[0].format);
        return AVERROR(EINVAL);
    }
    src_full = image->full_range || image->rgb;
    colorspace = colorspace_matrix(image, dst_colorspace);
    vulkan_coded_size(image, &coded_w, &coded_h);

    if (!enc->graph || enc->src_w != coded_w || enc->src_h != coded_h || enc->src_sw != src_sw
        || enc->src_full != src_full || enc->dst_w != dst_w || enc->dst_h != dst_h
        || enc->dst_sw != dst_sw || enc->dst_full != dst_full_range) {
        err = configure_encoder_graph(enc,
                                      device,
                                      image,
                                      coded_w,
                                      coded_h,
                                      src_sw,
                                      src_full,
                                      dst_w,
                                      dst_h,
                                      dst_sw,
                                      dst_full_range,
                                      colorspace);
        if (err < 0)
            return err;
    }

    err = wrap_vulkan_image(image, enc->in_frames, colorspace, src_full, &in, &owner);
    if (err < 0)
        return err;
    err = av_buffersrc_add_frame_flags(enc->src, in, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (err >= 0) {
        filtered = av_frame_alloc();
        if (!filtered)
            err = AVERROR(ENOMEM);
        else
            err = av_buffersink_get_frame(enc->sink, filtered);
    }
    release_input_frame(in, owner, image);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan image conversion failed: %d\n", err);
        av_frame_free(&filtered);
        return err;
    }
    if (!enc->logged) {
        enc->logged = 1;
        mlt_log_info(NULL,
                     "encoding Vulkan image %dx%d as %s\n",
                     image->width,
                     image->height,
                     av_get_pix_fmt_name(dst_sw));
    }
    *out = filtered;
    return 0;
}

struct avformat_vulkan_avfilter_s
{
    AVFilterGraph *graph;
    AVFilterContext *src;
    AVFilterContext *filter;
    AVFilterContext *sink;
    AVBufferRef *device;
    AVBufferRef *in_frames;
    int width;
    int height;
    int coded_w;
    int coded_h;
    enum AVPixelFormat sw;
    int full;
    int colorspace;
    int sar_num;
    int sar_den;
    int tb_num;
    int tb_den;
    int fps_num;
    int fps_den;
    int logged;
};

avformat_vulkan_avfilter *avformat_vulkan_avfilter_new(void)
{
    return calloc(1, sizeof(avformat_vulkan_avfilter));
}

void avformat_vulkan_avfilter_close(avformat_vulkan_avfilter *self)
{
    if (!self)
        return;
    avfilter_graph_free(&self->graph);
    av_buffer_unref(&self->in_frames);
    av_buffer_unref(&self->device);
    free(self);
}

AVFilterGraph *avformat_vulkan_avfilter_graph(avformat_vulkan_avfilter *self)
{
    return self ? self->graph : NULL;
}

AVFilterContext *avformat_vulkan_avfilter_ctx(avformat_vulkan_avfilter *self)
{
    return self ? self->filter : NULL;
}

static int configure_user_filter(avformat_vulkan_avfilter *self,
                                 const AVFilter *filter,
                                 const mlt_vulkan_image *image,
                                 int sar_num,
                                 int sar_den,
                                 int tb_num,
                                 int tb_den,
                                 int fps_num,
                                 int fps_den,
                                 int threads,
                                 avformat_vulkan_avfilter_init_fn init_filter,
                                 void *opaque)
{
    AVFilterGraph *graph = NULL;
    AVFilterContext *src = NULL;
    AVFilterContext *user = NULL;
    AVFilterContext *sink = NULL;
    AVBufferSrcParameters *par = NULL;
    AVBufferRef *in_frames = NULL;
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_VULKAN, AV_PIX_FMT_NONE};
    int coded_w = 0;
    int coded_h = 0;
    int full;
    int colorspace;
    int err;

    if (sar_den == 0) {
        sar_num = 1;
        sar_den = 1;
    }
    if (tb_den == 0) {
        tb_num = 1;
        tb_den = 1;
    }
    if (fps_den == 0) {
        fps_num = 0;
        fps_den = 1;
    }

    full = image->full_range || image->rgb;
    colorspace = colorspace_matrix(image, mlt_to_av_colorspace(image->colorspace, image->height));
    vulkan_coded_size(image, &coded_w, &coded_h);

    if (!self->device) {
        self->device = avformat_vulkan_adopt_placebo(0);
        if (!self->device)
            return AVERROR(ENODEV);
    }

    err = alloc_input_frames(self->device, image, coded_w, coded_h, &in_frames);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan avfilter input frames init failed: %d\n", err);
        return err;
    }

    graph = avfilter_graph_alloc();
    if (!graph) {
        av_buffer_unref(&in_frames);
        return AVERROR(ENOMEM);
    }
    if (threads >= 0)
        av_opt_set_int(graph, "threads", threads, 0);

    src = avfilter_graph_alloc_filter(graph, avfilter_get_by_name("buffer"), "in");
    if (!src) {
        err = AVERROR_FILTER_NOT_FOUND;
        goto fail;
    }
    par = av_buffersrc_parameters_alloc();
    if (!par) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
    par->format = AV_PIX_FMT_VULKAN;
    par->width = image->width;
    par->height = image->height;
    par->time_base = (AVRational){tb_num, tb_den};
    par->frame_rate = (AVRational){fps_num, fps_den};
    par->sample_aspect_ratio = (AVRational){sar_num, sar_den};
    par->hw_frames_ctx = in_frames;
    par->color_space = (enum AVColorSpace) colorspace;
    par->color_range = full ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
    err = av_buffersrc_parameters_set(src, par);
    av_free(par);
    par = NULL;
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan avfilter buffer parameters failed: %d\n", err);
        goto fail;
    }
    err = avfilter_init_dict(src, NULL);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan avfilter buffer source failed: %d\n", err);
        goto fail;
    }

    user = avfilter_graph_alloc_filter(graph, filter, filter->name);
    if (!user) {
        err = AVERROR_FILTER_NOT_FOUND;
        goto fail;
    }
    user->hw_device_ctx = av_buffer_ref(self->device);
    if (!user->hw_device_ctx) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
    user->extra_hw_frames = 4;
    if (!init_filter) {
        err = AVERROR(EINVAL);
        goto fail;
    }
    err = init_filter(user, opaque);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan avfilter %s init failed: %d\n", filter->name, err);
        goto fail;
    }

    sink = avfilter_graph_alloc_filter(graph, avfilter_get_by_name("buffersink"), "out");
    if (!sink) {
        err = AVERROR_FILTER_NOT_FOUND;
        goto fail;
    }
#if LIBAVFILTER_VERSION_INT >= ((10 << 16) + (6 << 8) + 100)
    err = av_opt_set_array(sink,
                           "pixel_formats",
                           AV_OPT_SEARCH_CHILDREN,
                           0,
                           1,
                           AV_OPT_TYPE_PIXEL_FMT,
                           pix_fmts);
#else
    err = av_opt_set_int_list(sink, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
#endif
    if (err < 0)
        goto fail;
    err = avfilter_init_dict(sink, NULL);
    if (err < 0)
        goto fail;

    err = avfilter_link(src, 0, user, 0);
    if (err < 0)
        goto fail;
    err = avfilter_link(user, 0, sink, 0);
    if (err < 0)
        goto fail;
    err = avfilter_graph_config(graph, NULL);
    if (err < 0) {
        mlt_log_error(NULL, "Vulkan avfilter graph failed: %d\n", err);
        goto fail;
    }

    avfilter_graph_free(&self->graph);
    av_buffer_unref(&self->in_frames);
    self->graph = graph;
    self->src = src;
    self->filter = user;
    self->sink = sink;
    self->in_frames = in_frames;
    self->width = image->width;
    self->height = image->height;
    self->coded_w = coded_w;
    self->coded_h = coded_h;
    self->sw = vulkan_sw_format(image);
    self->full = full;
    self->colorspace = colorspace;
    self->sar_num = sar_num;
    self->sar_den = sar_den;
    self->tb_num = tb_num;
    self->tb_den = tb_den;
    self->fps_num = fps_num;
    self->fps_den = fps_den;
    if (!self->logged) {
        self->logged = 1;
        mlt_log_info(NULL, "Vulkan avfilter %s %dx%d\n", filter->name, image->width, image->height);
    }
    return 0;

fail:
    avfilter_graph_free(&graph);
    av_buffer_unref(&in_frames);
    return err;
}

int avformat_vulkan_avfilter_configure(avformat_vulkan_avfilter *self,
                                       const AVFilter *filter,
                                       mlt_vulkan_image *image,
                                       int sar_num,
                                       int sar_den,
                                       int tb_num,
                                       int tb_den,
                                       int fps_num,
                                       int fps_den,
                                       int threads,
                                       int force,
                                       avformat_vulkan_avfilter_init_fn init_filter,
                                       void *opaque)
{
    enum AVPixelFormat sw;
    int coded_w = 0;
    int coded_h = 0;
    int full;
    int colorspace;

    if (!self || !filter || !image || image->nb_planes < 1 || !image->planes[0].image)
        return AVERROR(EINVAL);
    sw = vulkan_sw_format(image);
    if (sw == AV_PIX_FMT_NONE) {
        mlt_log_error(NULL,
                      "Vulkan image VkFormat %d has no software format\n",
                      image->planes[0].format);
        return AVERROR(EINVAL);
    }
    full = image->full_range || image->rgb;
    colorspace = colorspace_matrix(image, mlt_to_av_colorspace(image->colorspace, image->height));
    vulkan_coded_size(image, &coded_w, &coded_h);
    if (sar_den == 0) {
        sar_num = 1;
        sar_den = 1;
    }
    if (tb_den == 0) {
        tb_num = 1;
        tb_den = 1;
    }
    if (fps_den == 0) {
        fps_num = 0;
        fps_den = 1;
    }
    if (!force && self->graph && self->width == image->width && self->height == image->height
        && self->coded_w == coded_w && self->coded_h == coded_h && self->sw == sw
        && self->full == full && self->colorspace == colorspace && self->sar_num == sar_num
        && self->sar_den == sar_den && self->tb_num == tb_num && self->tb_den == tb_den
        && self->fps_num == fps_num && self->fps_den == fps_den)
        return 0;
    return configure_user_filter(self,
                                 filter,
                                 image,
                                 sar_num,
                                 sar_den,
                                 tb_num,
                                 tb_den,
                                 fps_num,
                                 fps_den,
                                 threads,
                                 init_filter,
                                 opaque);
}

int avformat_vulkan_avfilter_frame(avformat_vulkan_avfilter *self,
                                   mlt_vulkan_image *image,
                                   int64_t pts,
                                   int sar_num,
                                   int sar_den,
                                   int interlaced,
                                   int top_field_first,
                                   AVFrame **out)
{
    AVFrame *in = NULL;
    AVFrame *filtered = NULL;
    vulkan_wrap_owner *owner = NULL;
    int err;

    if (out)
        *out = NULL;
    if (!self || !self->graph || !self->src || !self->sink || !image || !out)
        return AVERROR(EINVAL);
    if (sar_den == 0) {
        sar_num = 1;
        sar_den = 1;
    }

    err = wrap_vulkan_image(image, self->in_frames, self->colorspace, self->full, &in, &owner);
    if (err < 0)
        return err;
    in->pts = pts;
    in->sample_aspect_ratio = (AVRational){sar_num, sar_den};
#if LIBAVUTIL_VERSION_INT >= ((58 << 16) + (7 << 8) + 100)
    if (interlaced)
        in->flags |= AV_FRAME_FLAG_INTERLACED;
    else
        in->flags &= ~AV_FRAME_FLAG_INTERLACED;
    if (top_field_first)
        in->flags |= AV_FRAME_FLAG_TOP_FIELD_FIRST;
    else
        in->flags &= ~AV_FRAME_FLAG_TOP_FIELD_FIRST;
#else
    in->interlaced_frame = interlaced;
    in->top_field_first = top_field_first;
#endif
    err = av_buffersrc_add_frame_flags(self->src, in, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (err >= 0) {
        filtered = av_frame_alloc();
        if (!filtered)
            err = AVERROR(ENOMEM);
        else
            err = av_buffersink_get_frame(self->sink, filtered);
    }
    release_input_frame(in, owner, image);
    if (err < 0) {
        mlt_log_error(NULL,
                      "Vulkan avfilter %s failed: %d\n",
                      self->filter ? self->filter->name : "filter",
                      err);
        av_frame_free(&filtered);
        return err;
    }
    *out = filtered;
    return 0;
}
#endif

int avformat_avfilter_is_vulkan(const char *name)
{
#if !HAVE_FFMPEG_VULKAN
    (void) name;
    return 0;
#else
    size_t len;

    if (!name)
        return 0;
    len = strlen(name);
    return len > 7 && strcmp(name + len - 7, "_vulkan") == 0;
#endif
}

/* Copy the decoded VkImages into an mlt_image_vulkan. The AVFrame stays alive
   so the pool images and timeline semaphores do. This does not use
   allocate_buffer(): the payload is not a pixel buffer. */
int avformat_install_vulkan_frame(mlt_service service,
                                  struct AVFrame *video_frame,
                                  mlt_frame frame,
                                  uint8_t **buffer,
                                  mlt_image_format *format)
{
#if !HAVE_FFMPEG_VULKAN
    (void) service;
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

    if (mlt_frame_set_image(frame, (uint8_t *) image, 0, destroy_vulkan_image)) {
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
        mlt_log_info(service,
                     "Vulkan texture %dx%d (%d planes)\n",
                     image->width,
                     image->height,
                     image->nb_planes);
    }
    return 0;
#endif
}

int avformat_export_vulkan_frame(mlt_producer producer,
                                 struct AVFrame *video_frame,
                                 mlt_frame frame,
                                 uint8_t **buffer,
                                 mlt_image_format *format)
{
    return avformat_install_vulkan_frame(producer ? MLT_PRODUCER_SERVICE(producer) : NULL,
                                         video_frame,
                                         frame,
                                         buffer,
                                         format);
}

#if !HAVE_FFMPEG_VULKAN
struct AVBufferRef *avformat_vulkan_adopt_placebo(int need_encode)
{
    (void) need_encode;
    return NULL;
}

avformat_vulkan_encoder *avformat_vulkan_encoder_new(void)
{
    return NULL;
}

void avformat_vulkan_encoder_close(avformat_vulkan_encoder *enc)
{
    (void) enc;
}

int avformat_vulkan_encoder_frame(avformat_vulkan_encoder *enc,
                                  struct AVBufferRef *device,
                                  int dst_w,
                                  int dst_h,
                                  int dst_sw_fmt,
                                  int dst_full_range,
                                  int dst_colorspace,
                                  mlt_vulkan_image *image,
                                  struct AVFrame **out)
{
    (void) enc;
    (void) device;
    (void) dst_w;
    (void) dst_h;
    (void) dst_sw_fmt;
    (void) dst_full_range;
    (void) dst_colorspace;
    (void) image;
    if (out)
        *out = NULL;
    return -1;
}

avformat_vulkan_avfilter *avformat_vulkan_avfilter_new(void)
{
    return NULL;
}

void avformat_vulkan_avfilter_close(avformat_vulkan_avfilter *self)
{
    (void) self;
}

int avformat_vulkan_avfilter_configure(avformat_vulkan_avfilter *self,
                                       const struct AVFilter *filter,
                                       mlt_vulkan_image *image,
                                       int sar_num,
                                       int sar_den,
                                       int tb_num,
                                       int tb_den,
                                       int fps_num,
                                       int fps_den,
                                       int threads,
                                       int force,
                                       avformat_vulkan_avfilter_init_fn init_filter,
                                       void *opaque)
{
    (void) self;
    (void) filter;
    (void) image;
    (void) sar_num;
    (void) sar_den;
    (void) tb_num;
    (void) tb_den;
    (void) fps_num;
    (void) fps_den;
    (void) threads;
    (void) force;
    (void) init_filter;
    (void) opaque;
    return -1;
}

struct AVFilterGraph *avformat_vulkan_avfilter_graph(avformat_vulkan_avfilter *self)
{
    (void) self;
    return NULL;
}

struct AVFilterContext *avformat_vulkan_avfilter_ctx(avformat_vulkan_avfilter *self)
{
    (void) self;
    return NULL;
}

int avformat_vulkan_avfilter_frame(avformat_vulkan_avfilter *self,
                                   mlt_vulkan_image *image,
                                   int64_t pts,
                                   int sar_num,
                                   int sar_den,
                                   int interlaced,
                                   int top_field_first,
                                   struct AVFrame **out)
{
    (void) self;
    (void) image;
    (void) pts;
    (void) sar_num;
    (void) sar_den;
    (void) interlaced;
    (void) top_field_first;
    if (out)
        *out = NULL;
    return -1;
}
#endif
