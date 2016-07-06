/*
 * filter_glsl_manager.cpp
 * Copyright (C) 2011-2012 Christophe Thommeret <hftom@free.fr>
 * Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
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

#include <stdlib.h>
#include <string>
#include "filter_glsl_manager.h"
#include <movit/init.h>
#include <movit/util.h>
#include <movit/effect_chain.h>
#include <movit/resource_pool.h>
#include "mlt_movit_input.h"
#include "mlt_flip_effect.h"
#include <mlt++/MltEvent.h>
#include <mlt++/MltProducer.h>

extern "C" {
#include <framework/mlt_factory.h>
}

#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#elif defined(_WIN32)
#include <windows.h>
#include <wingdi.h>
#else
#include <GL/glx.h>
#endif

using namespace movit;

void dec_ref_and_delete(GlslManager *p)
{
	if (p->dec_ref() == 0) {
		delete p;
	}
}

GlslManager::GlslManager()
	: Mlt::Filter( mlt_filter_new() )
	, resource_pool(new ResourcePool())
	, pbo(0)
	, initEvent(0)
	, closeEvent(0)
	, prev_sync(NULL)
{
	mlt_filter filter = get_filter();
	if ( filter ) {
		// Set the mlt_filter child in case we choose to override virtual functions.
		filter->child = this;
		add_ref(mlt_global_properties());

		mlt_events_register( get_properties(), "init glsl", NULL );
		mlt_events_register( get_properties(), "close glsl", NULL );
		initEvent = listen("init glsl", this, (mlt_listener) GlslManager::onInit);
		closeEvent = listen("close glsl", this, (mlt_listener) GlslManager::onClose);
	}
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
		glDeleteSync( prev_sync );
	}
	while (syncs_to_delete.count() > 0) {
		GLsync sync = (GLsync) syncs_to_delete.pop_front();
		glDeleteSync( sync );
	}
	delete resource_pool;
}

void GlslManager::add_ref(mlt_properties properties)
{
	inc_ref();
	mlt_properties_set_data(properties, "glslManager", this, 0,
	    (mlt_destructor) dec_ref_and_delete, NULL);
}

GlslManager* GlslManager::get_instance()
{
	return (GlslManager*) mlt_properties_get_data(mlt_global_properties(), "glslManager", 0);
}

glsl_texture GlslManager::get_texture(int width, int height, GLint internal_format)
{
	lock();
	for (int i = 0; i < texture_list.count(); ++i) {
		glsl_texture tex = (glsl_texture) texture_list.peek(i);
		if (!tex->used && (tex->width == width) && (tex->height == height) && (tex->internal_format == internal_format)) {
			glBindTexture(GL_TEXTURE_2D, tex->texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindTexture( GL_TEXTURE_2D, 0);
			tex->used = 1;
			unlock();
			return tex;
		}
	}
	unlock();

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

	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D( GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_2D, 0 );

	gtex->texture = tex;
	gtex->width = width;
	gtex->height = height;
	gtex->internal_format = internal_format;
	gtex->used = 1;
	lock();
	texture_list.push_back(gtex);
	unlock();
	return gtex;
}

void GlslManager::release_texture(glsl_texture texture)
{
	texture->used = 0;
}

void GlslManager::delete_sync(GLsync sync)
{
	// We do not know which thread we are called from, and we can only
	// delete this if we are in one with a valid OpenGL context.
	// Thus, store it for later deletion in render_frame_texture().
	GlslManager* g = GlslManager::get_instance();
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
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo->pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, size, NULL, GL_STREAM_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
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

void GlslManager::onInit( mlt_properties owner, GlslManager* filter )
{
	mlt_log_debug( filter->get_service(), "%s\n", __FUNCTION__ );
#ifdef _WIN32
	std::string path = std::string(mlt_environment("MLT_APPDIR")).append("\\share\\movit");
#elif defined(__APPLE__) && defined(RELOCATABLE)
	std::string path = std::string(mlt_environment("MLT_APPDIR")).append("/share/movit");
#else
	std::string path = std::string(getenv("MLT_MOVIT_PATH") ? getenv("MLT_MOVIT_PATH") : SHADERDIR);
#endif
	bool success = init_movit( path, mlt_log_get_level() == MLT_LOG_DEBUG? MOVIT_DEBUG_ON : MOVIT_DEBUG_OFF );
	filter->set( "glsl_supported", success );
}

void GlslManager::onClose( mlt_properties owner, GlslManager *filter )
{
	filter->cleanupContext();
}

void GlslManager::onServiceChanged( mlt_properties owner, mlt_service aservice )
{
	Mlt::Service service( aservice );
	service.lock();
	service.set( "movit chain", NULL, 0 );
	service.unlock();
}

void GlslManager::onPropertyChanged( mlt_properties owner, mlt_service service, const char* property )
{
	if ( property && std::string( property ) == "disable" )
		onServiceChanged( owner, service );
}

extern "C" {

mlt_filter filter_glsl_manager_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	GlslManager* g = GlslManager::get_instance();
	if (g)
		g->inc_ref();
	else
		g = new GlslManager();
	return g->get_filter();
}

} // extern "C"

static void deleteChain( GlslChain* chain )
{
	// The Input* is owned by the EffectChain, but the MltInput* is not.
	// Thus, we have to delete it here.
	for (std::map<mlt_producer, MltInput*>::iterator input_it = chain->inputs.begin();
	     input_it != chain->inputs.end();
	     ++input_it) {
		delete input_it->second;
	}
	delete chain->effect_chain;
	delete chain;
}
	
void* GlslManager::get_frame_specific_data( mlt_service service, mlt_frame frame, const char *key, int *length )
{
	const char *unique_id = mlt_properties_get( MLT_SERVICE_PROPERTIES(service), "_unique_id" );
	char buf[256];
	snprintf( buf, sizeof(buf), "%s_%s", key, unique_id );
	return mlt_properties_get_data( MLT_FRAME_PROPERTIES(frame), buf, length );
}

int GlslManager::set_frame_specific_data( mlt_service service, mlt_frame frame, const char *key, void *value, int length, mlt_destructor destroy, mlt_serialiser serialise )
{
	const char *unique_id = mlt_properties_get( MLT_SERVICE_PROPERTIES(service), "_unique_id" );
	char buf[256];
	snprintf( buf, sizeof(buf), "%s_%s", key, unique_id );
	return mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), buf, value, length, destroy, serialise );
}

void GlslManager::set_chain( mlt_service service, GlslChain* chain )
{
	mlt_properties_set_data( MLT_SERVICE_PROPERTIES(service), "_movit chain", chain, 0, (mlt_destructor) deleteChain, NULL );
}

GlslChain* GlslManager::get_chain( mlt_service service )
{
	return (GlslChain*) mlt_properties_get_data( MLT_SERVICE_PROPERTIES(service), "_movit chain", NULL );
}
	
Effect* GlslManager::get_effect( mlt_service service, mlt_frame frame )
{
	return (Effect*) get_frame_specific_data( service, frame, "_movit effect", NULL );
}

Effect* GlslManager::set_effect( mlt_service service, mlt_frame frame, Effect* effect )
{
	set_frame_specific_data( service, frame, "_movit effect", effect, 0, NULL, NULL );
	return effect;
}

MltInput* GlslManager::get_input( mlt_producer producer, mlt_frame frame )
{
	return (MltInput*) get_frame_specific_data( MLT_PRODUCER_SERVICE(producer), frame, "_movit input", NULL );
}

MltInput* GlslManager::set_input( mlt_producer producer, mlt_frame frame, MltInput* input )
{
	set_frame_specific_data( MLT_PRODUCER_SERVICE(producer), frame, "_movit input", input, 0, NULL, NULL );
	return input;
}

uint8_t* GlslManager::get_input_pixel_pointer( mlt_producer producer, mlt_frame frame )
{
	return (uint8_t*) get_frame_specific_data( MLT_PRODUCER_SERVICE(producer), frame, "_movit input pp", NULL );
}

uint8_t* GlslManager::set_input_pixel_pointer( mlt_producer producer, mlt_frame frame, uint8_t* image )
{
	set_frame_specific_data( MLT_PRODUCER_SERVICE(producer), frame, "_movit input pp", image, 0, NULL, NULL );
	return image;
}

mlt_service GlslManager::get_effect_input( mlt_service service, mlt_frame frame )
{
	return (mlt_service) get_frame_specific_data( service, frame, "_movit effect input", NULL );
}

void GlslManager::set_effect_input( mlt_service service, mlt_frame frame, mlt_service input_service )
{
	set_frame_specific_data( service, frame, "_movit effect input", input_service, 0, NULL, NULL );
}

void GlslManager::get_effect_secondary_input( mlt_service service, mlt_frame frame, mlt_service *input_service, mlt_frame *input_frame)
{
	*input_service = (mlt_service) get_frame_specific_data( service, frame, "_movit effect secondary input", NULL );
	*input_frame = (mlt_frame) get_frame_specific_data( service, frame, "_movit effect secondary input frame", NULL );
}

void GlslManager::set_effect_secondary_input( mlt_service service, mlt_frame frame, mlt_service input_service, mlt_frame input_frame )
{
	set_frame_specific_data( service, frame, "_movit effect secondary input", input_service, 0, NULL, NULL );
	set_frame_specific_data( service, frame, "_movit effect secondary input frame", input_frame, 0, NULL, NULL );
}

void GlslManager::get_effect_third_input( mlt_service service, mlt_frame frame, mlt_service *input_service, mlt_frame *input_frame)
{
	*input_service = (mlt_service) get_frame_specific_data( service, frame, "_movit effect third input", NULL );
	*input_frame = (mlt_frame) get_frame_specific_data( service, frame, "_movit effect third input frame", NULL );
}

void GlslManager::set_effect_third_input( mlt_service service, mlt_frame frame, mlt_service input_service, mlt_frame input_frame )
{
	set_frame_specific_data( service, frame, "_movit effect third input", input_service, 0, NULL, NULL );
	set_frame_specific_data( service, frame, "_movit effect third input frame", input_frame, 0, NULL, NULL );
}

int GlslManager::render_frame_texture(EffectChain *chain, mlt_frame frame, int width, int height, uint8_t **image)
{
	glsl_texture texture = get_texture( width, height, GL_RGBA8 );
	if (!texture) {
		return 1;
	}

	GLuint fbo;
	glGenFramebuffers( 1, &fbo );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, fbo );
	check_error();
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();

	lock();
	while (syncs_to_delete.count() > 0) {
		GLsync sync = (GLsync) syncs_to_delete.pop_front();
		glDeleteSync( sync );
	}
	unlock();

	// Make sure we never have more than one frame pending at any time.
	// This ensures we do not swamp the GPU with so much work
	// that we cannot actually display the frames we generate.
	if (prev_sync != NULL) {
		glFlush();
		glClientWaitSync( prev_sync, 0, GL_TIMEOUT_IGNORED );
		glDeleteSync( prev_sync );
	}
	chain->render_to_fbo( fbo, width, height );
	prev_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	GLsync sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );

	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();
	glDeleteFramebuffers( 1, &fbo );
	check_error();

	*image = (uint8_t*) &texture->texture;
	mlt_frame_set_image( frame, *image, 0, NULL );
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), "movit.convert.texture", texture, 0,
		(mlt_destructor) GlslManager::release_texture, NULL );
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), "movit.convert.fence", sync, 0,
		(mlt_destructor) GlslManager::delete_sync, NULL );

	return 0;
}

int GlslManager::render_frame_rgba(EffectChain *chain, mlt_frame frame, int width, int height, uint8_t **image)
{
	glsl_texture texture = get_texture( width, height, GL_RGBA8 );
	if (!texture) {
		return 1;
	}

	// Use a PBO to hold the data we read back with glReadPixels().
	// (Intel/DRI goes into a slow path if we don't read to PBO.)
	int img_size = width * height * 4;
	glsl_pbo pbo = get_pbo( img_size );
	if (!pbo) {
		release_texture(texture);
		return 1;
	}

	// Set the FBO
	GLuint fbo;
	glGenFramebuffers( 1, &fbo );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, fbo );
	check_error();
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();

	chain->render_to_fbo( fbo, width, height );

	// Read FBO into PBO
	glBindFramebuffer( GL_FRAMEBUFFER, fbo );
	check_error();
	glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, pbo->pbo );
	check_error();
	glBufferData( GL_PIXEL_PACK_BUFFER_ARB, img_size, NULL, GL_STREAM_READ );
	check_error();
	glReadPixels( 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0) );
	check_error();

	// Copy from PBO
	uint8_t* buf = (uint8_t*) glMapBuffer( GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY );
	check_error();
	*image = (uint8_t*) mlt_pool_alloc( img_size );
	mlt_frame_set_image( frame, *image, img_size, mlt_pool_release );
	memcpy( *image, buf, img_size );

	// Convert BGRA to RGBA
	register uint8_t *p = *image;
	register int n = width * height + 1;
	while ( --n ) {
		uint8_t b = p[0];
		*p = p[2]; p += 2;
		*p = b; p += 2;
	}

	// Release PBO and FBO
	glUnmapBuffer( GL_PIXEL_PACK_BUFFER_ARB );
	check_error();
	glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, 0 );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();
	glBindTexture( GL_TEXTURE_2D, 0 );
	check_error();
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), "movit.convert.texture", texture, 0,
		(mlt_destructor) GlslManager::release_texture, NULL);
	glDeleteFramebuffers( 1, &fbo );
	check_error();

	return 0;
}

void GlslManager::lock_service( mlt_frame frame )
{
	Mlt::Producer producer( mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) ) );
	producer.lock();
}

void GlslManager::unlock_service( mlt_frame frame )
{
	Mlt::Producer producer( mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) ) );
	producer.unlock();
}
