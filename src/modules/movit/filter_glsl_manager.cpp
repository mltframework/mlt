/*
 * filter_glsl_manager.cpp
 * Copyright (C) 2011-2012 Christophe Thommeret <hftom@free.fr>
 * Copyright (C) 2013-2025 Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "filter_glsl_manager.h"
#include "mlt_movit_input.h"
#include <effect_chain.h>
#include <init.h>
#include <math.h>
#include <mlt++/MltEvent.h>
#include <mlt++/MltProducer.h>
#include <resource_pool.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <util.h>

extern "C" {
#include <framework/mlt_factory.h>
}

// Do not include platform-specific GL headers here.
// Movit includes and config will provide the appropriate GL headers.
// Minimum OpenGL version is desktop core profile 3 or OpenGL ES 3.

// Texture pool may cause frames to appear out-of-order with NVIDIA
// threaded optimizations.
#define USE_TEXTURE_POOL 1

using namespace movit;

void dec_ref_and_delete(GlslManager *p)
{
    if (p->dec_ref() == 0) {
        delete p;
    }
}

GlslManager::GlslManager()
    : Mlt::Filter(mlt_filter_new())
    , resource_pool(new ResourcePool())
    , pbo(0)
    , initEvent(0)
    , closeEvent(0)
    , prev_sync(NULL)
{
    mlt_filter filter = get_filter();
    if (filter) {
        // Set the mlt_filter child in case we choose to override virtual functions.
        filter->child = this;
        add_ref(mlt_global_properties());

        mlt_events_register(get_properties(), "init glsl");
        mlt_events_register(get_properties(), "close glsl");
        initEvent = listen("init glsl", this, (mlt_listener) GlslManager::onInit);
        closeEvent = listen("close glsl", this, (mlt_listener) GlslManager::onClose);
    }
    // Initialize runtime GL capabilities for readbacks.
    init_ycbcr_runtime_caps();
}

GlslManager::~GlslManager()
{
    mlt_log_debug(get_service(), "%s\n", __FUNCTION__);
    cleanupContext();
    // XXX If there is still a frame holding a reference to a texture after this
    // destructor is called, then it will crash in release_texture().
    //	while (texture_list.peek_back())
    //		delete (glsl_texture) texture_list.pop_back();
    delete initEvent;
    delete closeEvent;
    if (prev_sync != NULL) {
        glDeleteSync(prev_sync);
    }
    while (syncs_to_delete.count() > 0) {
        GLsync sync = (GLsync) syncs_to_delete.pop_front();
        glDeleteSync(sync);
    }
    delete resource_pool;
}

void GlslManager::add_ref(mlt_properties properties)
{
    inc_ref();
    mlt_properties_set_data(properties,
                            "glslManager",
                            this,
                            0,
                            (mlt_destructor) dec_ref_and_delete,
                            NULL);
}

GlslManager *GlslManager::get_instance()
{
    return (GlslManager *) mlt_properties_get_data(mlt_global_properties(), "glslManager", 0);
}

glsl_texture GlslManager::get_texture(int width, int height, GLint internal_format)
{
    if (width < 1 || height < 1) {
        return NULL;
    }

#if USE_TEXTURE_POOL
    lock();
    for (int i = 0; i < texture_list.count(); ++i) {
        glsl_texture tex = (glsl_texture) texture_list.peek(i);
        if (!tex->used && (tex->width == width) && (tex->height == height)
            && (tex->internal_format == internal_format)) {
            glBindTexture(GL_TEXTURE_2D, tex->texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
            tex->used = 1;
            unlock();
            return tex;
        }
    }
    unlock();
#endif

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (!tex) {
        return NULL;
    }

    glsl_texture gtex = new glsl_texture_s;
    if (!gtex) {
        glDeleteTextures(1, &tex);
        return NULL;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    GLenum alloc_type = GL_UNSIGNED_BYTE;
    if (internal_format == GL_RGBA16F) {
        alloc_type = GL_HALF_FLOAT;
    } else if (internal_format == GL_RGBA16) {
        alloc_type = GL_UNSIGNED_SHORT;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, alloc_type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    gtex->texture = tex;
    gtex->width = width;
    gtex->height = height;
    gtex->internal_format = internal_format;
    gtex->used = 1;
#if USE_TEXTURE_POOL
    lock();
    texture_list.push_back(gtex);
    unlock();
#endif
    return gtex;
}

void GlslManager::release_texture(glsl_texture texture)
{
#if USE_TEXTURE_POOL
    texture->used = 0;
#else
    GlslManager *g = GlslManager::get_instance();
    g->lock();
    g->texture_list.push_back(texture);
    g->unlock();
#endif
}

void GlslManager::delete_sync(GLsync sync)
{
    // We do not know which thread we are called from, and we can only
    // delete this if we are in one with a valid OpenGL context.
    // Thus, store it for later deletion in render_frame_texture().
    GlslManager *g = GlslManager::get_instance();
    g->lock();
    g->syncs_to_delete.push_back(sync);
    g->unlock();
}

glsl_pbo GlslManager::get_pbo(int size)
{
    lock();
    if (!pbo) {
        GLuint pb = 0;
        glGenBuffers(1, &pb);
        if (!pb) {
            unlock();
            return NULL;
        }

        pbo = new glsl_pbo_s;
        if (!pbo) {
            glDeleteBuffers(1, &pb);
            unlock();
            return NULL;
        }
        pbo->pbo = pb;
        pbo->size = 0;
    }
    if (size > pbo->size) {
        // This PBO is used for readback, so use PACK target consistently.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        pbo->size = size;
    }
    unlock();
    return pbo;
}

void GlslManager::cleanupContext()
{
    lock();
    while (texture_list.peek_back()) {
        glsl_texture texture = (glsl_texture) texture_list.peek_back();
        glDeleteTextures(1, &texture->texture);
        delete texture;
        texture_list.pop_back();
    }
    if (pbo) {
        glDeleteBuffers(1, &pbo->pbo);
        delete pbo;
        pbo = 0;
    }
    unlock();
}

void GlslManager::onInit(mlt_properties owner, GlslManager *filter, mlt_event_data)
{
    mlt_log_debug(filter->get_service(), "%s\n", __FUNCTION__);
#ifdef _WIN32
    std::string path = std::string(mlt_environment("MLT_APPDIR")).append("\\share\\movit");
#elif defined(RELOCATABLE)
#ifdef __APPLE__
    std::string path = std::string(mlt_environment("MLT_APPDIR")).append("/Resources/movit");
#else
    std::string path = std::string(mlt_environment("MLT_APPDIR")).append("/share/movit");
#endif
#else
    std::string path = std::string(getenv("MLT_MOVIT_PATH") ? getenv("MLT_MOVIT_PATH") : SHADERDIR);
#endif
    bool success = init_movit(path,
                              mlt_log_get_level() == MLT_LOG_DEBUG ? MOVIT_DEBUG_ON
                                                                   : MOVIT_DEBUG_OFF);
    filter->set("glsl_supported", success);
}

void GlslManager::onClose(mlt_properties owner, GlslManager *filter, mlt_event_data)
{
    filter->cleanupContext();
}

extern "C" {

mlt_filter filter_glsl_manager_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg)
{
    GlslManager *g = GlslManager::get_instance();
    if (g)
        g->inc_ref();
    else
        g = new GlslManager();
    return g->get_filter();
}
static bool has_extension(const char *ext_list, const char *ext)
{
    if (!ext_list || !ext)
        return false;
    const char *p = ext_list;
    size_t elen = strlen(ext);
    while ((p = strstr(p, ext))) {
        // Ensure whole token match (space or end).
        const char c = p[elen];
        if ((p == ext_list || *(p - 1) == ' ') && (c == ' ' || c == '\0'))
            return true;
        p += elen;
    }
    return false;
}

void GlslManager::init_ycbcr_runtime_caps()
{
    if (ycbcr_caps_initialized)
        return;

    // Default to desktop-friendly path: 16-bit normalized.
    ycbcr_internal_format = GL_RGBA16;
    ycbcr_read_type = GL_UNSIGNED_SHORT;
    ycbcr_needs_float_to_u16 = false;

    const char *version = (const char *) glGetString(GL_VERSION);
    const char *extensions = (const char *) glGetString(GL_EXTENSIONS);
    const char *renderer = (const char *) glGetString(GL_RENDERER);
    const char *vendor = (const char *) glGetString(GL_VENDOR);
    (void) renderer;
    (void) vendor; // Not used currently.

    bool is_gles = false;
    if (version
        && (strstr(version, "OpenGL ES") || strstr(version, "OpenGL ES-CM")
            || strstr(version, "OpenGL ES-CL"))) {
        is_gles = true;
    }

    // Log basic GL context facts.
    mlt_log_verbose(get_service(),
                    "GLSL caps: version='%s' renderer='%s' vendor='%s' is_gles=%d\n",
                    version ? version : "",
                    renderer ? renderer : "",
                    vendor ? vendor : "",
                    (int) is_gles);

    if (is_gles) {
        // On ES, RGBA16 as a color renderable internal format is not guaranteed.
        // Prefer RGBA16F if color buffer half float is available; read back as float and convert.
        bool has_half_float_color = false;
        if (extensions) {
            has_half_float_color = has_extension(extensions, "GL_EXT_color_buffer_half_float")
                                   || has_extension(extensions, "GL_OES_texture_half_float")
                                   || has_extension(extensions, "GL_EXT_color_buffer_float");
        }
        if (has_half_float_color) {
            ycbcr_internal_format = GL_RGBA16F;
            ycbcr_read_type = GL_FLOAT;
            ycbcr_needs_float_to_u16 = true;
        } else {
            // As a fallback, try RGBA8 and still read back as bytes; later conversion to 10-bit
            // would be lossy—but we retain functionality. Keep existing defaults for now.
            ycbcr_internal_format = GL_RGBA8;
            ycbcr_read_type = GL_UNSIGNED_BYTE;
            ycbcr_needs_float_to_u16 = false;
        }
    }

    // Keep track of our initial choice before probing.
    GLint initial_internal_format = ycbcr_internal_format;
    GLenum initial_read_type = ycbcr_read_type;

    // Probe that the chosen internal format is actually color-renderable by creating
    // a tiny FBO and checking completeness. If it fails, fall back to safer formats.
    auto fbo_renders = [&](GLint internal_fmt, GLenum *out_read_type) -> bool {
        GLuint tex = 0, fbo = 0;
        glGenTextures(1, &tex);
        if (!tex)
            return false;
        glBindTexture(GL_TEXTURE_2D, tex);
        GLenum alloc_type = GL_UNSIGNED_BYTE;
        if (internal_fmt == GL_RGBA16F)
            alloc_type = GL_HALF_FLOAT;
        else if (internal_fmt == GL_RGBA16)
            alloc_type = GL_UNSIGNED_SHORT;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt, 4, 4, 0, GL_RGBA, alloc_type, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &tex);

        if (status != GL_FRAMEBUFFER_COMPLETE)
            return false;
        if (out_read_type) {
            // Prefer readback types that align with the internal format choice.
            *out_read_type = (internal_fmt == GL_RGBA16F)  ? GL_FLOAT
                             : (internal_fmt == GL_RGBA16) ? GL_UNSIGNED_SHORT
                                                           : GL_UNSIGNED_BYTE;
        }
        return true;
    };

    GLenum validated_read_type = ycbcr_read_type;
    bool used_fallback = false;
    if (!fbo_renders(ycbcr_internal_format, &validated_read_type)) {
        // Build an ordered list of fallbacks depending on context.
        GLint candidates[3];
        int n = 0;
        if (is_gles) {
            // On ES, try half-float (if extension suggests support) then RGBA8.
            if (has_extension(extensions, "GL_EXT_color_buffer_half_float")
                || has_extension(extensions, "GL_OES_texture_half_float")
                || has_extension(extensions, "GL_EXT_color_buffer_float")) {
                candidates[n++] = GL_RGBA16F;
            }
            candidates[n++] = GL_RGBA8;
        } else {
            // Desktop GL: prefer 16-bit normalized, then 16F, then 8-bit.
            candidates[n++] = GL_RGBA16;
            candidates[n++] = GL_RGBA16F;
            candidates[n++] = GL_RGBA8;
        }
        bool found = false;
        for (int i = 0; i < n; ++i) {
            GLenum tmp_type = GL_UNSIGNED_BYTE;
            if (fbo_renders(candidates[i], &tmp_type)) {
                ycbcr_internal_format = candidates[i];
                validated_read_type = tmp_type;
                found = true;
                used_fallback = true;
                break;
            }
        }
        if (!found) {
            // Last resort: RGBA8 with byte readback.
            ycbcr_internal_format = GL_RGBA8;
            validated_read_type = GL_UNSIGNED_BYTE;
            used_fallback = true;
        }
    }

    ycbcr_read_type = validated_read_type;
    ycbcr_needs_float_to_u16 = (ycbcr_read_type == GL_FLOAT);

    // Log the final choice (and whether we had to fall back from the initial one).
    mlt_log_debug(get_service(),
                  "YCbCr readback: internal=0x%x read_type=0x%x float_to_u16=%d%s\n",
                  (unsigned int) ycbcr_internal_format,
                  (unsigned int) ycbcr_read_type,
                  (int) ycbcr_needs_float_to_u16,
                  used_fallback || (initial_internal_format != ycbcr_internal_format)
                          || (initial_read_type != ycbcr_read_type)
                      ? " (fallback)"
                      : "");
    ycbcr_caps_initialized = true;
}

} // extern "C"

static void deleteChain(GlslChain *chain)
{
    // The Input* is owned by the EffectChain, but the MltInput* is not.
    // Thus, we have to delete it here.
    for (std::map<mlt_producer, MltInput *>::iterator input_it = chain->inputs.begin();
         input_it != chain->inputs.end();
         ++input_it) {
        delete input_it->second;
    }
    delete chain->effect_chain;
    delete chain;
}

void *GlslManager::get_frame_specific_data(mlt_service service,
                                           mlt_frame frame,
                                           const char *key,
                                           int *length)
{
    const char *unique_id = mlt_properties_get(MLT_SERVICE_PROPERTIES(service), "_unique_id");
    char buf[256];
    snprintf(buf, sizeof(buf), "%s_%s", key, unique_id);
    return mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), buf, length);
}

int GlslManager::set_frame_specific_data(mlt_service service,
                                         mlt_frame frame,
                                         const char *key,
                                         void *value,
                                         int length,
                                         mlt_destructor destroy,
                                         mlt_serialiser serialise)
{
    const char *unique_id = mlt_properties_get(MLT_SERVICE_PROPERTIES(service), "_unique_id");
    char buf[256];
    snprintf(buf, sizeof(buf), "%s_%s", key, unique_id);
    return mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                                   buf,
                                   value,
                                   length,
                                   destroy,
                                   serialise);
}

void GlslManager::set_chain(mlt_service service, GlslChain *chain)
{
    mlt_properties_set_data(MLT_SERVICE_PROPERTIES(service),
                            "_movit chain",
                            chain,
                            0,
                            (mlt_destructor) deleteChain,
                            NULL);
}

GlslChain *GlslManager::get_chain(mlt_service service)
{
    return (
        GlslChain *) mlt_properties_get_data(MLT_SERVICE_PROPERTIES(service), "_movit chain", NULL);
}

Effect *GlslManager::get_effect(mlt_service service, mlt_frame frame)
{
    return (Effect *) get_frame_specific_data(service, frame, "_movit effect", NULL);
}

Effect *GlslManager::set_effect(mlt_service service, mlt_frame frame, Effect *effect)
{
    set_frame_specific_data(service, frame, "_movit effect", effect, 0, NULL, NULL);
    return effect;
}

MltInput *GlslManager::get_input(mlt_producer producer, mlt_frame frame)
{
    return (MltInput *)
        get_frame_specific_data(MLT_PRODUCER_SERVICE(producer), frame, "_movit input", NULL);
}

MltInput *GlslManager::set_input(mlt_producer producer, mlt_frame frame, MltInput *input)
{
    set_frame_specific_data(MLT_PRODUCER_SERVICE(producer),
                            frame,
                            "_movit input",
                            input,
                            0,
                            NULL,
                            NULL);
    return input;
}

uint8_t *GlslManager::get_input_pixel_pointer(mlt_producer producer, mlt_frame frame)
{
    return (uint8_t *)
        get_frame_specific_data(MLT_PRODUCER_SERVICE(producer), frame, "_movit input pp", NULL);
}

uint8_t *GlslManager::set_input_pixel_pointer(mlt_producer producer, mlt_frame frame, uint8_t *image)
{
    set_frame_specific_data(MLT_PRODUCER_SERVICE(producer),
                            frame,
                            "_movit input pp",
                            image,
                            0,
                            NULL,
                            NULL);
    return image;
}

mlt_service GlslManager::get_effect_input(mlt_service service, mlt_frame frame)
{
    return (mlt_service) get_frame_specific_data(service, frame, "_movit effect input", NULL);
}

void GlslManager::set_effect_input(mlt_service service, mlt_frame frame, mlt_service input_service)
{
    set_frame_specific_data(service, frame, "_movit effect input", input_service, 0, NULL, NULL);
}

void GlslManager::get_effect_secondary_input(mlt_service service,
                                             mlt_frame frame,
                                             mlt_service *input_service,
                                             mlt_frame *input_frame)
{
    *input_service = (mlt_service)
        get_frame_specific_data(service, frame, "_movit effect secondary input", NULL);
    *input_frame = (mlt_frame)
        get_frame_specific_data(service, frame, "_movit effect secondary input frame", NULL);
}

void GlslManager::set_effect_secondary_input(mlt_service service,
                                             mlt_frame frame,
                                             mlt_service input_service,
                                             mlt_frame input_frame)
{
    set_frame_specific_data(service,
                            frame,
                            "_movit effect secondary input",
                            input_service,
                            0,
                            NULL,
                            NULL);
    set_frame_specific_data(service,
                            frame,
                            "_movit effect secondary input frame",
                            input_frame,
                            0,
                            NULL,
                            NULL);
}

void GlslManager::get_effect_third_input(mlt_service service,
                                         mlt_frame frame,
                                         mlt_service *input_service,
                                         mlt_frame *input_frame)
{
    *input_service
        = (mlt_service) get_frame_specific_data(service, frame, "_movit effect third input", NULL);
    *input_frame = (mlt_frame)
        get_frame_specific_data(service, frame, "_movit effect third input frame", NULL);
}

void GlslManager::set_effect_third_input(mlt_service service,
                                         mlt_frame frame,
                                         mlt_service input_service,
                                         mlt_frame input_frame)
{
    set_frame_specific_data(service,
                            frame,
                            "_movit effect third input",
                            input_service,
                            0,
                            NULL,
                            NULL);
    set_frame_specific_data(service,
                            frame,
                            "_movit effect third input frame",
                            input_frame,
                            0,
                            NULL,
                            NULL);
}

int GlslManager::render_frame_texture(
    EffectChain *chain, mlt_frame frame, int width, int height, uint8_t **image)
{
    if (width < 1 || height < 1) {
        return 1;
    }

    glsl_texture texture = get_texture(width, height, GL_RGBA8);
    if (!texture) {
        return 1;
    }

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    check_error();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_error();

    lock();
    while (syncs_to_delete.count() > 0) {
        GLsync sync = (GLsync) syncs_to_delete.pop_front();
        glDeleteSync(sync);
    }
#if !USE_TEXTURE_POOL
    while (texture_list.count() > 0) {
        glsl_texture texture = (glsl_texture) texture_list.pop_back();
        glDeleteTextures(1, &texture->texture);
        delete texture;
    }
#endif
    unlock();

    // Make sure we never have more than one frame pending at any time.
    // This ensures we do not swamp the GPU with so much work
    // that we cannot actually display the frames we generate.
    if (prev_sync != NULL) {
        glFlush();
        glClientWaitSync(prev_sync, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(prev_sync);
    }
    chain->render_to_fbo(fbo, width, height);
    prev_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_error();
    glDeleteFramebuffers(1, &fbo);
    check_error();

    *image = (uint8_t *) &texture->texture;
    mlt_frame_set_image(frame, *image, 0, NULL);
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                            "movit.convert.texture",
                            texture,
                            0,
                            (mlt_destructor) GlslManager::release_texture,
                            NULL);
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                            "movit.convert.fence",
                            sync,
                            0,
                            (mlt_destructor) GlslManager::delete_sync,
                            NULL);

    return 0;
}

int GlslManager::render_frame_rgba(
    EffectChain *chain, mlt_frame frame, int width, int height, uint8_t **image)
{
    if (width < 1 || height < 1) {
        return 1;
    }

    glsl_texture texture = get_texture(width, height, GL_RGBA8);
    if (!texture) {
        return 1;
    }

    // Use a PBO to hold the data we read back with glReadPixels().
    // (Intel/DRI goes into a slow path if we don't read to PBO.)
    int img_size = width * height * 4;
    glsl_pbo pbo = get_pbo(img_size);
    if (!pbo) {
        release_texture(texture);
        return 1;
    }

    // Set the FBO
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    check_error();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_error();

    chain->render_to_fbo(fbo, width, height);

    // Read FBO into PBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    check_error();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->pbo);
    check_error();
    glBufferData(GL_PIXEL_PACK_BUFFER, img_size, NULL, GL_STREAM_READ);
    check_error();
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
    check_error();

    // Copy from PBO
    uint8_t *buf = (uint8_t *) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, img_size, GL_MAP_READ_BIT);
    *image = (uint8_t *) mlt_pool_alloc(img_size);
    mlt_frame_set_image(frame, *image, img_size, mlt_pool_release);
    memcpy(*image, buf, img_size);

    // Release PBO and FBO
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    check_error();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_error();
    glBindTexture(GL_TEXTURE_2D, 0);
    check_error();
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                            "movit.convert.texture",
                            texture,
                            0,
                            (mlt_destructor) GlslManager::release_texture,
                            NULL);
    glDeleteFramebuffers(1, &fbo);
    check_error();

    return 0;
}

int GlslManager::render_frame_ycbcr(
    EffectChain *chain, mlt_frame frame, int width, int height, uint8_t **image)
{
    if (width < 1 || height < 1) {
        return 1;
    }

    // Ensure runtime capabilities are initialized.
    if (!ycbcr_caps_initialized)
        init_ycbcr_runtime_caps();

    glsl_texture texture = get_texture(width, height, ycbcr_internal_format);
    if (!texture) {
        return 1;
    }

    // Use a PBO to hold the data we read back with glReadPixels().
    // (Intel/DRI goes into a slow path if we don't read to PBO.)
    int img_size = width * height * 4 * sizeof(uint16_t);
    glsl_pbo pbo = get_pbo(img_size);
    if (!pbo) {
        release_texture(texture);
        return 1;
    }

    // Set the FBO
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    check_error();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_error();

    chain->render_to_fbo(fbo, width, height);

    // Read FBO into PBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    check_error();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->pbo);
    check_error();
    // Size depends on chosen readback format below.
    size_t pbo_size = (size_t) width * height * 4
                      * ((ycbcr_read_type == GL_FLOAT)            ? sizeof(float)
                         : (ycbcr_read_type == GL_UNSIGNED_SHORT) ? sizeof(uint16_t)
                                                                  : sizeof(uint8_t));
    glBufferData(GL_PIXEL_PACK_BUFFER, pbo_size, NULL, GL_STREAM_READ);
    check_error();
    // Read FBO; choose type based on internal format used above.
    glReadPixels(0, 0, width, height, GL_RGBA, ycbcr_read_type, BUFFER_OFFSET(0));
    check_error();

    // Copy from PBO
    // Map for read (minimum is GL 3.0 / ES 3.0, so glMapBufferRange is available).
    void *raw = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
    check_error();
    int mlt_size = mlt_image_format_size(mlt_image_yuv444p10, width, height, nullptr);
    *image = (uint8_t *) mlt_pool_alloc(mlt_size);
    mlt_frame_set_image(frame, *image, mlt_size, mlt_pool_release);
    uint8_t *planes[4];
    int strides[4];
    mlt_image_format_planes(mlt_image_yuv444p10, width, height, *image, planes, strides);
    uint16_t **p = (uint16_t **) planes;
    for (int i = 0; i < width * height; ++i) {
        if (ycbcr_read_type == GL_FLOAT) {
            float *buf = (float *) raw;
            // Convert float [0,1] to 16-bit [0,65535].
            // Clamp to avoid potential out-of-range due to FP error.
            float yf = buf[4 * i + 0];
            float uf = buf[4 * i + 1];
            float vf = buf[4 * i + 2];
            yf = yf < 0.0f ? 0.0f : (yf > 1.0f ? 1.0f : yf);
            uf = uf < 0.0f ? 0.0f : (uf > 1.0f ? 1.0f : uf);
            vf = vf < 0.0f ? 0.0f : (vf > 1.0f ? 1.0f : vf);
            p[0][i] = (uint16_t) lroundf(yf * 65535.0f);
            p[1][i] = (uint16_t) lroundf(uf * 65535.0f);
            p[2][i] = (uint16_t) lroundf(vf * 65535.0f);
        } else if (ycbcr_read_type == GL_UNSIGNED_SHORT) {
            uint16_t *buf = (uint16_t *) raw;
            p[0][i] = buf[4 * i + 0];
            p[1][i] = buf[4 * i + 1];
            p[2][i] = buf[4 * i + 2];
        } else { // GL_UNSIGNED_BYTE fallback
            uint8_t *buf = (uint8_t *) raw;
            // Scale 8-bit [0,255] to 16-bit [0,65535].
            p[0][i] = (uint16_t) (buf[4 * i + 0] * 257u);
            p[1][i] = (uint16_t) (buf[4 * i + 1] * 257u);
            p[2][i] = (uint16_t) (buf[4 * i + 2] * 257u);
        }
    }

    // Release PBO and FBO
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    check_error();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    check_error();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_error();
    glBindTexture(GL_TEXTURE_2D, 0);
    check_error();
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                            "movit.convert.texture",
                            texture,
                            0,
                            (mlt_destructor) GlslManager::release_texture,
                            NULL);
    glDeleteFramebuffers(1, &fbo);
    check_error();

    return 0;
}

void GlslManager::lock_service(mlt_frame frame)
{
    Mlt::Producer producer(mlt_producer_cut_parent(mlt_frame_get_original_producer(frame)));
    producer.lock();
}

void GlslManager::unlock_service(mlt_frame frame)
{
    Mlt::Producer producer(mlt_producer_cut_parent(mlt_frame_get_original_producer(frame)));
    producer.unlock();
}
