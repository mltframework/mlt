/*
 * vulkan_device.c -- Vulkan device extensions and device selection for the placebo GPU
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

#include "vulkan_device.h"

#ifdef PL_HAVE_VULKAN

#include <framework/mlt_log.h>

#include <stdlib.h>

void placebo_ext_add(struct placebo_ext_list *list, const char *name)
{
    if (list->count < PLACEBO_MAX_DEVICE_EXTS)
        list->names[list->count++] = name;
    else
        list->overflow = 1;
}

void placebo_ext_add_external(struct placebo_ext_list *list)
{
    placebo_ext_add(list, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    placebo_ext_add(list, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
#ifdef _WIN32
    placebo_ext_add(list, "VK_KHR_external_memory_win32");
    placebo_ext_add(list, "VK_KHR_external_semaphore_win32");
#else
    placebo_ext_add(list, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    placebo_ext_add(list, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
#endif
}

/* An unset, empty or malformed variable is not a request. */
static int parse_id(const char *name, unsigned long *id)
{
    const char *text = getenv(name);
    char *end = NULL;

    if (!text || !text[0])
        return 0;
    *id = strtoul(text, &end, 0);
    return !*end;
}

/* The physical devices of an existing instance, or NULL. The caller frees. */
static VkPhysicalDevice *enumerate_devices(pl_vk_inst instance, uint32_t *count)
{
    PFN_vkEnumeratePhysicalDevices enumerate
        = (PFN_vkEnumeratePhysicalDevices) instance->get_proc_addr(instance->instance,
                                                                   "vkEnumeratePhysicalDevices");
    VkPhysicalDevice *list;

    *count = 0;
    if (!enumerate || enumerate(instance->instance, count, NULL) != VK_SUCCESS || !*count)
        return NULL;
    list = calloc(*count, sizeof(*list));
    if (list && enumerate(instance->instance, count, list) != VK_SUCCESS) {
        free(list);
        return NULL;
    }
    return list;
}

VkPhysicalDevice placebo_vulkan_device_from_env(pl_vk_inst instance)
{
    static int logged;
    unsigned long vendor;
    unsigned long device_id;
    PFN_vkGetPhysicalDeviceProperties get_props;
    VkPhysicalDevice *list;
    VkPhysicalDevice found = VK_NULL_HANDLE;
    uint32_t count;
    uint32_t i;

    if (!instance || !instance->get_proc_addr || !parse_id("MLT_VULKAN_VENDOR_ID", &vendor)
        || !vendor || !parse_id("MLT_VULKAN_DEVICE_ID", &device_id))
        return VK_NULL_HANDLE;
    get_props = (PFN_vkGetPhysicalDeviceProperties)
                    instance->get_proc_addr(instance->instance, "vkGetPhysicalDeviceProperties");
    list = get_props ? enumerate_devices(instance, &count) : NULL;
    for (i = 0; list && i < count && !found; i++) {
        VkPhysicalDeviceProperties props;

        get_props(list[i], &props);
        if (props.vendorID == (uint32_t) vendor && props.deviceID == (uint32_t) device_id) {
            found = list[i];
            if (!logged)
                mlt_log_info(NULL,
                             "[placebo] Using Vulkan device %s (vendor %u device %u)\n",
                             props.deviceName,
                             props.vendorID,
                             props.deviceID);
        }
    }
    free(list);
    if (!found && !logged)
        mlt_log_warning(NULL,
                        "[placebo] No Vulkan device matches vendor %lu device %lu\n",
                        vendor,
                        device_id);
    logged = 1;
    return found;
}

#else

typedef int placebo_vulkan_device_not_built;

#endif /* PL_HAVE_VULKAN */
