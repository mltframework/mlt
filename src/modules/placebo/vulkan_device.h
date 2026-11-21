/*
 * vulkan_device.h -- Vulkan device extensions and device selection for the placebo GPU
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

#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <libplacebo/config.h>

#ifdef PL_HAVE_VULKAN

#include <libplacebo/vulkan.h>

#define PLACEBO_MAX_DEVICE_EXTS 24

/* The optional device extensions for pl_vulkan_params. A name that does not
   fit sets overflow instead of being dropped, so the caller can fail. */
struct placebo_ext_list
{
    const char *names[PLACEBO_MAX_DEVICE_EXTS];
    int count;
    int overflow;
};

void placebo_ext_add(struct placebo_ext_list *list, const char *name);

/* The external memory and semaphore extensions that let another VkDevice
   import the images and semaphores of the display export. */
void placebo_ext_add_external(struct placebo_ext_list *list);

/* The physical device named by MLT_VULKAN_VENDOR_ID and MLT_VULKAN_DEVICE_ID,
   or VK_NULL_HANDLE to leave the choice to libplacebo. */
VkPhysicalDevice placebo_vulkan_device_from_env(pl_vk_inst instance);

#endif /* PL_HAVE_VULKAN */

#endif /* VULKAN_DEVICE_H */
