/*
 * transition_luma.c -- a generic dissolve/wipe processor
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "transition_luma.h"
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** Luma class.
*/

typedef struct 
{
	struct mlt_transition_s parent;
	int width;
	int height;
	uint16_t *bitmap;
}
transition_luma;


// forward declarations
static void transition_close( mlt_transition parent );

/** Calculate the position for this frame.
*/

static float position_calculate( mlt_transition this, mlt_frame frame )
{
	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Get the position of the frame
	mlt_position position = mlt_frame_get_position( frame );

	// Now do the calcs
	return ( float )( position - in ) / ( float )( out - in + 1 );
}

/** Calculate the field delta for this frame - position between two frames.
*/

static float delta_calculate( mlt_transition this, mlt_frame frame )
{
	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Get the position of the frame
	mlt_position position = mlt_frame_get_position( frame );

	// Now do the calcs
	float x = ( float )( position - in ) / ( float )( out - in + 1 );
	float y = ( float )( position + 1 - in ) / ( float )( out - in + 1 );

	return ( y - x ) / 2.0;
}

static inline int dissolve_yuv( mlt_frame this, mlt_frame that, float weight, int width, int height )
{
	int ret = 0;
	int width_src = width, height_src = height;
	mlt_image_format format = mlt_image_yuv422;
	uint8_t *p_src, *p_dest;
	uint8_t *p;
	uint8_t *limit;

	int32_t weigh = weight * ( 1 << 16 );
	int32_t weigh_complement = ( 1 - weight ) * ( 1 << 16 );

	mlt_frame_get_image( this, &p_dest, &format, &width, &height, 1 );
	mlt_frame_get_image( that, &p_src, &format, &width_src, &height_src, 0 );

	p = p_dest;
	limit = p_dest + height_src * width_src * 2;

	while ( p < limit )
	{
		*p_dest++ = ( *p_src++ * weigh + *p++ * weigh_complement ) >> 16;
		*p_dest++ = ( *p_src++ * weigh + *p++ * weigh_complement ) >> 16;
	}

	return ret;
}

// image processing functions

static inline uint32_t smoothstep( int32_t edge1, int32_t edge2, uint32_t a )
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

	format_src = mlt_image_yuv422;
	format_dest = mlt_image_yuv422;

	mlt_frame_get_image( a_frame, &p_dest, &format_dest, &width_dest, &height_dest, 1 );
	mlt_frame_get_image( b_frame, &p_src, &format_src, &width_src, &height_src, 0 );

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

	int field_count = field_order <= 0 ? 1 : 2;
	int field_stride_src = field_count * stride_src;
	int field_stride_dest = field_count * stride_dest;

	int field = 0;

	// composite using luma map
	while ( field < field_count )
	{
		p_row = p_src + field * stride_src;
		q_row = p_dest + field * stride_dest;
		y_offset = ( field * luma_width ) << 16;
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

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties of the a frame
	mlt_properties a_props = mlt_frame_properties( a_frame );

	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );

	// Arbitrary composite defaults
	float mix = mlt_properties_get_double( b_props, "image.mix" );
	float frame_delta = mlt_properties_get_double( b_props, "luma.delta" );
	int luma_width = mlt_properties_get_int( b_props, "luma.width" );
	int luma_height = mlt_properties_get_int( b_props, "luma.height" );
	uint16_t *luma_bitmap = mlt_properties_get_data( b_props, "luma.bitmap", NULL );
	float luma_softness = mlt_properties_get_double( b_props, "luma.softness" );
	int progressive = mlt_properties_get_int( b_props, "progressive" ) ||
			mlt_properties_get_int( a_props, "consumer_progressive" ) ||
			mlt_properties_get_int( b_props, "luma.progressive" );

	int top_field_first =  mlt_properties_get_int( b_props, "top_field_first" );
	int reverse = mlt_properties_get_int( b_props, "luma.reverse" );

	// Since we are the consumer of the b_frame, we must pass along this
	// consumer property from the a_frame
	mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );
	mlt_properties_set_double( b_props, "consumer_scale", mlt_properties_get_double( a_props, "consumer_scale" ) );
		
	// Honour the reverse here
	mix = reverse ? 1 - mix : mix;
	frame_delta *= reverse ? -1.0 : 1.0;

	// Ensure we get scaling on the b_frame
	mlt_properties_set( b_props, "rescale.interp", "nearest" );

	if ( luma_width > 0 && luma_height > 0 && luma_bitmap != NULL )
		// Composite the frames using a luma map
		luma_composite( a_frame, b_frame, luma_width, luma_height, luma_bitmap, mix, frame_delta,
			luma_softness, progressive ? -1 : top_field_first, width, height );
	else
		// Dissolve the frames using the time offset for mix value
		dissolve_yuv( a_frame, b_frame, mix, *width, *height );

	// Extract the a_frame image info
	*width = mlt_properties_get_int( a_props, "width" );
	*height = mlt_properties_get_int( a_props, "height" );
	*image = mlt_properties_get_data( a_props, "image", NULL );

	return 0;
}

/** Load the luma map from PGM stream.
*/

static void luma_read_pgm( FILE *f, uint16_t **map, int *width, int *height )
{
	uint8_t *data = NULL;
	while (1)
	{
		char line[128];
		int i = 2;
		int maxval;
		int bpp;
		uint16_t *p;
		
		line[127] = '\0';

		// get the magic code
		if ( fgets( line, 127, f ) == NULL )
			break;
		if ( line[0] != 'P' || line[1] != '5' )
			break;

		// skip white space and see if a new line must be fetched
		for ( i = 2; i < 127 && line[i] != '\0' && isspace( line[i] ); i++ );
		if ( line[i] == '\0' && fgets( line, 127, f ) == NULL )
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


/** Luma transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	transition_luma *this = (transition_luma*) transition->child;

	// Get the properties of the transition
	mlt_properties properties = mlt_transition_properties( transition );
	
	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );
	
	// If the filename property changed, reload the map
	char *lumafile = mlt_properties_get( properties, "resource" );
	if ( this->bitmap == NULL && lumafile != NULL )
	{
		FILE *pipe = fopen( lumafile, "r" );
		if ( pipe != NULL )
		{
			luma_read_pgm( pipe, &this->bitmap, &this->width, &this->height );
			fclose( pipe );
		}
	}

	// Set the b frame properties
	mlt_properties_set_double( b_props, "image.mix", position_calculate( transition, b_frame ) );
	mlt_properties_set_double( b_props, "luma.delta", delta_calculate( transition, b_frame ) );
	mlt_properties_set_int( b_props, "luma.width", this->width );
	mlt_properties_set_int( b_props, "luma.height", this->height );
	mlt_properties_set_data( b_props, "luma.bitmap", this->bitmap, 0, NULL, NULL );
	mlt_properties_set_int( b_props, "luma.reverse", mlt_properties_get_int( properties, "reverse" ) );
	mlt_properties_set_double( b_props, "luma.softness", mlt_properties_get_double( properties, "softness" ) );

	mlt_frame_push_get_image( a_frame, transition_get_image );
	mlt_frame_push_frame( a_frame, b_frame );

	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_luma_init( char *lumafile )
{
	transition_luma *this = calloc( sizeof( transition_luma ), 1 );
	if ( this != NULL )
	{
		mlt_transition transition = &this->parent;
		mlt_transition_init( transition, this );
		transition->process = transition_process;
		transition->close = transition_close;
		mlt_properties_set( mlt_transition_properties( transition ), "resource", lumafile );
		return &this->parent;
	}
	return NULL;
}

/** Close the transition.
*/

static void transition_close( mlt_transition parent )
{
	transition_luma *this = (transition_luma*) parent->child;
	mlt_pool_release( this->bitmap );
	free( this );
}

