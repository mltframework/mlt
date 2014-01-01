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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string>
#include "glsl_manager.h"
#include <movit/init.h>
#include <movit/util.h>
#include <movit/effect_chain.h>
#include "mlt_movit_input.h"
#include "mlt_flip_effect.h"
#include <mlt++/MltEvent.h>
#include <mlt++/MltProducer.h>

extern "C" {
#include <framework/mlt_factory.h>
}

#if defined(__DARWIN__)
#include <OpenGL/OpenGL.h>
#elif defined(WIN32)
#include <wingdi.h>
#else
#include <GL/glx.h>
#endif

void deleteManager(GlslManager *p)
{
	delete p;
}

GlslManager::GlslManager()
	: Mlt::Filter( mlt_filter_new() )
	, pbo(0)
	, initEvent(0)
	, closeEvent(0)
{
	mlt_filter filter = get_filter();
	if ( filter ) {
		// Set the mlt_filter child in case we choose to override virtual functions.
		filter->child = this;
		mlt_properties_set_data(mlt_global_properties(), "glslManager", this, 0,
			(mlt_destructor) deleteManager, NULL);

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
}

GlslManager* GlslManager::get_instance()
{
	return (GlslManager*) mlt_properties_get_data(mlt_global_properties(), "glslManager", 0);
}

glsl_fbo GlslManager::get_fbo(int width, int height)
{
#if defined(__DARWIN__)
	CGLContextObj context = CGLGetCurrentContext();
#elif defined(WIN32)
	HGLRC context = wglGetCurrentContext();
#else
	GLXContext context = glXGetCurrentContext();
#endif

	lock();
	for (int i = 0; i < fbo_list.count(); ++i) {
		glsl_fbo fbo = (glsl_fbo) fbo_list.peek(i);
		if (!fbo->used && (fbo->width == width) && (fbo->height == height) && (fbo->context == context)) {
			fbo->used = 1;
			unlock();
			return fbo;
		}
	}
	unlock();

	GLuint fb = 0;
	glGenFramebuffers(1, &fb);
	if (!fb)
		return NULL;

	glsl_fbo fbo = new glsl_fbo_s;
	if (!fbo) {
		glDeleteFramebuffers(1, &fb);
		return NULL;
	}
	fbo->fbo = fb;
	fbo->width = width;
	fbo->height = height;
	fbo->used = 1;
	fbo->context = context;
	lock();
	fbo_list.push_back(fbo);
	unlock();
	return fbo;
}

void GlslManager::release_fbo(glsl_fbo fbo)
{
	fbo->used = 0;
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

	// Recycle a glsl_texture with deleted glTexture.
	glsl_texture gtex = 0;
	for (int i = 0; i < texture_list.count(); ++i) {
		glsl_texture tex = (glsl_texture) texture_list.peek(i);
		if (!tex->used && !tex->width && !tex->height) {
			gtex = tex;
			break;
		}
	}
	unlock();

	GLuint tex = 0;
	glGenTextures(1, &tex);
	if (!tex) {
		glDeleteTextures(1, &tex);
		return NULL;
	}

	if (!gtex) {
		gtex = new glsl_texture_s;
		if (!gtex)
			return NULL;
	}

	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D( GL_TEXTURE_2D, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, NULL );
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
	while (fbo_list.peek_back()) {
		glsl_fbo fbo = (glsl_fbo) fbo_list.pop_back();
		glDeleteFramebuffers(1, &fbo->fbo);
		delete fbo;
	}
	for (int i = 0; i < texture_list.count(); ++i) {
		glsl_texture texture = (glsl_texture) texture_list.peek(i);
		glDeleteTextures(1, &texture->texture);
		texture->used = 0;
		texture->width = 0;
		texture->height = 0;
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
#ifdef WIN32
	std::string path = std::string(mlt_environment("MLT_APPDIR")).append("\\share\\movit");
#elif defined(__DARWIN__) && defined(RELOCATABLE)
	std::string path = std::string(mlt_environment("MLT_APPDIR")).append("/share/movit");
#else
	std::string path = std::string(getenv("MLT_MOVIT_PATH") ? getenv("MLT_MOVIT_PATH") : SHADERDIR);
#endif
	::init_movit( path, mlt_log_get_level() == MLT_LOG_DEBUG? MOVIT_DEBUG_ON : MOVIT_DEBUG_OFF );
	filter->set( "glsl_supported", movit_initialized );
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
	service.set( "movit input", NULL, 0 );
	// Destroy the effect list.
	GlslManager::get_instance()->set( service.get( "_unique_id" ), NULL, 0 );
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

Mlt::Properties GlslManager::effect_list( Mlt::Service& service )
{
	char *unique_id =  service.get( "_unique_id" );
	mlt_properties properties = (mlt_properties) get_data( unique_id );
	if ( !properties ) {
		properties = mlt_properties_new();
		set( unique_id, properties, 0, (mlt_destructor) mlt_properties_close );
	}
	Mlt::Properties p( properties );
	return p;
}

static void deleteChain( EffectChain* chain )
{
	delete chain;
}

bool GlslManager::init_chain( mlt_service aservice )
{
	bool error = true;
	Mlt::Service service( aservice );
	EffectChain* chain = (EffectChain*) service.get_data( "movit chain" );
	if ( !chain ) {
		mlt_profile profile = mlt_service_profile( aservice );
		Input* input = new MltInput( profile->width, profile->height );
		chain = new EffectChain( profile->display_aspect_num, profile->display_aspect_den );
		chain->add_input( input );
		service.set( "movit chain", chain, 0, (mlt_destructor) deleteChain );
		service.set( "movit input", input, 0 );
		service.set( "_movit finalized", 0 );
		service.listen( "service-changed", aservice, (mlt_listener) GlslManager::onServiceChanged );
		service.listen( "property-changed", aservice, (mlt_listener) GlslManager::onPropertyChanged );
		error = false;
	}
	return error;
}

EffectChain* GlslManager::get_chain( mlt_service service )
{
	return (EffectChain*) mlt_properties_get_data( MLT_SERVICE_PROPERTIES(service), "movit chain", NULL );
}

MltInput *GlslManager::get_input( mlt_service service )
{
	return (MltInput*) mlt_properties_get_data( MLT_SERVICE_PROPERTIES(service), "movit input", NULL );
}

void GlslManager::reset_finalized( mlt_service service )
{
	mlt_properties_set_int( MLT_SERVICE_PROPERTIES(service), "_movit finalized", 0 );
}

Effect* GlslManager::get_effect( mlt_filter filter, mlt_frame frame )
{
	Mlt::Producer producer( mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) ) );
	char *unique_id = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "_unique_id" );
	return (Effect*) GlslManager::get_instance()->effect_list( producer ).get_data( unique_id );
}

Effect* GlslManager::add_effect( mlt_filter filter, mlt_frame frame, Effect* effect )
{
	Mlt::Producer producer( mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) ) );
	EffectChain* chain = (EffectChain*) producer.get_data( "movit chain" );
	chain->add_effect( effect );
	char *unique_id = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "_unique_id" );
	GlslManager::get_instance()->effect_list( producer ).set( unique_id, effect, 0 );
	return effect;
}

Effect* GlslManager::add_effect( mlt_filter filter, mlt_frame frame, Effect* effect, Effect* input_b )
{
	Mlt::Producer producer( mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) ) );
	EffectChain* chain = (EffectChain*) producer.get_data( "movit chain" );
	chain->add_effect( effect, chain->last_added_effect(),
		input_b? input_b : chain->last_added_effect() );
	char *unique_id = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "_unique_id" );
	GlslManager::get_instance()->effect_list( producer ).set( unique_id, effect, 0 );
	return effect;
}

void GlslManager::render_fbo( mlt_service service, void* chain, GLuint fbo, int width, int height )
{
	EffectChain* effect_chain = (EffectChain*) chain;
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
	if ( !mlt_properties_get_int( properties, "_movit finalized" ) ) {
		mlt_properties_set_int( properties, "_movit finalized", 1 );
		effect_chain->add_effect( new Mlt::VerticalFlip() );
		effect_chain->finalize();
	}
	effect_chain->render_to_fbo( fbo, width, height );
}

int GlslManager::render_frame_texture(mlt_service service, mlt_frame frame, int width, int height, uint8_t **image)
{
	EffectChain* chain = get_chain( service );
	if (!chain) return 1;
	glsl_fbo fbo = get_fbo( width, height );
	if (!fbo) return 1;
	glsl_texture texture = get_texture( width, height, GL_RGBA );
	if (!texture) {
		release_fbo( fbo );
		return 1;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
	check_error();
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();

	render_fbo( service, chain, fbo->fbo, width, height );

	glFinish();
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();
	release_fbo( fbo );

	*image = (uint8_t*) &texture->texture;
	mlt_frame_set_image( frame, *image, 0, NULL );
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), "movit.convert.texture", texture, 0,
		(mlt_destructor) GlslManager::release_texture, NULL );

	return 0;
}

int GlslManager::render_frame_rgba(mlt_service service, mlt_frame frame, int width, int height, uint8_t **image)
{
	EffectChain* chain = get_chain( service );
	if (!chain) return 1;
	glsl_fbo fbo = get_fbo( width, height );
	if (!fbo) return 1;
	glsl_texture texture = get_texture( width, height, GL_RGBA );
	if (!texture) {
		release_fbo( fbo );
		return 1;
	}

	// Use a PBO to hold the data we read back with glReadPixels().
	// (Intel/DRI goes into a slow path if we don't read to PBO.)
	int img_size = width * height * 4;
	glsl_pbo pbo = get_pbo( img_size );
	if (!pbo) {
		release_fbo( fbo );
		release_texture(texture);
		return 1;
	}

	// Set the FBO
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
	check_error();
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
	check_error();
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	check_error();

	render_fbo( service, chain, fbo->fbo, width, height );

	// Read FBO into PBO
	glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
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
	release_fbo( fbo );

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
