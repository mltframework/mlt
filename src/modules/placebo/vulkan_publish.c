/*
 * vulkan_publish.c -- share the Vulkan device with other modules
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

#include "vulkan_publish.h"

#include <libplacebo/config.h>

#include <framework/mlt_factory.h>
#include <framework/mlt_properties.h>

#ifdef PL_HAVE_VULKAN

#include "display_export.h"

#include <framework/mlt_log.h>

#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(void *) == sizeof(void (*)(void)),
               "Vulkan lock callbacks are passed through mlt_properties as void *");

/* This bit is VK_DEVICE_QUEUE_CREATE_INTERNALLY_SYNCHRONIZED_BIT_KHR. The
   system Vulkan headers this module builds against may not define it. */
#ifndef VK_DEVICE_QUEUE_CREATE_INTERNALLY_SYNCHRONIZED_BIT_KHR
#define VK_DEVICE_QUEUE_CREATE_INTERNALLY_SYNCHRONIZED_BIT_KHR 0x00000004u
#endif
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INTERNALLY_SYNCHRONIZED_QUEUES_FEATURES_KHR
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INTERNALLY_SYNCHRONIZED_QUEUES_FEATURES_KHR 1000504000
#endif

/* libplacebo enables VK_KHR_internally_synchronized_queues when the device
   supports it. FFmpeg then calls vkGetDeviceQueue2 with flags 0 unless it was
   itself built with that extension, and RADV returns a null queue. Submitting
   that queue crashes inside the driver. Publish a get_proc_addr that retrieves
   the queue through placebo_vulkan_get_queue(). */
static PFN_vkGetInstanceProcAddr s_real_get_proc;
static PFN_vkGetDeviceProcAddr s_real_get_device_proc;
static PFN_vkGetDeviceQueue2 s_real_get_device_queue2;
static pl_vulkan s_vk;

static int vulkan_queues_are_internally_synchronized(pl_vulkan vk)
{
    const VkBaseInStructure *in;

    if (!vk || !vk->features)
        return 0;
    for (in = (const VkBaseInStructure *) vk->features; in; in = in->pNext) {
        if (in->sType
            == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INTERNALLY_SYNCHRONIZED_QUEUES_FEATURES_KHR) {
            const VkBool32 *enabled = (const VkBool32 *) (in + 1);
            return *enabled == VK_TRUE;
        }
    }
    return 0;
}

VkQueue placebo_vulkan_get_queue(pl_vulkan vk, uint32_t family, uint32_t index)
{
    PFN_vkGetDeviceQueue2 get_queue2;
    PFN_vkGetDeviceQueue get_queue;
    VkQueue queue = VK_NULL_HANDLE;

    if (!vk || !vk->device || !vk->get_proc_addr || !vk->instance)
        return VK_NULL_HANDLE;
    get_queue2 = (PFN_vkGetDeviceQueue2) vk->get_proc_addr(vk->instance, "vkGetDeviceQueue2");
    get_queue = (PFN_vkGetDeviceQueue) vk->get_proc_addr(vk->instance, "vkGetDeviceQueue");
    if (vulkan_queues_are_internally_synchronized(vk) && get_queue2) {
        VkDeviceQueueInfo2 info;

        memset(&info, 0, sizeof(info));
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        info.flags = VK_DEVICE_QUEUE_CREATE_INTERNALLY_SYNCHRONIZED_BIT_KHR;
        info.queueFamilyIndex = family;
        info.queueIndex = index;
        get_queue2(vk->device, &info, &queue);
        if (queue)
            return queue;
    }
    if (get_queue)
        get_queue(vk->device, family, index, &queue);
    return queue;
}

static void VKAPI_CALL wrapped_get_device_queue2(VkDevice device,
                                                 const VkDeviceQueueInfo2 *info,
                                                 VkQueue *queue)
{
    if (!queue)
        return;
    *queue = VK_NULL_HANDLE;
    if (!info)
        return;
    if (s_vk && device == s_vk->device) {
        *queue = placebo_vulkan_get_queue(s_vk, info->queueFamilyIndex, info->queueIndex);
        return;
    }
    if (s_real_get_device_queue2)
        s_real_get_device_queue2(device, info, queue);
}

static PFN_vkVoidFunction VKAPI_CALL wrapped_get_device_proc(VkDevice device, const char *name)
{
    if (name && !strcmp(name, "vkGetDeviceQueue2"))
        return (PFN_vkVoidFunction) wrapped_get_device_queue2;
    if (!s_real_get_device_proc)
        return NULL;
    return s_real_get_device_proc(device, name);
}

static PFN_vkVoidFunction VKAPI_CALL wrapped_get_instance_proc(VkInstance instance, const char *name)
{
    if (name && !strcmp(name, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) wrapped_get_device_proc;
    if (name && !strcmp(name, "vkGetDeviceQueue2"))
        return (PFN_vkVoidFunction) wrapped_get_device_queue2;
    if (!s_real_get_proc)
        return NULL;
    return s_real_get_proc(instance, name);
}

static void publish_fn(mlt_properties props, const char *name, const void *fn)
{
    void *ptr = NULL;
    if (!fn)
        return;
    memcpy(&ptr, fn, sizeof(ptr));
    if (ptr)
        mlt_properties_set_data(props, name, ptr, 0, NULL, NULL);
}

static void publish_get_proc_addr(mlt_properties props, pl_vulkan vk, pl_vk_inst inst)
{
    PFN_vkGetInstanceProcAddr published = vk->get_proc_addr;

    s_vk = vk;
    s_real_get_proc = vk->get_proc_addr;
    s_real_get_device_proc = NULL;
    s_real_get_device_queue2 = NULL;
    if (s_real_get_proc && vulkan_queues_are_internally_synchronized(vk)) {
        s_real_get_device_proc = (PFN_vkGetDeviceProcAddr) s_real_get_proc(inst->instance,
                                                                           "vkGetDeviceProcAddr");
        if (s_real_get_device_proc)
            s_real_get_device_queue2
                = (PFN_vkGetDeviceQueue2) s_real_get_device_proc(vk->device, "vkGetDeviceQueue2");
        if (s_real_get_device_queue2)
            published = wrapped_get_instance_proc;
    }
    publish_fn(props, "get_proc_addr", &published);
}

/* flags is a queue capability bit such as VK_QUEUE_VIDEO_DECODE_BIT_KHR.
   index is -1 when no family has that bit. */
static void find_queue_with_flags(pl_vulkan vk, VkQueueFlags flags, int *index, int *count)
{
    *index = -1;
    *count = 0;
    if (!flags || !vk || !vk->get_proc_addr || !vk->instance || !vk->phys_device)
        return;

    PFN_vkGetPhysicalDeviceQueueFamilyProperties get_qf
        = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)
              vk->get_proc_addr(vk->instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    if (!get_qf)
        return;

    uint32_t num = 0;
    get_qf(vk->phys_device, &num, NULL);
    if (!num)
        return;

    VkQueueFamilyProperties *families = calloc(num, sizeof(*families));
    if (!families)
        return;
    get_qf(vk->phys_device, &num, families);
    for (uint32_t i = 0; i < num; i++) {
        if (families[i].queueFlags & flags) {
            *index = (int) i;
            *count = (int) families[i].queueCount;
            break;
        }
    }
    free(families);
}

static void forget_procs(void)
{
    s_real_get_proc = NULL;
    s_real_get_device_proc = NULL;
    s_real_get_device_queue2 = NULL;
    s_vk = NULL;
}

int placebo_vulkan_publish_device(pl_vulkan vk, pl_vk_inst inst)
{
    if (!vk || !inst)
        return 0;

    mlt_properties global = mlt_global_properties();
    if (!global || mlt_properties_get_data(global, "placebo.vulkan", NULL))
        return 0;

    int decode_index = -1;
    int decode_count = 0;
    int encode_index = -1;
    int encode_count = 0;
#ifdef VK_KHR_video_decode_queue
    find_queue_with_flags(vk, VK_QUEUE_VIDEO_DECODE_BIT_KHR, &decode_index, &decode_count);
#endif
#ifdef VK_KHR_video_encode_queue
    find_queue_with_flags(vk, VK_QUEUE_VIDEO_ENCODE_BIT_KHR, &encode_index, &encode_count);
#endif

    mlt_properties props = mlt_properties_new();
    if (!props)
        return 0;

    mlt_properties_set_data(props, "instance", (void *) inst->instance, 0, NULL, NULL);
    mlt_properties_set_data(props, "phys_dev", (void *) vk->phys_device, 0, NULL, NULL);
    mlt_properties_set_data(props, "device", (void *) vk->device, 0, NULL, NULL);
    publish_get_proc_addr(props, vk, inst);
    mlt_properties_set_data(props, "features", (void *) vk->features, 0, NULL, NULL);
    mlt_properties_set_data(props, "inst_extensions", (void *) inst->extensions, 0, NULL, NULL);
    mlt_properties_set_int(props, "nb_inst_extensions", inst->num_extensions);
    mlt_properties_set_data(props, "dev_extensions", (void *) vk->extensions, 0, NULL, NULL);
    mlt_properties_set_int(props, "nb_dev_extensions", vk->num_extensions);
    mlt_properties_set_int(props, "queue_graphics_index", (int) vk->queue_graphics.index);
    mlt_properties_set_int(props, "queue_graphics_count", (int) vk->queue_graphics.count);
    mlt_properties_set_int(props, "queue_compute_index", (int) vk->queue_compute.index);
    mlt_properties_set_int(props, "queue_compute_count", (int) vk->queue_compute.count);
    mlt_properties_set_int(props, "queue_transfer_index", (int) vk->queue_transfer.index);
    mlt_properties_set_int(props, "queue_transfer_count", (int) vk->queue_transfer.count);
    mlt_properties_set_int(props, "queue_decode_index", decode_index);
    mlt_properties_set_int(props, "queue_decode_count", decode_count);
    mlt_properties_set_int(props, "queue_encode_index", encode_index);
    mlt_properties_set_int(props, "queue_encode_count", encode_count);
    publish_fn(props, "lock_queue", &vk->lock_queue);
    publish_fn(props, "unlock_queue", &vk->unlock_queue);
    mlt_properties_set_data(props,
                            "export_api",
                            (void *) placebo_display_export_api(),
                            0,
                            NULL,
                            NULL);
    mlt_properties_set_data(props, "lock_ctx", (void *) vk, 0, NULL, NULL);

    /* Set the global property only after every key is populated. */
    mlt_properties_set_data(global,
                            "placebo.vulkan",
                            props,
                            0,
                            (mlt_destructor) mlt_properties_close,
                            NULL);
    mlt_log_info(
        NULL,
        "[placebo] GPU initialized via Vulkan (decode queue %d x %d, encode queue %d x %d)\n",
        decode_index,
        decode_count,
        encode_index,
        encode_count);
    if (vk->extensions) {
        for (int i = 0; i < vk->num_extensions; i++) {
            const char *ext = vk->extensions[i];
            if (ext && strstr(ext, "video_encode"))
                mlt_log_info(NULL, "[placebo] Vulkan device extension %s\n", ext);
        }
    }
    return 1;
}

#endif /* PL_HAVE_VULKAN */

void placebo_vulkan_unpublish(void)
{
    mlt_properties global = mlt_global_properties();

    if (global && mlt_properties_get_data(global, "placebo.vulkan", NULL))
        mlt_properties_set_data(global, "placebo.vulkan", NULL, 0, NULL, NULL);
#ifdef PL_HAVE_VULKAN
    forget_procs();
#endif
}
