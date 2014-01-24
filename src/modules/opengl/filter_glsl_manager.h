/*
 * filter_glsl_manager.h
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

#ifndef GLSL_MANAGER_H
#define GLSL_MANAGER_H

#include <glew.h>
#include <mlt++/MltFilter.h>
#include <mlt++/MltDeque.h>
#include <map>
#include <string>

#define MAXLISTCOUNT 1024
typedef struct glsl_list_s *glsl_list;
struct glsl_list_s
{
	void *items[MAXLISTCOUNT];
	int count;

	int ( *add )( glsl_list, void* );
	void* ( *at )( glsl_list, int );
	void* ( *take_at )( glsl_list, int );
	void* ( *take )( glsl_list, void* );
};

struct glsl_texture_s
{
	int used;
	GLuint texture;
	int width;
	int height;
	GLint internal_format;
};
typedef struct glsl_texture_s *glsl_texture;

struct glsl_fbo_s
{
	int used;
	int width;
	int height;
	GLuint fbo;
	void* context;
};
typedef struct glsl_fbo_s *glsl_fbo;

struct glsl_pbo_s
{
	int size;
	GLuint pbo;
};
typedef struct glsl_pbo_s *glsl_pbo;

class Effect;
class EffectChain;
class MltInput;

struct GlslChain
{
	EffectChain *effect_chain;

	// All services owned by the effect chain and their associated Movit effect.
	std::map<mlt_service, Effect*> effects;

	// For each effect in the Movit graph, a unique identifier for the service
	// and whether it's disabled or not, using post-order traversal.
	// We need to generate the chain if and only if this has changed.
	std::string fingerprint;
};

class GlslManager : public Mlt::Filter
{
public:
	GlslManager();
	~GlslManager();
	void add_ref(mlt_properties properties);
	static GlslManager* get_instance();

	glsl_fbo get_fbo(int width, int height);
	static void release_fbo(glsl_fbo);
	glsl_texture get_texture(int width, int height, GLint internal_format);
	static void release_texture(glsl_texture);
	static void delete_sync(GLsync sync);
	glsl_pbo get_pbo(int size);
	void cleanupContext();

	static void set_chain(mlt_service, GlslChain*);
	static GlslChain* get_chain(mlt_service);

	static Effect* get_effect(mlt_service, mlt_frame);
	static Effect* set_effect(mlt_service, mlt_frame, Effect*);
	static MltInput* get_input(mlt_producer, mlt_frame);
	static MltInput* set_input(mlt_producer, mlt_frame, MltInput*);
	static uint8_t* get_input_pixel_pointer(mlt_producer, mlt_frame);
	static uint8_t* set_input_pixel_pointer(mlt_producer, mlt_frame, uint8_t*);

	static mlt_service get_effect_input(mlt_service, mlt_frame);
	static void set_effect_input(mlt_service, mlt_frame, mlt_service);
	static void get_effect_secondary_input(mlt_service, mlt_frame, mlt_service*, mlt_frame*);
	static void set_effect_secondary_input(mlt_service, mlt_frame, mlt_service, mlt_frame);

	int render_frame_texture(EffectChain*, mlt_frame, int width, int height, uint8_t **image);
	int render_frame_rgba(EffectChain*, mlt_frame, int width, int height, uint8_t **image);
	static void lock_service(mlt_frame frame);
	static void unlock_service(mlt_frame frame);

private:
	static void* get_frame_specific_data( mlt_service service, mlt_frame frame, const char *key, int *length );
	static int set_frame_specific_data( mlt_service service, mlt_frame frame, const char *key, void *value, int length, mlt_destructor destroy, mlt_serialiser serialise );

	static void onInit( mlt_properties owner, GlslManager* filter );
	static void onClose( mlt_properties owner, GlslManager* filter );
	static void onServiceChanged( mlt_properties owner, mlt_service service );
	static void onPropertyChanged( mlt_properties owner, mlt_service service, const char* property );
	Mlt::Deque fbo_list;
	Mlt::Deque texture_list;
	Mlt::Deque syncs_to_delete;
	glsl_pbo  pbo;
	Mlt::Event* initEvent;
	Mlt::Event* closeEvent;
	GLsync prev_sync;
};

#endif // GLSL_MANAGER_H
