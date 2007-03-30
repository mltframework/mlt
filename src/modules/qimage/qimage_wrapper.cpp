/*
 * qimage_wrapper.cpp -- a QT/QImage based producer for MLT
 * Copyright (C) 2006 Visual Media
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * NB: This module is designed to be functionally equivalent to the 
 * gtk2 image loading module so it can be used as replacement.
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

#include "qimage_wrapper.h"
#include <qimage.h>
#include <cmath>

extern "C" {

#include <framework/mlt_pool.h>

static void qimage_delete( void *data )
{
	QImage *image = ( QImage * )data;
	delete image;
}

static void clear_buffered_image( mlt_properties producer_props, uint8_t **current_image, uint8_t **current_alpha )
{
	mlt_events_block( producer_props, NULL );
	mlt_properties_set_data( producer_props, "_qimage_image", NULL, 0, NULL, NULL );
	mlt_properties_set_data( producer_props, "_qimage_alpha", NULL, 0, NULL, NULL );
	*current_image = NULL;
	*current_alpha = NULL;
	mlt_events_unblock( producer_props, NULL );
}

static void assign_buffered_image( mlt_properties producer_props, uint8_t *current_image, uint8_t *current_alpha, int width, int height )
{
	int use_cache = mlt_properties_get_int( producer_props, "cache" );
	mlt_destructor destructor = use_cache ? NULL : mlt_pool_release;
	mlt_events_block( producer_props, NULL );
	mlt_properties_set_data( producer_props, "_qimage_image", current_image, 0, destructor, NULL );
	mlt_properties_set_data( producer_props, "_qimage_alpha", current_alpha, 0, destructor, NULL );
	mlt_properties_set_int( producer_props, "_qimage_width", width );
	mlt_properties_set_int( producer_props, "_qimage_height", height );
	mlt_events_unblock( producer_props, NULL );
}

void refresh_qimage( mlt_frame frame, int width, int height )
{
	// Obtain a previous assigned qimage (if it exists) 
	QImage *qimage = ( QImage * )mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "qimage", NULL );

	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the producer for this frame
	producer_qimage self = ( producer_qimage )mlt_properties_get_data( properties, "producer_qimage", NULL );

	// Obtain the producer 
	mlt_producer producer = &self->parent;

	// Obtain properties of producer
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Obtain the cache flag and structure
	int use_cache = mlt_properties_get_int( producer_props, "cache" );
	mlt_properties cache = ( mlt_properties )mlt_properties_get_data( producer_props, "_cache", NULL );
	int update_cache = 0;

	// Retrieve current info if available
	uint8_t *current_image = ( uint8_t * )mlt_properties_get_data( producer_props, "_qimage_image", NULL );
	uint8_t *current_alpha = ( uint8_t * )mlt_properties_get_data( producer_props, "_qimage_alpha", NULL );
	int current_width = mlt_properties_get_int( producer_props, "_qimage_width" );
	int current_height = mlt_properties_get_int( producer_props, "_qimage_height" );

	// Get the time to live for each frame
	double ttl = mlt_properties_get_int( producer_props, "ttl" );

	// Get the original position of this frame
	mlt_position position = mlt_properties_get_position( properties, "qimage_position" );

	// Image index
	int image_idx = ( int )floor( ( double )position / ttl ) % self->count;

	// Key for the cache
	char image_key[ 10 ];
	sprintf( image_key, "%d", image_idx );

	// Check if the frame is already loaded
	if ( use_cache )
	{
		if ( cache == NULL )
		{
			cache = mlt_properties_new( );
			mlt_properties_set_data( producer_props, "_cache", cache, 0, ( mlt_destructor )mlt_properties_close, NULL );
		}

		mlt_frame cached = ( mlt_frame )mlt_properties_get_data( cache, image_key, NULL );

		if ( cached )
		{
			self->image_idx = image_idx;
			mlt_properties cached_props = MLT_FRAME_PROPERTIES( cached );
			current_width = mlt_properties_get_int( cached_props, "width" );
			current_height = mlt_properties_get_int( cached_props, "height" );
			mlt_properties_set_int( producer_props, "_real_width", mlt_properties_get_int( cached_props, "real_width" ) );
			mlt_properties_set_int( producer_props, "_real_height", mlt_properties_get_int( cached_props, "real_height" ) );
			current_image = ( uint8_t * )mlt_properties_get_data( cached_props, "image", NULL );
			current_alpha = ( uint8_t * )mlt_properties_get_data( cached_props, "alpha", NULL );

			if ( width != 0 && ( width != current_width || height != current_height ) )
				current_image = NULL;

			assign_buffered_image( producer_props, current_image, current_alpha, current_width, current_height );
		}
	}

    // optimization for subsequent iterations on single picture
	if ( width != 0 && current_image != NULL && image_idx == self->image_idx )
	{
		if ( width != current_width || height != current_height )
		{
			qimage = ( QImage * )mlt_properties_get_data( producer_props, "_qimage", NULL );
			clear_buffered_image( producer_props, &current_image, &current_alpha );
		}
	}
	else if ( qimage == NULL && ( current_image == NULL || image_idx != self->image_idx ) )
	{
		clear_buffered_image( producer_props, &current_image, &current_alpha );

		self->image_idx = image_idx;
		qimage = new QImage( mlt_properties_get_value( self->filenames, image_idx ) );

		if ( !qimage->isNull( ) )
		{
			QImage *frame_copy = new QImage( *qimage );

			// Store the width/height of the pixbuf 
			current_width = qimage->width( );
			current_height = qimage->height( );

			// Register qimage for destruction and reuse
			mlt_events_block( producer_props, NULL );
			mlt_properties_set_data( producer_props, "_qimage", qimage, 0, ( mlt_destructor )qimage_delete, NULL );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "qimage", frame_copy, 0, ( mlt_destructor )qimage_delete, NULL );
			mlt_properties_set_int( producer_props, "_real_width", current_width );
			mlt_properties_set_int( producer_props, "_real_height", current_height );
			mlt_events_unblock( producer_props, NULL );
		}
		else
		{
			delete qimage;
			qimage = NULL;
		}
	}

	// If we have a pixbuf and this request specifies a valid dimension and we haven't already got a cached version...
	if ( qimage && width > 0 && current_image == NULL )
	{
		char *interps = mlt_properties_get( properties, "rescale.interp" );
		int interp = 0;

		// QImage has two scaling modes - we'll toggle between them here
		if ( strcmp( interps, "tiles" ) == 0 )
			interp = 1;
		else if ( strcmp( interps, "hyper" ) == 0 )
			interp = 1;

		// Note - the original qimage is already safe and ready for destruction
		QImage scaled = interp == 0 ? qimage->scale( width, height, QImage::ScaleFree ) : qimage->smoothScale( width, height, QImage::ScaleFree );
		QImage temp = scaled.convertDepth( 32 );

		// Store width and height
		current_width = width;
		current_height = height;
		
		// Allocate/define image
		current_image = ( uint8_t * )mlt_pool_alloc( width * ( height + 1 ) * 2 );

		// Allocate the alpha mask
		current_alpha = ( uint8_t * )mlt_pool_alloc( current_width * current_height );

		// Convert the image
		if ( QImage::systemByteOrder( ) == QImage::BigEndian )
			mlt_convert_argb_to_yuv422( temp.bits( ), current_width, current_height, temp.bytesPerLine( ), current_image, current_alpha );
		else
			mlt_convert_bgr24a_to_yuv422( temp.bits( ), current_width, current_height, temp.bytesPerLine( ), current_image, current_alpha );

		assign_buffered_image( producer_props, current_image, current_alpha, current_width, current_height );

		// Ensure we update the cache when we need to
		update_cache = use_cache;
	}

	// Set width/height of frame
	mlt_properties_set_int( properties, "width", current_width );
	mlt_properties_set_int( properties, "height", current_height );
	mlt_properties_set_int( properties, "real_width", mlt_properties_get_int( producer_props, "_real_width" ) );
	mlt_properties_set_int( properties, "real_height", mlt_properties_get_int( producer_props, "_real_height" ) );

	// pass the image data without destructor
	mlt_properties_set_data( properties, "image", current_image, current_width * ( current_height + 1 ) * 2, NULL, NULL );
	mlt_properties_set_data( properties, "alpha", current_alpha, current_width * current_height, NULL, NULL );

	if ( update_cache )
	{
		mlt_frame cached = mlt_frame_init( );
		mlt_properties cached_props = MLT_FRAME_PROPERTIES( cached );
		mlt_properties_set_int( cached_props, "width", current_width );
		mlt_properties_set_int( cached_props, "height", current_height );
		mlt_properties_set_int( cached_props, "real_width", mlt_properties_get_int( producer_props, "_real_width" ) );
		mlt_properties_set_int( cached_props, "real_height", mlt_properties_get_int( producer_props, "_real_height" ) );
		mlt_properties_set_data( cached_props, "image", current_image, current_width * ( current_height + 1 ) * 2, mlt_pool_release, NULL );
		mlt_properties_set_data( cached_props, "alpha", current_alpha, current_width * current_height, mlt_pool_release, NULL );
		mlt_properties_set_data( cache, image_key, cached, 0, ( mlt_destructor )mlt_frame_close, NULL );
	}
}

}

