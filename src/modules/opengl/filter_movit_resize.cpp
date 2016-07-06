/*
 * filter_movit_resize.cpp
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
#include <stdlib.h>
#include <assert.h>

#include "filter_glsl_manager.h"
#include <movit/init.h>
#include <movit/padding_effect.h>
#include "optional_effect.h"

using namespace movit;

static float alignment_parse( char* align )
{
	int ret = 0.0f;

	if ( align == NULL );
	else if ( isdigit( align[ 0 ] ) )
		ret = atoi( align );
	else if ( align[ 0 ] == 'c' || align[ 0 ] == 'm' )
		ret = 1.0f;
	else if ( align[ 0 ] == 'r' || align[ 0 ] == 'b' )
		ret = 2.0f;

	return ret;
}

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );

	// Retrieve the aspect ratio
	double aspect_ratio = mlt_frame_get_aspect_ratio( frame );
	double consumer_aspect = mlt_profile_sar( profile );

	// Correct Width/height if necessary
	if ( *width == 0 || *height == 0 )
	{
		*width = profile->width;
		*height = profile->height;
	}

	// Assign requested width/height from our subordinate
	int owidth = *width;
	int oheight = *height;

	// Use a mlt_rect to compute position and size
	mlt_rect rect;
	rect.x = rect.y = 0.0;
	if ( mlt_properties_get( properties, "resize.rect" ) ) {
		mlt_position position = mlt_filter_get_position( filter, frame );
		mlt_position length = mlt_filter_get_length2( filter, frame );
		rect = mlt_properties_anim_get_rect( properties, "resize.rect", position, length );
		if ( strchr( mlt_properties_get( properties, "resize.rect" ), '%' ) ) {
			rect.x *= profile->width;
			rect.w *= profile->width;
			rect.y *= profile->height;
			rect.h *= profile->height;
		}
		if ( !mlt_properties_get_int( properties, "resize.fill" ) ) {
			int x = mlt_properties_get_int( properties, "meta.media.width" );
			owidth = lrintf( rect.w > x ? x : rect.w );
			x = mlt_properties_get_int( properties, "meta.media.height" );
			oheight = lrintf( rect.h > x ? x : rect.h );
		} else {
			owidth = lrintf( rect.w );
			oheight = lrintf( rect.h );
		}
	}

	// Check for the special case - no aspect ratio means no problem :-)
	if ( aspect_ratio == 0.0 )
		aspect_ratio = consumer_aspect;

	// Reset the aspect ratio
	mlt_properties_set_double( properties, "aspect_ratio", aspect_ratio );

	// Skip processing if requested.
	char *rescale = mlt_properties_get( properties, "rescale.interp" );
	if ( *format == mlt_image_none || ( rescale && !strcmp( rescale, "none" ) ) )
		return mlt_frame_get_image( frame, image, format, width, height, writable );

	if ( mlt_properties_get_int( properties, "distort" ) == 0 )
	{
		// Normalise the input and out display aspect
		int normalised_width = profile->width;
		int normalised_height = profile->height;
		int real_width = mlt_properties_get_int( properties, "meta.media.width" );
		int real_height = mlt_properties_get_int( properties, "meta.media.height" );
		if ( real_width == 0 )
			real_width = mlt_properties_get_int( properties, "width" );
		if ( real_height == 0 )
			real_height = mlt_properties_get_int( properties, "height" );
		double input_ar = aspect_ratio * real_width / real_height;
		double output_ar = consumer_aspect * owidth / oheight;
		
		// Optimised for the input_ar > output_ar case (e.g. widescreen on standard)
		int scaled_width = lrint( ( input_ar * normalised_width ) / output_ar );
		int scaled_height = normalised_height;

		// Now ensure that our images fit in the output frame
		if ( scaled_width > normalised_width )
		{
			scaled_width = normalised_width;
			scaled_height = lrint( ( output_ar * normalised_height ) / input_ar );
		}

		// Now calculate the actual image size that we want
		owidth = lrint( scaled_width * owidth / normalised_width );
		oheight = lrint( scaled_height * oheight / normalised_height );

 		mlt_log_debug( MLT_FILTER_SERVICE(filter),
			"real %dx%d normalised %dx%d output %dx%d sar %f in-dar %f out-dar %f\n",
			real_width, real_height, normalised_width, normalised_height, owidth, oheight, aspect_ratio, input_ar, output_ar);

		// Tell frame we have conformed the aspect to the consumer
		mlt_frame_set_aspect_ratio( frame, consumer_aspect );
	}

	mlt_properties_set_int( properties, "distort", 0 );

	// Now get the image
	*format = mlt_image_glsl;
	error = mlt_frame_get_image( frame, image, format, &owidth, &oheight, writable );

	// Offset the position according to alignment
	if ( mlt_properties_get( properties, "resize.rect" ) ) {
		// default top left if rect supplied
		float w = float( rect.w - owidth );
		float h = float( rect.h - oheight );
		rect.x += w * alignment_parse( mlt_properties_get( properties, "resize.halign" ) ) / 2.0f;
		rect.y += h * alignment_parse( mlt_properties_get( properties, "resize.valign" ) ) / 2.0f;
	} else {
		// default center if no rect
		float w = float( *width - owidth );
		float h = float( *height - oheight );
		rect.x = w * 0.5f;
		rect.y = h * 0.5f;
	}

	if ( !error ) {
		mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
		GlslManager::get_instance()->lock_service( frame );
		mlt_properties_set_int( filter_properties, "_movit.parms.int.width", *width );
		mlt_properties_set_int( filter_properties, "_movit.parms.int.height", *height );
		mlt_properties_set_double( filter_properties, "_movit.parms.float.left", rect.x );
		mlt_properties_set_double( filter_properties, "_movit.parms.float.top", rect.y );

		bool disable = ( *width == owidth && *height == oheight && rect.x == 0 && rect.y == 0 );
		mlt_properties_set_int( filter_properties, "_movit.parms.int.disable", disable );

		GlslManager::get_instance()->unlock_service( frame );

		GlslManager::set_effect_input( MLT_FILTER_SERVICE( filter ), frame, (mlt_service) *image );
		GlslManager::set_effect( MLT_FILTER_SERVICE( filter ), frame, new OptionalEffect<PaddingEffect> );
		*image = (uint8_t *) MLT_FILTER_SERVICE( filter );
		return error;
	}

	return error;
}

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, get_image );
	return frame;
}

extern "C"
mlt_filter filter_movit_resize_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		glsl->add_ref( properties );
		filter->process = process;
	}
	return filter;
}
