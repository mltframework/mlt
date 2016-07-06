/*
 * filter_movit_resample.cpp
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
#include <movit/resample_effect.h>
#include "optional_effect.h"

using namespace movit;

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );

	// Correct width/height if necessary
	if ( *width == 0 || *height == 0 )
	{
		*width = profile->width;
		*height = profile->height;
	}

	int iwidth = *width;
	int iheight = *height;
	double factor = mlt_properties_get_double( filter_properties, "factor" );
	factor = factor > 0 ? factor : 1.0;
	int owidth = *width * factor;
	int oheight = *height * factor;

	// If meta.media.width/height exist, we want that as minimum information
	if ( mlt_properties_get_int( properties, "meta.media.width" ) )
	{
		iwidth = mlt_properties_get_int( properties, "meta.media.width" );
		iheight = mlt_properties_get_int( properties, "meta.media.height" );
	}

	mlt_properties_set_int( properties, "rescale_width", *width );
	mlt_properties_set_int( properties, "rescale_height", *height );

	// Deinterlace if height is changing to prevent fields mixing on interpolation
	if ( iheight != oheight )
		mlt_properties_set_int( properties, "consumer_deinterlace", 1 );

	GlslManager::get_instance()->lock_service( frame );
	mlt_properties_set_int( filter_properties, "_movit.parms.int.width", owidth );
	mlt_properties_set_int( filter_properties, "_movit.parms.int.height", oheight );

	bool disable = ( iwidth == owidth && iheight == oheight );
	mlt_properties_set_int( filter_properties, "_movit.parms.int.disable", disable );

	*width = owidth;
	*height = oheight;

	GlslManager::get_instance()->unlock_service( frame );

	// Get the image as requested
	if ( *format != mlt_image_none )
		*format = mlt_image_glsl;
	int error = mlt_frame_get_image( frame, image, format, &iwidth, &iheight, writable );
	GlslManager::set_effect_input( MLT_FILTER_SERVICE( filter ), frame, (mlt_service) *image );
	Effect *effect = GlslManager::set_effect( MLT_FILTER_SERVICE( filter ), frame, new OptionalEffect<ResampleEffect> );
	// This needs to be something else than 0x0 at chain finalization time.
	bool ok = effect->set_int("width", owidth);
	ok |= effect->set_int("height", oheight);
	assert( ok );
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
mlt_filter filter_movit_resample_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) ) {
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		glsl->add_ref( properties );
		filter->process = process;
	}
	return filter;
}
