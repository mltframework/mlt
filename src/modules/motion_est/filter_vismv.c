/*
 *	/brief Draw motion vectors
 *	/author Zachary Drew, Copyright 2004
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "filter_motion_est.h"
#include "arrow_code.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

static void paint_arrows( uint8_t *image, struct motion_vector_s *vectors, int w, int h, int mb_w, int mb_h )
{
	int i, j, x, y;
	struct motion_vector_s *p;
	for( i = 0; i < w/mb_w; i++ ){
		for( j = 0; j < h/mb_h; j++ ){
			x = i*mb_w;
			y = j*mb_h;
			p = vectors + (w/mb_w)*j + i;

			if ( p->valid == 1 ) {
				//draw_rectangle_outline(image, x-1, y-1, mb_w+1, mb_h+1,100);
				//x += mb_w/4;
				//y += mb_h/4;
				//draw_rectangle_outline(image, x + p->dx, y + p->dy, mb_w, mb_h,100);
				x += mb_w/2;
				y += mb_h/2;
				draw_arrow(image, x, y, x + p->dx, y + p->dy, 100);
				//draw_rectangle_fill(image, x + p->dx, y + p->dy, mb_w, mb_h, 100);
			}
			else if ( p->valid == 2 ) {
				draw_rectangle_outline(image, x+1, y+1, mb_w-2, mb_h-2,100);
			}
			else if ( p->valid == 3 ) {
				draw_rectangle_fill(image, x-p->dx, y-p->dy, mb_w, mb_h,0);
			}
			else if ( p->valid == 4 ) {
				draw_line(image, x, y, x + 4, y, 100);
				draw_line(image, x, y, x, y + 4, 100);
				draw_line(image, x + 4, y, x, y + 4, 100);

				draw_line(image, x+mb_w-1, y+mb_h-1, x+mb_w-5, y+mb_h-1, 100);
				draw_line(image, x+mb_w-1, y+mb_h-1, x+mb_w-1, y+mb_h-5, 100);
				draw_line(image, x+mb_w-5, y+mb_h-1, x+mb_w-1, y+mb_h-5, 100);
			}
		}
	}
}

// Image stack(able) method
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the frame properties
	mlt_properties properties = MLT_FRAME_PROPERTIES(frame);

	// Get the new image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if( error != 0 )
		mlt_properties_debug( MLT_FRAME_PROPERTIES(frame), "error after mlt_frame_get_image()", stderr );


	// Get the size of macroblocks in pixel units
	int macroblock_height = mlt_properties_get_int( properties, "motion_est.macroblock_height" );
	int macroblock_width = mlt_properties_get_int( properties, "motion_est.macroblock_width" );

	// Get the motion vectors
	struct motion_vector_s *current_vectors = mlt_properties_get_data( properties, "motion_est.vectors", NULL );

	init_arrows( format, *width, *height );

	if ( mlt_properties_get_int( properties, "shot_change" ) == 1 )
	{
		draw_line(*image, 0, 0, *width, *height, 100);
		draw_line(*image, 0, *height, *width, 0, 100);
	}
	if( current_vectors != NULL ) {
		paint_arrows( *image, current_vectors, *width, *height, macroblock_width, macroblock_height);
	}

	return error;
}



/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Push the frame filter
	mlt_frame_push_get_image( frame, filter_get_image );


	return frame;
}

/** Constructor for the filter.
*/


mlt_filter filter_vismv_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;

	}

	return this;
}

/** This source code will self destruct in 5...4...3...
*/








