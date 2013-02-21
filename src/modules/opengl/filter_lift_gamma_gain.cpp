/*
 * filter_lift_gamma_gain.cpp
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
#include <movit/lift_gamma_gain_effect.h>

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	if ( !mlt_frame_is_test_card( frame ) ) {
		Effect* effect = GlslManager::get_effect( filter, frame );
		if ( !effect )
			effect = GlslManager::add_effect( filter, frame, new LiftGammaGainEffect );
		if ( effect ) {
			mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter );
			RGBTriplet triplet(
				mlt_properties_get_double( filter_props, "lift_r" ),
				mlt_properties_get_double( filter_props, "lift_g" ),
				mlt_properties_get_double( filter_props, "lift_b" )
			);
			bool ok = effect->set_vec3( "lift", (float*) &triplet );
			triplet.r = mlt_properties_get_double( filter_props, "gamma_r" );
			triplet.g = mlt_properties_get_double( filter_props, "gamma_g" );
			triplet.b = mlt_properties_get_double( filter_props, "gamma_b" );
			ok |= effect->set_vec3( "gamma", (float*) &triplet );
			triplet.r = mlt_properties_get_double( filter_props, "gain_r" );
			triplet.g = mlt_properties_get_double( filter_props, "gain_g" );
			triplet.b = mlt_properties_get_double( filter_props, "gain_b" );
			ok |= effect->set_vec3( "gain", (float*) &triplet );
			assert(ok);
		}
	}
	return frame;
}

extern "C" {

mlt_filter filter_lift_gamma_gain_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) ) {
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_double( properties, "lift_r", 0.0 );
		mlt_properties_set_double( properties, "lift_g", 0.0 );
		mlt_properties_set_double( properties, "lift_b", 0.0 );
		mlt_properties_set_double( properties, "gamma_r", 1.0 );
		mlt_properties_set_double( properties, "gamma_g", 1.0 );
		mlt_properties_set_double( properties, "gamma_b", 1.0 );
		mlt_properties_set_double( properties, "gain_r", 1.0 );
		mlt_properties_set_double( properties, "gain_g", 1.0 );
		mlt_properties_set_double( properties, "gain_b", 1.0 );
		filter->process = process;
	}
	return filter;
}

}
