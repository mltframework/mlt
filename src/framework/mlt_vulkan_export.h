/**
 * \file mlt_vulkan_export.h
 * \brief hand a Vulkan image to a window that draws it on its own VkDevice
 *
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

#ifndef MLT_VULKAN_EXPORT_H
#define MLT_VULKAN_EXPORT_H

#include "mlt_image.h"

/** Number of exported images kept in flight. Changing it changes the size of
 * mlt_vulkan_export_desc, so it is an ABI break. */
#define MLT_VULKAN_EXPORT_SLOTS 2

/** Number of independent windows, called sides, that can import each exported
 * image. A side is anything with its own VkDevice queue that draws the image,
 * for example a preview and a separate full-screen window. Changing it changes
 * the size of mlt_vulkan_export_desc, so it is an ABI break. */
#define MLT_VULKAN_EXPORT_SIDES 2

/** What the caller wants rendered into an exported image.
 *
 * size is sizeof this struct, set by the caller, so a later version of this
 * header can add fields. tone_mapping is non-zero to tone map into the display
 * peak. Zero leaves out-of-range samples clipped so an SDR shader can tone map
 * the same image. max_display_nits and max_content_nits are 0 for the default
 * (about 1000 nits for the display, and the source metadata for the content).
 */
typedef struct mlt_vulkan_export_params_s
{
    int size;
    int tone_mapping;
    int max_display_nits;
    int max_content_nits;
} mlt_vulkan_export_params;

/** The binary semaphores one side uses for one exported image.
 *
 * armed says whether render signaled ready for this generation. The side waits
 * ready only when armed, and signals done in the same queue submit that draws
 * the image. The handles are POSIX fds stored in a pointer when handle_is_fd
 * of the desc is set, and Win32 HANDLEs otherwise.
 */
typedef struct mlt_vulkan_export_side_s
{
    int armed;
    void *ready;
    void *done;
} mlt_vulkan_export_side;

/** One exported RGBA half-float image and its binary semaphores.
 *
 * The image is a dedicated OPAQUE_WIN32 allocation on Windows and a dedicated
 * OPAQUE_FD allocation elsewhere. width and height are the visible picture.
 * vk_format is the VkFormat (R16G16B16A16_SFLOAT). usage is the VkImageUsageFlags
 * the image was created with. memory_size is the dedicated allocation in bytes.
 * An importer creates a 2D optimal-tiling image with the same format, size,
 * usage, one mip, one layer, and one sample, and binds memory as a dedicated
 * allocation of memory_size bytes. The export module owns the handles. The
 * importer duplicates a HANDLE or dup()s an fd before importing and does not
 * close the original.
 *
 * generation starts at 0 and increases only when a new render was submitted.
 * An unchanged generation means the picture was not updated. Waiting a
 * semaphore that was not signaled deadlocks the queue. The importer acquires
 * the image from VK_QUEUE_FAMILY_EXTERNAL into its graphics queue (old and new
 * layout both shader-read-only), draws, and signals its side's done semaphore
 * in that same queue submit.
 *
 * size is sizeof this struct, set by the caller before the call, so a later
 * version of this header can add fields. The module fills only the fields that
 * fit in it.
 */
typedef struct mlt_vulkan_export_desc_s
{
    int size;
    int slot;
    int width;
    int height;
    int vk_format;
    int usage;
    int generation;
    int handle_is_fd;
    uint64_t memory_size;
    void *memory;
    mlt_vulkan_export_side side[MLT_VULKAN_EXPORT_SIDES];
} mlt_vulkan_export_desc;

/** The calls an importer makes. The placebo module publishes one in the
 * "export_api" entry of the "placebo.vulkan" global property.
 *
 * The module remembers which sides are attached and which generation of each
 * slot each side has drawn, so a caller reports events instead of keeping that
 * state in step with the module.
 */
typedef struct mlt_vulkan_export_api_s
{
    /** sizeof this struct, so an importer built against an older header can
     * check that the entries it uses exist. */
    int size;

    /** Render image into an exported slot, which the function chooses. Returns
     * 0 on success and fills desc. Does not download pixels and does not wait
     * on the CPU. When no side is attached nothing is rendered or signaled and
     * desc is the last picture, if any. */
    int (*render)(mlt_vulkan_image *image,
                  const mlt_vulkan_export_params *params,
                  mlt_vulkan_export_desc *desc);

    /** The side will wait its ready semaphore and signal its done semaphore
     * for every armed render from now on. */
    void (*attach)(int side);

    /** The side no longer waits. Its pending ready semaphores are consumed by
     * the module. Call it only after every submit that waited a ready
     * semaphore has been queued. Detaching earlier makes the next render wait
     * a ready semaphore the side already consumed. */
    void (*detach)(int side);

    /** The side has queued the submit that waits ready and signals done for
     * this generation of the slot. Call it after that submit is queued, never
     * before: a render that sees the generation waits for a done signal that
     * would still be sitting on the caller's thread. */
    void (*submitted)(int side, int slot, int generation);

    /** The highest generation of the slot the side has reported submitted. A
     * new importer for the same side uses it to skip ready semaphores an
     * earlier one already consumed. */
    int (*seen)(int side, int slot);
} mlt_vulkan_export_api;

#endif /* MLT_VULKAN_EXPORT_H */
