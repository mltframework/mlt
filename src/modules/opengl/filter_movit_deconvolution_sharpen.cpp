/*
 * filter_deconvolution_sharpen.cpp
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
#include <movit/deconvolution_sharpen_effect.h>

using namespace movit;

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	GlslManager::get_instance()->lock_service( frame );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	int matrix_size = mlt_properties_anim_get_int( properties, "matrix_size", position, length );
	mlt_properties_set_int( properties, "_movit.parms.int.matrix_size", matrix_size );
	mlt_properties_set_double( properties, "_movit.parms.float.circle_radius",
		mlt_properties_anim_get_double( properties, "circle_radius", position, length ) );
	mlt_properties_set_double( properties, "_movit.parms.float.gaussian_radius",
		mlt_properties_anim_get_double( properties, "gaussian_radius", position, length ) );
	mlt_properties_set_double( properties, "_movit.parms.float.correlation",
		mlt_properties_anim_get_double( properties, "correlation", position, length ) );
	mlt_properties_set_double( properties, "_movit.parms.float.noise",
		mlt_properties_anim_get_double( properties, "noise", position, length ) );

	// DeconvolutionSharpenEffect compiles the matrix size into the shader,
	// so we need to regenerate the chain if this changes.
	char fingerprint[256];
	snprintf( fingerprint, sizeof( fingerprint ), "s=%d", matrix_size );
	mlt_properties_set( properties, "_movit fingerprint", fingerprint );

	GlslManager::get_instance()->unlock_service( frame );
	*format = mlt_image_glsl;
	int error = mlt_frame_get_image( frame, image, format, width, height, writable );
	GlslManager::set_effect_input( MLT_FILTER_SERVICE( filter ), frame, (mlt_service) *image );
	GlslManager::set_effect( MLT_FILTER_SERVICE( filter ), frame, new DeconvolutionSharpenEffect );
	*image = (uint8_t *) MLT_FILTER_SERVICE( filter );
	return error;
}

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, get_image );
	return frame;
}

extern "C" {

mlt_filter filter_deconvolution_sharpen_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) ) {
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		glsl->add_ref( properties );
		mlt_properties_set_int( properties, "matrix_size", 5 );
		mlt_properties_set_double( properties, "circle_radius", 2.0 );
		mlt_properties_set_double( properties, "gaussian_radius", 0.0 );
		mlt_properties_set_double( properties, "correlation", 0.95 );
		mlt_properties_set_double( properties, "noise", 0.01 );
		filter->process = process;
	}
	return filter;
}

}
