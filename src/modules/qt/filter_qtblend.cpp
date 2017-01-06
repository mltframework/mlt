/*
 * filter_lightshow.cpp -- animate color to the audio
 * Copyright (C) 2015 Meltytech, LLC
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
#include <stdlib.h> // calloc(), free()
#include <math.h>   // sin()
#include <QPainter>
#include <QTransform>
#include <QImage>

static int get_value( mlt_properties properties, const char *preferred, const char *fallback )
{
	int value = mlt_properties_get_int( properties, preferred );
	if ( value == 0 )
		value = mlt_properties_get_int( properties, fallback );
	return value;
}


/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	// Get the filter
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );

	// Get the properties
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Get the image
	*format = mlt_image_rgb24a;

	// Only process if we have no error and a valid colour space
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
		
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	double output_ar = mlt_profile_sar( profile );
	// Special case - aspect_ratio = 0
	if ( mlt_frame_get_aspect_ratio( frame ) == 0 )
		mlt_frame_set_aspect_ratio( frame, output_ar );

	// Check transform
	QTransform transform;
	int normalised_width = profile->width;
	int normalised_height = profile->height;
	double consumer_ar = mlt_profile_sar( profile );

	// Destination rect
	mlt_rect rect;
	rect.w = normalised_width;
	rect.h = normalised_height;
	int b_width = get_value( frame_properties, "meta.media.width", "width" );
	int b_height = get_value( frame_properties, "meta.media.height", "height" );
	double b_ar = mlt_frame_get_aspect_ratio( frame );
	double b_dar = b_ar * b_width / b_height;

	double opacity = 1.0;
	if ( mlt_properties_get( properties, "rect" ) )
	{
		rect = mlt_properties_anim_get_rect( properties, "rect", position, length );
		transform.translate(rect.x, rect.y);
		opacity = rect.o;
	}

	if ( mlt_properties_get( properties, "rotation" ) )
	{
		double angle = mlt_properties_anim_get_double( properties, "rotation", position, length );
		transform.rotate( angle );
	}

	// fetch image
	*format = mlt_image_rgb24a;
	uint8_t *src_image = NULL;

	error = mlt_frame_get_image( frame, &src_image, format, &b_width, &b_height, 0 );

	// Put source buffer into QImage
	QImage sourceImage;
	convert_mlt_to_qimage_rgba( src_image, &sourceImage, b_width, b_height );
	
	int image_size = mlt_image_format_size( *format, *width, *height, NULL );
	
	// resize to rect
	if ( mlt_properties_get_int( properties, "distort" ) )
	{
		transform.scale( rect.w / b_width, rect.h / b_height );
	}
	else
	{
		// Take the smaller scale between horizontal and vertical
		float scale_x = MIN( rect.w / b_width, rect.h / b_height );
		float scale_y = scale_x;

		// Determine scale with respect to aspect ratio.
		double consumer_dar = consumer_ar * normalised_width / normalised_height;
		if ( b_dar > consumer_dar )
		{
			scale_y = scale_x;
			scale_y *= consumer_ar / b_ar;
		}
		else
		{
			scale_x = scale_y;
			scale_x *= b_ar / consumer_ar;
		}
		// Center image in rect
		transform.translate( ( rect.w - ( b_width * scale_x ) ) / 2.0, ( rect.h - ( b_height * scale_y ) ) / 2.0 );
		transform.scale( scale_x, scale_y );
	}

	uint8_t *dest_image = NULL;
	dest_image = (uint8_t *) mlt_pool_alloc( image_size );
	
	QImage destImage;
	convert_mlt_to_qimage_rgba( dest_image, &destImage, *width, *height );
	destImage.fill(0);
	
	QPainter painter( &destImage );
	painter.setCompositionMode( ( QPainter::CompositionMode ) mlt_properties_get_int( properties, "compositing" ) );
	painter.setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
	painter.setTransform(transform);
	painter.setOpacity(opacity);
	// Composite top frame
	painter.drawImage(0, 0, sourceImage);
	// finish Qt drawing
	painter.end();
	convert_qimage_to_mlt_rgba( &destImage, dest_image, *width, *height );
	*image = dest_image;
	mlt_frame_set_image( frame, *image, *width * *height * 4, mlt_pool_release );
	return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{	
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

extern "C" {

mlt_filter filter_qtblend_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();

	if ( filter && createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )
	{
		filter->process = filter_process;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter qtblend failed\n" );

		if( filter )
		{
			mlt_filter_close( filter );
		}

		filter = NULL;
	}
	return filter;
}

}

