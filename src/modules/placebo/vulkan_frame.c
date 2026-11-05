/*
 * vulkan_frame.c -- map an mlt_image_vulkan texture onto a libplacebo frame
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

#include "vulkan_frame.h"

#include "color_space.h"
#include "gpu_context.h"

#include <framework/mlt_image.h>
#include <framework/mlt_log.h>

#include <libplacebo/vulkan.h>

#include <stdlib.h>
#include <string.h>

/* A texture this module created and then held for another Vulkan user.
   The VkImage stays alive until the mlt image is destroyed. */
typedef struct
{
    pl_tex tex;
    VkSemaphore sem;
} vulkan_owner;

typedef struct
{
    mlt_vulkan_image *image;
    pl_tex textures[MLT_VULKAN_MAX_PLANES];
    pl_tex planar;
    int acquired;
    int owned;
} map_state;

static int real_plane_count(const mlt_vulkan_image *image)
{
    int count = 0;
    int i;

    for (i = 0; i < image->nb_planes; i++) {
        if (image->planes[i].image)
            count++;
    }
    return count;
}

static enum pl_color_system system_from_image(const mlt_vulkan_image *image)
{
    if (image->rgb)
        return PL_COLOR_SYSTEM_RGB;
    switch ((mlt_colorspace) image->colorspace) {
    case mlt_colorspace_bt709:
        return PL_COLOR_SYSTEM_BT_709;
    case mlt_colorspace_smpte170m:
    case mlt_colorspace_bt470bg:
    case mlt_colorspace_bt601:
    case mlt_colorspace_fcc:
        return PL_COLOR_SYSTEM_BT_601;
    case mlt_colorspace_smpte240m:
        return PL_COLOR_SYSTEM_SMPTE_240M;
    case mlt_colorspace_bt2020_ncl:
        return PL_COLOR_SYSTEM_BT_2020_NC;
    case mlt_colorspace_bt2020_cl:
        return PL_COLOR_SYSTEM_BT_2020_C;
    case mlt_colorspace_ycgco:
        return PL_COLOR_SYSTEM_YCGCO;
    default:
        return pl_color_system_guess_ycbcr(image->width, image->height);
    }
}

static enum pl_chroma_location chroma_from_image(int location)
{
    switch (location) {
    case 1:
        return PL_CHROMA_LEFT;
    case 2:
        return PL_CHROMA_CENTER;
    case 3:
        return PL_CHROMA_TOP_LEFT;
    case 4:
        return PL_CHROMA_TOP_CENTER;
    case 5:
        return PL_CHROMA_BOTTOM_LEFT;
    case 6:
        return PL_CHROMA_BOTTOM_CENTER;
    default:
        return PL_CHROMA_UNKNOWN;
    }
}

static void fill_plane_meta(struct pl_plane *dst, const mlt_vulkan_plane *src)
{
    int c;

    dst->components = src->components;
    for (c = 0; c < 4; c++)
        dst->component_mapping[c] = src->component_mapping[c];
}

static bool acquire_vulkan(pl_gpu gpu, struct pl_frame *frame)
{
    map_state *state = frame->user_data;
    mlt_vulkan_image *image = state->image;
    int n;

    if (image->lock)
        image->lock(image->sync_opaque);
    for (n = 0; n < frame->num_planes; n++) {
        pl_tex tex = state->planar ? state->planar : state->textures[n];
        mlt_vulkan_plane *plane = &image->planes[state->planar ? 0 : n];

        if (!tex)
            continue;
        pl_vulkan_release_ex(gpu,
                             pl_vulkan_release_params(.tex = tex,
                                                      .layout = (VkImageLayout) plane->layout,
                                                      .qf = VK_QUEUE_FAMILY_IGNORED,
                                                      .semaphore = {
                                                          .sem = (VkSemaphore) (uintptr_t)
                                                                     plane->semaphore,
                                                          .value = plane->sem_value,
                                                      }, ));
        if (state->planar)
            break;
    }
    state->acquired = 1;
    return true;
}

static void release_vulkan(pl_gpu gpu, struct pl_frame *frame)
{
    map_state *state = frame->user_data;
    mlt_vulkan_image *image = state->image;
    int n;

    if (!state->acquired)
        return;
    for (n = 0; n < frame->num_planes; n++) {
        pl_tex tex = state->planar ? state->planar : state->textures[n];
        mlt_vulkan_plane *plane = &image->planes[state->planar ? 0 : n];
        VkImageLayout layout = (VkImageLayout) plane->layout;
        int ok;

        if (!tex)
            continue;
        ok = pl_vulkan_hold_ex(gpu,
                               pl_vulkan_hold_params(
                                       .tex = tex,
                                       .out_layout = &layout,
                                       .qf = VK_QUEUE_FAMILY_IGNORED,
                                       .semaphore = {
                                           .sem = (VkSemaphore) (uintptr_t) plane->semaphore,
                                           .value = plane->sem_value + (plane->semaphore ? 1 : 0),
                                       }, ));
        plane->layout = (int) layout;
        if (ok && plane->semaphore)
            plane->sem_value += 1;
        if (state->planar)
            break;
    }
    state->acquired = 0;
    if (image->unlock)
        image->unlock(image->sync_opaque);
}

static void destroy_wrapped(pl_gpu gpu, map_state *state)
{
    int n;

    if (state->planar)
        pl_tex_destroy(gpu, &state->planar);
    if (state->owned)
        return;
    for (n = 0; n < MLT_VULKAN_MAX_PLANES; n++) {
        if (state->textures[n])
            pl_tex_destroy(gpu, &state->textures[n]);
    }
}

int placebo_map_vulkan_image(pl_gpu gpu, const uint8_t *bytes, struct pl_frame *out)
{
    mlt_vulkan_image *image = (mlt_vulkan_image *) bytes;
    vulkan_owner *owner;
    map_state *state;
    enum pl_color_transfer transfer;
    int i;
    static int logged;

    if (!gpu || !image || !out || image->nb_planes < 1 || !pl_vulkan_get(gpu))
        return 0;
    state = calloc(1, sizeof(*state));
    if (!state)
        return 0;
    state->image = image;
    owner = image->module_data;

    memset(out, 0, sizeof(*out));
    out->num_planes = image->nb_planes;
    if (image->crop_left || image->crop_top || image->crop_right || image->crop_bottom) {
        out->crop.x0 = image->crop_left;
        out->crop.y0 = image->crop_top;
        out->crop.x1 = image->width - image->crop_right;
        out->crop.y1 = image->height - image->crop_bottom;
    }
    transfer = placebo_transfer_from_mlt((mlt_color_trc) image->color_trc);
    out->color = placebo_color_space((mlt_color_primaries) image->color_primaries,
                                     (mlt_colorspace) image->colorspace,
                                     image->height,
                                     transfer);
    out->repr.sys = system_from_image(image);
    out->repr.levels = (image->rgb || image->full_range) ? PL_COLOR_LEVELS_FULL
                                                         : PL_COLOR_LEVELS_LIMITED;
    out->repr.alpha = image->alpha ? PL_ALPHA_INDEPENDENT : PL_ALPHA_NONE;
    out->repr.bits.color_depth = image->bit_depth;
    out->repr.bits.bit_shift = image->bit_shift;
    for (i = 0; i < image->nb_planes; i++)
        fill_plane_meta(&out->planes[i], &image->planes[i]);
    if (image->log2_chroma_w || image->log2_chroma_h)
        pl_frame_set_chroma_location(out, chroma_from_image(image->chroma_location));

    if (owner && owner->tex) {
        state->textures[0] = owner->tex;
        state->owned = 1;
        out->num_planes = 1;
        out->planes[0].texture = owner->tex;
    } else if (real_plane_count(image) == 1 && image->nb_planes > 1) {
        pl_tex tex
            = pl_vulkan_wrap(gpu,
                             pl_vulkan_wrap_params(.image
                                                   = (VkImage) (uintptr_t) image->planes[0].image,
                                                   .width = image->planes[0].width,
                                                   .height = image->planes[0].height,
                                                   .format = (VkFormat) image->planes[0].format,
                                                   .usage = image->usage, ));
        int subplanes = tex && tex->params.format ? tex->params.format->num_planes : 0;

        if (!tex || subplanes != image->nb_planes) {
            if (tex)
                pl_tex_destroy(gpu, &tex);
            free(state);
            return 0;
        }
        state->planar = tex;
        for (i = 0; i < subplanes; i++)
            out->planes[i].texture = tex->planes[i];
    } else {
        for (i = 0; i < image->nb_planes; i++) {
            mlt_vulkan_plane *plane = &image->planes[i];

            if (!plane->image)
                continue;
            state->textures[i]
                = pl_vulkan_wrap(gpu,
                                 pl_vulkan_wrap_params(.image = (VkImage) (uintptr_t) plane->image,
                                                       .width = plane->width,
                                                       .height = plane->height,
                                                       .format = (VkFormat) plane->format,
                                                       .usage = image->usage, ));
            if (!state->textures[i]) {
                destroy_wrapped(gpu, state);
                free(state);
                return 0;
            }
            out->planes[i].texture = state->textures[i];
        }
    }

    out->user_data = state;
    out->acquire = acquire_vulkan;
    out->release = release_vulkan;
    if (!logged) {
        logged = 1;
        mlt_log_info(NULL,
                     "[placebo] Mapping Vulkan texture %dx%d (%d planes, %s)\n",
                     image->width,
                     image->height,
                     image->nb_planes,
                     image->rgb ? "rgb" : "yuv");
    }
    return 1;
}

void placebo_unmap_vulkan_image(pl_gpu gpu, struct pl_frame *frame)
{
    map_state *state;

    if (!gpu || !frame)
        return;
    state = frame->user_data;
    if (!state)
        return;
    if (state->acquired && frame->release)
        frame->release(gpu, frame);
    destroy_wrapped(gpu, state);
    free(state);
    frame->user_data = NULL;
    frame->acquire = NULL;
    frame->release = NULL;
}

static void destroy_placebo_vulkan(void *p)
{
    mlt_vulkan_image *image = p;
    vulkan_owner *owner;
    pl_gpu gpu;

    if (!image)
        return;
    owner = image->module_data;
    gpu = placebo_gpu_get();
    if (gpu && owner && owner->tex) {
        placebo_render_lock();
        pl_vulkan_release_ex(gpu,
                             pl_vulkan_release_params(.tex = owner->tex,
                                                      .layout
                                                      = (VkImageLayout) image->planes[0].layout,
                                                      .qf = VK_QUEUE_FAMILY_IGNORED,
                                                      .semaphore = {
                                                          .sem = owner->sem,
                                                          .value = image->planes[0].sem_value,
                                                      }, ));
        pl_tex_destroy(gpu, &owner->tex);
        if (owner->sem)
            pl_vulkan_sem_destroy(gpu, &owner->sem);
        placebo_render_unlock();
    }
    free(owner);
    free(image);
}

static int publish_vulkan(
    mlt_frame frame, pl_tex tex, uint8_t **image, mlt_image_format *format, mlt_color_trc color_trc)
{
    pl_gpu gpu = placebo_gpu_get();
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    mlt_vulkan_image *vulkan;
    vulkan_owner *owner;
    VkSemaphore sem = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkFormat vk_format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usage = 0;
    VkImage vk_image;
    mlt_color_primaries primaries;
    mlt_colorspace colorspace;
    static int logged;

    vulkan = calloc(1, sizeof(*vulkan));
    owner = calloc(1, sizeof(*owner));
    if (!vulkan || !owner || !gpu) {
        free(vulkan);
        free(owner);
        placebo_render_lock();
        pl_tex_destroy(gpu, &tex);
        placebo_render_unlock();
        *image = NULL;
        return 1;
    }

    placebo_render_lock();
    sem = pl_vulkan_sem_create(gpu,
                               pl_vulkan_sem_params(.type = VK_SEMAPHORE_TYPE_TIMELINE,
                                                    .initial_value = 0, ));
    if (!sem
        || !pl_vulkan_hold_ex(gpu,
                              pl_vulkan_hold_params(.tex = tex,
                                                    .out_layout = &layout,
                                                    .qf = VK_QUEUE_FAMILY_IGNORED,
                                                    .semaphore = {
                                                        .sem = sem,
                                                        .value = 1,
                                                    }, ))) {
        if (sem)
            pl_vulkan_sem_destroy(gpu, &sem);
        pl_tex_destroy(gpu, &tex);
        placebo_render_unlock();
        free(vulkan);
        free(owner);
        *image = NULL;
        return 1;
    }
    vk_image = pl_vulkan_unwrap(gpu, tex, &vk_format, &usage);
    placebo_render_unlock();

    primaries = mlt_image_color_pri_id(mlt_properties_get(props, "color_primaries"));
    colorspace = mlt_image_colorspace_id(mlt_properties_get(props, "colorspace"));
    owner->tex = tex;
    owner->sem = sem;
    vulkan->width = tex->params.w;
    vulkan->height = tex->params.h;
    vulkan->nb_planes = 1;
    vulkan->usage = (uint32_t) usage;
    vulkan->rgb = 1;
    vulkan->full_range = 1;
    vulkan->alpha = 1;
    vulkan->bit_depth = tex->params.format ? tex->params.format->component_depth[0] : 8;
    vulkan->colorspace = colorspace != mlt_colorspace_invalid ? (int) colorspace
                                                              : (int) mlt_colorspace_rgb;
    vulkan->color_primaries = (int) primaries;
    vulkan->planes[0].image = (uint64_t) (uintptr_t) vk_image;
    vulkan->planes[0].format = (int) vk_format;
    vulkan->planes[0].width = vulkan->width;
    vulkan->planes[0].height = vulkan->height;
    vulkan->planes[0].layout = (int) layout;
    vulkan->planes[0].semaphore = (uint64_t) (uintptr_t) sem;
    vulkan->planes[0].sem_value = 1;
    vulkan->planes[0].components = 4;
    vulkan->planes[0].component_mapping[0] = 0;
    vulkan->planes[0].component_mapping[1] = 1;
    vulkan->planes[0].component_mapping[2] = 2;
    vulkan->planes[0].component_mapping[3] = 3;
    vulkan->module_data = owner;

    *format = mlt_image_vulkan;
    *image = (uint8_t *) vulkan;
    mlt_properties_set_int(props, "format", mlt_image_vulkan);
    mlt_properties_set(props, MLT_PLACEBO_IMAGE_PRIVATE, NULL);
    if (!logged) {
        logged = 1;
        mlt_log_info(NULL, "[placebo] Vulkan texture %dx%d (rgba)\n", vulkan->width, vulkan->height);
    }
    if (mlt_frame_set_image(frame, (uint8_t *) vulkan, 0, destroy_placebo_vulkan)) {
        destroy_placebo_vulkan(vulkan);
        *image = NULL;
        return 1;
    }
    vulkan->color_trc = (int) color_trc;
    mlt_properties_set_int(props, "color_trc", color_trc);
    return 0;
}

int placebo_frame_publish_tex(mlt_frame frame,
                              pl_tex tex,
                              mlt_image_format requested,
                              uint8_t **image,
                              mlt_image_format *format,
                              mlt_color_trc color_trc)
{
    pl_gpu gpu = placebo_gpu_get();

    if (!tex || !image || !format)
        return 1;
    if (requested == mlt_image_vulkan && gpu && pl_vulkan_get(gpu))
        return publish_vulkan(frame, tex, image, format, color_trc);

    *format = mlt_image_private;
    *image = (uint8_t *) tex;
    if (placebo_frame_set_tex(frame, tex)) {
        placebo_render_lock();
        pl_tex_destroy(gpu, &tex);
        placebo_render_unlock();
        *image = NULL;
        return 1;
    }
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "color_trc", color_trc);
    return 0;
}
