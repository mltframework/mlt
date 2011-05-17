/*
 * transition_composite.c -- compose one image over another using alpha channel
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "transition_composite.h"
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

typedef void ( *composite_line_fn )( uint8_t *dest, uint8_t *src, int width_src, uint8_t *alpha_b, uint8_t *alpha_a, int weight, uint16_t *luma, int softness, uint32_t step );

/** Geometry struct.
*/

struct geometry_s
{
	struct mlt_geometry_item_s item;
	int nw; // normalised width
	int nh; // normalised height
	int sw; // scaled width, not including consumer scale based upon w/nw
	int sh; // scaled height, not including consumer scale based upon h/nh
	int halign; // horizontal alignment: 0=left, 1=center, 2=right
	int valign; // vertical alignment: 0=top, 1=middle, 2=bottom
	int x_src;
	int y_src;
};

/** Parse the alignment properties into the geometry.
*/

static int alignment_parse( char* align )
{
	int ret = 0;
	
	if ( align == NULL );
	else if ( isdigit( align[ 0 ] ) )
		ret = atoi( align );
	else if ( align[ 0 ] == 'c' || align[ 0 ] == 'm' )
		ret = 1;
	else if ( align[ 0 ] == 'r' || align[ 0 ] == 'b' )
		ret = 2;

	return ret;
}

/** Calculate real geometry.
*/

static void geometry_calculate( mlt_transition this, struct geometry_s *output, double position )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
	mlt_geometry geometry = mlt_properties_get_data( properties, "geometries", NULL );
	int mirror_off = mlt_properties_get_int( properties, "mirror_off" );
	int repeat_off = mlt_properties_get_int( properties, "repeat_off" );
	int length = mlt_geometry_get_length( geometry );

	// Allow wrapping
	if ( !repeat_off && position >= length && length != 0 )
	{
		int section = position / length;
		position -= section * length;
		if ( !mirror_off && section % 2 == 1 )
			position = length - position;
	}

	// Fetch the key for the position
	mlt_geometry_fetch( geometry, &output->item, position );
}

static mlt_geometry transition_parse_keys( mlt_transition this, int normalised_width, int normalised_height )
{
	// Loop variable for property interrogation
	int i = 0;

	// Get the properties of the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

	// Create an empty geometries object
	mlt_geometry geometry = mlt_geometry_init( );

	// Get the duration
	mlt_position length = mlt_transition_get_length( this );
	double cycle = mlt_properties_get_double( properties, "cycle" );

	// Get the new style geometry string
	char *property = mlt_properties_get( properties, "geometry" );

	// Allow a geometry repeat cycle
	if ( cycle >= 1 )
		length = cycle;
	else if ( cycle > 0 )
		length *= cycle;

	// Parse the geometry if we have one
	mlt_geometry_parse( geometry, property, length, normalised_width, normalised_height );

	// Check if we're using the old style geometry
	if ( property == NULL )
	{
		// DEPRECATED: Multiple keys for geometry information is inefficient and too rigid for 
		// practical use - while deprecated, it has been slightly extended too - keys can now
		// be specified out of order, and can be blanked or NULL to simulate removal

		// Structure to use for parsing and inserting
		struct mlt_geometry_item_s item;

		// Parse the start property
		item.frame = 0;
		if ( mlt_geometry_parse_item( geometry, &item, mlt_properties_get( properties, "start" ) ) == 0 )
			mlt_geometry_insert( geometry, &item );

		// Parse the keys in between
		for ( i = 0; i < mlt_properties_count( properties ); i ++ )
		{
			// Get the name of the property
			char *name = mlt_properties_get_name( properties, i );
	
			// Check that it's valid
			if ( !strncmp( name, "key[", 4 ) )
			{
				// Get the value of the property
				char *value = mlt_properties_get_value( properties, i );
	
				// Determine the frame number
				item.frame = atoi( name + 4 );
	
				// Parse and add to the list
				if ( mlt_geometry_parse_item( geometry, &item, value ) == 0 )
					mlt_geometry_insert( geometry, &item );
				else
					fprintf( stderr, "Invalid Key - skipping %s = %s\n", name, value );
			}
		}

		// Parse the end
		item.frame = -1;
		if ( mlt_geometry_parse_item( geometry, &item, mlt_properties_get( properties, "end" ) ) == 0 )
			mlt_geometry_insert( geometry, &item );
	}
	
	return geometry;
}

/** Adjust position according to scaled size and alignment properties.
*/

static void alignment_calculate( struct geometry_s *geometry )
{
	geometry->item.x += ( geometry->item.w - geometry->sw ) * geometry->halign / 2;
	geometry->item.y += ( geometry->item.h - geometry->sh ) * geometry->valign / 2;
}

/** Calculate the position for this frame.
*/

static int position_calculate( mlt_transition this, mlt_position position )
{
	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );

	// Now do the calcs
	return position - in;
}

/** Calculate the field delta for this frame - position between two frames.
*/

static int get_value( mlt_properties properties, const char *preferred, const char *fallback )
{
	int value = mlt_properties_get_int( properties, preferred );
	if ( value == 0 )
		value = mlt_properties_get_int( properties, fallback );
	return value;
}

/** A linear threshold determination function.
*/

static inline int32_t linearstep( int32_t edge1, int32_t edge2, int32_t a )
{
	if ( a < edge1 )
		return 0;

	if ( a >= edge2 )
		return 0x10000;

	return ( ( a - edge1 ) << 16 ) / ( edge2 - edge1 );
}

/** A smoother, non-linear threshold determination function.
*/

static inline int32_t smoothstep( int32_t edge1, int32_t edge2, uint32_t a )
{
	if ( a < edge1 )
		return 0;

	if ( a >= edge2 )
		return 0x10000;

	a = ( ( a - edge1 ) << 16 ) / ( edge2 - edge1 );

	return ( ( ( a * a ) >> 16 )  * ( ( 3 << 16 ) - ( 2 * a ) ) ) >> 16;
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
				*p++ = ( data[ i ] << 8 ) + data[ i + 1 ];
		}

		break;
	}

	if ( data != NULL )
		mlt_pool_release( data );
}

/** Generate a luma map from any YUV image.
*/

static void luma_read_yuv422( uint8_t *image, uint16_t **map, int width, int height )
{
	int i;
	
	// allocate the luma bitmap
	uint16_t *p = *map = ( uint16_t* )mlt_pool_alloc( width * height * sizeof( uint16_t ) );
	if ( *map == NULL )
		return;

	// proces the image data into the luma bitmap
	for ( i = 0; i < width * height * 2; i += 2 )
		*p++ = ( image[ i ] - 16 ) * 299; // 299 = 65535 / 219
}

static inline int calculate_mix( uint16_t *luma, int j, int softness, int weight, int alpha, uint32_t step )
{
	return ( ( luma ? smoothstep( luma[ j ], luma[ j ] + softness, step ) : weight ) * alpha ) >> 8;
}

static inline uint8_t sample_mix( uint8_t dest, uint8_t src, int mix )
{
	return ( src * mix + dest * ( ( 1 << 16 ) - mix ) ) >> 16;
}

/** Composite a source line over a destination line
*/

static void composite_line_yuv( uint8_t *dest, uint8_t *src, int width, uint8_t *alpha_b, uint8_t *alpha_a, int weight, uint16_t *luma, int soft, uint32_t step )
{
	register int j;
	register int mix;

	for ( j = 0; j < width; j ++ )
	{
		mix = calculate_mix( luma, j, soft, weight, *alpha_b ++, step );
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*alpha_a = ( mix >> 8 ) | *alpha_a;
		alpha_a ++;
	}
}

static void composite_line_yuv_or( uint8_t *dest, uint8_t *src, int width, uint8_t *alpha_b, uint8_t *alpha_a, int weight, uint16_t *luma, int soft, uint32_t step )
{
	register int j;
	register int mix;

	for ( j = 0; j < width; j ++ )
	{
		mix = calculate_mix( luma, j, soft, weight, *alpha_b ++ | *alpha_a, step );
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*alpha_a ++ = mix >> 8;
	}
}

static void composite_line_yuv_and( uint8_t *dest, uint8_t *src, int width, uint8_t *alpha_b, uint8_t *alpha_a, int weight, uint16_t *luma, int soft, uint32_t step  )
{
	register int j;
	register int mix;

	for ( j = 0; j < width; j ++ )
	{
		mix = calculate_mix( luma, j, soft, weight, *alpha_b ++ & *alpha_a, step );
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*alpha_a ++ = mix >> 8;
	}
}

static void composite_line_yuv_xor( uint8_t *dest, uint8_t *src, int width, uint8_t *alpha_b, uint8_t *alpha_a, int weight, uint16_t *luma, int soft, uint32_t step )
{
	register int j;
	register int mix;

	for ( j = 0; j < width; j ++ )
	{
		mix = calculate_mix( luma, j, soft, weight, *alpha_b ++ ^ *alpha_a, step );
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*dest = sample_mix( *dest, *src++, mix );
		dest++;
		*alpha_a ++ = mix >> 8;
	}
}

/** Composite function.
*/

static int composite_yuv( uint8_t *p_dest, int width_dest, int height_dest, uint8_t *p_src, int width_src, int height_src, uint8_t *alpha_b, uint8_t *alpha_a, struct geometry_s geometry, int field, uint16_t *p_luma, double softness, composite_line_fn line_fn )
{
	int ret = 0;
	int i;
	int x_src = -geometry.x_src, y_src = -geometry.y_src;
	int uneven_x_src = ( x_src % 2 );
	int step = ( field > -1 ) ? 2 : 1;
	int bpp = 2;
	int stride_src = geometry.sw * bpp;
	int stride_dest = width_dest * bpp;
	int i_softness = ( 1 << 16 ) * softness;
	int weight = ( ( 1 << 16 ) - 1 ) * geometry.item.mix / 100;
	uint32_t luma_step = ( ( 1 << 16 ) - 1 ) * geometry.item.mix / 100 * ( 1.0 + softness );

	// Adjust to consumer scale
	int x = rint( geometry.item.x * width_dest / geometry.nw );
	int y = rint( geometry.item.y * height_dest / geometry.nh );
	int uneven_x = ( x % 2 );

	// optimization points - no work to do
	if ( width_src <= 0 || height_src <= 0 || y_src >= height_src || x_src >= width_src )
		return ret;

	if ( ( x < 0 && -x >= width_src ) || ( y < 0 && -y >= height_src ) )
		return ret;

	// cropping affects the source width
	if ( x_src > 0 )
	{
		width_src -= x_src;
		// and it implies cropping
		if ( width_src > geometry.item.w )
			width_src = geometry.item.w;
	}

	// cropping affects the source height
	if ( y_src > 0 )
	{
		height_src -= y_src;
		// and it implies cropping
		if ( height_src > geometry.item.h )
			height_src = geometry.item.h;
	}

	// crop overlay off the left edge of frame
	if ( x < 0 )
	{
		x_src = -x;
		width_src -= x_src;
		x = 0;
	}

	// crop overlay beyond right edge of frame
	if ( x + width_src > width_dest )
		width_src = width_dest - x;

	// crop overlay off the top edge of the frame
	if ( y < 0 )
	{
		y_src = -y;
		height_src -= y_src;
		y = 0;
	}
	
	// crop overlay below bottom edge of frame
	if ( y + height_src > height_dest )
		height_src = height_dest - y;

	// offset pointer into overlay buffer based on cropping
	p_src += x_src * bpp + y_src * stride_src;

	// offset pointer into frame buffer based upon positive coordinates only!
	p_dest += ( x < 0 ? 0 : x ) * bpp + ( y < 0 ? 0 : y ) * stride_dest;

	// offset pointer into alpha channel based upon cropping
	alpha_b += x_src + y_src * stride_src / bpp;
	alpha_a += x + y * stride_dest / bpp;

	// offset pointer into luma channel based upon cropping
	if ( p_luma )
		p_luma += x_src + y_src * stride_src / bpp;
	
	// Assuming lower field first
	// Special care is taken to make sure the b_frame is aligned to the correct field.
	// field 0 = lower field and y should be odd (y is 0-based).
	// field 1 = upper field and y should be even.
	if ( ( field > -1 ) && ( y % 2 == field ) )
	{
		if ( ( field == 1 && y < height_dest - 1 ) || ( field == 0 && y == 0 ) )
			p_dest += stride_dest;
		else
			p_dest -= stride_dest;
	}

	// On the second field, use the other lines from b_frame
	if ( field == 1 )
	{
		p_src += stride_src;
		alpha_b += stride_src / bpp;
		alpha_a += stride_dest / bpp;
		height_src--;
	}

	stride_src *= step;
	stride_dest *= step;
	int alpha_b_stride = stride_src / bpp;
	int alpha_a_stride = stride_dest / bpp;

	// Align chroma of source and destination
	if ( uneven_x != uneven_x_src )
	{
		p_src += 2;
		width_src -= 2;
		alpha_b += 1;
	}

	// now do the compositing only to cropped extents
	for ( i = 0; i < height_src; i += step )
	{
		line_fn( p_dest, p_src, width_src, alpha_b, alpha_a, weight, p_luma, i_softness, luma_step );

		p_src += stride_src;
		p_dest += stride_dest;
		alpha_b += alpha_b_stride;
		alpha_a += alpha_a_stride;
		if ( p_luma )
			p_luma += alpha_b_stride;
	}

	return ret;
}


/** Scale 16bit greyscale luma map using nearest neighbor.
*/

static inline void
scale_luma ( uint16_t *dest_buf, int dest_width, int dest_height, const uint16_t *src_buf, int src_width, int src_height, int invert )
{
	register int i, j;
	register int x_step = ( src_width << 16 ) / dest_width;
	register int y_step = ( src_height << 16 ) / dest_height;
	register int x, y = 0;

	for ( i = 0; i < dest_height; i++ )
	{
		const uint16_t *src = src_buf + ( y >> 16 ) * src_width;
		x = 0;
		
		for ( j = 0; j < dest_width; j++ )
		{
			*dest_buf++ = src[ x >> 16 ] ^ invert;
			x += x_step;
		}
		y += y_step;
	}
}

static uint16_t* get_luma( mlt_transition this, mlt_properties properties, int width, int height )
{
	// The cached luma map information
	int luma_width = mlt_properties_get_int( properties, "_luma.width" );
	int luma_height = mlt_properties_get_int( properties, "_luma.height" );
	uint16_t *luma_bitmap = mlt_properties_get_data( properties, "_luma.bitmap", NULL );
	int invert = mlt_properties_get_int( properties, "luma_invert" );
	
	// If the filename property changed, reload the map
	char *resource = mlt_properties_get( properties, "luma" );

	char temp[ 512 ];

	if ( luma_width == 0 || luma_height == 0 )
	{
		luma_width = width;
		luma_height = height;
	}

	if ( resource && resource[0] && strchr( resource, '%' ) )
	{
		// TODO: Clean up quick and dirty compressed/existence check
		FILE *test;
		sprintf( temp, "%s/lumas/%s/%s", mlt_environment( "MLT_DATA" ), mlt_environment( "MLT_NORMALISATION" ), strchr( resource, '%' ) + 1 );
		test = fopen( temp, "r" );
		if ( test == NULL )
			strcat( temp, ".png" );
		else
			fclose( test );
		resource = temp;
	}

	if ( resource && resource[0] )
	{
		char *old_luma = mlt_properties_get( properties, "_luma" );
		int old_invert = mlt_properties_get_int( properties, "_luma_invert" );

		if ( invert != old_invert || ( old_luma && old_luma[0] && strcmp( resource, old_luma ) ) )
		{
			mlt_properties_set_data( properties, "_luma.orig_bitmap", NULL, 0, NULL, NULL );
			luma_bitmap = NULL;
		}
	}
	else {
		char *old_luma = mlt_properties_get( properties, "_luma" );
		if ( old_luma && old_luma[0] )
		{
			mlt_properties_set_data( properties, "_luma.orig_bitmap", NULL, 0, NULL, NULL );
			luma_bitmap = NULL;
			mlt_properties_set( properties, "_luma", NULL);
		}
	}

	if ( resource && resource[0] && ( luma_bitmap == NULL || luma_width != width || luma_height != height ) )
	{
		uint16_t *orig_bitmap = mlt_properties_get_data( properties, "_luma.orig_bitmap", NULL );
		luma_width = mlt_properties_get_int( properties, "_luma.orig_width" );
		luma_height = mlt_properties_get_int( properties, "_luma.orig_height" );

		// Load the original luma once
		if ( orig_bitmap == NULL )
		{
			char *extension = strrchr( resource, '.' );
			
			// See if it is a PGM
			if ( extension != NULL && strcmp( extension, ".pgm" ) == 0 )
			{
				// Open PGM
				FILE *f = fopen( resource, "r" );
				if ( f != NULL )
				{
					// Load from PGM
					luma_read_pgm( f, &orig_bitmap, &luma_width, &luma_height );
					fclose( f );
					
					// Remember the original size for subsequent scaling
					mlt_properties_set_data( properties, "_luma.orig_bitmap", orig_bitmap, luma_width * luma_height * 2, mlt_pool_release, NULL );
					mlt_properties_set_int( properties, "_luma.orig_width", luma_width );
					mlt_properties_set_int( properties, "_luma.orig_height", luma_height );
				}
			}
			else
			{
				// Get the factory producer service
				char *factory = mlt_properties_get( properties, "factory" );
	
				// Create the producer
				mlt_profile profile = mlt_service_profile( MLT_TRANSITION_SERVICE( this ) );
				mlt_producer producer = mlt_factory_producer( profile, factory, resource );
	
				// If we have one
				if ( producer != NULL )
				{
					// Get the producer properties
					mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	
					// Ensure that we loop
					mlt_properties_set( producer_properties, "eof", "loop" );
	
					// Now pass all producer. properties on the transition down
					mlt_properties_pass( producer_properties, properties, "luma." );
	
					// We will get the alpha frame from the producer
					mlt_frame luma_frame = NULL;
	
					// Get the luma frame
					if ( mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &luma_frame, 0 ) == 0 )
					{
						uint8_t *luma_image;
						mlt_image_format luma_format = mlt_image_yuv422;
	
						// Get image from the luma producer
						mlt_properties_set( MLT_FRAME_PROPERTIES( luma_frame ), "rescale.interp", "none" );
						mlt_frame_get_image( luma_frame, &luma_image, &luma_format, &luma_width, &luma_height, 0 );
	
						// Generate the luma map
						if ( luma_image != NULL && luma_format == mlt_image_yuv422 )
							luma_read_yuv422( luma_image, &orig_bitmap, luma_width, luma_height );
	
						// Remember the original size for subsequent scaling
						mlt_properties_set_data( properties, "_luma.orig_bitmap", orig_bitmap, luma_width * luma_height * 2, mlt_pool_release, NULL );
						mlt_properties_set_int( properties, "_luma.orig_width", luma_width );
						mlt_properties_set_int( properties, "_luma.orig_height", luma_height );
						
						// Cleanup the luma frame
						mlt_frame_close( luma_frame );
					}
	
					// Cleanup the luma producer
					mlt_producer_close( producer );
				}
			}
		}
		// Scale luma map
		luma_bitmap = mlt_pool_alloc( width * height * sizeof( uint16_t ) );
		scale_luma( luma_bitmap, width, height, orig_bitmap, luma_width, luma_height, invert * ( ( 1 << 16 ) - 1 ) );

		// Remember the scaled luma size to prevent unnecessary scaling
		mlt_properties_set_int( properties, "_luma.width", width );
		mlt_properties_set_int( properties, "_luma.height", height );
		mlt_properties_set_data( properties, "_luma.bitmap", luma_bitmap, width * height * 2, mlt_pool_release, NULL );
		mlt_properties_set( properties, "_luma", resource );
		mlt_properties_set_int( properties, "_luma_invert", invert );
	}
	return luma_bitmap;
}

/** Get the properly sized image from b_frame.
*/

static int get_b_frame_image( mlt_transition this, mlt_frame b_frame, uint8_t **image, int *width, int *height, struct geometry_s *geometry )
{
	int ret = 0;
	mlt_image_format format = mlt_image_yuv422;

	// Get the properties objects
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
	uint8_t resize_alpha = mlt_properties_get_int( b_props, "resize_alpha" );
	double consumer_ar = mlt_profile_sar( mlt_service_profile( MLT_TRANSITION_SERVICE(this) ) );

	// Do not scale if we are cropping - the compositing rectangle can crop the b image
	// TODO: Use the animatable w and h of the crop geometry to scale independently of crop rectangle
	if ( mlt_properties_get( properties, "crop" ) )
	{
		int real_width = get_value( b_props, "real_width", "width" );
		int real_height = get_value( b_props, "real_height", "height" );
		double input_ar = mlt_properties_get_double( b_props, "aspect_ratio" );
		double background_ar = mlt_properties_get_double( b_props, "output_ratio" );
		double output_ar = background_ar != 0.0 ? background_ar : consumer_ar;
		int scaled_width = rint( ( input_ar == 0.0 ? output_ar : input_ar ) / output_ar * real_width );
		int scaled_height = real_height;
		geometry->sw = scaled_width;
		geometry->sh = scaled_height;
	}
	// Normalise aspect ratios and scale preserving aspect ratio
	else if ( mlt_properties_get_int( properties, "aligned" ) && mlt_properties_get_int( properties, "distort" ) == 0 && mlt_properties_get_int( b_props, "distort" ) == 0 && geometry->item.distort == 0 )
	{
		// Adjust b_frame pixel aspect
		int normalised_width = geometry->item.w;
		int normalised_height = geometry->item.h;
		int real_width = get_value( b_props, "real_width", "width" );
		int real_height = get_value( b_props, "real_height", "height" );
		double input_ar = mlt_properties_get_double( b_props, "aspect_ratio" );
		double background_ar = mlt_properties_get_double( b_props, "output_ratio" );
		double output_ar = background_ar != 0.0 ? background_ar : consumer_ar;
		int scaled_width = rint( ( input_ar == 0.0 ? output_ar : input_ar ) / output_ar * real_width );
		int scaled_height = real_height;
// fprintf(stderr, "%s: scaled %dx%d norm %dx%d real %dx%d output_ar %f => %f\n", __FILE__,
// scaled_width, scaled_height, normalised_width, normalised_height, real_width, real_height,
// background_ar, output_ar);

		// Now ensure that our images fit in the normalised frame
		if ( scaled_width > normalised_width )
		{
			scaled_height = rint( scaled_height * normalised_width / scaled_width );
			scaled_width = normalised_width;
		}
		if ( scaled_height > normalised_height )
		{
			scaled_width = rint( scaled_width * normalised_height / scaled_height );
			scaled_height = normalised_height;
		}

		// Honour the fill request - this will scale the image to fill width or height while maintaining a/r
		// ????: Shouln't this be the default behaviour?
		if ( mlt_properties_get_int( properties, "fill" ) && scaled_width > 0 && scaled_height > 0 )
		{
			if ( scaled_height < normalised_height && scaled_width * normalised_height / scaled_height <= normalised_width )
			{
				scaled_width = rint( scaled_width * normalised_height / scaled_height );
				scaled_height = normalised_height;
			}
			else if ( scaled_width < normalised_width && scaled_height * normalised_width / scaled_width < normalised_height )
			{
				scaled_height = rint( scaled_height * normalised_width / scaled_width );
				scaled_width = normalised_width;
			}
		}

		// Save the new scaled dimensions
		geometry->sw = scaled_width;
		geometry->sh = scaled_height;
	}
	else
	{
		geometry->sw = geometry->item.w;
		geometry->sh = geometry->item.h;
	}

	// We want to ensure that we bypass resize now...
	if ( resize_alpha == 0 )
		mlt_properties_set_int( b_props, "distort", mlt_properties_get_int( properties, "distort" ) );

	// If we're not aligned, we want a non-transparent background
	if ( mlt_properties_get_int( properties, "aligned" ) == 0 )
		mlt_properties_set_int( b_props, "resize_alpha", 255 );

	// Take into consideration alignment for optimisation (titles are a special case)
	if ( !mlt_properties_get_int( properties, "titles" ) &&
		 mlt_properties_get( properties, "crop" ) == NULL )
		alignment_calculate( geometry );

	// Adjust to consumer scale
	*width = rint( geometry->sw * *width / geometry->nw );
	*width -= *width % 2; // coerce to even width for yuv422
	*height = rint( geometry->sh * *height / geometry->nh );
// fprintf(stderr, "%s: scaled %dx%d norm %dx%d resize %dx%d\n", __FILE__,
// geometry->sw, geometry->sh, geometry->nw, geometry->nh, *width, *height);

	ret = mlt_frame_get_image( b_frame, image, &format, width, height, 1 );

	// composite_yuv uses geometry->sw to determine source stride, which
	// should equal the image width if not using crop property.
	if ( !mlt_properties_get( properties, "crop" ) )
		geometry->sw = *width;

	// Set the frame back
	mlt_properties_set_int( b_props, "resize_alpha", resize_alpha );

	return ret && image != NULL;
}

static void crop_calculate( mlt_transition this, mlt_properties properties, struct geometry_s *result, double position )
{
	// Initialize panning info
	result->x_src = 0;
	result->y_src = 0;
	if ( mlt_properties_get( properties, "crop" ) )
	{
		mlt_geometry crop = mlt_properties_get_data( properties, "crop_geometry", NULL );
		if ( !crop )
		{
			crop = mlt_geometry_init();
			mlt_position length = mlt_transition_get_length( this );
			double cycle = mlt_properties_get_double( properties, "cycle" );

			// Allow a geometry repeat cycle
			if ( cycle >= 1 )
				length = cycle;
			else if ( cycle > 0 )
				length *= cycle;
			mlt_geometry_parse( crop, mlt_properties_get( properties, "crop" ), length, result->sw, result->sh );
			mlt_properties_set_data( properties, "crop_geometry", crop, 0, (mlt_destructor)mlt_geometry_close, NULL );
		}

		// Repeat processing
		int length = mlt_geometry_get_length( crop );
		int mirror_off = mlt_properties_get_int( properties, "mirror_off" );
		int repeat_off = mlt_properties_get_int( properties, "repeat_off" );
		if ( !repeat_off && position >= length && length != 0 )
		{
			int section = position / length;
			position -= section * length;
			if ( !mirror_off && section % 2 == 1 )
				position = length - position;
		}

		// Compute the pan
		struct mlt_geometry_item_s crop_item;
		mlt_geometry_fetch( crop, &crop_item, position );
		result->x_src = rint( crop_item.x );
		result->y_src = rint( crop_item.y );
	}
}

static mlt_geometry composite_calculate( mlt_transition this, struct geometry_s *result, mlt_frame a_frame, double position )
{
	// Get the properties from the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

	// Get the properties from the frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
	
	// Structures for geometry
	mlt_geometry start = mlt_properties_get_data( properties, "geometries", NULL );

	// Obtain the normalised width and height from the a_frame
	int normalised_width = mlt_properties_get_int( a_props, "normalised_width" );
	int normalised_height = mlt_properties_get_int( a_props, "normalised_height" );

	char *name = mlt_properties_get( properties, "_unique_id" );
	char key[ 256 ];

	sprintf( key, "%s.in", name );
	if ( mlt_properties_get( a_props, key ) )
	{
		sscanf( mlt_properties_get( a_props, key ), "%f,%f,%f,%f,%f,%d,%d", &result->item.x, &result->item.y, &result->item.w, &result->item.h, &result->item.mix, &result->nw, &result->nh );
	}
	else
	{
		// Now parse the geometries
		if ( start == NULL )
		{
			// Parse the transitions properties
			start = transition_parse_keys( this, normalised_width, normalised_height );

			// Assign to properties to ensure we get destroyed
			mlt_properties_set_data( properties, "geometries", start, 0, ( mlt_destructor )mlt_geometry_close, NULL );
		}
		else
		{
			mlt_position length = mlt_transition_get_length( this );
			double cycle = mlt_properties_get_double( properties, "cycle" );
			if ( cycle > 1 )
				length = cycle;
			else if ( cycle > 0 )
				length *= cycle;
			mlt_geometry_refresh( start, mlt_properties_get( properties, "geometry" ), length, normalised_width, normalised_height );
		}

		// Do the calculation
		geometry_calculate( this, result, position );

		// Assign normalised info
		result->nw = normalised_width;
		result->nh = normalised_height;
	}

	// Now parse the alignment
	result->halign = alignment_parse( mlt_properties_get( properties, "halign" ) );
	result->valign = alignment_parse( mlt_properties_get( properties, "valign" ) );

	crop_calculate( this, properties, result, position );

	return start;
}

mlt_frame composite_copy_region( mlt_transition this, mlt_frame a_frame, mlt_position frame_position )
{
	// Create a frame to return
	mlt_frame b_frame = mlt_frame_init( MLT_TRANSITION_SERVICE( this ) );

	// Get the properties of the a frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// Get the position
	int position = position_calculate( this, frame_position );

	// Get the unique id of the transition
	char *name = mlt_properties_get( MLT_TRANSITION_PROPERTIES( this ), "_unique_id" );
	char key[ 256 ];

	// Destination image
	uint8_t *dest = NULL;

	// Get the image and dimensions
	uint8_t *image = NULL;
	int width = mlt_properties_get_int( a_props, "width" );
	int height = mlt_properties_get_int( a_props, "height" );
	mlt_image_format format = mlt_image_yuv422;

	mlt_frame_get_image( a_frame, &image, &format, &width, &height, 0 );
	if ( !image )
		return b_frame;

	// Pointers for copy operation
	uint8_t *p;

	// Coordinates
	int w = 0;
	int h = 0;
	int x = 0;
	int y = 0;

	int ss = 0;
	int ds = 0;

	// Will need to know region to copy
	struct geometry_s result;

	// Calculate the region now
	composite_calculate( this, &result, a_frame, position );

	// Need to scale down to actual dimensions
	x = rint( result.item.x * width / result.nw );
	y = rint( result.item.y * height / result.nh );
	w = rint( result.item.w * width / result.nw );
	h = rint( result.item.h * height / result.nh );

	if ( x % 2 )
	{
		x --;
		w ++;
	}

	// Store the key
	sprintf( key, "%s.in=%d,%d,%d,%d,%f,%d,%d", name, x, y, w, h, result.item.mix, width, height );
	mlt_properties_parse( a_props, key );
	sprintf( key, "%s.out=%d,%d,%d,%d,%f,%d,%d", name, x, y, w, h, result.item.mix, width, height );
	mlt_properties_parse( a_props, key );

	ds = w * 2;
	ss = width * 2;

	// Now we need to create a new destination image
	dest = mlt_pool_alloc( w * h * 2 );

	// Assign to the new frame
	mlt_frame_set_image( b_frame, dest, w * h * 2, mlt_pool_release );
	mlt_properties_set_int( b_props, "width", w );
	mlt_properties_set_int( b_props, "height", h );
	mlt_properties_set_int( b_props, "format", format );

	if ( y < 0 )
	{
		dest += ( ds * -y );
		h += y;
		y = 0;
	}

	if ( y + h > height )
		h -= ( y + h - height );

	if ( x < 0 )
	{
		dest += -x * 2;
		w += x;
		x = 0;
	}

	if ( w > 0 && h > 0 )
	{
		// Copy the region of the image
		p = image + y * ss + x * 2;

		while ( h -- )
		{
			memcpy( dest, p, w * 2 );
			dest += ds;
			p += ss;
		}
	}

	// Assign this position to the b frame
	mlt_frame_set_position( b_frame, frame_position );
	mlt_properties_set_int( b_props, "distort", 1 );

	// Return the frame
	return b_frame;
}

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// Get the transition from the a frame
	mlt_transition this = mlt_frame_pop_service( a_frame );

	// Get in and out
	double position = mlt_deque_pop_back_double( MLT_FRAME_IMAGE_STACK( a_frame ) );
	int out = mlt_frame_pop_service_int( a_frame );
	int in = mlt_frame_pop_service_int( a_frame );

	// Get the properties from the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

	// TODO: clean up always_active behaviour
	if ( mlt_properties_get_int( properties, "always_active" ) )
	{
		mlt_events_block( properties, properties );
		mlt_properties_set_int( properties, "in", in );
		mlt_properties_set_int( properties, "out", out );
		mlt_events_unblock( properties, properties );
	}

	// This compositer is yuv422 only
	*format = mlt_image_yuv422;

	if ( b_frame != NULL )
	{
		// Get the properties of the a frame
		mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

		// Get the properties of the b frame
		mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

		// Structures for geometry
		struct geometry_s result;

		// Calculate the position
		double delta = mlt_transition_get_progress_delta( this, a_frame );

		// Get the image from the b frame
		uint8_t *image_b = NULL;
		int width_b = *width > 0 ? *width : mlt_properties_get_int( a_props, "normalised_width" );
		int height_b = *height > 0 ? *height : mlt_properties_get_int( a_props, "normalised_height" );
	
		// Vars for alphas
		uint8_t *alpha_a = NULL;
		uint8_t *alpha_b = NULL;

		// Do the calculation
		// NB: Locks needed here since the properties are being modified
		int invert = mlt_properties_get_int( properties, "invert" );
		mlt_service_lock( MLT_TRANSITION_SERVICE( this ) );
		composite_calculate( this, &result, invert ? b_frame : a_frame, position );
		mlt_service_unlock( MLT_TRANSITION_SERVICE( this ) );

		// Manual option to deinterlace
		if ( mlt_properties_get_int( properties, "deinterlace" ) )
		{
			mlt_properties_set_int( a_props, "consumer_deinterlace", 1 );
			mlt_properties_set_int( b_props, "consumer_deinterlace", 1 );
		}

		// TODO: Dangerous/temporary optimisation - if nothing to do, then do nothing
		if ( mlt_properties_get_int( properties, "no_alpha" ) && 
			 result.item.x == 0 && result.item.y == 0 && result.item.w == *width && result.item.h == *height && result.item.mix == 100 )
		{
			mlt_frame_get_image( b_frame, image, format, width, height, 1 );
			if ( !mlt_frame_is_test_card( a_frame ) )
				mlt_frame_replace_image( a_frame, *image, *format, *width, *height );
			return 0;
		}

		if ( a_frame == b_frame )
		{
			double aspect_ratio = mlt_frame_get_aspect_ratio( b_frame );
			get_b_frame_image( this, b_frame, &image_b, &width_b, &height_b, &result );
			alpha_b = mlt_frame_get_alpha_mask( b_frame );
			mlt_properties_set_double( a_props, "aspect_ratio", aspect_ratio );
		}

		// Get the image from the a frame
		mlt_frame_get_image( a_frame, invert ? &image_b : image, format, width, height, 1 );
		alpha_a = mlt_frame_get_alpha_mask( a_frame );

		// Optimisation - no compositing required
		if ( result.item.mix == 0 || ( result.item.w == 0 && result.item.h == 0 ) )
			return 0;

		// Need to keep the width/height of the a_frame on the b_frame for titling
		if ( mlt_properties_get( a_props, "dest_width" ) == NULL )
		{
			mlt_properties_set_int( a_props, "dest_width", *width );
			mlt_properties_set_int( a_props, "dest_height", *height );
			mlt_properties_set_int( b_props, "dest_width", *width );
			mlt_properties_set_int( b_props, "dest_height", *height );
		}
		else
		{
			mlt_properties_set_int( b_props, "dest_width", mlt_properties_get_int( a_props, "dest_width" ) );
			mlt_properties_set_int( b_props, "dest_height", mlt_properties_get_int( a_props, "dest_height" ) );
		}

		// Special case for titling...
		if ( mlt_properties_get_int( properties, "titles" ) )
		{
			if ( mlt_properties_get( b_props, "rescale.interp" ) == NULL )
				mlt_properties_set( b_props, "rescale.interp", "hyper" );
			width_b = mlt_properties_get_int( a_props, "dest_width" );
			height_b = mlt_properties_get_int( a_props, "dest_height" );
		}

		if ( *image != image_b && ( ( invert ? 0 : image_b ) || get_b_frame_image( this, b_frame, invert ? image : &image_b, &width_b, &height_b, &result ) == 0 ) )
		{
			uint8_t *dest = *image;
			uint8_t *src = image_b;
			int progressive = 
					mlt_properties_get_int( a_props, "consumer_deinterlace" ) ||
					mlt_properties_get_int( properties, "progressive" );
			int field;
			
			double luma_softness = mlt_properties_get_double( properties, "softness" );
			mlt_service_lock( MLT_TRANSITION_SERVICE( this ) );
			uint16_t *luma_bitmap = get_luma( this, properties, width_b, height_b );
			mlt_service_unlock( MLT_TRANSITION_SERVICE( this ) );
			char *operator = mlt_properties_get( properties, "operator" );

			alpha_b = alpha_b == NULL ? mlt_frame_get_alpha_mask( b_frame ) : alpha_b;

			composite_line_fn line_fn = composite_line_yuv;

			// Replacement and override
			if ( operator != NULL )
			{
				if ( !strcmp( operator, "or" ) )
					line_fn = composite_line_yuv_or;
				if ( !strcmp( operator, "and" ) )
					line_fn = composite_line_yuv_and;
				if ( !strcmp( operator, "xor" ) )
					line_fn = composite_line_yuv_xor;
			}

			// Allow the user to completely obliterate the alpha channels from both frames
			if ( mlt_properties_get( properties, "alpha_a" ) )
				memset( alpha_a, mlt_properties_get_int( properties, "alpha_a" ), *width * *height );

			if ( mlt_properties_get( properties, "alpha_b" ) )
				memset( alpha_b, mlt_properties_get_int( properties, "alpha_b" ), width_b * height_b );

			for ( field = 0; field < ( progressive ? 1 : 2 ); field++ )
			{
				// Assume lower field (0) first
				double field_position = position + field * delta;
				
				// Do the calculation if we need to
				// NB: Locks needed here since the properties are being modified
				mlt_service_lock( MLT_TRANSITION_SERVICE( this ) );
				composite_calculate( this, &result, invert ? b_frame : a_frame, field_position );
				mlt_service_unlock( MLT_TRANSITION_SERVICE( this ) );

				if ( mlt_properties_get_int( properties, "titles" ) )
				{
					result.item.w = rint( *width * ( result.item.w / result.nw ) );
					result.nw = result.item.w;
					result.item.h = rint( *height * ( result.item.h / result.nh ) );
					result.nh = *height;
					result.sw = width_b;
					result.sh = height_b;
				}

				// Enforce cropping
				if ( mlt_properties_get( properties, "crop" ) )
				{
					if ( result.x_src == 0 )
						width_b = width_b > result.item.w ? result.item.w : width_b;
					if ( result.y_src == 0 )
						height_b = height_b > result.item.h ? result.item.h : height_b;
				}
				else
				{
					// Otherwise, align
					alignment_calculate( &result );
				}

				// Composite the b_frame on the a_frame
				if ( invert )
					composite_yuv( dest, width_b, height_b, src, *width, *height, alpha_a, alpha_b, result, progressive ? -1 : field, luma_bitmap, luma_softness, line_fn );
				else
					composite_yuv( dest, *width, *height, src, width_b, height_b, alpha_b, alpha_a, result, progressive ? -1 : field, luma_bitmap, luma_softness, line_fn );
			}
		}
	}
	else
	{
		mlt_frame_get_image( a_frame, image, format, width, height, 1 );
	}

	return 0;
}

/** Composition transition processing.
*/

static mlt_frame composite_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	// UGH - this is a TODO - find a more reliable means of obtaining in/out for the always_active case
	if ( mlt_properties_get_int(  MLT_TRANSITION_PROPERTIES( this ), "always_active" ) == 0 )
	{
		mlt_frame_push_service_int( a_frame, mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( this ), "in" ) );
		mlt_frame_push_service_int( a_frame, mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( this ), "out" ) );
		mlt_deque_push_back_double( MLT_FRAME_IMAGE_STACK( a_frame ), position_calculate( this, mlt_frame_get_position( a_frame ) ) );
	}
	else
	{
		mlt_properties props = mlt_properties_get_data( MLT_FRAME_PROPERTIES( b_frame ), "_producer", NULL );
		mlt_frame_push_service_int( a_frame, mlt_properties_get_int( props, "in" ) );
		mlt_frame_push_service_int( a_frame, mlt_properties_get_int( props, "out" ) );
		mlt_deque_push_back_double( MLT_FRAME_IMAGE_STACK( a_frame ), mlt_properties_get_int( props, "_frame" ) - mlt_properties_get_int( props, "in" ) );
	}
	
	mlt_frame_push_service( a_frame, this );
	mlt_frame_push_frame( a_frame, b_frame );
	mlt_frame_push_get_image( a_frame, transition_get_image );
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_composite_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_transition this = calloc( sizeof( struct mlt_transition_s ), 1 );
	if ( this != NULL && mlt_transition_init( this, NULL ) == 0 )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
		
		this->process = composite_process;
		
		// Default starting motion and zoom
		mlt_properties_set( properties, "start", arg != NULL ? arg : "0,0:100%x100%" );
		
		// Default factory
		mlt_properties_set( properties, "factory", mlt_environment( "MLT_PRODUCER" ) );

		// Use alignment (and hence alpha of b frame)
		mlt_properties_set_int( properties, "aligned", 1 );

		// Default to progressive rendering
		mlt_properties_set_int( properties, "progressive", 1 );
		
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( properties, "_transition_type", 1 );
	}
	return this;
}
