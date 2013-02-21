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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <framework/mlt.h>
#include <string.h>
#include <assert.h>

#include "glsl_manager.h"
#include <movit/white_balance_effect.h>

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	if ( !mlt_frame_is_test_card( frame ) ) {
		Effect* effect = GlslManager::get_effect( filter, frame );
		if ( !effect )
			effect = GlslManager::add_effect( filter, frame, new WhiteBalanceEffect );
		if ( effect ) {
			mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter );
			int color_int = mlt_properties_get_int( filter_props, "neutral_color" );
			RGBTriplet color(
				float((color_int >> 24) & 0xff) / 255.0f,
				float((color_int >> 16) & 0xff) / 255.0f,
				float((color_int >> 8) & 0xff) / 255.0f
			);
			bool ok = effect->set_vec3( "neutral_color", (float*) &color );
			ok |= effect->set_float( "output_color_temperature", mlt_properties_get_double( filter_props, "color_temperature" ) );
			assert(ok);
		}
	}
	return frame;
}

extern "C" {

mlt_filter filter_white_balance_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) ) {
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set( properties, "neutral_color", arg? arg : "#7f7f7f" );
		mlt_properties_set_double( properties, "color_temperature", 6500.0 );
		filter->process = process;
	}
	return filter;
}

}
