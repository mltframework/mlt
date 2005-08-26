/*
 *	filter_autotrack_rectangle.c
 *
 *	/brief 
 *	/author Zachary Drew, Copyright 2005
 *
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

#include "filter_motion_est.h"
#include "arrow_code.h"

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MIN(a,b) ((a) > (b) ? (b) : (a))

#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

void caculate_motion( struct motion_vector_s *vectors,
		      mlt_geometry_item boundry,
		      int macroblock_width,
		      int macroblock_height,
		      int mv_buffer_width,
		      int method )
{


	// translate pixel units (from bounds) to macroblock units
	// make sure whole macroblock stay within bounds
	int left_mb = ( boundry->x + macroblock_width - 1 ) / macroblock_width;
        int top_mb = ( boundry->y + macroblock_height - 1 ) / macroblock_height;
        int right_mb = ( boundry->x + boundry->w ) / macroblock_width - 1;
        int bottom_mb = ( boundry->y + boundry->h ) / macroblock_height - 1;

	int i, j, n = 0;

	int average_x = 0, average_y = 0;

	#define CURRENT         ( vectors + j*mv_buffer_width + i )

	for( i = left_mb; i <= right_mb; i++ ){
		for( j = top_mb; j <= bottom_mb; j++ )
		{
			n++;
			average_x += CURRENT->dx;
			average_y += CURRENT->dy;
		}
	}

	if ( n == 0 ) return;

	average_x /= n;
	average_y /= n;

	n = 0;
	int average2_x = 0, average2_y = 0;
	for( i = left_mb; i <= right_mb; i++ ){
		for( j = top_mb; j <= bottom_mb; j++ ){

			if( ABS(CURRENT->dx - average_x) < 3 &&
			    ABS(CURRENT->dy - average_y) < 3 )
			{
				n++;
				average2_x += CURRENT->dx;
				average2_y += CURRENT->dy;
			}
		}
	}

	if ( n == 0 ) return;

	boundry->x -= (double)average2_x / (double)n;
	boundry->y -= (double)average2_y / (double)n;
}

// Image stack(able) method
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	// Get the filter object
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the filter's property object
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);

	// Get the frame position
	mlt_position position = mlt_frame_get_position( frame );

	// Get the new image
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if( error != 0 )
		mlt_properties_debug( frame_properties, "error after mlt_frame_get_image() in autotrack_rectangle", stderr );

	// Get the geometry object
	mlt_geometry geometry = mlt_properties_get_data(filter_properties, "geometry", NULL);

	// Get the current geometry item
	struct mlt_geometry_item_s boundry;
	mlt_geometry_fetch(geometry, &boundry, position);

	// Get the motion vectors
	struct motion_vector_s *vectors = mlt_properties_get_data( frame_properties, "motion_est.vectors", NULL );

	// How did the rectangle move?
	if( vectors != NULL &&
	    boundry.key != 1 ) // Paused?
	{

		int method = mlt_properties_get_int( filter_properties, "method" );

		// Get the size of macroblocks in pixel units
		int macroblock_height = mlt_properties_get_int( frame_properties, "motion_est.macroblock_height" );
		int macroblock_width = mlt_properties_get_int( frame_properties, "motion_est.macroblock_width" );
		int mv_buffer_width = *width / macroblock_width;

		caculate_motion( vectors, &boundry, macroblock_width, macroblock_height, mv_buffer_width, method );


		// Make the geometry object a real boy
		boundry.key = 1;
		boundry.f[0] = 1;
		boundry.f[1] = 1;
		boundry.f[2] = 1;
		boundry.f[3] = 1;
		boundry.f[4] = 1;
		mlt_geometry_insert(geometry, &boundry);
	}

       	if( mlt_properties_get_int( filter_properties, "debug" ) == 1 )
	{
		init_arrows( format, *width, *height );
		draw_rectangle_outline(*image, boundry.x, boundry.y, boundry.w, boundry.h, 100);
	}        

	if( mlt_properties_get_int( filter_properties, "obscure" ) == 1 )
	{
		mlt_filter obscure = mlt_properties_get_data( filter_properties, "_obscure", NULL );

		mlt_properties_pass_list( MLT_FILTER_PROPERTIES(obscure), filter_properties, "in, out");

		// Because filter_obscure needs to be rewritten to use mlt_geometry
		char geom[100];
		sprintf( geom, "%d,%d:%dx%d", (int)boundry.x, (int)boundry.y, (int)boundry.w, (int)boundry.h );
		mlt_properties_set( MLT_FILTER_PROPERTIES( obscure ), "start", geom );
		mlt_properties_set( MLT_FILTER_PROPERTIES( obscure ), "end", geom );
	}
		

	return error;
}

static int attach_boundry_to_frame( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter object
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the filter's property object
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);

	// Get the frame position
	mlt_position position = mlt_frame_get_position( frame );

	// Get the geometry object
	mlt_geometry geometry = mlt_properties_get_data(filter_properties, "geometry", NULL);

	// Get the current geometry item
	mlt_geometry_item geometry_item = mlt_pool_alloc( sizeof( struct mlt_geometry_item_s ) );
	mlt_geometry_fetch(geometry, geometry_item, position);

	mlt_properties_set_data( frame_properties, "bounds", geometry_item, sizeof( struct mlt_geometry_item_s ), mlt_pool_release, NULL );

	// Get the new image
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if( error != 0 )
		mlt_properties_debug( frame_properties, "error after mlt_frame_get_image() in autotrack_rectangle attach_boundry_to_frame", stderr );

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{

        /* modify the frame with the current geometry */
	mlt_frame_push_service( frame, this);
	mlt_frame_push_get_image( frame, attach_boundry_to_frame );



	/* apply the motion estimation filter */
	mlt_filter motion_est = mlt_properties_get_data( MLT_FILTER_PROPERTIES(this), "_motion_est", NULL ); 
	mlt_filter_process( motion_est, frame);



	/* calculate the new geometry based on the motion */
	mlt_frame_push_service( frame, this);
	mlt_frame_push_get_image( frame, filter_get_image );


	/* visualize the motion vectors */
	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(this), "debug" ) == 1 )
	{
		mlt_filter vismv = mlt_properties_get_data( MLT_FILTER_PROPERTIES(this), "_vismv", NULL );
		if( vismv == NULL )
		{
			vismv = mlt_factory_filter( "vismv", NULL );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES(this), "_vismv", vismv, 0, (mlt_destructor)mlt_filter_close, NULL );
		}

		mlt_filter_process( vismv, frame );
	}

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(this), "obscure" ) == 1 )
	{
		mlt_filter obscure = mlt_properties_get_data( MLT_FILTER_PROPERTIES(this), "_obscure", NULL );
		if( obscure == NULL )
		{
			obscure = mlt_factory_filter( "obscure", NULL );
			mlt_properties_set_data( MLT_FILTER_PROPERTIES(this), "_obscure", obscure, 0, (mlt_destructor)mlt_filter_close, NULL );
		}

		mlt_filter_process( obscure, frame );
	}

	return frame;
}

/** Constructor for the filter.
*/


mlt_filter filter_autotrack_rectangle_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;


		mlt_geometry geometry = mlt_geometry_init();

		// Initialize with the supplied geometry
		if( arg != NULL ) {

			struct mlt_geometry_item_s item;

			mlt_geometry_parse_item( geometry, &item, arg  );

			item.frame = 0;
			item.key = 1;
			item.mix = 100;

			mlt_geometry_insert( geometry, &item );

		}

		// ... and attach it to the filter
		mlt_properties_set_data( MLT_FILTER_PROPERTIES(this), "geometry", geometry, 0, (mlt_destructor)mlt_geometry_close, (mlt_serialiser)mlt_geometry_serialise );

		// create an instance of the motion_est and obscure filter
		mlt_filter motion_est = mlt_factory_filter( "motion_est", NULL );
		if( motion_est != NULL )
			mlt_properties_set_data( MLT_FILTER_PROPERTIES(this), "_motion_est", motion_est, 0, (mlt_destructor)mlt_filter_close, NULL );
		else {
			mlt_filter_close( this );
			return NULL;
		}


	}

	return this;
}

/** This source code will self destruct in 5...4...3...
*/
