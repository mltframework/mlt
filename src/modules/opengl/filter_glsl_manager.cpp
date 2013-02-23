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
#include <movit/effect_chain.h>
#include "mlt_movit_input.h"
#include "mlt_flip_effect.h"

extern "C" {
#include <framework/mlt_factory.h>
}

void deleteManager(GlslManager *p)
{
	delete p;
}

GlslManager::GlslManager()
	: Mlt::Filter( mlt_filter_new() )
	, pbo(0)
{
	mlt_filter filter = get_filter();
	if ( filter ) {
		// Set the mlt_filter child in case we choose to override virtual functions.
		filter->child = this;
		mlt_properties_set_data(mlt_global_properties(), "glslManager", this, 0,
			(mlt_destructor) deleteManager, NULL);

		mlt_events_register( get_properties(), "init glsl", NULL );
		listen("init glsl", this, (mlt_listener) GlslManager::onInit);
	}
}

GlslManager::~GlslManager()
{
	mlt_log_debug(get_service(), "%s\n", __FUNCTION__);
	while (fbo_list.peek_back())
		delete (glsl_fbo) fbo_list.pop_back();
	while (texture_list.peek_back())
		delete (glsl_texture) texture_list.pop_back();
	delete pbo;
}

GlslManager* GlslManager::get_instance()
{
	return (GlslManager*) mlt_properties_get_data(mlt_global_properties(), "glslManager", 0);
}

glsl_fbo GlslManager::get_fbo(int width, int height)
{
	for (int i = 0; i < fbo_list.count(); ++i) {
		glsl_fbo fbo = (glsl_fbo) fbo_list.peek(i);
		if (!fbo->used && (fbo->width == width) && (fbo->height == height)) {
			fbo->used = 1;
			return fbo;
		}
	}
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
	fbo_list.push_back(fbo);
	return fbo;
}

void GlslManager::release_fbo(glsl_fbo fbo)
{
	fbo->used = 0;
}

glsl_texture GlslManager::get_texture(int width, int height, GLint internal_format)
{
	for (int i = 0; i < texture_list.count(); ++i) {
		glsl_texture tex = (glsl_texture) texture_list.peek(i);
		if (!tex->used && (tex->width == width) && (tex->height == height) && (tex->internal_format == internal_format)) {
			glBindTexture(GL_TEXTURE_2D, tex->texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindTexture( GL_TEXTURE_2D, 0);
			tex->used = 1;
			return tex;
		}
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	if (!tex)
		return NULL;

	glsl_texture gtex = new glsl_texture_s;
	if (!gtex) {
		glDeleteTextures(1, &tex);
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
	texture_list.push_back(gtex);
	return gtex;
}

void GlslManager::release_texture(glsl_texture texture)
{
	texture->used = 0;
}

glsl_pbo GlslManager::get_pbo(int size)
{
	if (!pbo) {
		GLuint pb = 0;
		glGenBuffers(1, &pb);
		if (!pb)
			return NULL;

		pbo = new glsl_pbo_s;
		if (!pbo) {
			glDeleteBuffers(1, &pb);
			return NULL;
		}
		pbo->pbo = pb;
	}
	if (size > pbo->size) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo->pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, size, NULL, GL_STREAM_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		pbo->size = size;
	}
	return pbo;
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

static void deleteChain( EffectChain* chain )
{
	delete chain;
}

bool GlslManager::init_chain( mlt_service service )
{
	bool error = true;
	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
	EffectChain* chain = (EffectChain*) mlt_properties_get_data( properties, "movit chain", NULL );
	if ( !chain ) {
		mlt_profile profile = mlt_service_profile( service );
		Input* input = new MltInput( profile->width, profile->height );
		chain = new EffectChain( profile->display_aspect_num, profile->display_aspect_den );
		chain->add_input( input );
		mlt_properties_set_data( properties, "movit chain", chain, 0, (mlt_destructor) deleteChain, NULL );
		mlt_properties_set_data( properties, "movit input", input, 0, NULL, NULL );
		mlt_properties_set_int( properties, "_movit finalized", 0 );
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
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	char *unique_id = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "_unique_id" );
	return (Effect*) mlt_properties_get_data( properties, unique_id, NULL );
}

Effect* GlslManager::add_effect( mlt_filter filter, mlt_frame frame, Effect* effect )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	char *unique_id = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "_unique_id" );
	EffectChain* chain = (EffectChain*) mlt_properties_get_data( properties, "movit chain", NULL );
	chain->add_effect( effect );
	mlt_properties_set_data( properties, unique_id, effect, 0, NULL, NULL );
	return effect;
}

Effect* GlslManager::add_effect( mlt_filter filter, mlt_frame frame, Effect* effect, Effect* input_b )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	char *unique_id = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "_unique_id" );
	EffectChain* chain = (EffectChain*) mlt_properties_get_data( properties, "movit chain", NULL );
	chain->add_effect( effect, chain->last_added_effect(),
		input_b? input_b : chain->last_added_effect() );
	mlt_properties_set_data( properties, unique_id, effect, 0, NULL, NULL );
	return effect;
}

void GlslManager::render( mlt_service service, void* chain, GLuint fbo, int width, int height )
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
