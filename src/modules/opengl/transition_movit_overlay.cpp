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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <framework/mlt.h>
#include <string.h>
#include <assert.h>

#include "filter_glsl_manager.h"
#include <movit/init.h>
#include <movit/effect_chain.h>
#include <movit/util.h>
#include <movit/overlay_effect.h>

using namespace movit;

static int get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error;

	// Get the b frame from the stack
	mlt_frame b_frame = (mlt_frame) mlt_frame_pop_frame( a_frame );

	// Get the transition object
	mlt_transition transition = (mlt_transition) mlt_frame_pop_service( a_frame );
	mlt_service service = MLT_TRANSITION_SERVICE( transition );
	mlt_service_lock( service );

	uint8_t *a_image, *b_image;

	// Get the two images.
	*format = mlt_image_glsl;
	error = mlt_frame_get_image( a_frame, &a_image, format, width, height, writable );
	error = mlt_frame_get_image( b_frame, &b_image, format, width, height, writable );

	GlslManager::set_effect_input( service, a_frame, (mlt_service) a_image );
	GlslManager::set_effect_secondary_input( service, a_frame, (mlt_service) b_image, b_frame );
	GlslManager::set_effect( service, a_frame, new OverlayEffect );
	*image = (uint8_t *) service;

	mlt_service_unlock( service );
	return error;
}

static mlt_frame process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_frame_push_service( a_frame, transition );
	mlt_frame_push_frame( a_frame, b_frame );
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
