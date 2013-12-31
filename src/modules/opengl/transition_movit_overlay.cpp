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
	if ( output_format == mlt_image_glsl_texture ) {
		error = glsl->render_frame_texture( service, a_frame, *width, *height, image );
		*format = output_format;
	}
	else {
		error = glsl->render_frame_rgba( service, a_frame, *width, *height, image );
		*format = mlt_image_rgb24a;
	}
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

		chain->add_effect( new OverlayEffect(), GlslManager::get_input( service ), b_input );

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
