/*
 * gpu_context.c -- shared libplacebo GPU lifecycle (singleton)
 * Copyright (C) 2025 D-Ogi
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
    char path[512];
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

    /* Try Vulkan */
#ifdef PL_HAVE_VULKAN
    {
        PFN_vkGetInstanceProcAddr vk_get_proc = NULL;
#ifdef PL_HAVE_VK_PROC_ADDR
        /* libplacebo was linked against the Vulkan loader */
        vk_get_proc = NULL; /* libplacebo will use its own */
#elif defined(_WIN32)
        /* Dynamically load vulkan-1.dll to get vkGetInstanceProcAddr */
        s_vulkan_dll = LoadLibraryA("vulkan-1.dll");
        if (s_vulkan_dll) {
            vk_get_proc = (PFN_vkGetInstanceProcAddr) GetProcAddress(s_vulkan_dll,
                                                                     "vkGetInstanceProcAddr");
            if (!vk_get_proc) {
                mlt_log_warning(
                    NULL, "[placebo] vulkan-1.dll loaded but vkGetInstanceProcAddr not found\n");
                FreeLibrary(s_vulkan_dll);
                s_vulkan_dll = NULL;
            }
        } else {
            mlt_log_warning(NULL, "[placebo] vulkan-1.dll not found\n");
        }
#else
        /* On Linux/macOS, dlopen the Vulkan loader */
        mlt_log_warning(NULL,
                        "[placebo] Vulkan: no VK_PROC_ADDR and no dynamic loader implemented\n");
#endif

        if (vk_get_proc || 0
#ifdef PL_HAVE_VK_PROC_ADDR
            || 1
#endif
        ) {
            s_vk_inst = pl_vk_inst_create(s_log,
                                          pl_vk_inst_params(.get_proc_addr = vk_get_proc,
                                                            .debug = false, ));
            if (s_vk_inst) {
                s_vulkan = pl_vulkan_create(s_log,
                                            pl_vulkan_params(.instance = s_vk_inst->instance,
                                                             .get_proc_addr = vk_get_proc, ));
                if (s_vulkan) {
                    s_gpu = s_vulkan->gpu;
                    mlt_log_info(NULL, "[placebo] GPU initialized via Vulkan\n");
                    goto done;
                }
                pl_vk_inst_destroy(&s_vk_inst);
                s_vk_inst = NULL;
            }
        }
        mlt_log_warning(NULL, "[placebo] Vulkan init failed\n");
    }
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
    if (s_cache)
        pl_gpu_set_cache(s_gpu, s_cache);

    load_cache();

    atexit(placebo_gpu_release);

    return 1;
}

/* ---------- Public API ---------- */

pl_gpu placebo_gpu_get(void)
{
    LOCK();
    if (!s_initialized) {
        s_initialized = 1;
        if (!init_gpu()) {
            /* Stay initialized=1 to prevent retry spam on every frame */
            UNLOCK();
            return NULL;
        }
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

void placebo_render_lock(void)
{
    RENDER_LOCK();
}

void placebo_render_unlock(void)
{
    RENDER_UNLOCK();
}

/* ---------- Frame GPU texture reuse ----------
 *
 * When multiple placebo filters are chained on one producer, each filter
 * would normally upload the frame to GPU, render, and download back to RAM.
 * For N filters that means N uploads and N downloads — the intermediate
 * RAM roundtrips are pure waste because the next placebo filter will
 * re-upload the same data immediately.
 *
 * To avoid this, each placebo filter attaches its output texture to the
 * mlt_frame via put_tex(). The next placebo filter in the chain calls
 * take_tex() to grab it and use it directly as its source, skipping the
 * upload. The download to RAM still happens every time (MLT expects the
 * image buffer to be current), but the upload is eliminated for all
 * filters after the first one.
 *
 * Staleness detection: if a non-placebo CPU filter runs between two
 * placebo filters, it may reallocate the image buffer (e.g. via
 * mlt_frame_get_image with writable=1, which triggers a copy).
 * put_tex() records the buffer pointer, and take_tex() compares it
 * against the current pointer. A mismatch means the texture content
 * no longer matches RAM, so take_tex() returns NULL and the caller
 * falls back to a fresh upload. This is safe because MLT's standard
 * mechanism for a filter to modify frame data is to request a writable
 * buffer, which always produces a new allocation.
 */

typedef struct
{
    pl_tex tex;
    const uint8_t *image_ptr; /* RAM buffer address at time of put_tex */
} placebo_frame_gpu;

/* Called by mlt_properties when the frame is destroyed or the property
 * is overwritten. Must acquire render_lock because pl_tex_destroy
 * touches GPU state, and this destructor may fire from any thread. */
static void frame_gpu_destroy(void *ptr)
{
    placebo_frame_gpu *d = ptr;
    if (d && d->tex) {
        pl_gpu gpu = placebo_gpu_get();
        if (gpu) {
            placebo_render_lock();
            pl_tex_destroy(gpu, &d->tex);
            placebo_render_unlock();
        }
    }
    free(d);
}

pl_tex placebo_frame_take_tex(mlt_frame frame, const uint8_t *current_image)
{
    mlt_properties props = MLT_FRAME_PROPERTIES(frame);
    placebo_frame_gpu *d = mlt_properties_get_data(props, "_placebo_gpu", NULL);
    if (!d || !d->tex)
        return NULL;
    if (d->image_ptr != current_image)
        return NULL; /* buffer was reallocated — texture is stale */
    pl_tex tex = d->tex;
    d->tex = NULL; /* transfer ownership to caller; disarm destructor */
    return tex;
}

void placebo_frame_put_tex(mlt_frame frame, pl_tex tex, const uint8_t *image_ptr)
{
    placebo_frame_gpu *d = calloc(1, sizeof(placebo_frame_gpu));
    d->tex = tex;
    d->image_ptr = image_ptr;
    /* If a previous texture was attached, set_data replaces it and
     * frame_gpu_destroy fires for the old one. After take_tex() the
     * old entry has tex=NULL so the destructor becomes a no-op. */
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame), "_placebo_gpu", d, 0, frame_gpu_destroy, NULL);
}

/* ---------- GPU teardown ---------- */

void placebo_gpu_release(void)
{
    LOCK();
    if (!s_initialized) {
        UNLOCK();
        return;
    }

    save_cache();

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

    UNLOCK();
}
