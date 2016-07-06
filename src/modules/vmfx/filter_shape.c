/*
 * filter_shape.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2005 Visual Media Fx Inc.
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt.h>
#include <string.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_geometry.h>

static inline double smoothstep( const double e1, const double e2, const double a )
{
    if ( a < e1 ) return 0.0;
    if ( a > e2 ) return 1.0;
    double v = ( a - e1 ) / ( e2 - e1 );
    return ( v * v * ( 3 - 2 * v ) );
}

/** Get the images and apply the luminance of the mask to the alpha of the frame.
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Fetch the data from the stack (mix, mask, filter)
	double mix = mlt_deque_pop_back_double( MLT_FRAME_IMAGE_STACK( this ) );
	mlt_frame mask = mlt_frame_pop_service( this );
	mlt_filter filter = mlt_frame_pop_service( this );

	// Obtain the constants
	double softness = mlt_properties_get_double( MLT_FILTER_PROPERTIES( filter ), "softness" );
	int use_luminance = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "use_luminance" );
	int invert = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "invert" ) * 255;

	// Render the frame
	*format = mlt_image_yuv422;
	if ( mlt_frame_get_image( this, image, format, width, height, writable ) == 0 && ( !use_luminance || ( int )mix != 1 ) )
	{
		// Get the alpha mask of the source
		uint8_t *alpha = mlt_frame_get_alpha_mask( this );

		// Obtain a scaled/distorted mask to match
		uint8_t *mask_img = NULL;
		mlt_image_format mask_fmt = mlt_image_yuv422;
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( mask ), "distort", 1 );
		mlt_properties_pass_list( MLT_FRAME_PROPERTIES( mask ), MLT_FRAME_PROPERTIES( this ), "deinterlace,deinterlace_method,rescale.interp" );

		if ( mlt_frame_get_image( mask, &mask_img, &mask_fmt, width, height, 0 ) == 0 )
		{
			int size = *width * *height;
			uint8_t *p = alpha;
			double a = 0;
			double b = 0;
			if ( !use_luminance )
			{
				uint8_t *q = mlt_frame_get_alpha_mask( mask );
				while( size -- )
				{
					a = ( double )*q ++ / 255.0;
            		b = 1.0 - smoothstep( a, a + softness, mix );
					*p = ( uint8_t )( *p * b ) ^ invert;
					p ++;
				}
			}
			else if ( ( int )mix != 1 )
			{
				uint8_t *q = mask_img;
				// Ensure softness tends to zero has mix tends to 1
				softness *= ( 1.0 - mix );
				while( size -- )
				{
					a = ( ( double )*q - 16 ) / 235.0;
            		b = smoothstep( a, a + softness, mix );
					*p = ( uint8_t )( *p * b ) ^ invert;
					p ++;
					q += 2;
				}
			}
		}
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Obtain the shape instance
	char *resource = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "resource" );
	char *last_resource = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "_resource" );
	mlt_producer producer = mlt_properties_get_data( MLT_FILTER_PROPERTIES( this ), "instance", NULL );

	// Get the key framed values
	mlt_geometry alpha = mlt_properties_get_data( MLT_FILTER_PROPERTIES( this ), "_alpha", NULL );
	char *alpha_data = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "mix" );
	double alpha_mix = 0.0;

	// Calculate the position and length
	int position = mlt_filter_get_position( this, frame );
	mlt_position length = mlt_filter_get_length2( this, frame );

	// If we haven't created the instance or it's changed
	if ( producer == NULL || strcmp( resource, last_resource ) )
	{
		char temp[ 512 ];
		char *extension = strrchr( resource, '.' );

		// Store the last resource now
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "_resource", resource );

		// This is a hack - the idea is that we can indirectly reference the
		// luma modules pgm or png images by a short cut like %luma01.pgm - we then replace
		// the % with the full path to the image and use it if it exists, if not, check for
		// the file ending in a .png, and failing that, default to a fade in
		if ( strchr( resource, '%' ) )
		{
			FILE *test;
			sprintf( temp, "%s/lumas/%s/%s", mlt_environment( "MLT_DATA" ), mlt_environment( "MLT_NORMALISATION" ), strchr( resource, '%' ) + 1 );
			test = fopen( temp, "r" );

			if ( test == NULL )
			{
				strcat( temp, ".png" );
				test = fopen( temp, "r" );
			}

			if ( test )
				fclose( test ); 
			else
				strcpy( temp, "colour:0x00000080" );

			resource = temp;
			extension = strrchr( resource, '.' );
		}

		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( this ) );
		producer = mlt_factory_producer( profile, NULL, resource );
		if ( producer != NULL )
			mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "instance", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
	}

	// Construct the geometry item if needed, otherwise refresh it
	if ( alpha == NULL )
	{
		alpha = mlt_geometry_init( );
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "_alpha", alpha, 0, ( mlt_destructor )mlt_geometry_close, NULL );
		mlt_geometry_parse( alpha, alpha_data, length, 100, 100 );
	}
	else
	{
		mlt_geometry_refresh( alpha, alpha_data, length, 100, 100 );
	}

	// We may still not have a producer in which case, we do nothing
	if ( producer != NULL )
	{
		mlt_frame mask = NULL;
		struct mlt_geometry_item_s item;
		mlt_geometry_fetch( alpha, &item, position );
		alpha_mix = item.x;
		mlt_properties_pass( MLT_PRODUCER_PROPERTIES( producer ), MLT_FILTER_PROPERTIES( this ), "producer." );
		mlt_producer_seek( producer, position );
		if ( mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &mask, 0 ) == 0 )
		{
			char *name = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "_unique_id" );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), name, mask, 0, ( mlt_destructor )mlt_frame_close, NULL );
			mlt_frame_push_service( frame, this );
			mlt_frame_push_service( frame, mask );
			mlt_deque_push_back_double( MLT_FRAME_IMAGE_STACK( frame ), alpha_mix / 100.0 );
			mlt_frame_push_get_image( frame, filter_get_image );
			if ( mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "audio_match" ) )
			{
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "meta.mixdown", 1 );
				mlt_properties_set_double( MLT_FRAME_PROPERTIES( frame ), "meta.volume", alpha_mix / 100.0 );
			}
		}
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_shape_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "resource", arg );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "mix", "100" );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "audio_match", 1 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "invert", 0 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( this ), "softness", 0.1 );
		this->process = filter_process;
	}
	return this;
}

