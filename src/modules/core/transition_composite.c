/*
 * transition_composite.c -- compose one image over another using alpha channel
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

#include "transition_composite.h"
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

/** Geometry struct.
*/

struct geometry_s
{
	float position;
	float mix;
	int nw; // normalised width
	int nh; // normalised height
	int sw; // scaled width, not including consumer scale based upon w/nw
	int sh; // scaled height, not including consumer scale based upon h/nh
	float x;
	float y;
	float w;
	float h;
	int halign; // horizontal alignment: 0=left, 1=center, 2=right
	int valign; // vertical alignment: 0=top, 1=middle, 2=bottom
	int distort;
	struct geometry_s *next;
};

/** Parse a value from a geometry string.
*/

static float parse_value( char **ptr, int normalisation, char delim, float defaults )
{
	float value = defaults;

	if ( *ptr != NULL && **ptr != '\0' )
	{
		char *end = NULL;
		value = strtod( *ptr, &end );
		if ( end != NULL )
		{
			if ( *end == '%' )
				value = ( value / 100.0 ) * normalisation;
			while ( *end == delim || *end == '%' )
				end ++;
		}
		*ptr = end;
	}

	return value;
}

/** Parse a geometry property string with the syntax X,Y:WxH:MIX. Any value can be 
	expressed as a percentage by appending a % after the value, otherwise values are
	assumed to be relative to the normalised dimensions of the consumer.
*/

static void geometry_parse( struct geometry_s *geometry, struct geometry_s *defaults, char *property, int nw, int nh )
{
	// Assign normalised width and height
	geometry->nw = nw;
	geometry->nh = nh;

	// Assign from defaults if available
	if ( defaults != NULL )
	{
		geometry->x = defaults->x;
		geometry->y = defaults->y;
		geometry->w = geometry->sw = defaults->w;
		geometry->h = geometry->sh = defaults->h;
		geometry->distort = defaults->distort;
		geometry->mix = defaults->mix;
		defaults->next = geometry;
	}
	else
	{
		geometry->mix = 100;
	}

	// Parse the geomtry string
	if ( property != NULL && strcmp( property, "" ) )
	{
		char *ptr = property;
		geometry->x = parse_value( &ptr, nw, ',', geometry->x );
		geometry->y = parse_value( &ptr, nh, ':', geometry->y );
		geometry->w = geometry->sw = parse_value( &ptr, nw, 'x', geometry->w );
		geometry->h = geometry->sh = parse_value( &ptr, nh, ':', geometry->h );
		if ( *ptr == '!' )
		{
			geometry->distort = 1;
			ptr ++;
			if ( *ptr == ':' )
				ptr ++;
		}
		geometry->mix = parse_value( &ptr, 100, ' ', geometry->mix );
	}
}

/** Calculate real geometry.
*/

static void geometry_calculate( struct geometry_s *output, struct geometry_s *in, float position )
{
	// Search in for position
	struct geometry_s *out = in->next;

	if ( position >= 1.0 )
	{
		int section = floor( position );
		position -= section;
		if ( section % 2 == 1 )
			position = 1.0 - position;
	}

	while ( out->next != NULL )
	{
		if ( position >= in->position && position < out->position )
			break;

		in = out;
		out = in->next;
	}

	position = ( position - in->position ) / ( out->position - in->position );

	// Calculate this frames geometry
	output->nw = in->nw;
	output->nh = in->nh;
	output->x = in->x + ( out->x - in->x ) * position + 0.5;
	output->y = in->y + ( out->y - in->y ) * position + 0.5;
	output->w = in->w + ( out->w - in->w ) * position;
	output->h = in->h + ( out->h - in->h ) * position;
	output->mix = in->mix + ( out->mix - in->mix ) * position;
	output->sw = output->w;
	output->sh = output->h;
	output->distort = in->distort;
}

void transition_destroy_keys( void *arg )
{
	struct geometry_s *ptr = arg;
	struct geometry_s *next = NULL;

	while ( ptr != NULL )
	{
		next = ptr->next;
		free( ptr );
		ptr = next;
	}
}

static struct geometry_s *transition_parse_keys( mlt_transition this,  int normalised_width, int normalised_height )
{
	// Loop variable for property interrogation
	int i = 0;

	// Get the properties of the transition
	mlt_properties properties = mlt_transition_properties( this );

	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Create the start
	struct geometry_s *start = calloc( 1, sizeof( struct geometry_s ) );

	// Create the end (we always need two entries)
	struct geometry_s *end = calloc( 1, sizeof( struct geometry_s ) );

	// Pointer
	struct geometry_s *ptr = start;

	// Parse the start property
	geometry_parse( start, NULL, mlt_properties_get( properties, "start" ), normalised_width, normalised_height );

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
			int frame = atoi( name + 4 );

			// Determine the position
			float position = 0;
			
			if ( frame >= 0 && frame < ( out - in ) )
				position = ( float )frame / ( float )( out - in + 1 );
			else if ( frame < 0 && - frame < ( out - in ) )
				position = ( float )( out - in + frame ) / ( float )( out - in + 1 );

			// For now, we'll exclude all keys received out of order
			if ( position > ptr->position )
			{
				// Create a new geometry
				struct geometry_s *temp = calloc( 1, sizeof( struct geometry_s ) );

				// Parse and add to the list
				geometry_parse( temp, ptr, value, normalised_width, normalised_height );

				// Assign the position
				temp->position = position;

				// Allow the next to be appended after this one
				ptr = temp;
			}
			else
			{
				fprintf( stderr, "Key out of order - skipping %s\n", name );
			}
		}
	}
	
	// Parse the end
	geometry_parse( end, ptr, mlt_properties_get( properties, "end" ), normalised_width, normalised_height );
	if ( out > 0 )
		end->position = ( float )( out - in ) / ( float )( out - in + 1 );
	else
		end->position = 1;

	// Assign to properties to ensure we get destroyed
	mlt_properties_set_data( properties, "geometries", start, 0, transition_destroy_keys, NULL );

	return start;
}

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

/** Adjust position according to scaled size and alignment properties.
*/

static void alignment_calculate( struct geometry_s *geometry )
{
	geometry->x += ( geometry->w - geometry->sw ) * geometry->halign / 2 + 0.5;
	geometry->y += ( geometry->h - geometry->sh ) * geometry->valign / 2 + 0.5;
}

/** Calculate the position for this frame.
*/

static inline float position_calculate( mlt_transition this, mlt_frame frame )
{
	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Get the position
	mlt_position position = mlt_frame_get_position( frame );

	// Now do the calcs
	return ( float )( position - in ) / ( float )( out - in + 1 );
}

/** Calculate the field delta for this frame - position between two frames.
*/

static inline float delta_calculate( mlt_transition this, mlt_frame frame )
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

static int get_value( mlt_properties properties, char *preferred, char *fallback )
{
	int value = mlt_properties_get_int( properties, preferred );
	if ( value == 0 )
		value = mlt_properties_get_int( properties, fallback );
	return value;
}

/** Composite function.
*/

static int composite_yuv( uint8_t *p_dest, int width_dest, int height_dest, int bpp, uint8_t *p_src, int width_src, int height_src, uint8_t *p_alpha, struct geometry_s geometry, int field )
{
	int ret = 0;
	int i, j;
	int x_src = 0, y_src = 0;
	int32_t weight = ( 1 << 16 ) * ( geometry.mix / 100 );
	int stride_src = width_src * bpp;
	int stride_dest = width_dest * bpp;

	// Adjust to consumer scale
	int x = geometry.x * width_dest / geometry.nw + 0.5;
	int y = geometry.y * height_dest / geometry.nh + 0.5;

	if ( bpp == 2 )
		x -= x % 2;
		
	// optimization points - no work to do
	if ( width_src <= 0 || height_src <= 0 )
		return ret;

	if ( ( x < 0 && -x >= width_src ) || ( y < 0 && -y >= height_src ) )
		return ret;

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
	p_src += x_src * bpp + y_src * stride_src;

	// offset pointer into frame buffer based upon positive coordinates only!
	p_dest += ( x < 0 ? 0 : x ) * bpp + ( y < 0 ? 0 : y ) * stride_dest;

	// offset pointer into alpha channel based upon cropping
	if ( p_alpha )
		p_alpha += x_src + y_src * stride_src / bpp;

	// Assuming lower field first
	// Special care is taken to make sure the b_frame is aligned to the correct field.
	// field 0 = lower field and y should be odd (y is 0-based).
	// field 1 = upper field and y should be even.
	if ( ( field > -1 ) && ( y % 2 == field ) )
	{
		//fprintf( stderr, "field %d y %d\n", field, y );
		if ( ( field == 1 && y < height_dest - 1 ) || ( field == 0 && y == 0 ) )
			p_dest += stride_dest;
		else
			p_dest -= stride_dest;
	}

	// On the second field, use the other lines from b_frame
	if ( field == 1 )
	{
		p_src += stride_src;
		if ( p_alpha )
			p_alpha += stride_src / bpp;
		height_src--;
	}

	uint8_t *p = p_src;
	uint8_t *q = p_dest;
	uint8_t *o = p_dest;
	uint8_t *z = p_alpha;

	uint8_t a;
	int32_t value;
	int step = ( field > -1 ) ? 2 : 1;

	stride_src = stride_src * step;
	int alpha_stride = stride_src / bpp;
	stride_dest = stride_dest * step;

	// now do the compositing only to cropped extents
	for ( i = 0; i < height_src; i += step )
	{
		p = p_src;
		q = p_dest;
		o = q;
		z = p_alpha;

		for ( j = 0; j < width_src; j ++ )
		{
			a = ( z == NULL ) ? 255 : *z ++;
			value = ( weight * ( a + 1 ) ) >> 8;
			*o ++ = ( *p++ * value + *q++ * ( ( 1 << 16 ) - value ) ) >> 16;
			*o ++ = ( *p++ * value + *q++ * ( ( 1 << 16 ) - value ) ) >> 16;
		}

		p_src += stride_src;
		p_dest += stride_dest;
		if ( p_alpha )
			p_alpha += alpha_stride;
	}

	return ret;
}


/** Get the properly sized image from b_frame.
*/

static int get_b_frame_image( mlt_transition this, mlt_frame b_frame, uint8_t **image, int *width, int *height, struct geometry_s *geometry )
{
	int ret = 0;
	mlt_image_format format = mlt_image_yuv422;

	// Get the properties objects
	mlt_properties b_props = mlt_frame_properties( b_frame );
	mlt_properties properties = mlt_transition_properties( this );

	if ( mlt_properties_get( properties, "distort" ) == NULL && geometry->distort == 0 )
	{
		// Adjust b_frame pixel aspect
		int normalised_width = geometry->w;
		int normalised_height = geometry->h;
		int real_width = get_value( b_props, "real_width", "width" );
		int real_height = get_value( b_props, "real_height", "height" );
		double input_ar = mlt_frame_get_aspect_ratio( b_frame );
		double output_ar = mlt_properties_get_double( b_props, "consumer_aspect_ratio" );
		int scaled_width = real_width;
		int scaled_height = real_height;
		double output_sar = ( double ) geometry->nw / geometry->nh / output_ar;

		// If the output is fat pixels (NTSC) then stretch our input horizontally
		// derived from: output_sar / input_sar * real_width
		scaled_width = output_sar * real_height * input_ar;
			
		// Now ensure that our images fit in the normalised frame
		if ( scaled_width > normalised_width )
		{
			scaled_height = scaled_height * normalised_width / scaled_width;
			scaled_width = normalised_width;
		}
		if ( scaled_height > normalised_height )
		{
			scaled_width = scaled_width * normalised_height / scaled_height;
			scaled_height = normalised_height;
		}

		// Now apply the fill
		// TODO: Should combine fill/distort in one property
		if ( mlt_properties_get( properties, "fill" ) != NULL )
		{
			scaled_width = ( geometry->w / scaled_width ) * scaled_width;
			scaled_height = ( geometry->h / scaled_height ) * scaled_height;
		}

		// Save the new scaled dimensions
		geometry->sw = scaled_width;
		geometry->sh = scaled_height;
	}

	// We want to ensure that we bypass resize now...
	mlt_properties_set( b_props, "distort", "true" );

	// Take into consideration alignment for optimisation
	alignment_calculate( geometry );

	// Adjust to consumer scale
	int x = geometry->x * *width / geometry->nw + 0.5;
	int y = geometry->y * *height / geometry->nh + 0.5;
	*width = geometry->sw * *width / geometry->nw;
	*height = geometry->sh * *height / geometry->nh;

	x -= x % 2;

	// optimization points - no work to do
	if ( *width <= 0 || *height <= 0 )
		return 1;

	if ( ( x < 0 && -x >= *width ) || ( y < 0 && -y >= *height ) )
		return 1;

	ret = mlt_frame_get_image( b_frame, image, &format, width, height, 1 );

	return ret;
}


static uint8_t *transition_get_alpha_mask( mlt_frame this )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// Return the alpha mask
	return mlt_properties_get_data( properties, "alpha", NULL );
}

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// This compositer is yuv422 only
	*format = mlt_image_yuv422;

	// Get the transition from the a frame
	mlt_transition this = mlt_frame_pop_service( a_frame );

	// Get the image from the a frame
	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	if ( b_frame != NULL )
	{
		// Get the properties of the a frame
		mlt_properties a_props = mlt_frame_properties( a_frame );

		// Get the properties of the b frame
		mlt_properties b_props = mlt_frame_properties( b_frame );

		// Get the properties from the transition
		mlt_properties properties = mlt_transition_properties( this );

		// Structures for geometry
		struct geometry_s result;
		struct geometry_s *start = mlt_properties_get_data( properties, "geometries", NULL );

		// Calculate the position
		float position = mlt_properties_get_double( b_props, "relative_position" );
		float delta = delta_calculate( this, a_frame );

		// Now parse the geometries
		if ( start == NULL )
		{
			// Obtain the normalised width and height from the a_frame
			int normalised_width = mlt_properties_get_int( a_props, "normalised_width" );
			int normalised_height = mlt_properties_get_int( a_props, "normalised_height" );

			// Parse the transitions properties
			start = transition_parse_keys( this, normalised_width, normalised_height );
		}

		// Since we are the consumer of the b_frame, we must pass along these
		// consumer properties from the a_frame
		mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );
		mlt_properties_set_double( b_props, "consumer_scale", mlt_properties_get_double( a_props, "consumer_scale" ) );

		// Do the calculation
		geometry_calculate( &result, start, position );

		// Now parse the alignment
		result.halign = alignment_parse( mlt_properties_get( properties, "halign" ) );
		result.valign = alignment_parse( mlt_properties_get( properties, "valign" ) );

		// Get the image from the b frame
		uint8_t *image_b = NULL;
		int width_b = *width;
		int height_b = *height;
		
		if ( get_b_frame_image( this, b_frame, &image_b, &width_b, &height_b, &result ) == 0 )
		{
			uint8_t *dest = *image;
			uint8_t *src = image_b;
			int bpp = 2;
			uint8_t *alpha = mlt_frame_get_alpha_mask( b_frame );
			int progressive = mlt_properties_get_int( a_props, "progressive" ) ||
					mlt_properties_get_int( a_props, "consumer_progressive" ) ||
					mlt_properties_get_int( properties, "progressive" );
			int field;

			for ( field = 0; field < ( progressive ? 1 : 2 ); field++ )
			{
				// Assume lower field (0) first
				float field_position = position + field * delta;
				
				// Do the calculation if we need to
				geometry_calculate( &result, start, field_position );

				// Align
				alignment_calculate( &result );

				// Composite the b_frame on the a_frame
				composite_yuv( dest, *width, *height, bpp, src, width_b, height_b, alpha, result, progressive ? -1 : field );
			}
		}
	}

	return 0;
}

/** Composition transition processing.
*/

static mlt_frame composite_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	// Propogate the transition properties to the b frame
	mlt_properties_set_double( mlt_frame_properties( b_frame ), "relative_position", position_calculate( this, a_frame ) );
	mlt_frame_push_service( a_frame, this );
	mlt_frame_push_get_image( a_frame, transition_get_image );
	mlt_frame_push_frame( a_frame, b_frame );
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_composite_init( char *arg )
{
	mlt_transition this = calloc( sizeof( struct mlt_transition_s ), 1 );
	if ( this != NULL && mlt_transition_init( this, NULL ) == 0 )
	{
		this->process = composite_process;
		mlt_properties_set( mlt_transition_properties( this ), "start", arg != NULL ? arg : "85%,5%:10%x10%" );
	}
	return this;
}

