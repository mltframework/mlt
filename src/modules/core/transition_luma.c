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
	char *filename;
	int width;
	int height;
	float *bitmap;
}
transition_luma;


// forward declarations
static void transition_close( mlt_transition parent );


// image processing functions

static inline float smoothstep( float edge1, float edge2, float a )
{
	if ( a < edge1 )
		return 0.0;

	if ( a >= edge2 )
		return 1.0;

	a = ( a - edge1 ) / ( edge2 - edge1 );

	return ( a * a * ( 3 - 2 * a ) );
}

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
	position++;
	float y = ( float )( position - in ) / ( float )( out - in + 1 );

	return ( y - x ) / 2.0;
}

static int frame_composite_yuv( mlt_frame this, mlt_frame that, int x, int y, float weight, int *width, int *height )
{
	int ret = 0;
	int width_src = *width, height_src = *height;
	int width_dest = *width, height_dest = *height;
	mlt_image_format format_src = mlt_image_yuv422, format_dest = mlt_image_yuv422;
	uint8_t *p_src, *p_dest;
	int i, j;
	int stride_src;
	int stride_dest;
	int x_src = 0, y_src = 0;

	// optimization point - no work to do
	if ( ( x < 0 && -x >= width_src ) || ( y < 0 && -y >= height_src ) )
		return ret;

	format_src = mlt_image_yuv422;
	format_dest = mlt_image_yuv422;

	mlt_frame_get_image( this, &p_dest, &format_dest, &width_dest, &height_dest, 1 /* writable */ );
	mlt_frame_get_image( that, &p_src, &format_src, &width_src, &height_src, 0 /* writable */ );

	stride_src = width_src * 2;
	stride_dest = width_dest * 2;
	
	// crop overlay off the left edge of frame
	if ( x < 0 )
	{
		x_src = -x;
		width_src -= x_src;
		x = 0;
	}
	
	// crop overlay beyond right edge of frame
	else if ( x + width_src > width_dest )
		width_src = width_dest - x;

	// crop overlay off the top edge of the frame
	if ( y < 0 )
	{
		y_src = -y;
		height_src -= y_src;
	}
	// crop overlay below bottom edge of frame
	else if ( y + height_src > height_dest )
		height_src = height_dest - y;

	// offset pointer into overlay buffer based on cropping
	p_src += x_src * 2 + y_src * stride_src;

	// offset pointer into frame buffer based upon positive, even coordinates only!
	p_dest += ( x < 0 ? 0 : x ) * 2 + ( y < 0 ? 0 : y ) * stride_dest;

	// Get the alpha channel of the overlay
	uint8_t *p_alpha = mlt_frame_get_alpha_mask( that );

	// offset pointer into alpha channel based upon cropping
	if ( p_alpha )
		p_alpha += x_src + y_src * stride_src / 2;

	uint8_t *p = p_src;
	uint8_t *q = p_dest;
	uint8_t *o = p_dest;
	uint8_t *z = p_alpha;

	uint8_t Y;
	uint8_t UV;
	uint8_t a;
	float value;

	// now do the compositing only to cropped extents
	for ( i = 0; i < height_src; i++ )
	{
		p = p_src;
		q = p_dest;
		o = p_dest;
		z = p_alpha;

		for ( j = 0; j < width_src; j ++ )
		{
			Y = *p ++;
			UV = *p ++;
			a = ( z == NULL ) ? 255 : *z ++;
			value = ( weight * ( float ) a / 255.0 );
			*o ++ = (uint8_t)( Y * value + *q++ * ( 1 - value ) );
			*o ++ = (uint8_t)( UV * value + *q++ * ( 1 - value ) );
		}

		p_src += stride_src;
		p_dest += stride_dest;
		if ( p_alpha )
			p_alpha += stride_src / 2;
	}

	return ret;
}

/** powerful stuff

    \param field_order -1 = progressive, 0 = lower field first, 1 = top field first
*/
static void luma_composite( mlt_frame a_frame, mlt_frame b_frame, int luma_width, int luma_height,
							float *luma_bitmap, float pos, float frame_delta, float softness, int field_order,
							int *width, int *height )
{
	int width_src = *width, height_src = *height;
	int width_dest = *width, height_dest = *height;
	mlt_image_format format_src = mlt_image_yuv422, format_dest = mlt_image_yuv422;
	uint8_t *p_src, *p_dest;
	int i, j;
	int stride_src;
	int stride_dest;
	float weight = 0;
	int field;

	format_src = mlt_image_yuv422;
	format_dest = mlt_image_yuv422;

	mlt_frame_get_image( a_frame, &p_dest, &format_dest, &width_dest, &height_dest, 1 /* writable */ );
	mlt_frame_get_image( b_frame, &p_src, &format_src, &width_src, &height_src, 0 /* writable */ );

	stride_src = width_src * 2;
	stride_dest = width_dest * 2;

	// Offset the position based on which field we're looking at ...
	float field_pos[ 2 ];
	field_pos[ 0 ] = pos + ( ( field_order == 0 ? 1 : 0 ) * frame_delta * 0.5 );
	field_pos[ 1 ] = pos + ( ( field_order == 0 ? 0 : 1 ) * frame_delta * 0.5 );

	// adjust the position for the softness level
	field_pos[ 0 ] *= ( 1.0 + softness );
	field_pos[ 1 ] *= ( 1.0 + softness );

	uint8_t *p;
	uint8_t *q;
	uint8_t *o;
	float  *l;

	uint8_t y;
	uint8_t uv;
   	float value;

	float x_diff = ( float )luma_width / ( float )*width;
	float y_diff = ( float )luma_height / ( float )*height;

	// composite using luma map
	for ( field = 0; field < ( field_order < 0 ? 1 : 2 ); ++field )
	{
		for ( i = field; i < height_src; i += ( field_order < 0 ? 1 : 2 ) )
		{
			p = &p_src[ i * stride_src ];
			q = &p_dest[ i * stride_dest ];
			o = &p_dest[ i * stride_dest ];
			l = &luma_bitmap[ ( int )( ( float )i * y_diff ) * luma_width ];

			for ( j = 0; j < width_src; j ++ )
			{
				y = *p ++;
				uv = *p ++;
             	weight = l[ ( int )( ( float )j * x_diff ) ];
   				value = smoothstep( weight, weight + softness, field_pos[ field ] );

				*o ++ = (uint8_t)( y * value + *q++ * ( 1 - value ) );
				*o ++ = (uint8_t)( uv * value + *q++ * ( 1 - value ) );
			}
		}
	}
}

/** Get the image.
*/

static int transition_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties of the a frame
	mlt_properties a_props = mlt_frame_properties( this );

	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( this );

	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );

	// Arbitrary composite defaults
	float mix = mlt_properties_get_double( b_props, "image.mix" );
	float frame_delta = mlt_properties_get_double( b_props, "luma.delta" );
	int luma_width = mlt_properties_get_int( b_props, "luma.width" );
	int luma_height = mlt_properties_get_int( b_props, "luma.height" );
	float *luma_bitmap = mlt_properties_get_data( b_props, "luma.bitmap", NULL );
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
		luma_composite( this, b_frame, luma_width, luma_height, luma_bitmap, mix, frame_delta,
			luma_softness, progressive ? -1 : top_field_first, width, height );
	else
		// Dissolve the frames using the time offset for mix value
		frame_composite_yuv( this, b_frame, 0, 0, mix, width, height );

	// Extract the a_frame image info
	*width = mlt_properties_get_int( a_props, "width" );
	*height = mlt_properties_get_int( a_props, "height" );
	*image = mlt_properties_get_data( a_props, "image", NULL );

	return 0;
}

/** Load the luma map from PGM stream.
*/

static void luma_read_pgm( FILE *f, float **map, int *width, int *height )
{
	void *release = NULL;
	uint8_t *data = NULL;
	while (1)
	{
		char line[128];
		int i = 2;
		int maxval;
		int bpp;
		float *p;
		
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
		data = mlt_pool_allocate( *width * *height * bpp, &release );
		if ( data == NULL )
			break;

		// read the raw data
		if ( fread( data, *width * *height * bpp, 1, f ) != 1 )
			break;
		
		// allocate the luma bitmap
		// IRRIGATE ME
		// Difficult here - need to change the function prototype....
		*map =  p = (float*) malloc( *width * *height * sizeof( float ) );
		if ( *map == NULL )
			break;

		// proces the raw data into the luma bitmap
		for ( i = 0; i < *width * *height * bpp; i += bpp )
		{
			if ( bpp == 1 )
				*p++ = (float) data[ i ] / (float) maxval;
			else
				*p++ = (float) ( ( data[ i ] << 8 ) + data[ i+1 ] ) / (float) maxval;
		}

		break;
	}
		
	if ( release != NULL )
		mlt_pool_release( release );
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
	char *luma_file = mlt_properties_get( properties, "resource" );
	if ( luma_file != NULL && ( this->filename == NULL || ( this->filename && strcmp( luma_file, this->filename ) ) ) )
	{
		FILE *pipe;
		
		free( this->filename );
		this->filename = strdup( luma_file );
		pipe = fopen( luma_file, "r" );
		if ( pipe != NULL )
		{
			free( this->bitmap );
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
	free( this->bitmap );
	free( this->filename );
	free( this );
}

