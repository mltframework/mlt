/*
 * transition_qtblend.cpp -- Qt composite transition
 * Copyright (c) 2016 Jean-Baptiste Mardelle <jb@kdenlive.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "common.h"
#include <framework/mlt.h>
#include <stdio.h>
#include <string.h>
#include <QImage>
#include <QPainter>
#include <QTransform>

static int get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );
	mlt_properties b_properties = MLT_FRAME_PROPERTIES( b_frame );
	mlt_properties properties = MLT_FRAME_PROPERTIES( a_frame );
	mlt_transition transition = MLT_TRANSITION( mlt_frame_pop_service( a_frame ) );
	mlt_properties transition_properties = MLT_TRANSITION_PROPERTIES( transition );

	uint8_t *b_image = NULL;
	bool hasAlpha = false;
	double opacity = 1.0;
	QTransform transform;
	// reference rect
	mlt_rect rect;

	// Determine length
	mlt_position length = mlt_transition_get_length( transition );
	// Get current position
	mlt_position position =  mlt_transition_get_position( transition, a_frame );

	// Obtain the normalised width and height from the a_frame
	mlt_profile profile = mlt_service_profile( MLT_TRANSITION_SERVICE( transition ) );
	int normalised_width = profile->width;
	int normalised_height = profile->height;
	double consumer_ar = mlt_profile_sar( profile );
	int b_width = mlt_properties_get_int( b_properties, "meta.media.width" );
	int b_height = mlt_properties_get_int( b_properties, "meta.media.height" );
	if ( b_height == 0 )
	{
		b_width = normalised_width;
		b_height = normalised_height;
	}
	double b_ar = mlt_frame_get_aspect_ratio( b_frame );
	double b_dar = b_ar * b_width / b_height;
	rect.w = -1;
	rect.h = -1;
	bool consumerScaling = false;

	// Check transform
	if ( mlt_properties_get( transition_properties, "rect" ) )
	{
		rect = mlt_properties_anim_get_rect( transition_properties, "rect", position, length );
		if (mlt_properties_get(transition_properties, "rect") && ::strchr(mlt_properties_get(transition_properties, "rect"), '%')) {
			rect.x *= normalised_width;
			rect.y *= normalised_height;
			rect.w *= normalised_width;
			rect.h *= normalised_height;
		}
		double scale = mlt_profile_scale_width(profile, *width);
		if ( scale != 1.0 ) {
			consumerScaling = true;
		}
		rect.x *= scale;
		rect.w *= scale;
		scale = mlt_profile_scale_height(profile, *height);
		if ( !consumerScaling && scale != 1.0 ) {
			consumerScaling = true;
		}
		rect.y *= scale;
		rect.h *= scale;
		transform.translate(rect.x, rect.y);
		opacity = rect.o;
	}

	double output_ar = mlt_profile_sar( profile );
	if ( mlt_frame_get_aspect_ratio( b_frame ) == 0 )
	{
		mlt_frame_set_aspect_ratio( b_frame, output_ar );
	}

	if ( mlt_properties_get( transition_properties, "rotation" ) )
	{
		double angle = mlt_properties_anim_get_double( transition_properties, "rotation", position, length );
		if (angle != 0.0) {
			if ( mlt_properties_get_int( transition_properties, "rotate_center" ) )
			{
				transform.translate( rect.w / 2.0, rect.h / 2.0 );
				transform.rotate( angle );
				transform.translate( -rect.w / 2.0, -rect.h / 2.0 );
			}
			else
			{
				transform.rotate( angle );
			}
			hasAlpha = true;
		}
	}

	// This is not a field-aware transform.
	mlt_properties_set_int( b_properties, "consumer_deinterlace", 1 );

	// Suppress padding and aspect normalization.
	char *interps = mlt_properties_get( properties, "rescale.interp" );
	if ( interps )
		interps = strdup( interps );

	if ( error )
	{
		return error;
	}

	// Adjust if consumer is scaling
	if ( consumerScaling )
	{
		// Scale request of b frame image to consumer scale maintaining its aspect ratio.
		b_height = *height;
		b_width = b_height * b_dar / b_ar;
	}

	if ( rect.w != -1 )
	{
		if ( mlt_properties_get_int( transition_properties, "distort" ) && b_width != 0 && b_height != 0 )
		{
			transform.scale( rect.w / b_width, rect.h / b_height );
		}
		else
		{
			// Determine scale with respect to aspect ratio.
			double geometry_dar = rect.w * consumer_ar / rect.h;
			double scale;
			if ( b_dar > geometry_dar )
			{
				scale = rect.w / b_width;
			}
			else
			{
				scale = rect.h / b_height * b_ar;
			}

			transform.translate((rect.w - (b_width * scale)) / 2.0, (rect.h - (b_height * scale)) / 2.0);
			transform.scale( scale, scale );
		}

		if ( opacity < 1 || rect.x > 0 || rect.y > 0 || (rect.x + rect.w < *width ) || (rect.y + rect.w < *height ) )
		{
			// we will process operations on top frame, so also process b_frame
			hasAlpha = true;
		}
	}
	else
	{
		// No transform, request profile sized image
		if (b_dar != mlt_profile_dar( profile ) )
		{
			// Activate transparency if the clips don't have the same aspect ratio
			hasAlpha = true;
		}
		// resize to consumer request
		b_width = *width;
		b_height = *height;
	}
	if ( !hasAlpha && ( mlt_properties_get_int( transition_properties, "compositing" ) != 0 || b_width < *width || b_height < *height ) )
	{
		hasAlpha = true;
	}

	// Check if we have transparency
	if ( !hasAlpha )
	{
		// fetch image
		error = mlt_frame_get_image( b_frame, &b_image, format, width, height, 1 );
		if ( *format == mlt_image_rgba || mlt_frame_get_alpha( b_frame ) )
		{
			hasAlpha = true;
		}
	}
	if ( !hasAlpha )
	{
		// Prepare output image
		*image = b_image;
		mlt_frame_replace_image( a_frame, b_image, *format, *width, *height );
		free( interps );
		return 0;
	}
	// Get RGBA image to process
	*format = mlt_image_rgba;
	error = mlt_frame_get_image( b_frame, &b_image, format, &b_width, &b_height, writable );

	// Get bottom frame
	uint8_t *a_image = NULL;
	error = mlt_frame_get_image( a_frame, &a_image, format, width, height, 1 );
	if (error)
	{
		free( interps );
		return error;
	}
	// Prepare output image
	int image_size = mlt_image_format_size( *format, *width, *height, NULL );
	*image = (uint8_t *) mlt_pool_alloc( image_size );

	// Copy bottom frame in output
	memcpy( *image, a_image, image_size );

	bool hqPainting = false;
	if ( interps )
	{
		if ( strcmp( interps, "bilinear" ) == 0 || strcmp( interps, "bicubic" ) == 0 )
		{
			hqPainting = true;
		}
	}

	// convert bottom mlt image to qimage
	QImage bottomImg;
	convert_mlt_to_qimage_rgba( *image, &bottomImg, *width, *height );

	// convert top mlt image to qimage
	QImage topImg;
	convert_mlt_to_qimage_rgba( b_image, &topImg, b_width, b_height );


	// setup Qt drawing
	QPainter painter( &bottomImg );
	painter.setCompositionMode( ( QPainter::CompositionMode ) mlt_properties_get_int( transition_properties, "compositing" ) );
	painter.setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform, hqPainting );
	painter.setTransform(transform);
	painter.setOpacity(opacity);

	// Composite top frame
	painter.drawImage(0, 0, topImg);

	// finish Qt drawing
	painter.end();
	convert_qimage_to_mlt_rgba( &bottomImg, *image, *width, *height );
	mlt_frame_set_image( a_frame, *image, image_size, mlt_pool_release);
	free( interps );
	return error;
}

static mlt_frame process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_frame_push_service( a_frame, transition );
	mlt_frame_push_frame( a_frame, b_frame );
	mlt_frame_push_get_image( a_frame, get_image );
	return a_frame;
}

extern "C" {

mlt_transition transition_qtblend_init( mlt_profile profile, mlt_service_type type, const char *id, void *arg )
{
	mlt_transition transition = mlt_transition_new();

	if ( transition )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );

		if ( !createQApplicationIfNeeded( MLT_TRANSITION_SERVICE(transition) ) )
		{
			mlt_transition_close( transition );
			return NULL;
		}
		transition->process = process;
		mlt_properties_set_int( properties, "_transition_type", 1 ); // video only
		mlt_properties_set( properties, "rect", (char *) arg );
		mlt_properties_set_int( properties, "compositing", 0 );
		mlt_properties_set_int( properties, "distort", 0 );
		mlt_properties_set_int( properties, "rotate_center", 0 );
	}

	return transition;
}

} // extern "C"
