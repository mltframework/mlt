/*
 * consumer_placebo_sdl.c -- Vulkan preview through an SDL window
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

#include "color_space.h"
#include "gpu_context.h"
#include "vulkan_frame.h"

#include <framework/mlt.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <libplacebo/renderer.h>
#include <libplacebo/utils/upload.h>
#include <libplacebo/vulkan.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_VER
#include <gettimeofday.h>
#else
#include <sys/time.h>
#endif

#define SDL_AUDIO_BUFFER_MIN_BYTES (2048 * 5 * (int) sizeof(float) * 2)

typedef struct consumer_sdl_s *consumer_sdl;

struct consumer_sdl_s
{
    struct mlt_consumer_s parent;
    mlt_properties properties;
    mlt_deque queue;
    pthread_t thread;
    int joined;
    atomic_int running;
    uint8_t *audio_buffer;
    int audio_buffer_size;
    int audio_avail;
    int audio_counter;
    pthread_mutex_t audio_mutex;
    pthread_cond_t audio_cond;
    pthread_mutex_t video_mutex;
    pthread_cond_t video_cond;
    int window_width;
    int window_height;
    int width;
    int height;
    int out_channels;
    int is_purge;
    SDL_Window *sdl_window;
    SDL_AudioDeviceID audio_dev;
    VkSurfaceKHR surface;
    pl_swapchain swapchain;
    pl_tex upload_tex;
    int rect_x;
    int rect_y;
    int rect_w;
    int rect_h;
#ifdef _WIN32
    int no_quit_subsystem;
#endif
};

static int consumer_start(mlt_consumer parent);
static int consumer_stop(mlt_consumer parent);
static int consumer_is_stopped(mlt_consumer parent);
static void consumer_purge(mlt_consumer parent);
static void consumer_close(mlt_consumer parent);
static void *consumer_thread(void *arg);
static int ensure_display(consumer_sdl self);

static SDL_AudioDeviceID open_audio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    SDL_AudioDeviceID dev
        = SDL_OpenAudioDevice(NULL, 0, desired, obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    int i;

    if (dev != 0)
        return dev;

    mlt_log_info(NULL, "Failed to open audio device: %s\n", SDL_GetError());
    for (i = 0; i < SDL_GetNumAudioDrivers(); i++) {
        const char *driver = SDL_GetAudioDriver(i);
        if (!strcmp(driver, "disk") || !strcmp(driver, "dummy"))
            continue;
        if (SDL_AudioInit(driver) != 0)
            continue;
        dev = SDL_OpenAudioDevice(NULL, 0, desired, obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
        if (dev != 0)
            return dev;
    }

    if (desired->channels > 2) {
        SDL_AudioSpec stereo = *desired;
        stereo.channels = 2;
        SDL_AudioInit(NULL);
        return open_audio(&stereo, obtained);
    }
    return 0;
}

static int ensure_buffer_capacity(uint8_t **buffer, int *capacity, int minimum)
{
    uint8_t *resized;

    if (!buffer || !capacity || minimum <= 0)
        return 1;
    if (*capacity >= minimum)
        return 0;
    resized = realloc(*buffer, minimum);
    if (!resized)
        return 1;
    *buffer = resized;
    *capacity = minimum;
    return 0;
}

static void destroy_surface(consumer_sdl self)
{
    pl_gpu gpu;
    pl_vulkan vk;
    PFN_vkDestroySurfaceKHR destroy_surface_fn = NULL;

    if (!self->surface)
        return;
    gpu = placebo_gpu_get();
    vk = gpu ? pl_vulkan_get(gpu) : NULL;
    if (vk && vk->get_proc_addr)
        destroy_surface_fn = (PFN_vkDestroySurfaceKHR) vk->get_proc_addr(vk->instance,
                                                                         "vkDestroySurfaceKHR");
    if (destroy_surface_fn)
        destroy_surface_fn(vk->instance, self->surface, NULL);
    self->surface = VK_NULL_HANDLE;
}

static void destroy_display(consumer_sdl self)
{
    if (self->upload_tex) {
        pl_gpu gpu = placebo_gpu_get();
        placebo_render_lock();
        if (gpu)
            pl_tex_destroy(gpu, &self->upload_tex);
        placebo_render_unlock();
        self->upload_tex = NULL;
    }
    if (self->swapchain)
        pl_swapchain_destroy(&self->swapchain);
    destroy_surface(self);
    if (self->sdl_window) {
        SDL_DestroyWindow(self->sdl_window);
        self->sdl_window = NULL;
    }
}

static int create_window(consumer_sdl self)
{
    int width = self->window_width > 0 ? self->window_width : self->width;
    int height = self->window_height > 0 ? self->window_height : self->height;

    if (self->sdl_window)
        return 0;
    if (width < 1)
        width = 640;
    if (height < 1)
        height = 360;
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent),
                          "Failed to initialize SDL: %s\n",
                          SDL_GetError());
            return 1;
        }
    }
    self->sdl_window = SDL_CreateWindow("MLT",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        width,
                                        height,
                                        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
                                            | SDL_WINDOW_HIDDEN);
    if (!self->sdl_window) {
        mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent),
                      "Failed to create SDL Vulkan window: %s\n",
                      SDL_GetError());
        return 1;
    }
    self->window_width = width;
    self->window_height = height;
    return 0;
}

static uint64_t make_surface(void *instance, void *opaque)
{
    consumer_sdl self = opaque;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (!SDL_Vulkan_CreateSurface(self->sdl_window, (VkInstance) instance, &surface)) {
        mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent),
                      "SDL_Vulkan_CreateSurface failed: %s\n",
                      SDL_GetError());
        return 0;
    }
    self->surface = surface;
    return (uint64_t) surface;
}

static int init_device(consumer_sdl self)
{
    unsigned count = 0;
    const char **extensions = NULL;
    int error;

    if (!SDL_Vulkan_GetInstanceExtensions(self->sdl_window, &count, NULL) || count < 1) {
        mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent),
                      "SDL_Vulkan_GetInstanceExtensions failed: %s\n",
                      SDL_GetError());
        return 1;
    }
    extensions = calloc(count, sizeof(*extensions));
    if (!extensions)
        return 1;
    if (!SDL_Vulkan_GetInstanceExtensions(self->sdl_window, &count, extensions)) {
        free(extensions);
        return 1;
    }
    error = placebo_vulkan_init_display(extensions, (int) count, make_surface, self);
    free(extensions);
    if (error) {
        /* init_display destroys the surface it created when device setup fails. */
        self->surface = VK_NULL_HANDLE;
        return 1;
    }
    return 0;
}

static int ensure_surface(consumer_sdl self)
{
    pl_gpu gpu;
    pl_vulkan vk;

    if (self->surface)
        return 0;
    gpu = placebo_gpu_get();
    vk = gpu ? pl_vulkan_get(gpu) : NULL;
    if (!vk || !SDL_Vulkan_CreateSurface(self->sdl_window, vk->instance, &self->surface)) {
        mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent),
                      "SDL_Vulkan_CreateSurface failed: %s\n",
                      SDL_GetError());
        return 1;
    }
    return 0;
}

static int create_swapchain(consumer_sdl self)
{
    pl_gpu gpu = placebo_gpu_get();
    pl_vulkan vk = gpu ? pl_vulkan_get(gpu) : NULL;

    if (!vk || !self->surface)
        return 1;
    if (self->swapchain)
        return 0;
    /* Mailbox keeps a tear-free image without waiting for vblank. The
       playtime clock in the video thread is the pacing, same as the sdl2
       consumer, which does not set SDL_RENDERER_PRESENTVSYNC. FIFO acquire
       waits inside pl_swapchain_start_frame while placebo_render_lock is
       held, so the real_time=1 read-ahead misses the frame and starts
       dropping. Unsupported mailbox falls back to FIFO inside libplacebo. */
    self->swapchain = pl_vulkan_create_swapchain(vk,
                                                 pl_vulkan_swapchain_params(
                                                         .surface = self->surface,
                                                         .present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
                                                         .swapchain_depth = 3, ));
    if (!self->swapchain) {
        mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent), "Failed to create Vulkan swapchain\n");
        return 1;
    }
    mlt_log_info(MLT_CONSUMER_SERVICE(&self->parent), "Vulkan swapchain ready\n");
    return 0;
}

static int ensure_display(consumer_sdl self)
{
    int created = self->sdl_window == NULL;

    if (create_window(self))
        return 1;
    if (created && mlt_properties_get_int(self->properties, "fullscreen"))
        SDL_SetWindowFullscreen(self->sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (ensure_surface(self))
        return 1;
    return 0;
}

static void letterbox(consumer_sdl self, mlt_frame frame, int img_w, int img_h, int dst_w, int dst_h)
{
    mlt_properties properties = self->properties;
    double this_aspect = dst_h > 0 ? (double) dst_w / dst_h : 1.0;
    double display_ratio = mlt_properties_get_double(properties, "display_ratio");
    double frame_aspect = img_h > 0 ? mlt_frame_get_aspect_ratio(frame) * img_w / img_h
                                    : this_aspect;
    const char *rescale = mlt_properties_get(properties, "rescale");

    if (rescale && !strcmp(rescale, "none")) {
        self->rect_w = frame_aspect / this_aspect * dst_w;
        self->rect_h = dst_h;
        if (self->rect_w > dst_w) {
            self->rect_w = dst_w;
            self->rect_h = this_aspect / frame_aspect * dst_h;
        }
    } else if ((int) (this_aspect * 1000) == (int) (display_ratio * 1000)) {
        self->rect_w = dst_w;
        self->rect_h = dst_h;
    } else if (dst_h * display_ratio > dst_w) {
        self->rect_w = dst_w;
        self->rect_h = display_ratio > 0 ? dst_w / display_ratio : dst_h;
    } else {
        self->rect_w = dst_h * display_ratio;
        self->rect_h = dst_h;
    }
    if (self->rect_w < 1)
        self->rect_w = 1;
    if (self->rect_h < 1)
        self->rect_h = 1;
    self->rect_x = (dst_w - self->rect_w) / 2;
    self->rect_y = (dst_h - self->rect_h) / 2;
    self->rect_x -= self->rect_x % 2;
    mlt_properties_set_int(properties, "rect_x", self->rect_x);
    mlt_properties_set_int(properties, "rect_y", self->rect_y);
    mlt_properties_set_int(properties, "rect_w", self->rect_w);
    mlt_properties_set_int(properties, "rect_h", self->rect_h);
}

static uint8_t *yuv422_to_rgba(const uint8_t *src, int width, int height)
{
    int y, x;
    uint8_t *rgba = mlt_pool_alloc(width * height * 4);
    uint8_t *dst;

    if (!rgba)
        return NULL;
    dst = rgba;
    for (y = 0; y < height; y++) {
        for (x = 0; x + 1 < width; x += 2) {
            int y0 = src[0];
            int u = src[1] - 128;
            int y1 = src[2];
            int v = src[3] - 128;
            int r = y0 + ((359 * v) >> 8);
            int g = y0 - ((88 * u + 183 * v) >> 8);
            int b = y0 + ((454 * u) >> 8);
            src += 4;
            *dst++ = r < 0 ? 0 : (r > 255 ? 255 : r);
            *dst++ = g < 0 ? 0 : (g > 255 ? 255 : g);
            *dst++ = b < 0 ? 0 : (b > 255 ? 255 : b);
            *dst++ = 255;
            r = y1 + ((359 * v) >> 8);
            g = y1 - ((88 * u + 183 * v) >> 8);
            b = y1 + ((454 * u) >> 8);
            *dst++ = r < 0 ? 0 : (r > 255 ? 255 : r);
            *dst++ = g < 0 ? 0 : (g > 255 ? 255 : g);
            *dst++ = b < 0 ? 0 : (b > 255 ? 255 : b);
            *dst++ = 255;
        }
        if (x < width) {
            int y0 = src[0];
            int u = src[1] - 128;
            int r = y0 + 0;
            int g = y0 - ((88 * u) >> 8);
            int b = y0 + ((454 * u) >> 8);
            *dst++ = r < 0 ? 0 : (r > 255 ? 255 : r);
            *dst++ = g < 0 ? 0 : (g > 255 ? 255 : g);
            *dst++ = b < 0 ? 0 : (b > 255 ? 255 : b);
            *dst++ = 255;
        }
    }
    return rgba;
}

static int upload_rgba(
    consumer_sdl self, pl_gpu gpu, const uint8_t *rgba, int width, int height, struct pl_frame *src)
{
    struct pl_plane_data data = {
        .type = PL_FMT_UNORM,
        .width = width,
        .height = height,
        .component_size = {8, 8, 8, 8},
        .component_map = {0, 1, 2, 3},
        .pixel_stride = 4,
        .row_stride = (size_t) width * 4,
        .pixels = rgba,
    };

    memset(src, 0, sizeof(*src));
    if (!pl_upload_plane(gpu, &src->planes[0], &self->upload_tex, &data))
        return 1;
    src->num_planes = 1;
    src->repr = pl_color_repr_rgb;
    src->color = pl_color_space_srgb;
    return 0;
}

static int present_image(consumer_sdl self,
                         mlt_frame frame,
                         uint8_t *image,
                         mlt_image_format format,
                         int width,
                         int height)
{
    pl_gpu gpu = placebo_gpu_get();
    pl_renderer renderer;
    struct pl_swapchain_frame swframe;
    struct pl_frame src;
    struct pl_frame dst;
    struct pl_render_params params;
    int drawable_w = 0;
    int drawable_h = 0;
    int mapped = 0;
    uint8_t *owned = NULL;
    const uint8_t *pixels = image;
    int fbo_h;

    if (!gpu || !self->swapchain || !image || width < 1 || height < 1)
        return 1;
    SDL_Vulkan_GetDrawableSize(self->sdl_window, &drawable_w, &drawable_h);
    if (drawable_w < 1 || drawable_h < 1)
        return 0;

    renderer = placebo_renderer_get();
    if (!renderer)
        return 1;

    /* CPU conversion takes placebo_render_lock itself. Do it before this
       function locks. A placebo texture is only a pointer here. Vulkan wrap
       and the swapchain touch the shared pl_gpu, so they stay inside the lock
       where the real_time=1 read-ahead thread also renders. */
    memset(&src, 0, sizeof(src));
    if (format != mlt_image_vulkan && !placebo_frame_is_tex(frame, format)
        && format != mlt_image_rgba) {
        mlt_image_format wanted = mlt_image_rgba;
        if (mlt_frame_convert_image(frame, &image, &format, wanted)) {
            if (format == mlt_image_yuv422) {
                owned = yuv422_to_rgba(image, width, height);
                if (!owned)
                    return 1;
                pixels = owned;
                format = mlt_image_rgba;
            } else {
                return 1;
            }
        } else {
            pixels = image;
        }
    }

    placebo_render_lock();
    if (!pl_swapchain_resize(self->swapchain, &drawable_w, &drawable_h)) {
        mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent), "swapchain resize failed\n");
        if (owned)
            mlt_pool_release(owned);
        placebo_render_unlock();
        return 1;
    }
    if (format == mlt_image_vulkan) {
        if (!placebo_map_vulkan_image(gpu, image, &src)) {
            if (owned)
                mlt_pool_release(owned);
            placebo_render_unlock();
            return 1;
        }
        mapped = 1;
    } else if (placebo_frame_is_tex(frame, format)) {
        struct pl_color_space color;
        pl_fmt fmt = NULL;
        pl_tex tex = placebo_image_get_tex(image);

        if (!tex) {
            if (owned)
                mlt_pool_release(owned);
            placebo_render_unlock();
            return 1;
        }
        placebo_tex_encoding(gpu, frame, &color, &fmt);
        (void) fmt;
        src.num_planes = 1;
        src.planes[0].texture = tex;
        src.planes[0].components = 4;
        src.planes[0].component_mapping[0] = 0;
        src.planes[0].component_mapping[1] = 1;
        src.planes[0].component_mapping[2] = 2;
        src.planes[0].component_mapping[3] = 3;
        src.repr = pl_color_repr_rgb;
        src.color = color;
    }
    if (!pl_swapchain_start_frame(self->swapchain, &swframe) || !swframe.fbo) {
        if (mapped)
            placebo_unmap_vulkan_image(gpu, &src);
        if (owned)
            mlt_pool_release(owned);
        placebo_render_unlock();
        return 0;
    }

    {
        float black[4] = {0.f, 0.f, 0.f, 1.f};
        pl_tex_clear(gpu, swframe.fbo, black);
    }

    if (!mapped && !src.planes[0].texture) {
        if (upload_rgba(self, gpu, pixels, width, height, &src))
            goto fail;
    }

    letterbox(self, frame, width, height, swframe.fbo->params.w, swframe.fbo->params.h);
    memset(&dst, 0, sizeof(dst));
    dst.num_planes = 1;
    dst.planes[0].texture = swframe.fbo;
    dst.planes[0].components = 4;
    dst.planes[0].component_mapping[0] = 0;
    dst.planes[0].component_mapping[1] = 1;
    dst.planes[0].component_mapping[2] = 2;
    dst.planes[0].component_mapping[3] = 3;
    dst.repr = swframe.color_repr;
    dst.color = swframe.color_space;
    fbo_h = swframe.fbo->params.h;
    dst.crop.x0 = self->rect_x;
    dst.crop.x1 = self->rect_x + self->rect_w;
    if (swframe.flipped) {
        dst.crop.y0 = fbo_h - (self->rect_y + self->rect_h);
        dst.crop.y1 = fbo_h - self->rect_y;
    } else {
        dst.crop.y0 = self->rect_y;
        dst.crop.y1 = self->rect_y + self->rect_h;
    }

    params = pl_render_default_params;
    if (!pl_render_image(renderer, &src, &dst, &params))
        goto fail;

    if (mapped)
        placebo_unmap_vulkan_image(gpu, &src);
    if (owned)
        mlt_pool_release(owned);
    if (!pl_swapchain_submit_frame(self->swapchain)) {
        placebo_render_unlock();
        return 1;
    }
    placebo_render_unlock();
    pl_swapchain_swap_buffers(self->swapchain);
    return 0;

fail:
    if (mapped)
        placebo_unmap_vulkan_image(gpu, &src);
    if (owned)
        mlt_pool_release(owned);
    /* start_frame holds the swapchain until submit, including on failure. */
    pl_swapchain_submit_frame(self->swapchain);
    placebo_render_unlock();
    return 1;
}

static void sdl_fill_audio(void *udata, uint8_t *stream, int len)
{
    consumer_sdl self = udata;
    double volume = mlt_properties_get_double(self->properties, "volume");

    memset(stream, 0, len);
    pthread_mutex_lock(&self->audio_mutex);
    while (self->running && len > self->audio_avail)
        pthread_cond_wait(&self->audio_cond, &self->audio_mutex);

    if (self->audio_avail >= len) {
        if (volume != 1.0)
            SDL_MixAudioFormat(stream,
                               self->audio_buffer,
                               AUDIO_F32SYS,
                               len,
                               (int) ((float) SDL_MIX_MAXVOLUME * volume));
        else
            memcpy(stream, self->audio_buffer, len);
        self->audio_avail -= len;
        memmove(self->audio_buffer, self->audio_buffer + len, self->audio_avail);
    } else if (self->audio_avail > 0) {
        SDL_MixAudioFormat(stream,
                           self->audio_buffer,
                           AUDIO_F32SYS,
                           self->audio_avail,
                           (int) ((float) SDL_MIX_MAXVOLUME * volume));
        self->audio_avail = 0;
    }
    pthread_cond_broadcast(&self->audio_cond);
    pthread_mutex_unlock(&self->audio_mutex);
}

static int consumer_play_audio(consumer_sdl self, mlt_frame frame, int init_audio, int *duration)
{
    mlt_properties properties = self->properties;
    mlt_audio_format afmt = mlt_audio_f32le;
    int channels = mlt_properties_get_int(properties, "channels");
    int frequency = mlt_properties_get_int(properties, "frequency");
    int scrub = mlt_properties_get_int(properties, "scrub_audio");
    int samples = mlt_audio_calculate_frame_samples(mlt_properties_get_double(properties, "fps"),
                                                    frequency,
                                                    self->audio_counter++);
    float *pcm;

    mlt_frame_get_audio(frame, (void **) &pcm, &afmt, &frequency, &channels, &samples);
    *duration = ((samples * 1000) / frequency);
    pcm += mlt_properties_get_int(properties, "audio_offset");

    if (mlt_properties_get_int(properties, "audio_off"))
        return 1;

    if (init_audio == 1) {
        SDL_AudioSpec request;
        SDL_AudioSpec got;
        int audio_buffer = mlt_properties_get_int(properties, "audio_buffer");
        int audio_queue_size;

        memset(&request, 0, sizeof(request));
        request.freq = frequency;
        request.format = AUDIO_F32SYS;
        request.channels = channels;
        request.samples = audio_buffer > 0 ? audio_buffer : 2048;
        request.callback = sdl_fill_audio;
        request.userdata = self;
        self->audio_dev = open_audio(&request, &got);
        if (self->audio_dev == 0) {
            mlt_log_error(MLT_CONSUMER_SERVICE(&self->parent), "SDL failed to open audio\n");
            return 2;
        }
        audio_queue_size = got.size > SDL_AUDIO_BUFFER_MIN_BYTES ? got.size
                                                                 : SDL_AUDIO_BUFFER_MIN_BYTES;
        if (ensure_buffer_capacity(&self->audio_buffer, &self->audio_buffer_size, audio_queue_size)) {
            SDL_CloseAudioDevice(self->audio_dev);
            self->audio_dev = 0;
            return 2;
        }
        self->out_channels = got.channels;
        SDL_PauseAudioDevice(self->audio_dev, 0);
        init_audio = 0;
    }

    if (init_audio == 0 && pcm) {
        mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
        int samples_copied = 0;
        int dst_stride = self->out_channels * (int) sizeof(*pcm);

        pthread_mutex_lock(&self->audio_mutex);
        while (self->running && samples_copied < samples) {
            int sample_space = (self->audio_buffer_size - self->audio_avail) / dst_stride;
            while (self->running && sample_space == 0) {
                struct timeval now;
                struct timespec tm;

                gettimeofday(&now, NULL);
                tm.tv_sec = now.tv_sec + 1;
                tm.tv_nsec = now.tv_usec * 1000;
                pthread_cond_timedwait(&self->audio_cond, &self->audio_mutex, &tm);
                sample_space = (self->audio_buffer_size - self->audio_avail) / dst_stride;
                if (sample_space == 0 && self->running) {
                    mlt_log_warning(MLT_CONSUMER_SERVICE(&self->parent), "audio timed out\n");
                    pthread_mutex_unlock(&self->audio_mutex);
                    if (self->audio_dev) {
                        SDL_CloseAudioDevice(self->audio_dev);
                        self->audio_dev = 0;
                    }
                    return 1;
                }
            }
            if (self->running) {
                int samples_to_copy = samples - samples_copied;
                int dst_bytes;

                if (samples_to_copy > sample_space)
                    samples_to_copy = sample_space;
                dst_bytes = samples_to_copy * dst_stride;
                if (scrub || mlt_properties_get_double(frame_properties, "_speed") == 1) {
                    if (channels == self->out_channels) {
                        memcpy(&self->audio_buffer[self->audio_avail], pcm, dst_bytes);
                        pcm += samples_to_copy * channels;
                    } else {
                        float *dest = (float *) &self->audio_buffer[self->audio_avail];
                        int i = samples_to_copy + 1;
                        while (--i) {
                            memcpy(dest, pcm, dst_stride);
                            pcm += channels;
                            dest += self->out_channels;
                        }
                    }
                } else {
                    memset(&self->audio_buffer[self->audio_avail], 0, dst_bytes);
                    pcm += samples_to_copy * channels;
                }
                self->audio_avail += dst_bytes;
                samples_copied += samples_to_copy;
            }
            pthread_cond_broadcast(&self->audio_cond);
        }
        pthread_mutex_unlock(&self->audio_mutex);
    }
    return init_audio;
}

static int consumer_play_video(consumer_sdl self, mlt_frame frame)
{
    mlt_properties properties = self->properties;
    mlt_image_format format = mlt_image_vulkan;
    int width = self->width;
    int height = self->height;
    uint8_t *image = NULL;
    int video_off = mlt_properties_get_int(properties, "video_off");

    if (self->running && !video_off) {
        /* start() owns the window and the swapchain. Creating either here
           races melt's SDL_PollEvent and can build a swapchain on a hidden window. */
        if (!self->swapchain)
            return 1;
        mlt_frame_get_image(frame, &image, &format, &width, &height, 0);
        if (self->running && image && width > 0 && height > 0) {
            self->width = width;
            self->height = height;
            present_image(self, frame, image, format, width, height);
        }
        mlt_events_fire(properties, "consumer-frame-show", mlt_event_data_from_frame(frame));
    } else if (self->running) {
        mlt_events_fire(properties, "consumer-frame-show", mlt_event_data_from_frame(frame));
    }
    return 0;
}

static void *video_thread(void *arg)
{
    consumer_sdl self = arg;
    struct timeval now;
    int64_t start = 0;
    int64_t elapsed = 0;
    struct timespec tm;
    mlt_frame next = NULL;
    mlt_properties properties = NULL;
    double speed = 0;
    int real_time = mlt_properties_get_int(self->properties, "real_time");

    gettimeofday(&now, NULL);
    start = (int64_t) now.tv_sec * 1000000 + now.tv_usec;

    while (self->running) {
        pthread_mutex_lock(&self->video_mutex);
        next = mlt_deque_pop_front(self->queue);
        while (next == NULL && self->running) {
            pthread_cond_wait(&self->video_cond, &self->video_mutex);
            next = mlt_deque_pop_front(self->queue);
        }
        pthread_mutex_unlock(&self->video_mutex);

        if (!self->running || next == NULL)
            break;

        properties = MLT_FRAME_PROPERTIES(next);
        speed = mlt_properties_get_double(properties, "_speed");
        gettimeofday(&now, NULL);
        elapsed = ((int64_t) now.tv_sec * 1000000 + now.tv_usec) - start;

        /* Same clock as the sdl2 consumer. real_time=1 leaves rendered=0
           when the read-ahead skips a frame. Drawing it here would run
           get_image on this thread and miss the next deadline. */
        if (mlt_properties_get_int(properties, "rendered") == 1 && self->running) {
            int64_t scheduled = mlt_properties_get_int(properties, "playtime");
            int64_t difference = scheduled - elapsed;

            if (real_time && (difference > 20000 && speed == 1.0)) {
                tm.tv_sec = difference / 1000000;
                tm.tv_nsec = (difference % 1000000) * 500;
                nanosleep(&tm, NULL);
            }

            if (!real_time
                || (difference > -10000 || speed != 1.0 || mlt_deque_count(self->queue) < 2))
                consumer_play_video(self, next);

            if (real_time && (mlt_deque_count(self->queue) == 0 && speed == 1.0)) {
                gettimeofday(&now, NULL);
                start = ((int64_t) now.tv_sec * 1000000 + now.tv_usec) - scheduled + 20000;
            }
        } else {
            static int dropped = 0;
            mlt_log_info(MLT_CONSUMER_SERVICE(&self->parent), "dropped video frame %d\n", ++dropped);
        }

        mlt_frame_close(next);
        next = NULL;
    }

    if (next)
        mlt_frame_close(next);
    mlt_consumer_stopped(&self->parent);
    return NULL;
}

static void *consumer_thread(void *arg)
{
    consumer_sdl self = arg;
    mlt_consumer consumer = &self->parent;
    int terminate_on_pause = mlt_properties_get_int(MLT_CONSUMER_PROPERTIES(consumer),
                                                    "terminate_on_pause");
    int terminated = 0;
    pthread_t thread;
    int init_audio = 1;
    int init_video = 1;
    mlt_frame frame = NULL;
    int duration = 0;
    int64_t playtime = 0;
    struct timespec tm = {0, 100000};

    while (self->running) {
        frame = !terminated ? mlt_consumer_rt_frame(consumer) : NULL;
        if (terminate_on_pause && frame)
            terminated = mlt_properties_get_double(MLT_FRAME_PROPERTIES(frame), "_speed") == 0.0;

        if (frame) {
            init_audio = consumer_play_audio(self, frame, init_audio, &duration);
            if (init_video) {
                pthread_create(&thread, NULL, video_thread, self);
                init_video = 0;
            }
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "playtime", playtime);
            while (self->running && mlt_deque_count(self->queue) > 15)
                nanosleep(&tm, NULL);

            pthread_mutex_lock(&self->video_mutex);
            if (self->is_purge) {
                mlt_frame_close(frame);
                frame = NULL;
                self->is_purge = 0;
            } else {
                mlt_deque_push_back(self->queue, frame);
                pthread_cond_broadcast(&self->video_cond);
            }
            pthread_mutex_unlock(&self->video_mutex);
            playtime += (duration * 1000);
        } else if (terminated) {
            if (init_video || mlt_deque_count(self->queue) == 0)
                break;
            nanosleep(&tm, NULL);
        }
    }

    self->running = 0;
    if (init_video == 0) {
        pthread_mutex_lock(&self->video_mutex);
        pthread_cond_broadcast(&self->video_cond);
        pthread_mutex_unlock(&self->video_mutex);
        pthread_join(thread, NULL);
    }
    while (mlt_deque_count(self->queue))
        mlt_frame_close(mlt_deque_pop_back(self->queue));

    pthread_mutex_lock(&self->audio_mutex);
    self->audio_avail = 0;
    pthread_cond_broadcast(&self->audio_cond);
    pthread_mutex_unlock(&self->audio_mutex);
    return NULL;
}

static int consumer_start(mlt_consumer parent)
{
    consumer_sdl self = parent->child;
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(parent);

    if (self->running)
        return 0;

    consumer_stop(parent);
    self->running = 1;
    self->joined = 0;

    if (!mlt_properties_get_int(properties, "resolution")) {
        if (mlt_properties_get_int(properties, "width") > 0)
            self->width = mlt_properties_get_int(properties, "width");
        if (mlt_properties_get_int(properties, "height") > 0)
            self->height = mlt_properties_get_int(properties, "height");
    }
    if (mlt_properties_get_int(properties, "resolution")) {
        self->window_width = self->width;
        self->window_height = self->height;
    } else {
        double display_ratio = mlt_properties_get_double(properties, "display_ratio");
        self->window_width = (double) self->height * display_ratio + 0.5;
        self->window_height = self->height;
    }
    /* stop() destroys the window. Recreate it here, on the thread that starts
       the consumer. melt polls SDL on this thread afterwards, and a swapchain
       built while the window is still hidden makes start_frame wait forever.
       The video thread must not create either one. */
    if (!mlt_properties_get_int(properties, "video_off")) {
        if (ensure_display(self)) {
            self->running = 0;
            self->joined = 1;
            return 1;
        }
        SDL_SetWindowSize(self->sdl_window, self->window_width, self->window_height);
        if (mlt_properties_get_int(properties, "fullscreen"))
            SDL_SetWindowFullscreen(self->sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowWindow(self->sdl_window);
        SDL_PumpEvents();
        if (!self->swapchain && create_swapchain(self)) {
            self->running = 0;
            self->joined = 1;
            return 1;
        }
    }

    if (!mlt_properties_get_int(properties, "audio_off"))
        SDL_InitSubSystem(SDL_INIT_AUDIO);

    pthread_create(&self->thread, NULL, consumer_thread, self);
    return 0;
}

static int consumer_stop(mlt_consumer parent)
{
    consumer_sdl self = parent->child;

    if (self->joined)
        return 0;

    self->joined = 1;
    self->running = 0;
    pthread_mutex_lock(&self->audio_mutex);
    pthread_cond_broadcast(&self->audio_cond);
    pthread_mutex_unlock(&self->audio_mutex);
    pthread_mutex_lock(&self->video_mutex);
    pthread_cond_broadcast(&self->video_cond);
    pthread_mutex_unlock(&self->video_mutex);
    pthread_join(self->thread, NULL);

    if (self->audio_dev) {
        SDL_CloseAudioDevice(self->audio_dev);
        self->audio_dev = 0;
    }
    destroy_display(self);
    return 0;
}

static int consumer_is_stopped(mlt_consumer parent)
{
    consumer_sdl self = parent->child;
    return !self->running;
}

static void consumer_purge(mlt_consumer parent)
{
    consumer_sdl self = parent->child;

    if (!self->running)
        return;
    pthread_mutex_lock(&self->video_mutex);
    while (mlt_deque_count(self->queue))
        mlt_frame_close(mlt_deque_pop_back(self->queue));
    self->is_purge = 1;
    pthread_cond_broadcast(&self->video_cond);
    pthread_mutex_unlock(&self->video_mutex);
}

static void consumer_close(mlt_consumer parent)
{
    consumer_sdl self = parent->child;

    consumer_stop(parent);
    destroy_display(self);
    mlt_consumer_close(parent);
    if (self->queue)
        mlt_deque_close(self->queue);
    pthread_mutex_destroy(&self->audio_mutex);
    pthread_cond_destroy(&self->audio_cond);
    pthread_mutex_destroy(&self->video_mutex);
    pthread_cond_destroy(&self->video_cond);
    free(self->audio_buffer);
    free(self);
}

mlt_consumer consumer_placebo_sdl_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg)
{
    consumer_sdl self = calloc(1, sizeof(*self));
    (void) profile;
    (void) type;
    (void) id;

    if (self && mlt_consumer_init(&self->parent, self, profile) == 0) {
        mlt_consumer parent = &self->parent;

        self->queue = mlt_deque_init();
        parent->close = consumer_close;
        parent->start = consumer_start;
        parent->stop = consumer_stop;
        parent->is_stopped = consumer_is_stopped;
        parent->purge = consumer_purge;
        self->properties = MLT_CONSUMER_PROPERTIES(parent);
        self->joined = 1;

        pthread_mutex_init(&self->audio_mutex, NULL);
        pthread_cond_init(&self->audio_cond, NULL);
        pthread_mutex_init(&self->video_mutex, NULL);
        pthread_cond_init(&self->video_cond, NULL);

        mlt_properties_set_double(self->properties, "volume", 1.0);
        mlt_properties_set(self->properties, "rescale", "bilinear");
        mlt_properties_set(self->properties, "consumer.deinterlacer", "onefield");
        mlt_properties_set_int(self->properties, "top_field_first", -1);
        mlt_properties_set_int(self->properties, "buffer", 2);
        mlt_properties_set_int(self->properties, "audio_buffer", 2048);
        mlt_properties_set_int(self->properties, "scrub_audio", 1);
        mlt_properties_set(self->properties, "mlt_image_format", "vulkan");

        if (arg && sscanf(arg, "%dx%d", &self->width, &self->height) == 2) {
            mlt_properties_set_int(self->properties, "resolution", 1);
            self->window_width = self->width;
            self->window_height = self->height;
        } else {
            self->width = mlt_properties_get_int(self->properties, "width");
            self->height = mlt_properties_get_int(self->properties, "height");
            self->window_width = self->width;
            self->window_height = self->height;
        }

        mlt_events_register(self->properties, "consumer-frame-show");

        /* Publish a presentable device before returning. The swapchain itself
           is created in start(), after the window is shown. */
        if (create_window(self) || init_device(self) || ensure_surface(self)) {
            mlt_consumer_close(parent);
            return NULL;
        }
        return parent;
    }

    free(self);
    return NULL;
}
