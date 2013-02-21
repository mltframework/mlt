/*
 * glsl_manager.h
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

#include <GL/glew.h>
#include <mlt++/MltFilter.h>
#include <mlt++/MltDeque.h>

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

class GlslManager : public Mlt::Filter
{
public:
	GlslManager();
	~GlslManager();
	static GlslManager* get_instance();

	glsl_fbo get_fbo(int width, int height);
	static void release_fbo(glsl_fbo);
	glsl_texture get_texture(int width, int height, GLint internal_format);
	static void release_texture(glsl_texture);
	glsl_pbo get_pbo(int size);

	static bool init_chain(mlt_service);
	static EffectChain* get_chain(mlt_service);
	static MltInput* get_input(mlt_service);
	static void reset_finalized(mlt_service);
	static Effect* get_effect(mlt_filter, mlt_frame);
	static Effect* add_effect(mlt_filter, mlt_frame, Effect*);
	static Effect* add_effect(mlt_filter, mlt_frame, Effect*, Effect* input_b);
	static void render(mlt_service, void *chain, GLuint fbo, int width, int height);

private:
	static void onInit( mlt_properties owner, GlslManager* filter );

	Mlt::Deque fbo_list;
	Mlt::Deque texture_list;
	glsl_pbo  pbo;
	EffectChain* current_chain;
};

#endif // GLSL_MANAGER_H
