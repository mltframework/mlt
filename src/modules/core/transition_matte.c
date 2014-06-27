/*
 * transition_matte.c -- replace alpha channel of track
 *
 * Copyright (C) 2003-2014 Ushodaya Enterprises Limited
 * Author: Maksym Veremeyenko <verem@m1stereo.tv>
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

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

typedef void ( *copy_luma_fn )(uint8_t* alpha_a, int stride_a, uint8_t* image_b, int stride_b, int width, int height);

static void copy_Y_to_A_full_luma(uint8_t* alpha_a, int stride_a, uint8_t* image_b, int stride_b, int width, int height)
{
	int i, j;

	for(j = 0; j < height; j++)
	{
		for(i = 0; i < width; i++)
			alpha_a[i] = image_b[2*i];
		alpha_a += stride_a;
		image_b += stride_b;
	};
};

static void copy_Y_to_A_scaled_luma(uint8_t* alpha_a, int stride_a, uint8_t* image_b, int stride_b, int width, int height)
{
	int i, j;

	for(j = 0; j < height; j++)
	{
		for(i = 0; i < width; i++)
		{
			unsigned int p = image_b[2*i];

			if(p < 16)
				p = 16;
			if(p > 235)
				p = 235;
			/* p = (p - 16) * 255 / 219; */
			p -= 16;
			p = ((p << 8) + (p * 43)) >> 8;

			alpha_a[i] = p;
		};

		alpha_a += stride_a;
		image_b += stride_b;
	};
};

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	// Get the properties of the a frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	int
		width_a = mlt_properties_get_int( a_props, "width" ),
		width_b = mlt_properties_get_int( b_props, "width" ),
		height_a = mlt_properties_get_int( a_props, "height" ),
		height_b = mlt_properties_get_int( b_props, "height" );

	uint8_t *alpha_a, *image_b;

	copy_luma_fn copy_luma = mlt_properties_get_int(b_props, "full_luma")?
		copy_Y_to_A_full_luma:copy_Y_to_A_scaled_luma;

	// This transition is yuv422 only
	*format = mlt_image_yuv422;

	// Get the image from the a frame
	mlt_frame_get_image( b_frame, &image_b, format, &width_b, &height_b, 1 );
	alpha_a = mlt_frame_get_alpha_mask( a_frame );

	// copy data
	copy_luma
	(
		alpha_a, width_a, image_b, width_b * 2,
		(width_a > width_b)?width_b:width_a,
		(height_a > height_b)?height_b:height_a
	);

	// Extract the a_frame image info
	*width = mlt_properties_get_int( a_props, "width" );
	*height = mlt_properties_get_int( a_props, "height" );
	*image = mlt_properties_get_data( a_props, "image", NULL );

	return 0;
}


/** Matte transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	// Push the b_frame on to the stack
	mlt_frame_push_frame( a_frame, b_frame );

	// Push the transition method
	mlt_frame_push_get_image( a_frame, transition_get_image );
	
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_matte_init( mlt_profile profile, mlt_service_type type, const char *id, char *lumafile )
{
	mlt_transition transition = mlt_transition_new( );
	if ( transition != NULL )
	{
		// Set the methods
		transition->process = transition_process;
		
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );

		return transition;
	}
	return NULL;
}
