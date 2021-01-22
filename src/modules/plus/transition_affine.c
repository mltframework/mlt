/*
 * transition_affine.c -- affine transformations
 * Copyright (C) 2003-2021 Meltytech, LLC
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
#include <float.h>

#include "interp.h"

#define MLT_AFFINE_MAX_DIMENSION (16000)

static double alignment_parse( char* align )
{
	int ret = 0.0;

	if ( align == NULL );
	else if ( isdigit( align[ 0 ] ) )
		ret = atoi( align );
	else if ( align[ 0 ] == 'c' || align[ 0 ] == 'm' )
		ret = 1.0;
	else if ( align[ 0 ] == 'r' || align[ 0 ] == 'b' )
		ret = 2.0;

	return ret;
}

static mlt_position repeat_position(mlt_properties properties, const char* name, mlt_position position, int length)
{
	// Make mlt_properties parse and refresh animation.
	mlt_properties_anim_get_double(properties, name, position, length);
	mlt_animation animation = mlt_properties_get_animation(properties, name);
	if (animation) {
		// Apply repeat and mirror options.
		int anim_length = mlt_animation_get_length(animation);
		int repeat_off = mlt_properties_get_int(properties, "repeat_off");
		if (!repeat_off && position >= anim_length && anim_length != 0) {
			int section = position / anim_length;
			int mirror_off = mlt_properties_get_int(properties, "mirror_off");
			position -= section * anim_length;
			if (!mirror_off && section % 2 == 1)
				position = anim_length - position;
		}
	}
	return position;
}

static double anim_get_angle(mlt_properties properties, const char* name, mlt_position position, mlt_position length)
{
	double result = 0.0;
	if (mlt_properties_get(properties, name)) {
		position = repeat_position(properties, name, position, length);
		result = mlt_properties_anim_get_double(properties, name, position, length);
		if (strchr(mlt_properties_get(properties, name), '%'))
			result *= 360;
	}
	return result;
}

/** Calculate real geometry.
*/

static void geometry_calculate( mlt_transition transition, const char *store, struct mlt_geometry_item_s *output, double position )
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

static mlt_geometry composite_calculate( mlt_transition transition, struct mlt_geometry_item_s *result, int nw, int nh, double position )
{
	// Structures for geometry
	mlt_geometry start = transition_parse_keys( transition, "geometry", "geometries", nw, nh );

	// Do the calculation
	geometry_calculate( transition, "geometries", result, position );

	return start;
}

typedef struct
{
	double matrix[3][3];
}
affine_t;

static void affine_init( double affine[3][3] )
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
static void affine_multiply( double affine[3][3], double matrix[3][3] )
{
	double output[3][3];
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
static void affine_rotate_x( double affine[3][3], double angle )
{
	double matrix[3][3];
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

static void affine_rotate_y( double affine[3][3], double angle )
{
	double matrix[3][3];
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

static void affine_rotate_z( double affine[3][3], double angle )
{
	double matrix[3][3];
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

static void affine_scale( double affine[3][3], double sx, double sy )
{
	double matrix[3][3];
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
static void affine_shear( double affine[3][3], double shear_x, double shear_y, double shear_z )
{
	double matrix[3][3];
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

static void affine_offset( double affine[3][3], double x, double y )
{
	affine[0][2] += x;
	affine[1][2] += y;
}

// Obtain the mapped x coordinate of the input
static inline double MapX( double affine[3][3], double x, double y )
{
	return affine[0][0] * x + affine[0][1] * y + affine[0][2];
}

// Obtain the mapped y coordinate of the input
static inline double MapY( double affine[3][3], double x, double y )
{
	return affine[1][0] * x + affine[1][1] * y + affine[1][2];
}

static inline double MapZ( double affine[3][3], double x, double y )
{
	return affine[2][0] * x + affine[2][1] * y + affine[2][2];
}

static void affine_max_output( double affine[3][3], double *w, double *h, double dz, double max_width, double max_height )
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

	*w = ( double )( max_x - min_x + 1 ) / max_width / 2.0;
	*h = ( double )( max_y - min_y + 1 ) / max_height / 2.0;
}

#define IN_RANGE( v, r )	( v >= - r / 2 && v < r / 2 )

static inline void get_affine( affine_t *affine, mlt_transition transition,
	double position, int length, double scale_width, double scale_height )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	int keyed = mlt_properties_get_int( properties, "keyed" );

	if ( keyed == 0 )
	{
		double fix_rotate_x = anim_get_angle( properties, "fix_rotate_x", position, length );
		double fix_rotate_y = anim_get_angle( properties, "fix_rotate_y", position, length );
		double fix_rotate_z = anim_get_angle( properties, "fix_rotate_z", position, length );
		double rotate_x = mlt_properties_get_double( properties, "rotate_x" );
		double rotate_y = mlt_properties_get_double( properties, "rotate_y" );
		double rotate_z = mlt_properties_get_double( properties, "rotate_z" );
		double fix_shear_x = anim_get_angle( properties, "fix_shear_x", position, length );
		double fix_shear_y = anim_get_angle( properties, "fix_shear_y", position, length );
		double fix_shear_z = anim_get_angle( properties, "fix_shear_z", position, length );
		double shear_x = mlt_properties_get_double( properties, "shear_x" );
		double shear_y = mlt_properties_get_double( properties, "shear_y" );
		double shear_z = mlt_properties_get_double( properties, "shear_z" );
		double ox = mlt_properties_anim_get_double( properties, "ox", position, length );
		double oy = mlt_properties_anim_get_double( properties, "oy", position, length );

		affine_rotate_x( affine->matrix, fix_rotate_x + rotate_x * position );
		affine_rotate_y( affine->matrix, fix_rotate_y + rotate_y * position );
		affine_rotate_z( affine->matrix, fix_rotate_z + rotate_z * position );
		affine_shear( affine->matrix,
					  fix_shear_x + shear_x * position,
					  fix_shear_y + shear_y * position,
					  fix_shear_z + shear_z * position );
		affine_offset( affine->matrix, ox * scale_width, oy * scale_height );
	}
	else
	{
		double rotate_x = anim_get_angle(properties, "rotate_x", position, length);
		double rotate_y = anim_get_angle(properties, "rotate_y", position, length);
		double rotate_z = anim_get_angle(properties, "rotate_z", position, length);
		double shear_x = anim_get_angle(properties, "shear_x", position, length);
		double shear_y = anim_get_angle(properties, "shear_y", position, length);
		double shear_z = anim_get_angle(properties, "shear_z", position, length);
		double o_x = mlt_properties_anim_get_double(properties, "ox",
			repeat_position(properties, "ox", position, length), length);
		double o_y = mlt_properties_anim_get_double(properties, "oy",
			repeat_position(properties, "oy", position, length), length);
		
		affine_rotate_x( affine->matrix, rotate_x );
		affine_rotate_y( affine->matrix, rotate_y );
		affine_rotate_z( affine->matrix, rotate_z );
		affine_shear( affine->matrix, shear_x, shear_y, shear_z );
		affine_offset( affine->matrix, o_x * scale_width, o_y * scale_height );
	}
}

struct sliced_desc
{
	uint8_t *a_image, *b_image;
	interpp interp;
	affine_t affine;
	int a_width, a_height, b_width, b_height;
	double lower_x, lower_y;
	double dz, mix;
	double x_offset, y_offset;
	int b_alpha;
	double minima, xmax, ymax;
};

static int sliced_proc( int id, int index, int jobs, void* cookie )
{
	(void) id; // unused
	struct sliced_desc ctx = *((struct sliced_desc*) cookie);
	int height_slice = (ctx.a_height + jobs / 2) / jobs;
	int starty = height_slice * index;
	double x, y;
	double dx, dy;
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

	if ( mirror && position > length / 2 )
		position = abs( position - length );

	// Preview scaling is not working correctly when offsets are active. I have
	// not figured out the math; so, disable preview scaling for now.
	double ox = mlt_properties_anim_get_double( properties, "ox", position, length );
	double oy = mlt_properties_anim_get_double( properties, "oy", position, length );
	if (ox != 0.0 || oy != 0.0) {
		*width = normalised_width;
		*height = normalised_height;
	}
	
	// Fetch the a frame image
	*format = mlt_image_rgb24a;
	int error = mlt_frame_get_image( a_frame, image, format, width, height, 1 );
	if (error || !image)
		return error;

	// Calculate the region now
	double scale_width = mlt_profile_scale_width(profile, *width);
	double scale_height = mlt_profile_scale_height(profile, *height);
	mlt_rect result = {0, 0, normalised_width, normalised_height, 1.0};

	mlt_service_lock( MLT_TRANSITION_SERVICE( transition ) );

	if (mlt_properties_get(properties, "geometry"))
	{
		// Structures for geometry
		struct mlt_geometry_item_s geometry;
		composite_calculate( transition, &geometry, normalised_width, normalised_height, ( double )position );
		result.x = geometry.x;
		result.y = geometry.y;
		result.w = geometry.w;
		result.h = geometry.h;
		result.o = geometry.mix / 100.0f;
	}
	else if (mlt_properties_get(properties, "rect"))
	{
		// Determine length and obtain cycle
		double cycle = mlt_properties_get_double( properties, "cycle" );
	
		// Allow a repeat cycle
		if ( cycle >= 1 )
			length = cycle;
		else if ( cycle > 0 )
			length *= cycle;
		
		mlt_position anim_pos = repeat_position(properties, "rect", position, length);
		result = mlt_properties_anim_get_rect(properties, "rect", anim_pos, length);
		if (mlt_properties_get(properties, "rect") && strchr(mlt_properties_get(properties, "rect"), '%')) {
			result.x *= normalised_width;
			result.y *= normalised_height;
			result.w *= normalised_width;
			result.h *= normalised_height;
		}
		result.o = (result.o == DBL_MIN)? 1.0 : MIN(result.o, 1.0);
	}

	int threads = mlt_properties_get_int(properties, "threads");
	threads = CLAMP(threads, 0, mlt_slices_count_normal());
	if (threads == 1)
		mlt_service_unlock( MLT_TRANSITION_SERVICE( transition ) );

	result.x *= scale_width;
	result.y *= scale_height;
	result.w *= scale_width;
	result.h *= scale_height;

	double geometry_w = result.w;
	double geometry_h = result.h;
	int fill = mlt_properties_get_int( properties, "fill" );
	int distort = mlt_properties_get_int( properties, "distort" );

	if ( !fill )
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
	if (scale_width != 1.0 || scale_height != 1.0) {
			// Scale request of b frame image to consumer scale maintaining its aspect ratio.
			b_height = CLAMP(*height, 1, MLT_AFFINE_MAX_DIMENSION);
			b_width  = MAX(b_height * b_dar / b_ar, 1);
			if (b_width > MLT_AFFINE_MAX_DIMENSION) {
				b_width  = CLAMP(*width, 1, MLT_AFFINE_MAX_DIMENSION);
				b_height = MAX(b_width * b_ar / b_dar, 1);
			}
			// Set the rescale interpolation to match the frame
			mlt_properties_set( b_props, "rescale.interp", mlt_properties_get( a_props, "rescale.interp" ) );
			// Disable padding (resize filter)
			mlt_properties_set_int( b_props, "distort", 1 );
	} else if (!mlt_properties_get_int(b_props, "interpolation_not_required")
			   && (fill || distort || b_width > result.w || b_height > result.h
				   || mlt_properties_get_int(properties, "b_scaled") || mlt_properties_get_int(b_props, "always_scale"))) {
		// Request b frame image scaled to what is needed.
		b_height = CLAMP(result.h, 1, MLT_AFFINE_MAX_DIMENSION);
		b_width  = MAX(b_height * b_dar / b_ar, 1);
		if (b_width > MLT_AFFINE_MAX_DIMENSION) {
			b_width  = CLAMP(result.w, 1, MLT_AFFINE_MAX_DIMENSION);
			b_height = MAX(b_width * b_ar / b_dar, 1);
		}
		// Set the rescale interpolation to match the frame
		mlt_properties_set( b_props, "rescale.interp", mlt_properties_get( a_props, "rescale.interp" ) );
		// Disable padding (resize filter)
		mlt_properties_set_int( b_props, "distort", 1 );
	} else {
		// Request at resolution of b frame image. This only happens when not using fill or distort mode
		// and the image is smaller than the rect with the intention to prevent scaling of the
		// image and merely position and possibly transform.
		mlt_properties_set_int( b_props, "rescale_width", b_width );
		mlt_properties_set_int( b_props, "rescale_height", b_height );

		const char* b_resource = mlt_properties_get(
			MLT_PRODUCER_PROPERTIES(mlt_frame_get_original_producer(b_frame)), "resource");
		// Check if we are applied as a filter inside a transition
		if (b_resource && !strcmp("<track>", b_resource)) {
			// Set the rescale interpolation to match the frame
			mlt_properties_set( b_props, "rescale.interp", mlt_properties_get( a_props, "rescale.interp" ) );
		} else {
			// Suppress padding and aspect normalization.
			mlt_properties_set( b_props, "rescale.interp", "none" );
		}
	}
	mlt_log_debug(MLT_TRANSITION_SERVICE(transition), "requesting image B at resolution %dx%d\n", b_width, b_height);

	// This is not a field-aware transform.
	mlt_properties_set_int( b_props, "consumer_deinterlace", 1 );

	error = mlt_frame_get_image( b_frame, &b_image, &b_format, &b_width, &b_height, 0 );
	if (error || !b_image) {
		// Remove potentially large image on the B frame. 
		mlt_frame_set_image( b_frame, NULL, 0, NULL );
		if (threads != 1)
			mlt_service_unlock( MLT_TRANSITION_SERVICE( transition ) );		
		return error;
	}

	// Check that both images are of the correct format and process
	if ( *format == mlt_image_rgb24a && b_format == mlt_image_rgb24a )
	{
		double sw, sh;
		// Get values from the transition
		double scale_x = mlt_properties_anim_get_double( properties, "scale_x", position, length );
		double scale_y = mlt_properties_anim_get_double( properties, "scale_y", position, length );
		int scale = mlt_properties_get_int( properties, "scale" );
		double geom_scale_x = (double) b_width / result.w;
		double geom_scale_y = (double) b_height / result.h;
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
			.mix = result.o,
			.x_offset = (double) b_width / 2.0,
			.y_offset = (double) b_height / 2.0,
			.b_alpha = mlt_properties_get_int( properties, "b_alpha" ),
			// Affine boundaries
			.minima = 0,
			.xmax = b_width - 1,
			.ymax = b_height - 1
		};

		// Recalculate vars if alignment supplied.
		if ( mlt_properties_get( properties, "halign" ) || mlt_properties_get( properties, "valign" ) )
		{
			double halign = alignment_parse( mlt_properties_get( properties, "halign" ) );
			double valign = alignment_parse( mlt_properties_get( properties, "valign" ) );
			desc.x_offset = halign * b_width / 2.0;
			desc.y_offset = valign * b_height / 2.0;
			desc.lower_x = -(result.x + geometry_w * halign / 2.0f);
			desc.lower_y = -(result.y + geometry_h * valign / 2.0f);
		}

		affine_init( desc.affine.matrix );

		// Compute the affine transform
		get_affine( &desc.affine, transition, ( double )position, length, scale_width, scale_height );
		desc.dz = MapZ( desc.affine.matrix, 0, 0 );
		if ( (int) fabs( desc.dz * 1000 ) < 25 ) {
			if (threads != 1)
				mlt_service_unlock( MLT_TRANSITION_SERVICE( transition ) );		
			return 0;
		}

		if (mlt_properties_get_int(properties, "invert_scale")) {
			scale_x = 1.0 / scale_x;
			scale_y = 1.0 / scale_y;
		}

		// Factor scaling into the transformation based on output resolution.
		if ( distort )
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
		if (threads == 1)
			sliced_proc(0, 0, 1, &desc);
		else
			mlt_slices_run_normal(threads, sliced_proc, &desc);
		
		// Remove potentially large image on the B frame. 
		mlt_frame_set_image( b_frame, NULL, 0, NULL );
	}
	if (threads != 1)
		mlt_service_unlock( MLT_TRANSITION_SERVICE( transition ) );		
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
		mlt_properties_set( MLT_TRANSITION_PROPERTIES( transition ), "rect", "0%/0%:100%x100%:100%" );
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "fill", 1 );
		transition->process = transition_process;
	}
	return transition;
}
