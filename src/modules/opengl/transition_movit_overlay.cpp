/*
 * transition_movit_overlay.cpp
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


#include <framework/mlt.h>
#include <string.h>
#include <assert.h>

#include "glsl_manager.h"
#include <movit/init.h>
#include <movit/effect_chain.h>
#include <movit/util.h>
#include <movit/overlay_effect.h>
#include "mlt_movit_input.h"
#include "mlt_flip_effect.h"

static int get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;

	// Get the b frame from the stack
	mlt_frame b_frame = (mlt_frame) mlt_frame_pop_frame( a_frame );

	// Get the transition object
	mlt_transition transition = (mlt_transition) mlt_frame_pop_service( a_frame );

	// Get the properties of the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );

	// Get the properties of the a frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// Get the movit objects
	mlt_service service = MLT_TRANSITION_SERVICE( transition );
	mlt_service_lock( service );
	EffectChain* chain = GlslManager::get_chain( service );
	MltInput* a_input = GlslManager::get_input( service );
	MltInput* b_input = (MltInput*) mlt_properties_get_data( properties, "movit input B", NULL );
	mlt_image_format output_format = *format;

	if ( !chain || !a_input ) {
		mlt_service_unlock( service );
		return 2;
	}

	// Get the frames' textures
	GLuint* texture_id[2] = {0, 0};
	*format = mlt_image_glsl_texture;
	mlt_frame_get_image( a_frame, (uint8_t**) &texture_id[0], format, width, height, 0 );
	a_input->useFBOInput( chain, *texture_id[0] );
	*format = mlt_image_glsl_texture;
	mlt_frame_get_image( b_frame, (uint8_t**) &texture_id[1], format, width, height, 0 );
	b_input->useFBOInput( chain, *texture_id[1] );

	// Set resolution to that of the a_frame
	*width = mlt_properties_get_int( a_props, "width" );
	*height = mlt_properties_get_int( a_props, "height" );

	// Setup rendering to an FBO
	GlslManager* glsl = GlslManager::get_instance();
	glsl_fbo fbo = glsl->get_fbo( *width, *height );
	if ( output_format == mlt_image_glsl_texture ) {
		glsl_texture texture = glsl->get_texture( *width, *height, GL_RGBA );

		glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
		check_error();
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
		check_error();
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		check_error();

		GlslManager::render( service, chain, fbo->fbo, *width, *height );

		glFinish();
		check_error();
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		check_error();

		*image = (uint8_t*) &texture->texture;
		mlt_frame_set_image( a_frame, *image, 0, NULL );
		mlt_properties_set_data( properties, "movit.convert", texture, 0,
			(mlt_destructor) GlslManager::release_texture, NULL );
		*format = output_format;
	}
	else {
		// Use a PBO to hold the data we read back with glReadPixels()
		// (Intel/DRI goes into a slow path if we don't read to PBO)
		GLenum gl_format = ( output_format == mlt_image_rgb24a || output_format == mlt_image_opengl )?
			GL_RGBA : GL_RGB;
		int img_size = *width * *height * ( gl_format == GL_RGB? 3 : 4 );
		glsl_pbo pbo = glsl->get_pbo( img_size );
		glsl_texture texture = glsl->get_texture( *width, *height, gl_format );

		if ( fbo && pbo && texture ) {
			// Set the FBO
			glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
			check_error();
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
			check_error();
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );
			check_error();

			GlslManager::render( service, chain, fbo->fbo, *width, *height );

			// Read FBO into PBO
			glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, pbo->pbo );
			check_error();
			glBufferData( GL_PIXEL_PACK_BUFFER_ARB, img_size, NULL, GL_STREAM_READ );
			check_error();
			glReadPixels( 0, 0, *width, *height, gl_format, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0) );
			check_error();

			// Copy from PBO
			uint8_t* buf = (uint8_t*) glMapBuffer( GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY );
			check_error();

			*format = gl_format == GL_RGBA ? mlt_image_rgb24a : mlt_image_rgb24;
			*image = (uint8_t*) mlt_pool_alloc( img_size );
			mlt_frame_set_image( a_frame, *image, img_size, mlt_pool_release );
			memcpy( *image, buf, img_size );

			// Release PBO and FBO
			glUnmapBuffer( GL_PIXEL_PACK_BUFFER_ARB );
			check_error();
			glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, 0 );
			check_error();
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );
			check_error();
			glBindTexture( GL_TEXTURE_2D, 0 );
			check_error();
			GlslManager::release_texture( texture );
		}
		else {
			error = 1;
		}
	}
	if ( fbo ) GlslManager::release_fbo( fbo );
	mlt_service_unlock( service );

	return error;
}

static mlt_frame process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_service service = MLT_TRANSITION_SERVICE(transition);

	if ( !GlslManager::init_chain( service ) ) {
		// Create the Movit effect chain
		EffectChain* chain = GlslManager::get_chain( service );
		mlt_profile profile = mlt_service_profile( service );
		Input* b_input = new MltInput( profile->width, profile->height );
		ImageFormat output_format;
		output_format.color_space = COLORSPACE_sRGB;
		output_format.gamma_curve = GAMMA_sRGB;
		chain->add_input( b_input );
		chain->add_output( output_format, OUTPUT_ALPHA_FORMAT_POSTMULTIPLIED );
		chain->set_dither_bits( 8 );

		Effect* effect = chain->add_effect( new OverlayEffect(),
			GlslManager::get_input( service ), b_input );

		// Save these new input on properties for get_image
		mlt_properties_set_data( MLT_TRANSITION_PROPERTIES(transition),
			"movit input B", b_input, 0, NULL, NULL );
	}

	// Push the transition on to the frame
	mlt_frame_push_service( a_frame, transition );

	// Push the b_frame on to the stack
	mlt_frame_push_frame( a_frame, b_frame );

	// Push the transition method
	mlt_frame_push_get_image( a_frame, get_image );

	return a_frame;
}

extern "C"
mlt_transition transition_movit_overlay_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_transition transition = NULL;
	GlslManager* glsl = GlslManager::get_instance();
	if ( glsl && ( transition = mlt_transition_new() ) ) {
		transition->process = process;
		
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );
	}
	return transition;
}
