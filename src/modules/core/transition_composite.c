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

/** Geometry struct.
*/

struct geometry_s
{
	int nw;
	int nh;
	float x;
	float y;
	float w;
	float h;
	float mix;
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
		geometry->w = defaults->w;
		geometry->h = defaults->h;
		geometry->mix = defaults->mix;
	}
	else
	{
		geometry->mix = 100;
	}

	// Parse the geomtry string
	if ( property != NULL )
	{
		char *ptr = property;
		geometry->x = parse_value( &ptr, nw, ',', geometry->x );
		geometry->y = parse_value( &ptr, nh, ':', geometry->y );
		geometry->w = parse_value( &ptr, nw, 'x', geometry->w );
		geometry->h = parse_value( &ptr, nh, ':', geometry->h );
		geometry->mix = parse_value( &ptr, 100, ' ', geometry->mix );
	}
}

/** Calculate real geometry.
*/

static void geometry_calculate( struct geometry_s *output, struct geometry_s *in, struct geometry_s *out, float position )
{
	// Calculate this frames geometry
	output->nw = in->nw;
	output->nh = in->nh;
	output->x = in->x + ( out->x - in->x ) * position;
	output->y = in->y + ( out->y - in->y ) * position;
	output->w = in->w + ( out->w - in->w ) * position;
	output->h = in->h + ( out->h - in->h ) * position;
	output->mix = in->mix + ( out->mix - in->mix ) * position;
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

static int get_value( mlt_properties properties, char *preferred, char *fallback )
{
	int value = mlt_properties_get_int( properties, preferred );
	if ( value == 0 )
		value = mlt_properties_get_int( properties, fallback );
	return value;
}

/** Composite function.
*/

static int composite_yuv( uint8_t *p_dest, mlt_image_format format_dest, int width_dest, int height_dest, mlt_frame that, struct geometry_s geometry )
{
	int ret = 0;
	uint8_t *p_src;
	int i, j;
	int stride_src;
	int stride_dest;
	int x_src = 0, y_src = 0;

	mlt_image_format format_src = format_dest;
	float weight = geometry.mix / 100;

	// Compute the dimensioning rectangle
	mlt_properties b_props = mlt_frame_properties( that );
	mlt_transition this = mlt_properties_get_data( b_props, "transition_composite", NULL );
	mlt_properties properties = mlt_transition_properties( this );

	if ( mlt_properties_get( properties, "distort" ) == NULL )
	{
		// Now do additional calcs based on real_width/height etc
		//int normalised_width = mlt_properties_get_int( b_props, "normalised_width" );
		//int normalised_height = mlt_properties_get_int( b_props, "normalised_height" );
		int normalised_width = geometry.w;
		int normalised_height = geometry.h;
		int real_width = get_value( b_props, "real_width", "width" );
		int real_height = get_value( b_props, "real_height", "height" );
		double input_ar = mlt_frame_get_aspect_ratio( that );
		double output_ar = mlt_properties_get_double( b_props, "consumer_aspect_ratio" );
		int scaled_width = ( input_ar > output_ar ? input_ar / output_ar : output_ar / input_ar ) * real_width;
		int scaled_height = ( input_ar > output_ar ? input_ar / output_ar : output_ar / input_ar ) * real_height;

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

		// Special case 
		if ( scaled_height == normalised_height )
			scaled_width = normalised_width;

		// Now we need to align to the geometry
		if ( scaled_width <= geometry.w && scaled_height <= geometry.h )
		{
			// TODO: Should take into account requested alignment here...
			// Assume centred alignment for now
			
			geometry.x = geometry.x + ( geometry.w - scaled_width ) / 2;
			geometry.y = geometry.y + ( geometry.h - scaled_height ) / 2;
			geometry.w = scaled_width;
			geometry.h = scaled_height;
			mlt_properties_set( b_props, "distort", "true" );
		}
		else
		{
			mlt_properties_set( b_props, "distort", "true" );
		}
	}
	else
	{
		// We want to ensure that we bypass resize now...
		mlt_properties_set( b_props, "distort", "true" );
	}

	int x = ( geometry.x * width_dest ) / geometry.nw;
	int y = ( geometry.y * height_dest ) / geometry.nh;
	int width_src = ( geometry.w * width_dest ) / geometry.nw;
	int height_src = ( geometry.h * height_dest ) / geometry.nh;

	x -= x % 2;

	// optimization points - no work to do
	if ( width_src <= 0 || height_src <= 0 )
		return ret;

	if ( ( x < 0 && -x >= width_src ) || ( y < 0 && -y >= height_src ) )
		return ret;

	format_src = mlt_image_yuv422;
	format_dest = mlt_image_yuv422;

	mlt_frame_get_image( that, &p_src, &format_src, &width_src, &height_src, 1 /* writable */ );

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


/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// Get the image from the a frame
	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	if ( b_frame != NULL )
	{
		// Get the properties of the a frame
		mlt_properties a_props = mlt_frame_properties( a_frame );

		// Get the properties of the b frame
		mlt_properties b_props = mlt_frame_properties( b_frame );

		// Get the transition from the b frame
		mlt_transition this = mlt_properties_get_data( b_props, "transition_composite", NULL );

		// Get the properties from the transition
		mlt_properties properties = mlt_transition_properties( this );

		// Structures for geometry
		struct geometry_s result;
		struct geometry_s start;
		struct geometry_s end;

		// Calculate the position
		float position = position_calculate( this, a_frame );

		// Obtain the normalised width and height from the a_frame
		int normalised_width = mlt_properties_get_int( a_props, "normalised_width" );
		int normalised_height = mlt_properties_get_int( a_props, "normalised_height" );

		// Now parse the geometries
		geometry_parse( &start, NULL, mlt_properties_get( properties, "start" ), normalised_width, normalised_height );
		geometry_parse( &end, &start, mlt_properties_get( properties, "end" ), normalised_width, normalised_height );

		// Do the calculation
		geometry_calculate( &result, &start, &end, position );

		// Since we are the consumer of the b_frame, we must pass along these
		// consumer properties from the a_frame
		mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );
		mlt_properties_set_double( b_props, "consumer_scale", mlt_properties_get_double( a_props, "consumer_scale" ) );
		
		// Composite the b_frame on the a_frame
		composite_yuv( *image, *format, *width, *height, b_frame, result );
	}

	return 0;
}

/** Composition transition processing.
*/

static mlt_frame composite_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	// Propogate the transition properties to the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );
	mlt_properties_set_data( b_props, "transition_composite", this, 0, NULL, NULL );
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
		mlt_properties_set( mlt_transition_properties( this ), "end", "" );
	}
	return this;
}

