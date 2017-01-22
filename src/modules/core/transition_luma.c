/*
 * transition_luma.c -- a generic dissolve/wipe processor
 * Copyright (C) 2003-2014 Meltytech, LLC
 *
 * Adapted from Kino Plugin Timfx, which is
 * Copyright (C) 2002 Timothy M. Shead <tshead@k-3d.com>
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

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "transition_composite.h"

static inline int dissolve_yuv( mlt_frame frame, mlt_frame that, float weight, int width, int height )
{
	int ret = 0;
	int i = height + 1;
	int width_src = width, height_src = height;
	mlt_image_format format = mlt_image_yuv422;
	uint8_t *p_src, *p_dest;
	uint8_t *alpha_src;
	uint8_t *alpha_dst;
	int mix = weight * ( 1 << 16 );

	if ( mlt_properties_get( &frame->parent, "distort" ) )
		mlt_properties_set( &that->parent, "distort", mlt_properties_get( &frame->parent, "distort" ) );
	mlt_frame_get_image( frame, &p_dest, &format, &width, &height, 1 );
	alpha_dst = mlt_frame_get_alpha_mask( frame );
	mlt_frame_get_image( that, &p_src, &format, &width_src, &height_src, 0 );
	alpha_src = mlt_frame_get_alpha_mask( that );

	// Pick the lesser of two evils ;-)
	width_src = width_src > width ? width : width_src;
	height_src = height_src > height ? height : height_src;

	while ( --i )
	{
		composite_line_yuv( p_dest, p_src, width_src, alpha_src, alpha_dst, mix, NULL, 0, 0 );
		p_src += width_src << 1;
		p_dest += width << 1;
		alpha_src += width_src;
		alpha_dst += width;
	}

	return ret;
}

// image processing functions

static inline int32_t smoothstep( int32_t edge1, int32_t edge2, uint32_t a )
{
	if ( a < edge1 )
		return 0;

	if ( a >= edge2 )
		return 0x10000;

	a = ( ( a - edge1 ) << 16 ) / ( edge2 - edge1 );

	return ( ( ( a * a ) >> 16 )  * ( ( 3 << 16 ) - ( 2 * a ) ) ) >> 16;
}

/** powerful stuff

    \param field_order -1 = progressive, 0 = lower field first, 1 = top field first
*/
static void luma_composite( mlt_frame a_frame, mlt_frame b_frame, int luma_width, int luma_height,
							uint16_t *luma_bitmap, float pos, float frame_delta, float softness, int field_order,
							int *width, int *height )
{
	int width_src = *width, height_src = *height;
	int width_dest = *width, height_dest = *height;
	mlt_image_format format_src = mlt_image_yuv422, format_dest = mlt_image_yuv422;
	uint8_t *p_src, *p_dest;
	int i, j;
	int stride_src;
	int stride_dest;
	uint16_t weight = 0;

	if ( mlt_properties_get( &a_frame->parent, "distort" ) )
		mlt_properties_set( &b_frame->parent, "distort", mlt_properties_get( &a_frame->parent, "distort" ) );
	mlt_frame_get_image( a_frame, &p_dest, &format_dest, &width_dest, &height_dest, 1 );
	mlt_frame_get_image( b_frame, &p_src, &format_src, &width_src, &height_src, 0 );

	if ( *width == 0 || *height == 0 )
		return;

	// Pick the lesser of two evils ;-)
	width_src = width_src > width_dest ? width_dest : width_src;
	height_src = height_src > height_dest ? height_dest : height_src;
	
	stride_src = width_src * 2;
	stride_dest = width_dest * 2;

	// Offset the position based on which field we're looking at ...
	int32_t field_pos[ 2 ];
	field_pos[ 0 ] = ( pos + ( ( field_order == 0 ? 1 : 0 ) * frame_delta * 0.5 ) ) * ( 1 << 16 ) * ( 1.0 + softness );
	field_pos[ 1 ] = ( pos + ( ( field_order == 0 ? 0 : 1 ) * frame_delta * 0.5 ) ) * ( 1 << 16 ) * ( 1.0 + softness );

	register uint8_t *p;
	register uint8_t *q;
	register uint8_t *o;
	uint16_t  *l;

	uint32_t value;

	int32_t x_diff = ( luma_width << 16 ) / *width;
	int32_t y_diff = ( luma_height << 16 ) / *height;
	int32_t x_offset = 0;
	int32_t y_offset = 0;
	uint8_t *p_row;
	uint8_t *q_row;

	int32_t i_softness = softness * ( 1 << 16 );

	int field_count = field_order < 0 ? 1 : 2;
	int field_stride_src = field_count * stride_src;
	int field_stride_dest = field_count * stride_dest;
	int field = 0;

	// composite using luma map
	while ( field < field_count )
	{
		p_row = p_src + field * stride_src;
		q_row = p_dest + field * stride_dest;
		y_offset = field << 16;
		i = field;

		while ( i < height_src )
		{
			p = p_row;
			q = q_row;
			o = q;
			l = luma_bitmap + ( y_offset >> 16 ) * ( luma_width * field_count );
			x_offset = 0;
			j = width_src;

			while( j -- )
			{
             	weight = l[ x_offset >> 16 ];
   				value = smoothstep( weight, i_softness + weight, field_pos[ field ] );
				*o ++ = ( *p ++ * value + *q++ * ( ( 1 << 16 ) - value ) ) >> 16;
				*o ++ = ( *p ++ * value + *q++ * ( ( 1 << 16 ) - value ) ) >> 16;
				x_offset += x_diff;
			}

			y_offset += y_diff;
			i += field_count;
			p_row += field_stride_src;
			q_row += field_stride_dest;
		}

		field ++;
	}
}

/** Load the luma map from PGM stream.
*/

static void luma_read_pgm( FILE *f, uint16_t **map, int *width, int *height )
{
	uint8_t *data = NULL;
	while (1)
	{
		char line[128];
		char comment[128];
		int i = 2;
		int maxval;
		int bpp;
		uint16_t *p;

		line[127] = '\0';

		// get the magic code
		if ( fgets( line, 127, f ) == NULL )
			break;

		// skip comments
		while ( sscanf( line, " #%s", comment ) > 0 )
			if ( fgets( line, 127, f ) == NULL )
				break;

		if ( line[0] != 'P' || line[1] != '5' )
			break;

		// skip white space and see if a new line must be fetched
		for ( i = 2; i < 127 && line[i] != '\0' && isspace( line[i] ); i++ );
		if ( ( line[i] == '\0' || line[i] == '#' ) && fgets( line, 127, f ) == NULL )
			break;

		// skip comments
		while ( sscanf( line, " #%s", comment ) > 0 )
			if ( fgets( line, 127, f ) == NULL )
				break;

		// get the dimensions
		if ( line[0] == 'P' )
			i = sscanf( line, "P5 %d %d %d", width, height, &maxval );
		else
			i = sscanf( line, "%d %d %d", width, height, &maxval );

		// get the height value, if not yet
		if ( i < 2 )
		{
			if ( fgets( line, 127, f ) == NULL )
				break;

			// skip comments
			while ( sscanf( line, " #%s", comment ) > 0 )
				if ( fgets( line, 127, f ) == NULL )
					break;

			i = sscanf( line, "%d", height );
			if ( i == 0 )
				break;
			else
				i = 2;
		}

		// get the maximum gray value, if not yet
		if ( i < 3 )
		{
			if ( fgets( line, 127, f ) == NULL )
				break;

			// skip comments
			while ( sscanf( line, " #%s", comment ) > 0 )
				if ( fgets( line, 127, f ) == NULL )
					break;

			i = sscanf( line, "%d", &maxval );
			if ( i == 0 )
				break;
		}

		// determine if this is one or two bytes per pixel
		bpp = maxval > 255 ? 2 : 1;

		// allocate temporary storage for the raw data
		data = mlt_pool_alloc( *width * *height * bpp );
		if ( data == NULL )
			break;

		// read the raw data
		if ( fread( data, *width * *height * bpp, 1, f ) != 1 )
			break;

		// allocate the luma bitmap
		*map = p = (uint16_t*)mlt_pool_alloc( *width * *height * sizeof( uint16_t ) );
		if ( *map == NULL )
			break;

		// proces the raw data into the luma bitmap
		for ( i = 0; i < *width * *height * bpp; i += bpp )
		{
			if ( bpp == 1 )
				*p++ = data[ i ] << 8;
			else
				*p++ = ( data[ i ] << 8 ) + data[ i+1 ];
		}

		break;
	}

	if ( data != NULL )
		mlt_pool_release( data );
}

/** Generate a luma map from an RGB image.
*/

static void luma_read_yuv422( uint8_t *image, uint16_t **map, int width, int height )
{
	int i;
	int size = width * height * 2;
	
	// allocate the luma bitmap
	uint16_t *p = *map = ( uint16_t* )mlt_pool_alloc( width * height * sizeof( uint16_t ) );
	if ( *map == NULL )
		return;

	// proces the image data into the luma bitmap
	for ( i = 0; i < size; i += 2 )
		*p++ = ( image[ i ] - 16 ) * 299; // 299 = 65535 / 219
}

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// Get the transition object
	mlt_transition transition = mlt_frame_pop_service( a_frame );

	// Get the properties of the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );

	// Get the properties of the a frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// This compositer is yuv422 only
	*format = mlt_image_yuv422;

	mlt_service_lock( MLT_TRANSITION_SERVICE( transition ) );

	// The cached luma map information
	int luma_width = mlt_properties_get_int( properties, "width" );
	int luma_height = mlt_properties_get_int( properties, "height" );
	uint16_t *luma_bitmap = mlt_properties_get_data( properties, "bitmap", NULL );
	char *current_resource = mlt_properties_get( properties, "_resource" );
	
	// If the filename property changed, reload the map
	char *resource = mlt_properties_get( properties, "resource" );

	// Correct width/height if not specified
	if ( luma_width == 0 || luma_height == 0 )
	{
		luma_width = *width;
		luma_height = *height;
	}
		
	if ( resource && ( !current_resource || strcmp( resource, current_resource ) ) )
	{
		char temp[ 512 ];
		char *extension = strrchr( resource, '.' );
		char *orig_resource = resource;

		if ( strchr( resource, '%' ) )
		{
			FILE *test;
			sprintf( temp, "%s/lumas/%s/%s", mlt_environment( "MLT_DATA" ), mlt_environment( "MLT_NORMALISATION" ), strchr( resource, '%' ) + 1 );
			test = fopen( temp, "r" );
			if ( test == NULL )
				strcat( temp, ".png" );
			else
				fclose( test ); 
			resource = temp;
			extension = strrchr( resource, '.' );
		}

		// See if it is a PGM
		if ( extension != NULL && strcmp( extension, ".pgm" ) == 0 )
		{
			// Convert file name string encoding.
			mlt_properties_set( properties, "_resource_utf8", resource );
			mlt_properties_from_utf8( properties, "_resource_utf8", "_resource_local8" );

			// Open PGM
			FILE *f = fopen( mlt_properties_get( properties, "_resource_local8" ), "rb" );
			if ( f != NULL )
			{
				// Load from PGM
				luma_read_pgm( f, &luma_bitmap, &luma_width, &luma_height );
				fclose( f );

				// Set the transition properties
				mlt_properties_set_int( properties, "width", luma_width );
				mlt_properties_set_int( properties, "height", luma_height );
				mlt_properties_set( properties, "_resource", orig_resource );
				mlt_properties_set_data( properties, "bitmap", luma_bitmap, luma_width * luma_height * 2, mlt_pool_release, NULL );
			}
		}
		else if (!*resource) 
		{
		    luma_bitmap = NULL;
		    mlt_properties_set( properties, "_resource", NULL );
		    mlt_properties_set_data( properties, "bitmap", luma_bitmap, 0, mlt_pool_release, NULL );
		}
		else
		{
			// Get the factory producer service
			char *factory = mlt_properties_get( properties, "factory" );

			// Create the producer
			mlt_profile profile = mlt_service_profile( MLT_TRANSITION_SERVICE( transition ) );
			mlt_producer producer = mlt_factory_producer( profile, factory, resource );

			// If we have one
			if ( producer != NULL )
			{
				// Get the producer properties
				mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

				// Ensure that we loop
				mlt_properties_set( producer_properties, "eof", "loop" );

				// Now pass all producer. properties on the transition down
				mlt_properties_pass( producer_properties, properties, "producer." );

				// We will get the alpha frame from the producer
				mlt_frame luma_frame = NULL;

				// Get the luma frame
				if ( mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &luma_frame, 0 ) == 0 )
				{
					uint8_t *luma_image = NULL;
					mlt_image_format luma_format = mlt_image_yuv422;

					// Get image from the luma producer
					mlt_properties_set( MLT_FRAME_PROPERTIES( luma_frame ), "rescale.interp", "nearest" );
					mlt_frame_get_image( luma_frame, &luma_image, &luma_format, &luma_width, &luma_height, 0 );

					// Generate the luma map
					if ( luma_image != NULL )
						luma_read_yuv422( luma_image, &luma_bitmap, luma_width, luma_height );
					
					// Set the transition properties
					mlt_properties_set_int( properties, "width", luma_width );
					mlt_properties_set_int( properties, "height", luma_height );
					mlt_properties_set( properties, "_resource", orig_resource);
					mlt_properties_set_data( properties, "bitmap", luma_bitmap, luma_width * luma_height * 2, mlt_pool_release, NULL );

					// Cleanup the luma frame
					mlt_frame_close( luma_frame );
				}

				// Cleanup the luma producer
				mlt_producer_close( producer );
			}
		}
	}

	// Arbitrary composite defaults
	float mix = mlt_transition_get_progress( transition, a_frame );
	float frame_delta = mlt_transition_get_progress_delta( transition, a_frame );
	float luma_softness = mlt_properties_get_double( properties, "softness" );
	int progressive = 
			mlt_properties_get_int( a_props, "consumer_deinterlace" ) ||
			mlt_properties_get_int( properties, "progressive" ) ||
			mlt_properties_get_int( b_props, "luma.progressive" );
	int top_field_first =  mlt_properties_get_int( b_props, "top_field_first" );
	int reverse = mlt_properties_get_int( properties, "reverse" );
	int invert = mlt_properties_get_int( properties, "invert" );

	// Honour the reverse here
	if ( mix >= 1.0 )
		mix -= floor( mix );

	if ( mlt_properties_get( properties, "fixed" ) )
		mix = mlt_properties_get_double( properties, "fixed" );

	mlt_service_unlock( MLT_TRANSITION_SERVICE( transition ) );

	if ( luma_width > 0 && luma_height > 0 && luma_bitmap != NULL )
	{
		reverse = invert ? !reverse : reverse;
		mix = reverse ? 1 - mix : mix;
		frame_delta *= reverse ? -1.0 : 1.0;
		// Composite the frames using a luma map
		luma_composite( !invert ? a_frame : b_frame, !invert ? b_frame : a_frame, luma_width, luma_height, luma_bitmap, mix, frame_delta,
			luma_softness, progressive ? -1 : top_field_first, width, height );
	}
	else
	{
		mix = ( reverse || invert ) ? 1 - mix : mix;
		invert = 0;
		// Dissolve the frames using the time offset for mix value
		dissolve_yuv( a_frame, b_frame, mix, *width, *height );
	}

	// Extract the a_frame image info
	*width = mlt_properties_get_int( !invert ? a_props : b_props, "width" );
	*height = mlt_properties_get_int( !invert ? a_props : b_props, "height" );
	*image = mlt_properties_get_data( !invert ? a_props : b_props, "image", NULL );

	return 0;
}


/** Luma transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	// Push the transition on to the frame
	mlt_frame_push_service( a_frame, transition );

	// Push the b_frame on to the stack
	mlt_frame_push_frame( a_frame, b_frame );

	// Push the transition method
	mlt_frame_push_get_image( a_frame, transition_get_image );
	
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_luma_init( mlt_profile profile, mlt_service_type type, const char *id, char *lumafile )
{
	mlt_transition transition = mlt_transition_new( );
	if ( transition != NULL )
	{
		// Set the methods
		transition->process = transition_process;
		
		// Default factory
		mlt_properties_set( MLT_TRANSITION_PROPERTIES( transition ), "factory", mlt_environment( "MLT_PRODUCER" ) );

		// Set the main property
		mlt_properties_set( MLT_TRANSITION_PROPERTIES( transition ), "resource", lumafile );
		
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );

		return transition;
	}
	return NULL;
}
