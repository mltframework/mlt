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
	float x;
	float y;
	float w;
	float h;
	float mix;
};

/** Parse a geometry property string.
*/

static void geometry_parse( struct geometry_s *geometry, struct geometry_s *defaults, char *property )
{
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
		sscanf( property, "%f,%f:%fx%f:%f", &geometry->x, &geometry->y, &geometry->w, &geometry->h, &geometry->mix );
}

/** Calculate real geometry.
*/

static void geometry_calculate( struct geometry_s *output, struct geometry_s *in, struct geometry_s *out, float position )
{
	// Calculate this frames geometry
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
	int x = ( int )( ( float )width_dest * geometry.x / 100 );
	int y = ( int )( ( float )height_dest * geometry.y / 100 );
	float weight = geometry.mix / 100;

	// Compute the dimensioning rectangle
	int width_src = ( int )( ( float )width_dest * geometry.w / 100 );
	int height_src = ( int )( ( float )height_dest * geometry.h / 100 );

	mlt_properties b_props = mlt_frame_properties( that );
	mlt_transition this = mlt_properties_get_data( b_props, "transition_composite", NULL );
	mlt_properties properties = mlt_transition_properties( this );

	if ( mlt_properties_get( properties, "distort" ) == NULL &&
		 mlt_properties_get( mlt_frame_properties( that ), "real_width" ) != NULL )
	{
		int width_b = mlt_properties_get_double( b_props, "real_width" );
		int height_b = mlt_properties_get_double( b_props, "real_height" );

		// Maximise the dimensioning rectangle to the aspect of the b_frame
		if ( mlt_properties_get_double( b_props, "aspect_ratio" ) * height_src > width_src )
			height_src = ( double )width_src / mlt_properties_get_double( b_props, "aspect_ratio" ) + 0.5;
		else
			width_src = mlt_properties_get_double( b_props, "aspect_ratio" ) * height_src + 0.5;

		// See if we need to normalise pixel aspect ratio
		// We can use consumer_aspect_ratio because the a_frame will take on this aspect
		double aspect = mlt_properties_get_double( b_props, "consumer_aspect_ratio" );
		if ( aspect != 0 )
		{
			// Derive the consumer pixel aspect
			double oaspect = aspect / ( double )width_dest * height_dest;

			// Get the b frame pixel aspect - usually 1
			double iaspect = mlt_properties_get_double( b_props, "aspect_ratio" ) / width_b * height_b;

			// Normalise pixel aspect
			if ( iaspect != 0 && iaspect != oaspect )
			{
				width_b = iaspect / oaspect * ( double )width_b + 0.5;
				width_src = iaspect / oaspect * ( double )width_src + 0.5;
			}
				
			// Tell rescale not to normalise display aspect
			mlt_frame_set_aspect_ratio( that, aspect );
		}
		
		// Adjust overall scale for consumer
		double consumer_scale = mlt_properties_get_double( b_props, "consumer_scale" );
		if ( consumer_scale > 0 )
		{
			width_b = consumer_scale * width_b + 0.5;
			height_b = consumer_scale * height_b + 0.5;
		}

//		fprintf( stderr, "bounding rect %dx%d for overlay %dx%d\n",	width_src, height_src, width_b, height_b );
		// Constrain the overlay to the dimensioning rectangle
		if ( width_b < width_src && height_b < height_src )
		{
			width_src = width_b;
			height_src = height_b;
		}
	}
	else if ( mlt_properties_get( b_props, "real_width" ) != NULL )
	{
		// Tell rescale not to normalise display aspect
		mlt_properties_set_double( b_props, "consumer_aspect_ratio", 0 );
	}

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

		// Now parse the geometries
		geometry_parse( &start, NULL, mlt_properties_get( properties, "start" ) );
		geometry_parse( &end, &start, mlt_properties_get( properties, "end" ) );

		// Do the calculation
		geometry_calculate( &result, &start, &end, position );

		// Since we are the consumer of the b_frame, we must pass along these
		// consumer properties from the a_frame
		mlt_properties_set_double( b_props, "consumer_aspect_ratio",
			mlt_properties_get_double( mlt_frame_properties( a_frame ), "consumer_aspect_ratio" ) );
		mlt_properties_set_double( b_props, "consumer_scale",
			mlt_properties_get_double( mlt_frame_properties( a_frame ), "consumer_scale" ) );
		
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
		mlt_properties_set( mlt_transition_properties( this ), "start", arg != NULL ? arg : "85,5:10x10" );
		mlt_properties_set( mlt_transition_properties( this ), "end", "" );
	}
	return this;
}

