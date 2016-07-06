/*
 * filter_white_balance.cpp
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
#include <math.h>
#include <assert.h>

#include "filter_glsl_manager.h"
#include <movit/white_balance_effect.h>

using namespace movit;

static double srgb8_to_linear(int c)
{
	double x = c / 255.0f;
	if (x < 0.04045f) {
		return (1.0/12.92f) * x;
	} else {
		return pow((x + 0.055) * (1.0/1.055f), 2.4);
	}
}

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	GlslManager::get_instance()->lock_service( frame );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	int color_int = mlt_properties_anim_get_int( properties, "neutral_color", position, length );
	RGBTriplet color(
		srgb8_to_linear((color_int >> 24) & 0xff),
		srgb8_to_linear((color_int >> 16) & 0xff),
		srgb8_to_linear((color_int >> 8) & 0xff)
	);
	mlt_properties_set_double( properties, "_movit.parms.vec3.neutral_color[0]", color.r );
	mlt_properties_set_double( properties, "_movit.parms.vec3.neutral_color[1]", color.g );
	mlt_properties_set_double( properties, "_movit.parms.vec3.neutral_color[2]", color.b );
	double output_color_temperature =
		mlt_properties_anim_get_double( properties, "color_temperature", position, length );
	mlt_properties_set_double( properties, "_movit.parms.float.output_color_temperature",
                output_color_temperature );
	GlslManager::get_instance()->unlock_service( frame );
	*format = mlt_image_glsl;
	int error = mlt_frame_get_image( frame, image, format, width, height, writable );
	GlslManager::set_effect_input( MLT_FILTER_SERVICE( filter ), frame, (mlt_service) *image );
	GlslManager::set_effect( MLT_FILTER_SERVICE( filter ), frame, new WhiteBalanceEffect );
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

mlt_filter filter_white_balance_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) ) {
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		glsl->add_ref( properties );
		mlt_properties_set( properties, "neutral_color", arg? arg : "#7f7f7f" );
		mlt_properties_set_double( properties, "color_temperature", 6500.0 );
		filter->process = process;
	}
	return filter;
}

}
