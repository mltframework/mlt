/*
 * transition_affine.c -- affine transformations
 * Copyright (C) 2003-2017 Meltytech, LLC
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

#include <framework/mlt_transition.h>
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "interp.h"

static float alignment_parse( char* align )
{
	int ret = 0.0f;

	if ( align == NULL );
	else if ( isdigit( align[ 0 ] ) )
		ret = atoi( align );
	else if ( align[ 0 ] == 'c' || align[ 0 ] == 'm' )
		ret = 1.0f;
	else if ( align[ 0 ] == 'r' || align[ 0 ] == 'b' )
		ret = 2.0f;

	return ret;
}

/** Calculate real geometry.
*/

static void geometry_calculate( mlt_transition transition, const char *store, struct mlt_geometry_item_s *output, float position )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_geometry geometry = mlt_properties_get_data( properties, store, NULL );
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
	mlt_geometry_fetch( geometry, output, position );
}


static mlt_geometry transition_parse_keys( mlt_transition transition, const char *name, const char *store, int normalised_width, int normalised_height )
{
	// Get the properties of the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );

	// Try to fetch it first
	mlt_geometry geometry = mlt_properties_get_data( properties, store, NULL );

	// Determine length and obtain cycle
	mlt_position length = mlt_transition_get_length( transition );
	double cycle = mlt_properties_get_double( properties, "cycle" );

	// Allow a geometry repeat cycle
	if ( cycle >= 1 )
		length = cycle;
	else if ( cycle > 0 )
		length *= cycle;

	if ( geometry == NULL )
	{
		// Get the new style geometry string
		char *property = mlt_properties_get( properties, name );

		// Create an empty geometries object
		geometry = mlt_geometry_init( );

		// Parse the geometry if we have one
		mlt_geometry_parse( geometry, property, length, normalised_width, normalised_height );

		// Store it
		mlt_properties_set_data( properties, store, geometry, 0, ( mlt_destructor )mlt_geometry_close, NULL );
	}
	else
	{
		// Check for updates and refresh if necessary
		mlt_geometry_refresh( geometry, mlt_properties_get( properties, name ), length, normalised_width, normalised_height );
	}

	return geometry;
}

static mlt_geometry composite_calculate( mlt_transition transition, struct mlt_geometry_item_s *result, int nw, int nh, float position )
{
	// Structures for geometry
	mlt_geometry start = transition_parse_keys( transition, "geometry", "geometries", nw, nh );

	// Do the calculation
	geometry_calculate( transition, "geometries", result, position );

	return start;
}

static inline float composite_calculate_key( mlt_transition transition, const char *name, const char *store, int norm, float position )
{
	// Struct for the result
	struct mlt_geometry_item_s result;

	// Structures for geometry
	transition_parse_keys( transition, name, store, norm, 0 );

	// Do the calculation
	geometry_calculate( transition, store, &result, position );

	return result.x;
}

typedef struct
{
	float matrix[3][3];
}
affine_t;

static void affine_init( float affine[3][3] )
{
	affine[0][0] = 1;
	affine[0][1] = 0;
	affine[0][2] = 0;
	affine[1][0] = 0;
	affine[1][1] = 1;
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
}

// Multiply two this affine transform with that
static void affine_multiply( float affine[3][3], float matrix[3][3] )
{
	float output[3][3];
	int i;
	int j;

	for ( i = 0; i < 3; i ++ )
		for ( j = 0; j < 3; j ++ )
			output[i][j] = affine[i][0] * matrix[j][0] + affine[i][1] * matrix[j][1] + affine[i][2] * matrix[j][2];

	affine[0][0] = output[0][0];
	affine[0][1] = output[0][1];
	affine[0][2] = output[0][2];
	affine[1][0] = output[1][0];
	affine[1][1] = output[1][1];
	affine[1][2] = output[1][2];
	affine[2][0] = output[2][0];
	affine[2][1] = output[2][1];
	affine[2][2] = output[2][2];
}

// Rotate by a given angle
static void affine_rotate_x( float affine[3][3], float angle )
{
	float matrix[3][3];
	matrix[0][0] = cos( angle * M_PI / 180 );
	matrix[0][1] = 0 - sin( angle * M_PI / 180 );
	matrix[0][2] = 0;
	matrix[1][0] = sin( angle * M_PI / 180 );
	matrix[1][1] = cos( angle * M_PI / 180 );
	matrix[1][2] = 0;
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	affine_multiply( affine, matrix );
}

static void affine_rotate_y( float affine[3][3], float angle )
{
	float matrix[3][3];
	matrix[0][0] = cos( angle * M_PI / 180 );
	matrix[0][1] = 0;
	matrix[0][2] = 0 - sin( angle * M_PI / 180 );
	matrix[1][0] = 0;
	matrix[1][1] = 1;
	matrix[1][2] = 0;
	matrix[2][0] = sin( angle * M_PI / 180 );
	matrix[2][1] = 0;
	matrix[2][2] = cos( angle * M_PI / 180 );
	affine_multiply( affine, matrix );
}

static void affine_rotate_z( float affine[3][3], float angle )
{
	float matrix[3][3];
	matrix[0][0] = 1;
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[1][0] = 0;
	matrix[1][1] = cos( angle * M_PI / 180 );
	matrix[1][2] = sin( angle * M_PI / 180 );
	matrix[2][0] = 0;
	matrix[2][1] = - sin( angle * M_PI / 180 );
	matrix[2][2] = cos( angle * M_PI / 180 );
	affine_multiply( affine, matrix );
}

static void affine_scale( float affine[3][3], float sx, float sy )
{
	float matrix[3][3];
	matrix[0][0] = sx;
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[1][0] = 0;
	matrix[1][1] = sy;
	matrix[1][2] = 0;
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	affine_multiply( affine, matrix );
}

// Shear by a given value
static void affine_shear( float affine[3][3], float shear_x, float shear_y, float shear_z )
{
	float matrix[3][3];
	matrix[0][0] = 1;
	matrix[0][1] = tan( shear_x * M_PI / 180 );
	matrix[0][2] = 0;
	matrix[1][0] = tan( shear_y * M_PI / 180 );
	matrix[1][1] = 1;
	matrix[1][2] = tan( shear_z * M_PI / 180 );
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	affine_multiply( affine, matrix );
}

static void affine_offset( float affine[3][3], float x, float y )
{
	affine[0][2] += x;
	affine[1][2] += y;
}

// Obtain the mapped x coordinate of the input
static inline double MapX( float affine[3][3], float x, float y )
{
	return affine[0][0] * x + affine[0][1] * y + affine[0][2];
}

// Obtain the mapped y coordinate of the input
static inline double MapY( float affine[3][3], float x, float y )
{
	return affine[1][0] * x + affine[1][1] * y + affine[1][2];
}

static inline double MapZ( float affine[3][3], float x, float y )
{
	return affine[2][0] * x + affine[2][1] * y + affine[2][2];
}

static void affine_max_output( float affine[3][3], float *w, float *h, float dz, float max_width, float max_height )
{
	int tlx = MapX( affine, -max_width,  max_height ) / dz;
	int tly = MapY( affine, -max_width,  max_height ) / dz;
	int trx = MapX( affine,  max_width,  max_height ) / dz;
	int try = MapY( affine,  max_width,  max_height ) / dz;
	int blx = MapX( affine, -max_width, -max_height ) / dz;
	int bly = MapY( affine, -max_width, -max_height ) / dz;
	int brx = MapX( affine,  max_width, -max_height ) / dz;
	int bry = MapY( affine,  max_width, -max_height ) / dz;

	int max_x;
	int max_y;
	int min_x;
	int min_y;

	max_x = MAX( tlx, trx );
	max_x = MAX( max_x, blx );
	max_x = MAX( max_x, brx );

	min_x = MIN( tlx, trx );
	min_x = MIN( min_x, blx );
	min_x = MIN( min_x, brx );

	max_y = MAX( tly, try );
	max_y = MAX( max_y, bly );
	max_y = MAX( max_y, bry );

	min_y = MIN( tly, try );
	min_y = MIN( min_y, bly );
	min_y = MIN( min_y, bry );

	*w = ( float )( max_x - min_x + 1 ) / max_width / 2.0;
	*h = ( float )( max_y - min_y + 1 ) / max_height / 2.0;
}

#define IN_RANGE( v, r )	( v >= - r / 2 && v < r / 2 )

static inline void get_affine( affine_t *affine, mlt_transition transition, float position )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	int keyed = mlt_properties_get_int( properties, "keyed" );

	if ( keyed == 0 )
	{
		float fix_rotate_x = mlt_properties_get_double( properties, "fix_rotate_x" );
		float fix_rotate_y = mlt_properties_get_double( properties, "fix_rotate_y" );
		float fix_rotate_z = mlt_properties_get_double( properties, "fix_rotate_z" );
		float rotate_x = mlt_properties_get_double( properties, "rotate_x" );
		float rotate_y = mlt_properties_get_double( properties, "rotate_y" );
		float rotate_z = mlt_properties_get_double( properties, "rotate_z" );
		float fix_shear_x = mlt_properties_get_double( properties, "fix_shear_x" );
		float fix_shear_y = mlt_properties_get_double( properties, "fix_shear_y" );
		float fix_shear_z = mlt_properties_get_double( properties, "fix_shear_z" );
		float shear_x = mlt_properties_get_double( properties, "shear_x" );
		float shear_y = mlt_properties_get_double( properties, "shear_y" );
		float shear_z = mlt_properties_get_double( properties, "shear_z" );
		float ox = mlt_properties_get_double( properties, "ox" );
		float oy = mlt_properties_get_double( properties, "oy" );

		affine_rotate_x( affine->matrix, fix_rotate_x + rotate_x * position );
		affine_rotate_y( affine->matrix, fix_rotate_y + rotate_y * position );
		affine_rotate_z( affine->matrix, fix_rotate_z + rotate_z * position );
		affine_shear( affine->matrix,
					  fix_shear_x + shear_x * position,
					  fix_shear_y + shear_y * position,
					  fix_shear_z + shear_z * position );
		affine_offset( affine->matrix, ox, oy );
	}
	else
	{
		float rotate_x = composite_calculate_key( transition, "rotate_x", "rotate_x_info", 360, position );
		float rotate_y = composite_calculate_key( transition, "rotate_y", "rotate_y_info", 360, position );
		float rotate_z = composite_calculate_key( transition, "rotate_z", "rotate_z_info", 360, position );
		float shear_x = composite_calculate_key( transition, "shear_x", "shear_x_info", 360, position );
		float shear_y = composite_calculate_key( transition, "shear_y", "shear_y_info", 360, position );
		float shear_z = composite_calculate_key( transition, "shear_z", "shear_z_info", 360, position );
		float o_x = composite_calculate_key( transition, "ox", "ox_info", 0, position );
		float o_y = composite_calculate_key( transition, "oy", "oy_info", 0, position );
		
		affine_rotate_x( affine->matrix, rotate_x );
		affine_rotate_y( affine->matrix, rotate_y );
		affine_rotate_z( affine->matrix, rotate_z );
		affine_shear( affine->matrix, shear_x, shear_y, shear_z );
		affine_offset( affine->matrix, o_x, o_y );
	}
}

struct sliced_desc
{
	uint8_t *a_image, *b_image;
	interpp interp;
	affine_t affine;
	int a_width, a_height, b_width, b_height;
	float lower_x, lower_y;
	float dz, mix;
	float x_offset, y_offset;
	int b_alpha;
	float minima, xmax, ymax;
};

static int sliced_proc( int id, int index, int jobs, void* cookie )
{
	(void) id; // unused
	struct sliced_desc ctx = *((struct sliced_desc*) cookie);
	int height_slice = (ctx.a_height + jobs / 2) / jobs;
	int starty = height_slice * index;
	float x, y;
	float dx, dy;
	int i, j;

	ctx.a_image += (index * height_slice) * (ctx.a_width * 4);
	for (i = 0, y = ctx.lower_y; i < ctx.a_height; i++, y++) {
		if (i >= starty && i < (starty + height_slice)) {
			for (j = 0, x = ctx.lower_x; j < ctx.a_width; j++, x++) {
				dx = MapX( ctx.affine.matrix, x, y ) / ctx.dz + ctx.x_offset;
				dy = MapY( ctx.affine.matrix, x, y ) / ctx.dz + ctx.y_offset;
				if (dx >= ctx.minima && dx <= ctx.xmax && dy >= ctx.minima && dy <= ctx.ymax)
					ctx.interp(ctx.b_image, ctx.b_width, ctx.b_height, dx, dy, ctx.mix, ctx.a_image, ctx.b_alpha);
				ctx.a_image += 4;
			}
		}
	}
	return 0;
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

	// Image, format, width, height and image for the b frame
	uint8_t *b_image = NULL;
	mlt_image_format b_format = mlt_image_rgb24a;
	int b_width = mlt_properties_get_int( b_props, "meta.media.width" );
	int b_height = mlt_properties_get_int( b_props, "meta.media.height" );
	double b_ar = mlt_frame_get_aspect_ratio( b_frame );
	double b_dar = b_ar * b_width / b_height;

	// Assign the current position
	mlt_position position =  mlt_transition_get_position( transition, a_frame );

	int mirror = mlt_properties_get_position( properties, "mirror" );
	int length = mlt_transition_get_length( transition );
	if ( mlt_properties_get_int( properties, "always_active" ) )
	{
		mlt_properties props = mlt_properties_get_data( b_props, "_producer", NULL );
		mlt_position in = mlt_properties_get_int( props, "in" );
		mlt_position out = mlt_properties_get_int( props, "out" );
		length = out - in + 1;
	}

	// Obtain the normalised width and height from the a_frame
	mlt_profile profile = mlt_service_profile( MLT_TRANSITION_SERVICE( transition ) );
	int normalised_width = profile->width;
	int normalised_height = profile->height;

	double consumer_ar = mlt_profile_sar( profile );

	// Structures for geometry
	struct mlt_geometry_item_s result;

	if ( mirror && position > length / 2 )
		position = abs( position - length );

	// Fetch the a frame image
	*format = mlt_image_rgb24a;
	int error = mlt_frame_get_image( a_frame, image, format, width, height, 1 );
	if (error || !image)
		return error;

	// Calculate the region now
	mlt_service_lock( MLT_TRANSITION_SERVICE( transition ) );
	composite_calculate( transition, &result, normalised_width, normalised_height, ( float )position );
	mlt_service_unlock( MLT_TRANSITION_SERVICE( transition ) );

	float geometry_w = result.w;
	float geometry_h = result.h;

	if ( !mlt_properties_get_int( properties, "fill" ) )
	{
		double geometry_dar = result.w * consumer_ar / result.h;

		if ( b_dar > geometry_dar )
		{
			result.w = MIN( result.w, b_width * b_ar / consumer_ar );
			result.h = result.w * consumer_ar / b_dar;
		}
		else
		{
			result.h = MIN( result.h, b_height );
			result.w = result.h * b_dar / consumer_ar;
		}
	}

	// Fetch the b frame image
	result.w = ( result.w * *width / normalised_width );
	result.h = ( result.h * *height / normalised_height );
	result.x = ( result.x * *width / normalised_width );
	result.y = ( result.y * *height / normalised_height );

	// Request full resolution of b frame image.
	mlt_properties_set_int( b_props, "rescale_width", b_width );
	mlt_properties_set_int( b_props, "rescale_height", b_height );

	// Suppress padding and aspect normalization.
	mlt_properties_set( b_props, "rescale.interp", "none" );

	// This is not a field-aware transform.
	mlt_properties_set_int( b_props, "consumer_deinterlace", 1 );

	error = mlt_frame_get_image( b_frame, &b_image, &b_format, &b_width, &b_height, 0 );
	if (error || !b_image) 
		return error;

	// Check that both images are of the correct format and process
	if ( *format == mlt_image_rgb24a && b_format == mlt_image_rgb24a )
	{
		float sw, sh;
		// Get values from the transition
		float scale_x = mlt_properties_get_double( properties, "scale_x" );
		float scale_y = mlt_properties_get_double( properties, "scale_y" );
		int scale = mlt_properties_get_int( properties, "scale" );
		float geom_scale_x = (float) b_width / result.w;
		float geom_scale_y = (float) b_height / result.h;
		struct sliced_desc desc = {
			.a_image = *image,
			.b_image = b_image,
			.interp = interpBL_b32,
			.a_width = *width,
			.a_height = *height,
			.b_width = b_width,
			.b_height = b_height,
			.lower_x = -(result.x + result.w / 2.0), // center
			.lower_y = -(result.y + result.h / 2.0), // middle
			.mix = result.mix / 100.0f,
			.x_offset = (float) b_width / 2.0,
			.y_offset = (float) b_height / 2.0,
			.b_alpha = mlt_properties_get_int( properties, "b_alpha" ),
			// Affine boundaries
			.minima = 0,
			.xmax = b_width - 1,
			.ymax = b_height - 1
		};

		// Recalculate vars if alignment supplied.
		if ( mlt_properties_get( properties, "halign" ) || mlt_properties_get( properties, "valign" ) )
		{
			float halign = alignment_parse( mlt_properties_get( properties, "halign" ) );
			float valign = alignment_parse( mlt_properties_get( properties, "valign" ) );
			desc.x_offset = halign * b_width / 2.0f;
			desc.y_offset = valign * b_height / 2.0f;
			desc.lower_x = -(result.x + geometry_w * halign / 2.0f);
			desc.lower_y = -(result.y + geometry_h * valign / 2.0f);
		}

		affine_init( desc.affine.matrix );

		// Compute the affine transform
		get_affine( &desc.affine, transition, ( float )position );
		desc.dz = MapZ( desc.affine.matrix, 0, 0 );
		if ( (int) fabs( desc.dz * 1000 ) < 25 )
			return 0;

		// Factor scaling into the transformation based on output resolution.
		if ( mlt_properties_get_int( properties, "distort" ) )
		{
			scale_x = geom_scale_x * ( scale_x == 0 ? 1 : scale_x );
			scale_y = geom_scale_y * ( scale_y == 0 ? 1 : scale_y );
		}
		else
		{
			// Determine scale with respect to aspect ratio.
			double consumer_dar = consumer_ar * normalised_width / normalised_height;
			
			if ( b_dar > consumer_dar )
			{
				scale_x = geom_scale_x * ( scale_x == 0 ? 1 : scale_x );
				scale_y = geom_scale_x * ( scale_y == 0 ? 1 : scale_y );
				scale_y *= b_ar / consumer_ar;
			}
			else
			{
				scale_x = geom_scale_y * ( scale_x == 0 ? 1 : scale_x );
				scale_y = geom_scale_y * ( scale_y == 0 ? 1 : scale_y );
				scale_x *= consumer_ar / b_ar;
			}
		}
		if ( scale )
		{
			affine_max_output( desc.affine.matrix, &sw, &sh, desc.dz, *width, *height );
			affine_scale( desc.affine.matrix, sw * MIN( geom_scale_x, geom_scale_y ), sh * MIN( geom_scale_x, geom_scale_y ) );
		}
		else if ( scale_x != 0 && scale_y != 0 )
		{
			affine_scale( desc.affine.matrix, scale_x, scale_y );
		}


		char *interps = mlt_properties_get( a_props, "rescale.interp" );
		// Copy in case string is changed.
		if ( interps )
			interps = strdup( interps );

		// Set the interpolation function
		if ( interps == NULL || strcmp( interps, "nearest" ) == 0 || strcmp( interps, "neighbor" ) == 0 || strcmp( interps, "tiles" ) == 0 || strcmp( interps, "fast_bilinear" ) == 0 )
		{
			desc.interp = interpNN_b32;
			// uses lrintf. Values should be >= -0.5 and < max + 0.5
			desc.minima -= 0.5;
			desc.xmax += 0.49;
			desc.ymax += 0.49;
		}
		else if ( strcmp( interps, "bilinear" ) == 0 )
		{
			desc.interp = interpBL_b32;
			// uses floorf.
		}
		else if ( strcmp( interps, "bicubic" ) == 0 ||  strcmp( interps, "hyper" ) == 0 || strcmp( interps, "sinc" ) == 0 || strcmp( interps, "lanczos" ) == 0 || strcmp( interps, "spline" ) == 0 )
		{
			// TODO: lanczos 8x8
			// TODO: spline 4x4 or 6x6
			desc.interp = interpBC_b32;
			// uses ceilf. Values should be > -1 and <= max.
			desc.minima -= 1;
		}
		free( interps );

		// Do the transform with interpolation
		int threads = mlt_properties_get_int(properties, "threads");
		threads = CLAMP(threads, 0, mlt_slices_count_normal());
		if (threads == 1)
			sliced_proc(0, 0, 1, &desc);
		else
			mlt_slices_run_normal(threads, sliced_proc, &desc);
	}

	return 0;
}

/** Affine transition processing.
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

mlt_transition transition_affine_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_transition transition = mlt_transition_new( );
	if ( transition != NULL )
	{
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "distort", 0 );
		mlt_properties_set( MLT_TRANSITION_PROPERTIES( transition ), "geometry", "0/0:100%x100%" );
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "fill", 1 );
		transition->process = transition_process;
	}
	return transition;
}
