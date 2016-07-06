/*
 * filter_movit_opacity.cpp
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
#include <movit/multiply_effect.h>

using namespace movit;

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	GlslManager::get_instance()->lock_service( frame );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	double opacity = mlt_properties_anim_get_double( properties, "opacity", position, length );
	double alpha = mlt_properties_anim_get_double( properties, "alpha", position, length );
	mlt_properties_set_double( properties, "_movit.parms.vec4.factor[0]", opacity );
	mlt_properties_set_double( properties, "_movit.parms.vec4.factor[1]", opacity );
	mlt_properties_set_double( properties, "_movit.parms.vec4.factor[2]", opacity );
	mlt_properties_set_double( properties, "_movit.parms.vec4.factor[3]", alpha >= 0 ? alpha : opacity );
	GlslManager::get_instance()->unlock_service( frame );
	*format = mlt_image_glsl;
	int error = mlt_frame_get_image( frame, image, format, width, height, writable );
	GlslManager::set_effect_input( MLT_FILTER_SERVICE( filter ), frame, (mlt_service) *image );
	GlslManager::set_effect( MLT_FILTER_SERVICE( filter ), frame, new MultiplyEffect );
	*image = (uint8_t *) MLT_FILTER_SERVICE( filter );
	return error;
}

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, get_image );
	return frame;
}

extern "C"
mlt_filter filter_movit_opacity_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) ) {
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		glsl->add_ref( properties );
		mlt_properties_set( properties, "opacity", arg? arg : "1" );
		mlt_properties_set_double( properties, "alpha", -1.0 );
		filter->process = process;
	}
	return filter;
}
