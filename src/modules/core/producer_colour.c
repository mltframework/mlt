/*
 * producer_colour.c
 * Copyright (C) 2003-2020 Meltytech, LLC
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

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_pool.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_colour_init( mlt_profile profile, mlt_service_type type, const char *id, char *colour )
{
	mlt_producer producer = calloc( 1, sizeof( struct mlt_producer_s ) );
	if ( producer != NULL && mlt_producer_init( producer, NULL ) == 0 )
	{
		// Get the properties interface
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	
		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		// Set the default properties
		mlt_properties_set( properties, "resource", ( !colour || !strcmp( colour, "" ) ) ? "0x000000ff" : colour );
		mlt_properties_set( properties, "_resource", "" );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
		
		return producer;
	}
	free( producer );
	return NULL;
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the producer for this frame
	mlt_producer producer = mlt_frame_pop_service( frame );

	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	// Obtain properties of producer
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Get the current and previous colour strings
	char *now = mlt_properties_get( producer_props, "resource" );
	char *then = mlt_properties_get( producer_props, "_resource" );

	// Get the current image and dimensions cached in the producer
	int size = 0;
	uint8_t *image = mlt_properties_get_data( producer_props, "image", &size );
	int current_width = mlt_properties_get_int( producer_props, "_width" );
	int current_height = mlt_properties_get_int( producer_props, "_height" );
	mlt_image_format current_format = mlt_properties_get_int( producer_props, "_format" );

	// Parse the colour
	mlt_color color = mlt_properties_get_color( producer_props, "resource" );

	if ( mlt_properties_get( producer_props, "mlt_image_format") )
		*format = mlt_image_format_id( mlt_properties_get( producer_props, "mlt_image_format") );

	// Choose suitable out values if nothing specific requested
	if ( *format == mlt_image_none || *format == mlt_image_movit )
		*format = mlt_image_rgba;
	if ( *width <= 0 )
		*width = mlt_service_profile( MLT_PRODUCER_SERVICE(producer) )->width;
	if ( *height <= 0 )
		*height = mlt_service_profile( MLT_PRODUCER_SERVICE(producer) )->height;
	
	// Choose default image format if specific request is unsupported
	if (*format!=mlt_image_yuv420p  && *format!=mlt_image_yuv422  && *format!=mlt_image_rgb && *format!= mlt_image_movit && *format!= mlt_image_opengl_texture)
		*format = mlt_image_rgba;

	// See if we need to regenerate
	if ( !now || ( then && strcmp( now, then ) ) || *width != current_width || *height != current_height || *format != current_format )
	{
		// Color the image
		int i = *width * *height + 1;
		int bpp;

		// Allocate the image
		size = mlt_image_format_size( *format, *width, *height, &bpp );
		uint8_t *p = image = mlt_pool_alloc( size );

		// Update the producer
		mlt_properties_set_data( producer_props, "image", image, size, mlt_pool_release, NULL );
		mlt_properties_set_int( producer_props, "_width", *width );
		mlt_properties_set_int( producer_props, "_height", *height );
		mlt_properties_set_int( producer_props, "_format", *format );
		mlt_properties_set( producer_props, "_resource", now );

		mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

		switch ( *format )
		{
		case mlt_image_yuv420p:
		{
			int plane_size =  *width * *height;
			uint8_t y, u, v;

			RGB2YUV_601_SCALED( color.r, color.g, color.b, y, u, v );
			memset(p + 0, y, plane_size);
			memset(p + plane_size, u, plane_size/4);
			memset(p + plane_size + plane_size/4, v, plane_size/4);
			mlt_properties_set_int( properties, "colorspace", 601 );
			break;
		}
		case mlt_image_yuv422:
		{
			int uneven = *width % 2;
			int count = ( *width - uneven ) / 2 + 1;
			uint8_t y, u, v;

			RGB2YUV_601_SCALED( color.r, color.g, color.b, y, u, v );
			i = *height + 1;
			while ( --i )
			{
				int j = count;
				while ( --j )
				{
					*p ++ = y;
					*p ++ = u;
					*p ++ = y;
					*p ++ = v;
				}
				if ( uneven )
				{
					*p ++ = y;
					*p ++ = u;
				}
			}
			mlt_properties_set_int( properties, "colorspace", 601 );
			break;
		}
		case mlt_image_rgb:
			while ( --i )
			{
				*p ++ = color.r;
				*p ++ = color.g;
				*p ++ = color.b;
			}
			break;
		case mlt_image_movit:
		case mlt_image_opengl_texture:
			memset(p, 0, size);
			break;
		case mlt_image_rgba:
			while ( --i )
			{
				*p ++ = color.r;
				*p ++ = color.g;
				*p ++ = color.b;
				*p ++ = color.a;
			}
			break;
		default:
			mlt_log_error( MLT_PRODUCER_SERVICE( producer ),
				"invalid image format %s\n", mlt_image_format_name( *format ) );
		}
	}
	else
	{
		mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );
	}

	// Create the alpha channel
	int alpha_size = 0;
	uint8_t *alpha = NULL;

	// Initialise the alpha
	if (color.a < 255 || *format == mlt_image_rgba) {
		alpha_size = *width * *height;
		alpha = mlt_pool_alloc( alpha_size );
		if ( alpha )
			memset( alpha, color.a, alpha_size );
		else
			alpha_size = 0;
	}

	// Clone our image
	if (buffer && image && size > 0) {
		*buffer = mlt_pool_alloc( size );
		memcpy( *buffer, image, size );
	}

	// Now update properties so we free the copy after
	mlt_frame_set_image( frame, *buffer, size, mlt_pool_release );
	mlt_frame_set_alpha( frame, alpha, alpha_size, mlt_pool_release );
	mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( producer_props, "aspect_ratio" ) );
	mlt_properties_set_int( properties, "meta.media.width", *width );
	mlt_properties_set_int( properties, "meta.media.height", *height );


	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Obtain properties of frame and producer
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		// Obtain properties of producer
		mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

		// Update timecode on the frame we're creating
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", 1 );
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
		mlt_properties_set_int( properties, "meta.media.width", profile->width );
		mlt_properties_set_int( properties, "meta.media.height", profile->height );

		// colour is an alias for resource
		if ( mlt_properties_get( producer_props, "colour" ) != NULL )
			mlt_properties_set( producer_props, "resource", mlt_properties_get( producer_props, "colour" ) );

		char *colorstring = mlt_properties_get( producer_props, "resource" );
		if ( colorstring && strchr( colorstring, '/' ) )
		{
			colorstring = strdup( strrchr( colorstring, '/' ) + 1 );
			mlt_properties_set( producer_props, "resource", colorstring );
			free( colorstring );
		}

		// Check if we have a predefined image format
		if ( mlt_properties_exists( producer_props, "mlt_image_format") )
		{
			int image_format = mlt_image_format_id( mlt_properties_get( producer_props, "mlt_image_format") );
			mlt_properties_set_int( properties, "format", image_format );
		} else {
			mlt_color color = mlt_properties_get_color( producer_props, "resource" );
			// Inform framework of the default frame format for this producer
			mlt_properties_set_int( properties, "format", color.a < 255 ? mlt_image_rgba : mlt_image_yuv422 );
		}

		// Push the get_image method
		mlt_frame_push_service( *frame, producer );
		mlt_frame_push_get_image( *frame, producer_get_image );

		// A hint to scalers and affine transition that this producer does not
		// benefit from interpolation.
		mlt_properties_set_int(properties, "interpolation_not_required", 1);
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	producer->close = NULL;
	mlt_producer_close( producer );
	free( producer );
}
