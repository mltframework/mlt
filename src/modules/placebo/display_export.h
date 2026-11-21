/*
 * display_export.h -- render an mlt_image_vulkan into an exported image
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

#ifndef DISPLAY_EXPORT_H
#define DISPLAY_EXPORT_H

#include <framework/mlt_vulkan_export.h>

/* The calls an importer on another VkDevice makes to take pictures from this
   module. NULL when the build has no Vulkan. Rendering does not download pixels
   and does not wait on the CPU, and it is safe to call when no side is
   attached: nothing is signaled. */
const mlt_vulkan_export_api *placebo_display_export_api(void);

/* Drop exported images and semaphores. The Vulkan device must still be alive.
   Does not take the GPU locks; the caller already serializes with rendering. */
void placebo_display_export_close(void);

#endif /* DISPLAY_EXPORT_H */
