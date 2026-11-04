/*
 * gpu_context.c -- shared libplacebo GPU lifecycle (singleton)
 * Copyright (C) 2025-2026 D-Ogi
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

#include "gpu_context.h"

#include <string.h>

#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <libplacebo/cache.h>
#include <libplacebo/dispatch.h>
#include <libplacebo/gpu.h>
#include <libplacebo/log.h>
#include <libplacebo/renderer.h>

#ifdef PL_HAVE_D3D11
#include <libplacebo/d3d11.h>
#endif

#ifdef PL_HAVE_VULKAN
#include <libplacebo/vulkan.h>
#endif

#ifdef PL_HAVE_OPENGL
#include <libplacebo/opengl.h>
#endif

#include <framework/mlt_log.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

/* ---------- Singleton state ---------- */

static pl_log s_log = NULL;

#ifdef PL_HAVE_D3D11
static pl_d3d11 s_d3d11 = NULL;
#endif

#ifdef PL_HAVE_VULKAN
static pl_vulkan s_vulkan = NULL;
static pl_vk_inst s_vk_inst = NULL; /* must outlive pl_vulkan; destroyed in gpu_release */
#ifdef _WIN32
static HMODULE s_vulkan_dll = NULL; /* dynamically loaded vulkan-1.dll */
#endif
#endif

#ifdef PL_HAVE_OPENGL
static pl_opengl s_opengl = NULL;
#endif

static pl_gpu s_gpu = NULL;
static pl_dispatch s_dispatch = NULL;
static pl_renderer s_renderer = NULL;
static pl_cache s_cache = NULL;

static int s_initialized = 0;
static int s_gpu_failed = 0;   /* permanent failure flag; suppresses retries */
static int s_gpu_shutdown = 0; /* set during placebo_gpu_release(); prevents reinit */
static void release_gpu_locked(void);

/* ---------- Mutex (SRWLOCK on Windows is statically initializable, unlike CRITICAL_SECTION) ---------- */

#ifdef _WIN32
static SRWLOCK s_mutex = SRWLOCK_INIT;
static SRWLOCK s_render_mutex = SRWLOCK_INIT;
#define LOCK() AcquireSRWLockExclusive(&s_mutex)
#define UNLOCK() ReleaseSRWLockExclusive(&s_mutex)
#define RENDER_LOCK() AcquireSRWLockExclusive(&s_render_mutex)
#define RENDER_UNLOCK() ReleaseSRWLockExclusive(&s_render_mutex)
#else
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_render_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&s_mutex)
#define UNLOCK() pthread_mutex_unlock(&s_mutex)
#define RENDER_LOCK() pthread_mutex_lock(&s_render_mutex)
#define RENDER_UNLOCK() pthread_mutex_unlock(&s_render_mutex)
#endif

/* ---------- Shader cache path ---------- */

static void get_cache_path(char *buf, size_t len)
{
    const char *override = getenv("MLT_PLACEBO_CACHE_PATH");
    if (override && override[0] != '\0') {
        snprintf(buf, len, "%s", override);
        return;
    }
#ifdef _WIN32
    char appdata[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        snprintf(buf, len, "%s\\mlt\\placebo_shader_cache.bin", appdata);
    } else {
        buf[0] = '\0';
    }
#else
    const char *home = getenv("HOME");
    if (home) {
        snprintf(buf, len, "%s/.local/share/mlt/placebo_shader_cache.bin", home);
    } else {
        buf[0] = '\0';
    }
#endif
}

static void load_cache(void)
{
    char path[PATH_MAX];
    get_cache_path(path, sizeof(path));
    if (path[0] == '\0')
        return;

    FILE *f = fopen(path, "rb");
    if (!f)
        return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > 0) {
        uint8_t *data = malloc(size);
        if (data) {
            if ((long) fread(data, 1, size, f) == size) {
                pl_cache_load(s_cache, data, size);
                mlt_log_info(NULL,
                             "[placebo] Loaded shader cache (%" PRId64 " bytes) from %s\n",
                             (int64_t) size,
                             path);
            }
            free(data);
        }
    }
    fclose(f);
}

static void ensure_cache_dir(const char *path)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);

#ifdef _WIN32
    char *last_sep = strrchr(dir, '\\');
    if (last_sep) {
        *last_sep = '\0';
        CreateDirectoryA(dir, NULL);
    }
#else
    char *last_sep = strrchr(dir, '/');
    if (last_sep) {
        *last_sep = '\0';
        mkdir(dir, 0755);
    }
#endif
}

static void save_cache(void)
{
    if (!s_cache)
        return;

    char path[512];
    get_cache_path(path, sizeof(path));
    if (path[0] == '\0')
        return;

    ensure_cache_dir(path);

    size_t size = pl_cache_save(s_cache, NULL, 0);
    if (size == 0)
        return;

    uint8_t *data = malloc(size);
    if (!data)
        return;

    pl_cache_save(s_cache, data, size);

    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(data, 1, size, f);
        fclose(f);
        mlt_log_info(NULL,
                     "[placebo] Saved shader cache (%" PRIu64 " bytes) to %s\n",
                     (uint64_t) size,
                     path);
    }
    free(data);
}

/* ---------- Vulkan device shared with avformat ---------- */

static void clear_vulkan_publish(void)
{
    mlt_properties global = mlt_global_properties();
    if (global && mlt_properties_get_data(global, "placebo.vulkan", NULL))
        mlt_properties_set_data(global, "placebo.vulkan", NULL, 0, NULL, NULL);
}

#ifdef PL_HAVE_VULKAN
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
   that queue crashes inside the driver. Publish a get_proc_addr that sets the
   creation bit, and fall back to the caller's flags when the queues were not
   created that way. */
static PFN_vkGetInstanceProcAddr s_real_get_proc;
static PFN_vkGetDeviceProcAddr s_real_get_device_proc;
static PFN_vkGetDeviceQueue2 s_real_get_device_queue2;

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

static void VKAPI_CALL wrapped_get_device_queue2(VkDevice device,
                                                 const VkDeviceQueueInfo2 *info,
                                                 VkQueue *queue)
{
    VkDeviceQueueInfo2 forced;

    if (!queue)
        return;
    *queue = VK_NULL_HANDLE;
    if (!s_real_get_device_queue2 || !info)
        return;

    forced = *info;
    forced.flags |= VK_DEVICE_QUEUE_CREATE_INTERNALLY_SYNCHRONIZED_BIT_KHR;
    s_real_get_device_queue2(device, &forced, queue);
    if (*queue == VK_NULL_HANDLE && forced.flags != info->flags)
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

static void publish_get_proc_addr(mlt_properties props)
{
    PFN_vkGetInstanceProcAddr published = s_vulkan->get_proc_addr;

    s_real_get_proc = s_vulkan->get_proc_addr;
    s_real_get_device_proc = NULL;
    s_real_get_device_queue2 = NULL;
    if (s_real_get_proc && vulkan_queues_are_internally_synchronized(s_vulkan)) {
        s_real_get_device_proc = (PFN_vkGetDeviceProcAddr) s_real_get_proc(s_vk_inst->instance,
                                                                           "vkGetDeviceProcAddr");
        if (s_real_get_device_proc)
            s_real_get_device_queue2 = (PFN_vkGetDeviceQueue2)
                s_real_get_device_proc(s_vulkan->device, "vkGetDeviceQueue2");
        if (s_real_get_device_queue2)
            published = wrapped_get_instance_proc;
    }
    publish_fn(props, "get_proc_addr", &published);
}

static void find_decode_queue(pl_vulkan vk, int *index, int *count)
{
    *index = -1;
    *count = 0;
    /* VK_QUEUE_VIDEO_DECODE_BIT_KHR is an enum constant, not a macro. */
#ifdef VK_KHR_video_decode_queue
    if (!vk || !vk->get_proc_addr || !vk->instance || !vk->phys_device)
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
        if (families[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
            *index = (int) i;
            *count = (int) families[i].queueCount;
            break;
        }
    }
    free(families);
#else
    (void) vk;
#endif
}

/* Caller holds s_mutex. Publishes only after the properties object is complete. */
static void publish_vulkan_locked(void)
{
    if (!s_vulkan || !s_vk_inst)
        return;

    mlt_properties global = mlt_global_properties();
    if (!global || mlt_properties_get_data(global, "placebo.vulkan", NULL))
        return;

    int decode_index = -1;
    int decode_count = 0;
    find_decode_queue(s_vulkan, &decode_index, &decode_count);

    mlt_properties props = mlt_properties_new();
    if (!props)
        return;

    mlt_properties_set_data(props, "instance", (void *) s_vk_inst->instance, 0, NULL, NULL);
    mlt_properties_set_data(props, "phys_dev", (void *) s_vulkan->phys_device, 0, NULL, NULL);
    mlt_properties_set_data(props, "device", (void *) s_vulkan->device, 0, NULL, NULL);
    publish_get_proc_addr(props);
    mlt_properties_set_data(props, "features", (void *) s_vulkan->features, 0, NULL, NULL);
    mlt_properties_set_data(props, "inst_extensions", (void *) s_vk_inst->extensions, 0, NULL, NULL);
    mlt_properties_set_int(props, "nb_inst_extensions", s_vk_inst->num_extensions);
    mlt_properties_set_data(props, "dev_extensions", (void *) s_vulkan->extensions, 0, NULL, NULL);
    mlt_properties_set_int(props, "nb_dev_extensions", s_vulkan->num_extensions);
    mlt_properties_set_int(props, "queue_graphics_index", (int) s_vulkan->queue_graphics.index);
    mlt_properties_set_int(props, "queue_graphics_count", (int) s_vulkan->queue_graphics.count);
    mlt_properties_set_int(props, "queue_compute_index", (int) s_vulkan->queue_compute.index);
    mlt_properties_set_int(props, "queue_compute_count", (int) s_vulkan->queue_compute.count);
    mlt_properties_set_int(props, "queue_transfer_index", (int) s_vulkan->queue_transfer.index);
    mlt_properties_set_int(props, "queue_transfer_count", (int) s_vulkan->queue_transfer.count);
    mlt_properties_set_int(props, "queue_decode_index", decode_index);
    mlt_properties_set_int(props, "queue_decode_count", decode_count);
    publish_fn(props, "lock_queue", &s_vulkan->lock_queue);
    publish_fn(props, "unlock_queue", &s_vulkan->unlock_queue);
    mlt_properties_set_data(props, "lock_ctx", (void *) s_vulkan, 0, NULL, NULL);

    /* Set the global property only after every key is populated. */
    mlt_properties_set_data(global,
                            "placebo.vulkan",
                            props,
                            0,
                            (mlt_destructor) mlt_properties_close,
                            NULL);
    if (decode_count > 0)
        mlt_log_info(NULL,
                     "[placebo] GPU initialized via Vulkan (decode queue %d x %d)\n",
                     decode_index,
                     decode_count);
    else
        mlt_log_info(NULL, "[placebo] GPU initialized via Vulkan (no video decode queue)\n");
}

static int vulkan_loader(PFN_vkGetInstanceProcAddr *vk_get_proc)
{
    *vk_get_proc = NULL;
#ifdef PL_HAVE_VK_PROC_ADDR
    /* libplacebo was linked against the Vulkan loader and fills get_proc_addr. */
    return 1;
#elif defined(_WIN32)
    s_vulkan_dll = LoadLibraryA("vulkan-1.dll");
    if (s_vulkan_dll) {
        *vk_get_proc = (PFN_vkGetInstanceProcAddr) GetProcAddress(s_vulkan_dll,
                                                                  "vkGetInstanceProcAddr");
        if (*vk_get_proc)
            return 1;
        mlt_log_warning(NULL, "[placebo] vulkan-1.dll loaded but vkGetInstanceProcAddr not found\n");
        FreeLibrary(s_vulkan_dll);
        s_vulkan_dll = NULL;
    } else {
        mlt_log_warning(NULL, "[placebo] vulkan-1.dll not found\n");
    }
    return 0;
#else
    mlt_log_warning(NULL, "[placebo] Vulkan: no VK_PROC_ADDR and no dynamic loader implemented\n");
    return 0;
#endif
}

static int create_vulkan_device(PFN_vkGetInstanceProcAddr vk_get_proc, int with_decode)
{
    const char *opt_exts[8] = {0};
    int num_opt = 0;
    VkQueueFlags extra_queues = 0;
#if defined(VK_KHR_video_decode_queue) && defined(VK_KHR_video_maintenance1)
    VkPhysicalDeviceVideoMaintenance1FeaturesKHR video_maint = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_MAINTENANCE_1_FEATURES_KHR,
        .videoMaintenance1 = VK_TRUE,
    };
    VkPhysicalDeviceFeatures2 extra_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &video_maint,
    };
#endif
    const VkPhysicalDeviceFeatures2 *features = NULL;

#ifdef VK_KHR_video_decode_queue
    if (with_decode) {
        extra_queues = VK_QUEUE_VIDEO_DECODE_BIT_KHR;
#ifdef VK_KHR_video_queue
        opt_exts[num_opt++] = VK_KHR_VIDEO_QUEUE_EXTENSION_NAME;
#endif
        opt_exts[num_opt++] = VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME;
#ifdef VK_KHR_video_decode_h264
        opt_exts[num_opt++] = VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME;
#endif
#ifdef VK_KHR_video_decode_h265
        opt_exts[num_opt++] = VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME;
#endif
#ifdef VK_KHR_video_decode_av1
        opt_exts[num_opt++] = VK_KHR_VIDEO_DECODE_AV1_EXTENSION_NAME;
#endif
#ifdef VK_KHR_video_decode_vp9
        opt_exts[num_opt++] = VK_KHR_VIDEO_DECODE_VP9_EXTENSION_NAME;
#endif
#ifdef VK_KHR_video_maintenance1
        opt_exts[num_opt++] = VK_KHR_VIDEO_MAINTENANCE_1_EXTENSION_NAME;
        features = &extra_features;
#endif
    }
#else
    (void) with_decode;
#endif

    s_vulkan = pl_vulkan_create(s_log,
                                pl_vulkan_params(.instance = s_vk_inst->instance,
                                                 .get_proc_addr = vk_get_proc,
                                                 .extra_queues = extra_queues,
                                                 .opt_extensions = num_opt ? opt_exts : NULL,
                                                 .num_opt_extensions = num_opt,
                                                 .features = features, ));
    return s_vulkan != NULL;
}

static int try_vulkan(void)
{
    PFN_vkGetInstanceProcAddr vk_get_proc = NULL;
    if (!vulkan_loader(&vk_get_proc))
        return 0;

    s_vk_inst = pl_vk_inst_create(s_log,
                                  pl_vk_inst_params(.get_proc_addr = vk_get_proc, .debug = false, ));
    if (!s_vk_inst) {
        mlt_log_warning(NULL, "[placebo] Vulkan init failed\n");
        return 0;
    }

    if (!create_vulkan_device(vk_get_proc, 1)) {
        mlt_log_warning(NULL,
                        "[placebo] Vulkan device with video decode queues failed; retrying "
                        "without them\n");
        if (!create_vulkan_device(vk_get_proc, 0)) {
            pl_vk_inst_destroy(&s_vk_inst);
            s_vk_inst = NULL;
            mlt_log_warning(NULL, "[placebo] Vulkan init failed\n");
            return 0;
        }
    }

    s_gpu = s_vulkan->gpu;
    return 1;
}
#endif

/* ---------- GPU initialization ---------- */

static int init_gpu(void)
{
    /* Create logger */
    s_log = pl_log_create(PL_API_VER, pl_log_params(.log_level = PL_LOG_WARN, ));
    if (!s_log) {
        mlt_log_error(NULL, "[placebo] Failed to create pl_log\n");
        return 0;
    }

    /* Create shader cache */
    s_cache = pl_cache_create(pl_cache_params(.log = s_log, ));

    /* When avformat will decode with Vulkan, that device has to be the one
       placebo renders with. Prefer it over D3D11 so the singleton can be shared. */
    const char *hwaccel = getenv("MLT_AVFORMAT_HWACCEL");
    int vulkan_first = hwaccel && !strcmp(hwaccel, "vulkan");

#ifdef PL_HAVE_VULKAN
    if (vulkan_first && try_vulkan())
        goto done;
#endif

        /* Try D3D11 first on Windows */
#ifdef PL_HAVE_D3D11
    s_d3d11 = pl_d3d11_create(s_log, pl_d3d11_params(.allow_software = false, ));
    if (s_d3d11) {
        s_gpu = s_d3d11->gpu;
        mlt_log_info(NULL, "[placebo] GPU initialized via D3D11\n");
        goto done;
    }
    mlt_log_warning(NULL, "[placebo] D3D11 init failed, trying Vulkan...\n");
#endif

#ifdef PL_HAVE_VULKAN
    if (!vulkan_first && try_vulkan())
        goto done;
#else
    (void) vulkan_first;
#endif

        /* Try OpenGL as last resort */
#ifdef PL_HAVE_OPENGL
    {
        s_opengl = pl_opengl_create(s_log, pl_opengl_params());
        if (s_opengl) {
            s_gpu = s_opengl->gpu;
            mlt_log_info(NULL, "[placebo] GPU initialized via OpenGL\n");
            goto done;
        }
        mlt_log_warning(NULL, "[placebo] OpenGL init failed\n");
    }
#endif

    mlt_log_error(NULL, "[placebo] No GPU backend available\n");
    return 0;

done:
    if (s_cache) {
        pl_gpu_set_cache(s_gpu, s_cache);
        load_cache();
    }

#ifdef PL_HAVE_VULKAN
    publish_vulkan_locked();
#endif
    mlt_factory_register_for_clean_up(NULL, (mlt_destructor) placebo_gpu_release);

    return 1;
}

/* ---------- Public API ---------- */

pl_gpu placebo_gpu_get(void)
{
    LOCK();
    if (!s_initialized && !s_gpu_failed && !s_gpu_shutdown) {
        if (!init_gpu()) {
            /* Free any partial resources (pl_log, cache, loader handles) before
             * setting the permanent failure flag to suppress future retries. */
            release_gpu_locked();
            s_gpu_failed = 1;
            UNLOCK();
            return NULL;
        }
        s_initialized = 1;
    }
    pl_gpu result = s_gpu;
    UNLOCK();
    return result;
}

pl_dispatch placebo_dispatch_get(void)
{
    pl_gpu gpu = placebo_gpu_get();
    if (!gpu)
        return NULL;

    LOCK();
    if (!s_dispatch) {
        s_dispatch = pl_dispatch_create(s_log, gpu);
    }
    pl_dispatch result = s_dispatch;
    UNLOCK();
    return result;
}

pl_renderer placebo_renderer_get(void)
{
    pl_gpu gpu = placebo_gpu_get();
    if (!gpu)
        return NULL;

    LOCK();
    if (!s_renderer) {
        s_renderer = pl_renderer_create(s_log, gpu);
    }
    pl_renderer result = s_renderer;
    UNLOCK();
    return result;
}

void placebo_vulkan_publish(void)
{
#ifdef PL_HAVE_VULKAN
    LOCK();
    publish_vulkan_locked();
    UNLOCK();
#endif
}

int placebo_vulkan_has_decode_queue(void)
{
    mlt_properties published = mlt_properties_get_data(mlt_global_properties(),
                                                       "placebo.vulkan",
                                                       NULL);
    if (!published)
        return 0;
    return mlt_properties_get_int(published, "queue_decode_count") > 0;
}

void placebo_render_lock(void)
{
    RENDER_LOCK();
}

void placebo_render_unlock(void)
{
    RENDER_UNLOCK();
}

static void placebo_tex_destroy(void *ptr)
{
    pl_tex tex = (pl_tex) ptr;
    if (!tex)
        return;
    pl_gpu gpu = placebo_gpu_get();
    if (!gpu)
        return;
    placebo_render_lock();
    pl_tex_destroy(gpu, &tex);
    placebo_render_unlock();
}

pl_tex placebo_image_get_tex(const uint8_t *image)
{
    return (pl_tex) image;
}

/* ---------- mlt_image_private helpers ---------- */

/* Request state and result state must not share one property:
 * - requests are transient and may nest while multiple placebo filters pull
 *   upstream on the same frame;
 * - results describe the frame's current image payload and must only be set
 *   when that payload really is a placebo texture.
 * If these are conflated, one filter can clear another filter's in-flight
 * request or a non-placebo result can be mistaken for a valid private texture. */

void placebo_frame_set_requested_tex(mlt_frame frame, int requested)
{
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    int count = mlt_properties_get_int(props, "_placebo_requested");

    if (requested) {
        count++;
    } else if (count > 0) {
        count--;
    }

    mlt_properties_set_int(props, "_placebo_requested", count);
}

int placebo_frame_wants_tex(mlt_frame frame, mlt_image_format format)
{
    return format == mlt_image_private
           && mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "_placebo_requested") > 0;
}

int placebo_frame_is_tex(mlt_frame frame, mlt_image_format format)
{
    const char *image_format = mlt_properties_get(MLT_FRAME_PROPERTIES(frame),
                                                  MLT_PLACEBO_IMAGE_PRIVATE);
    return format == mlt_image_private && image_format && !strcmp(image_format, "placebo");
}

int placebo_frame_is_vulkan(mlt_frame frame, mlt_image_format format)
{
    const char *image_format = mlt_properties_get(MLT_FRAME_PROPERTIES(frame),
                                                  MLT_PLACEBO_IMAGE_PRIVATE);
    return format == mlt_image_private && image_format && !strcmp(image_format, "vulkan");
}

int placebo_frame_set_tex(mlt_frame frame, pl_tex tex)
{
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    mlt_properties_set_int(props, "format", mlt_image_private);
    mlt_properties_set(props, MLT_PLACEBO_IMAGE_PRIVATE, "placebo");
    mlt_properties_clear(props, "alpha");
    return mlt_frame_set_image(frame, (uint8_t *) tex, 0, placebo_tex_destroy);
}

/* ---------- GPU teardown ---------- */

/* Internal: free all GPU resources. Caller must hold s_mutex. */
static void release_gpu_locked(void)
{
    if (s_renderer) {
        pl_renderer_destroy(&s_renderer);
        s_renderer = NULL;
    }
    if (s_dispatch) {
        pl_dispatch_destroy(&s_dispatch);
        s_dispatch = NULL;
    }

    if (s_cache) {
        pl_cache_destroy(&s_cache);
        s_cache = NULL;
    }

    /* Drop the published handles before the VkDevice they point at is destroyed.
       Setting NULL runs the properties destructor installed at publish time. */
    clear_vulkan_publish();

#ifdef PL_HAVE_D3D11
    if (s_d3d11) {
        pl_d3d11_destroy(&s_d3d11);
        s_d3d11 = NULL;
    }
#endif

#ifdef PL_HAVE_VULKAN
    if (s_vulkan) {
        pl_vulkan_destroy(&s_vulkan);
        s_vulkan = NULL;
    }
    if (s_vk_inst) {
        pl_vk_inst_destroy(&s_vk_inst);
        s_vk_inst = NULL;
    }
    s_real_get_proc = NULL;
    s_real_get_device_proc = NULL;
    s_real_get_device_queue2 = NULL;
#ifdef _WIN32
    if (s_vulkan_dll) {
        FreeLibrary(s_vulkan_dll);
        s_vulkan_dll = NULL;
    }
#endif
#endif

#ifdef PL_HAVE_OPENGL
    if (s_opengl) {
        pl_opengl_destroy(&s_opengl);
        s_opengl = NULL;
    }
#endif

    if (s_log) {
        pl_log_destroy(&s_log);
        s_log = NULL;
    }

    s_gpu = NULL;
    s_initialized = 0;
    s_gpu_failed = 0;
    s_gpu_shutdown = 0;
}

void placebo_gpu_release(void)
{
    LOCK();
    if (!s_initialized) {
        UNLOCK();
        return;
    }

    s_gpu_shutdown = 1; /* block any reinit triggered by in-flight tex destructors */
    save_cache();
    release_gpu_locked();

    UNLOCK();
}
