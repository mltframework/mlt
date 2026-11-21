/*
 * gpu_context.c -- shared libplacebo GPU lifecycle (singleton)
 * Copyright (C) 2025-2026 D-Ogi
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

#include "gpu_context.h"

#include "display_export.h"
#include "render_cache.h"
#include "tex_pool.h"
#include "vulkan_device.h"
#include "vulkan_publish.h"

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
static int s_published = 0;    /* set by a publisher, not by scale or color_transform */
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

#ifdef PL_HAVE_VULKAN
/* Caller holds s_mutex. */
static void publish_vulkan_locked(void)
{
    if (placebo_vulkan_publish_device(s_vulkan, s_vk_inst) && s_gpu)
        s_published = 1;
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

static void destroy_vk_surface(VkSurfaceKHR surface)
{
    PFN_vkDestroySurfaceKHR destroy_surface = NULL;

    if (!surface || !s_vk_inst || !s_vk_inst->instance)
        return;
    if (s_vk_inst->get_proc_addr)
        destroy_surface = (PFN_vkDestroySurfaceKHR) s_vk_inst->get_proc_addr(s_vk_inst->instance,
                                                                             "vkDestroySurfaceKHR");
    if (destroy_surface)
        destroy_surface(s_vk_inst->instance, surface, NULL);
}

static int create_vulkan_device(PFN_vkGetInstanceProcAddr vk_get_proc,
                                int with_decode,
                                VkSurfaceKHR surface)
{
    struct placebo_ext_list exts = {0};
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
        placebo_ext_add(&exts, VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
#endif
        placebo_ext_add(&exts, VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
#ifdef VK_KHR_video_decode_h264
        placebo_ext_add(&exts, VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME);
#endif
#ifdef VK_KHR_video_decode_h265
        placebo_ext_add(&exts, VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME);
#endif
#ifdef VK_KHR_video_decode_av1
        placebo_ext_add(&exts, VK_KHR_VIDEO_DECODE_AV1_EXTENSION_NAME);
#endif
#ifdef VK_KHR_video_decode_vp9
        placebo_ext_add(&exts, VK_KHR_VIDEO_DECODE_VP9_EXTENSION_NAME);
#endif
#ifdef VK_KHR_video_maintenance1
        placebo_ext_add(&exts, VK_KHR_VIDEO_MAINTENANCE_1_EXTENSION_NAME);
        features = &extra_features;
#endif
#ifdef VK_KHR_video_encode_queue
        extra_queues |= VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
        placebo_ext_add(&exts, VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);
#ifdef VK_KHR_video_encode_h264
        placebo_ext_add(&exts, VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME);
#endif
#ifdef VK_KHR_video_encode_h265
        placebo_ext_add(&exts, VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME);
#endif
#endif
    }
#else
    (void) with_decode;
#endif
    placebo_ext_add_external(&exts);
    if (exts.overflow) {
        mlt_log_error(NULL, "[placebo] too many Vulkan device extensions\n");
        return 0;
    }

    s_vulkan = pl_vulkan_create(s_log,
                                pl_vulkan_params(.instance = s_vk_inst->instance,
                                                 .get_proc_addr = vk_get_proc,
                                                 .surface = surface,
                                                 .device = placebo_vulkan_device_from_env(s_vk_inst),
                                                 .extra_queues = extra_queues,
                                                 .opt_extensions = exts.count ? exts.names : NULL,
                                                 .num_opt_extensions = exts.count,
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

    if (!create_vulkan_device(vk_get_proc, 1, VK_NULL_HANDLE)) {
        mlt_log_warning(NULL,
                        "[placebo] Vulkan device with video decode queues failed; retrying "
                        "without them\n");
        if (!create_vulkan_device(vk_get_proc, 0, VK_NULL_HANDLE)) {
            pl_vk_inst_destroy(&s_vk_inst);
            s_vk_inst = NULL;
            mlt_log_warning(NULL, "[placebo] Vulkan init failed\n");
            return 0;
        }
    }

    s_gpu = s_vulkan->gpu;
    return 1;
}

int placebo_vulkan_init_display(const char *const *extensions,
                                int count,
                                uint64_t (*make_surface)(void *instance, void *opaque),
                                void *opaque)
{
    PFN_vkGetInstanceProcAddr vk_get_proc = NULL;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    int created_here = 0;

    LOCK();
    if (s_gpu_shutdown) {
        UNLOCK();
        return 1;
    }
    if (s_initialized) {
        int ok = s_vulkan != NULL;
        UNLOCK();
        return ok ? 0 : 1;
    }

    if (!s_log)
        s_log = pl_log_create(PL_API_VER, pl_log_params(.log_level = PL_LOG_WARN, ));
    if (!s_log) {
        UNLOCK();
        return 1;
    }
    if (!s_cache)
        s_cache = pl_cache_create(pl_cache_params(.log = s_log, ));

    if (!vulkan_loader(&vk_get_proc))
        goto fail;

    s_vk_inst = pl_vk_inst_create(s_log,
                                  pl_vk_inst_params(.get_proc_addr = vk_get_proc,
                                                    .debug = false,
                                                    .extensions = extensions,
                                                    .num_extensions = count, ));
    if (!s_vk_inst) {
        mlt_log_warning(NULL, "[placebo] Vulkan instance for display failed\n");
        goto fail;
    }
    created_here = 1;

    if (make_surface) {
        uint64_t raw = make_surface((void *) s_vk_inst->instance, opaque);
        surface = (VkSurfaceKHR) raw;
    }
    if (!surface) {
        mlt_log_warning(NULL, "[placebo] Vulkan display surface failed\n");
        goto fail;
    }

    if (!create_vulkan_device(vk_get_proc, 1, surface)) {
        mlt_log_warning(NULL,
                        "[placebo] Vulkan display device with video decode queues failed; "
                        "retrying without them\n");
        if (!create_vulkan_device(vk_get_proc, 0, surface)) {
            mlt_log_warning(NULL, "[placebo] Vulkan display device failed\n");
            goto fail;
        }
    }

    s_gpu = s_vulkan->gpu;
    if (s_cache) {
        pl_gpu_set_cache(s_gpu, s_cache);
        load_cache();
    }
    publish_vulkan_locked();
    mlt_factory_register_for_clean_up(NULL, (mlt_destructor) placebo_gpu_release);
    s_initialized = 1;
    UNLOCK();
    return 0;

fail:
    /* The caller drops its surface handle when this returns failure. */
    if (surface)
        destroy_vk_surface(surface);
    if (created_here || s_vk_inst || s_vulkan || s_log)
        release_gpu_locked();
    UNLOCK();
    return 1;
}
#endif

#ifndef PL_HAVE_VULKAN
int placebo_vulkan_init_display(const char *const *extensions,
                                int count,
                                uint64_t (*make_surface)(void *instance, void *opaque),
                                void *opaque)
{
    (void) extensions;
    (void) count;
    (void) make_surface;
    (void) opaque;
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
    LOCK();
#ifdef PL_HAVE_VULKAN
    publish_vulkan_locked();
#endif
    /* OpenGL and D3D11 never write placebo.vulkan. The flag still marks that
       a placebo service created this GPU, so scale and color_transform can init. */
    if (s_gpu)
        s_published = 1;
    UNLOCK();
}

int placebo_context_is_published(void)
{
    LOCK();
    int published = s_published;
    UNLOCK();
    return published;
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

/* A frame destroyed after the GPU was released must not bring it back. */
pl_gpu placebo_gpu_peek(void)
{
    LOCK();
    pl_gpu gpu = s_initialized ? s_gpu : NULL;
    UNLOCK();
    return gpu;
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
    placebo_vulkan_unpublish();

#ifdef PL_HAVE_D3D11
    if (s_d3d11) {
        pl_d3d11_destroy(&s_d3d11);
        s_d3d11 = NULL;
    }
#endif

#ifdef PL_HAVE_VULKAN
    /* Export images and semaphores use this device. Drop them first. */
    placebo_display_export_close();
    if (s_vulkan) {
        pl_vulkan_destroy(&s_vulkan);
        s_vulkan = NULL;
    }
    if (s_vk_inst) {
        pl_vk_inst_destroy(&s_vk_inst);
        s_vk_inst = NULL;
    }
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
    s_published = 0;
    s_gpu_failed = 0;
    s_gpu_shutdown = 0;
}

void placebo_gpu_release(void)
{
    /* The pool and the renderer cache belong to the render lock. Take it before
       s_mutex, the order render threads use, so none of them is mid-call. */
    RENDER_LOCK();
    LOCK();
    if (!s_initialized) {
        UNLOCK();
        RENDER_UNLOCK();
        return;
    }

    s_gpu_shutdown = 1; /* block any reinit triggered by in-flight tex destructors */
    save_cache();
    placebo_tex_pool_drain(s_gpu);
    placebo_render_cache_drain();
    release_gpu_locked();

    UNLOCK();
    RENDER_UNLOCK();
}
