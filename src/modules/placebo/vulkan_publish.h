/*
 * vulkan_publish.h -- share the Vulkan device with other modules
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

#ifndef VULKAN_PUBLISH_H
#define VULKAN_PUBLISH_H

#include <libplacebo/vulkan.h>

/* Publish the device as the "placebo.vulkan" global property, which avformat
   and the Vulkan preview read. The entries are in the documentation of
   placebo_vulkan_publish() in gpu_context.h. The property is set only after
   every entry is filled. Returns 1 if it published now, and 0 if it was
   already published or there is nothing to publish. The caller serializes
   with GPU creation and teardown. */
int placebo_vulkan_publish_device(pl_vulkan vk, pl_vk_inst inst);

/* The queue libplacebo created for this family and index. An internally
   synchronized queue is retrieved with vkGetDeviceQueue2 and its creation
   flag; vkGetDeviceQueue returns null for that queue on RADV. */
VkQueue placebo_vulkan_get_queue(pl_vulkan vk, uint32_t family, uint32_t index);

/* Withdraw the "placebo.vulkan" property, which runs its destructor, and forget
   the device's procedure addresses. Call it when the device goes away, before
   it is destroyed, so nothing is handed a handle to a dead device. Does nothing
   if nothing was published. */
void placebo_vulkan_unpublish(void);

#endif /* VULKAN_PUBLISH_H */
