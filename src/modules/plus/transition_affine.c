/*
 * transition_affine.c -- affine transformations
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "transition_affine.h"
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

typedef struct 
{
	float matrix[3][3];
}
affine_t;

static void affine_init( float this[3][3] )
{
	this[0][0] = 1;
	this[0][1] = 0;
	this[0][2] = 0;
	this[1][0] = 0;
	this[1][1] = 1;
	this[1][2] = 0;
	this[2][0] = 0;
	this[2][1] = 0;
	this[2][2] = 1;
}

// Multiply two this affine transform with that
static void affine_multiply( float this[3][3], float that[3][3] )
{
	float output[3][3];
	int i;
	int j;

	for ( i = 0; i < 3; i ++ )
		for ( j = 0; j < 3; j ++ )
			output[i][j] = this[i][0] * that[j][0] + this[i][1] * that[j][1] + this[i][2] * that[j][2];

	this[0][0] = output[0][0];
	this[0][1] = output[0][1];
	this[0][2] = output[0][2];
	this[1][0] = output[1][0];
	this[1][1] = output[1][1];
	this[1][2] = output[1][2];
	this[2][0] = output[2][0];
	this[2][1] = output[2][1];
	this[2][2] = output[2][2];
}

// Rotate by a given angle
static void affine_rotate( float this[3][3], float angle )
{
	float affine[3][3];
	affine[0][0] = cos( angle * M_PI / 180 );
	affine[0][1] = 0 - sin( angle * M_PI / 180 );
	affine[0][2] = 0;
	affine[1][0] = sin( angle * M_PI / 180 );
	affine[1][1] = cos( angle * M_PI / 180 );
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

static void affine_scale( float this[3][3], float sx, float sy )
{
	float affine[3][3];
	affine[0][0] = sx;
	affine[0][1] = 0;
	affine[0][2] = 0;
	affine[1][0] = 0;
	affine[1][1] = sy;
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

// Shear by a given value
static void affine_shear( float this[3][3], float shear )
{
	float affine[3][3];
	affine[0][0] = 1;
	affine[0][1] = shear;
	affine[0][2] = 0;
	affine[1][0] = 0;
	affine[1][1] = 1;
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

// Shear by a given value
static void affine_invert( float this[3][3] )
{
	float affine[3][3];
	affine[0][0] = 1;
	affine[0][1] = -1;
	affine[0][2] = 0;
	affine[1][0] = -1;
	affine[1][1] = 1;
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

static void affine_offset( float this[3][3], int x, int y )
{
	this[0][2] += x;
	this[1][2] += y;
}

// Obtain the mapped x coordinate of the input
static inline float MapX( float this[3][3], int x, int y )
{
	return this[0][0] * x + this[0][1] * y + this[0][2] + 0.5;
}

// Obtain the mapped y coordinate of the input
static inline float MapY( float this[3][3], int x, int y )
{
	return this[1][0] * x + this[1][1] * y + this[1][2] + 0.5;
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
	mlt_properties properties = mlt_transition_properties( transition );

	// Get the properties of the a frame
	//mlt_properties a_props = mlt_frame_properties( a_frame );

	// Get the properties of the b frame
	//mlt_properties b_props = mlt_frame_properties( b_frame );

	// Image, format, width, height and image for the b frame
	uint8_t *b_image = NULL;
	mlt_image_format b_format = mlt_image_yuv422;
	int b_width;
	int b_height;

	// Fetch the a frame image
	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	// Fetch the b frame image
	b_width = *width;
	b_height = *height;
	mlt_properties_set( mlt_frame_properties( b_frame ), "rescale.interp", "nearest" );
	mlt_properties_set( mlt_frame_properties( b_frame ), "distort", "true" );
	mlt_frame_get_image( b_frame, &b_image, &b_format, &b_width, &b_height, 0 );

	// Check that both images are of the correct format and process
	if ( *format == mlt_image_yuv422 && b_format == mlt_image_yuv422 )
	{
		int x, y;
		int dx, dy;

		// This is the matrix we're creating
		affine_t *affine = mlt_properties_get_data( properties, "affine", NULL );

		// Get values from the transition
		char *geometry = mlt_properties_get( properties, "geometry" );
		float rotate = mlt_properties_get_double( properties, "rotate" );
		float shear = mlt_properties_get_double( properties, "shear" );
		int invert = mlt_properties_get_int( properties, "invert" );
		float sx = mlt_properties_get_double( properties, "sx" );
		float sy = mlt_properties_get_double( properties, "sy" );
		float ox = mlt_properties_get_double( properties, "ox" );
		float oy = mlt_properties_get_double( properties, "oy" );

		// Geometry
		float gx = 0;
		float gy = 0;
		float gw = *width;
		float gh = *height;

		uint8_t *p = *image;
		//uint8_t *luma = mlt_properties_get_data( b_props, "luma", NULL );

		// Constructuct the matrix
		if ( rotate != 0 )
			affine_rotate( affine->matrix, rotate );
		if ( shear != 0 )
			affine_shear( affine->matrix, shear );

		affine_scale( affine->matrix, sx, sy );
		affine_offset( affine->matrix, ox, oy );
		if ( invert )
			affine_invert( affine->matrix );

		if ( geometry != NULL )
		{
			sscanf( geometry, "%f,%f:%fx%f", &gx, &gy, &gw, &gh );
			gx = gx / 100 * *width;
			gy = gy / 100 * *height;
			gw = gw / 100 * *width;
			gh = gh / 100 * *height;
		}

		for ( y = - *height / 2; y < *height / 2; y ++ )
		{
			for ( x = - *width / 2; x < *width / 2; x ++ )
			{
				dx = MapX( affine->matrix, x, y ) + b_width / 2;
				dy = MapY( affine->matrix, x, y ) + b_height / 2;

				if ( dx >= 0 && dx < b_width && dy >=0 && dy < b_height )
				{
					*p ++ = *( b_image + dy * b_width * 2 + dx * 2 );
					if ( x % 2 == 0 )
						*p ++ = *( b_image + dy * b_width * 2 + ( dx / 2 ) * 4 + 1 );
					else
						*p ++ = *( b_image + dy * b_width * 2 + ( dx / 2 ) * 4 + 3 );
				}
				else
				{
					p += 2;
				}
			}
		}
	}

	return 0;
}

/** Affine transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	// Get a unique name to store the frame position
	char *name = mlt_properties_get( mlt_transition_properties( transition ), "_unique_id" );

	// Assign the current position to the name
	mlt_properties_set_position( mlt_frame_properties( a_frame ), name, mlt_frame_get_position( a_frame ) );

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

mlt_transition transition_affine_init( char *arg )
{
	mlt_transition transition = mlt_transition_new( );
	if ( transition != NULL )
	{
		affine_t *affine = malloc( sizeof( affine_t ) );
		affine_init( affine->matrix );
		mlt_properties_set_data( mlt_transition_properties( transition ), "affine", affine, 0, free, NULL );
		mlt_properties_set_int( mlt_transition_properties( transition ), "sx", 1 );
		mlt_properties_set_int( mlt_transition_properties( transition ), "sy", 1 );
		mlt_properties_set( mlt_transition_properties( transition ), "geometry", "0,0:100%x100%" );
		transition->process = transition_process;
	}
	return transition;
}
