/*
 * producer_slowmotion.c -- create subspeed frames
 * Author: Zachary Drew
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
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#define SHIFT 8
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

// This is used to constrains pixel operations between two blocks to be within the image boundry
inline static int constrain(	int *x, int *y, int *w,	int *h,			
				const int dx, const int dy,
				const int left, const int right,
				const int top, const int bottom)
{
	uint32_t penalty = 1 << SHIFT;			// Retain a few extra bits of precision
	int x2 = *x + dx;
	int y2 = *y + dy;
	int w_remains = *w;
	int h_remains = *h;

	// Origin of macroblock moves left of image boundy
	if( *x < left || x2 < left ) {
		w_remains = *w - left + ((*x < x2) ?  *x : x2);
		*x += *w - w_remains;
	}
	// Portion of macroblock moves right of image boundry
	else if( *x + *w > right || x2 + *w > right )
		w_remains = right - ((*x > x2) ? *x : x2);

	// Origin of macroblock moves above image boundy
	if( *y < top || y2 < top ) {
		h_remains = *h - top + ((*y < y2) ? *y : y2);
		*y += *h - h_remains;
	}
	// Portion of macroblock moves bellow image boundry
	else if( *y + *h > bottom || y2 + *h > bottom )
		h_remains = bottom - ((*y > y2) ?  *y : y2);

	if( w_remains == *w && h_remains == *h ) return penalty; 
	if( w_remains <= 0 || h_remains <= 0) return 0;	// Block is clipped out of existance
	penalty = (*w * *h * penalty)
		/ ( w_remains * h_remains);		// Recipricol of the fraction of the block that remains

	*w = w_remains;					// Update the width and height
	*h = h_remains;

	return penalty;
}

static void motion_interpolate( uint8_t *first_image, uint8_t *second_image, uint8_t *output,
				int top_mb, int bottom_mb, int left_mb, int right_mb,
				int mb_w, int mb_h,
				int width, int height,
				int xstride, int ystride,
				double scale,
				motion_vector *vectors )
{
	assert ( scale >= 0.0 && scale <= 1.0 ); 

	int i, j;
	int x,y,w,h;
	int dx, dy;
	int scaled_dx, scaled_dy;
	int tx,ty;
	uint8_t *f,*s,*r;
	motion_vector *here;
	int mv_width = width / mb_w;

	for( j = top_mb; j <= bottom_mb; j++ ){  
	 for( i = left_mb; i <= right_mb; i++ ){

		here = vectors + j*mv_width + i; 
		scaled_dx = (1.0 - scale) * (double)here->dx;
		scaled_dy = (1.0 - scale) * (double)here->dy;
		dx = here->dx;
		dy = here->dy;
		w = mb_w; h = mb_h;
		x = i * w; y = j * h;

		// Denoise function caused some blocks to be completely clipped, ignore them	
		if (constrain( &x, &y, &w, &h, dx, dy, 0, width, 0, height) == 0 )
			continue;

		for( ty = y; ty < y + h ; ty++ ){
		 for( tx = x; tx < x + w ; tx++ ){

			f = first_image  + (tx + dx     )*xstride + (ty + dy     )*ystride;
			s = second_image + (tx          )*xstride + (ty	   )*ystride;
			r = output + (tx+scaled_dx)*xstride + (ty+scaled_dy)*ystride;
/*
			if( ABS(f[0] - s[0]) > 3 * here->msad / (mb_w * mb_h * 2) )
			{
				r[0] = f[0];
				r[1] = f[1];
			}

			else
			{

*/
				r[0] = ( 1.0 - scale ) * (double)f[0] + scale * (double)s[0];

				if( dx % 2 == 0 )
				{
					if( scaled_dx % 2 == 0 )
						r[1] =  ( 1.0 - scale ) * (double)f[1] + scale * (double) s[1];
					else
						*(r-1) =  ( 1.0 - scale ) * (double)f[1] + scale * (double) s[1];
				}
				else
				{
					if( scaled_dx %2 == 0 )
						// FIXME: may exceed boundies
						r[1] =  ( 1.0 - scale ) * ( (double)(*(f-1) + (double)f[3]) / 2.0 ) + scale * (double) s[1];
					else
						// FIXME: may exceed boundies
						*(r-1) =  ( 1.0 - scale ) * ( (double)(*(f-1) + (double)f[3]) / 2.0 ) + scale * (double) s[1];
				}
//			 }
		 }
		}
	 }
	}
}

// Image stack(able) method
static int slowmotion_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	// Get the filter object and properties
	mlt_producer producer = mlt_frame_pop_service( this );
	mlt_frame second_frame = mlt_frame_pop_service( this );
	mlt_frame first_frame = mlt_frame_pop_service( this );

	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Frame properties objects
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( this );
	mlt_properties first_frame_properties = MLT_FRAME_PROPERTIES( first_frame );
	mlt_properties second_frame_properties = MLT_FRAME_PROPERTIES( second_frame );

	// image stride
	*format = mlt_image_yuv422;
	int size = *width * *height * 2;
	int xstride = 2;
	int ystride = 2 * *width;

	uint8_t *output = mlt_properties_get_data( producer_properties, "output_buffer", 0 );
	if( output == NULL )
	{
		output = mlt_pool_alloc( size );

		// Let someone else clean up
		mlt_properties_set_data( producer_properties, "output_buffer", output, size, mlt_pool_release, NULL ); 
	}

	uint8_t *first_image = mlt_properties_get_data( first_frame_properties, "image", NULL );
	uint8_t *second_image = mlt_properties_get_data( second_frame_properties, "image", NULL );

	// which frames are buffered?

	int error = 0;

	if( first_image == NULL )
	{
		error = mlt_frame_get_image( first_frame, &first_image, format, width, height, writable );

		if( error != 0 ) {
			fprintf(stderr, "first_image == NULL get image died\n");
			return error;
		}
	}

	if( second_image == NULL )
	{
		error = mlt_frame_get_image( second_frame, &second_image, format, width, height, writable );

		if( error != 0 ) {
			fprintf(stderr, "second_image == NULL get image died\n");
			return error;
		}
	}

	// These need to passed onto the frame for other
	mlt_properties_pass_list( frame_properties, second_frame_properties,
			"motion_est.left_mb, motion_est.right_mb, \
			motion_est.top_mb, motion_est.bottom_mb, \
			motion_est.macroblock_width, motion_est.macroblock_height" );

	// Pass the pointer to the vectors without serializing
	mlt_properties_set_data( frame_properties, "motion_est.vectors", 
				mlt_properties_get_data( second_frame_properties, "motion_est.vectors", NULL ), 
				0, NULL, NULL );


	// Start with a base image
	memcpy( output, first_image, size );

	if( mlt_properties_get_int( producer_properties, "method" ) == 1 ) {

		mlt_position first_position = mlt_frame_get_position( first_frame );
		double actual_position = mlt_producer_get_speed( producer ) * (double)mlt_frame_get_position( this );
		double scale = actual_position - first_position;

		motion_interpolate
		(
			first_image, second_image, output,
			mlt_properties_get_int( second_frame_properties, "motion_est.top_mb" ),
			mlt_properties_get_int( second_frame_properties, "motion_est.bottom_mb" ),
			mlt_properties_get_int( second_frame_properties, "motion_est.left_mb" ),
			mlt_properties_get_int( second_frame_properties, "motion_est.right_mb" ),
			mlt_properties_get_int( second_frame_properties, "motion_est.macroblock_width" ),
			mlt_properties_get_int( second_frame_properties, "motion_est.macroblock_height" ),
			*width, *height,
			xstride, ystride,	
			scale,
			mlt_properties_get_data( second_frame_properties, "motion_est.vectors", NULL )
		);

		if( mlt_properties_get_int( producer_properties, "debug" ) == 1 ) {
			mlt_filter watermark = mlt_properties_get_data( producer_properties, "watermark", NULL );

			if( watermark == NULL ) {
				mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
				watermark = mlt_factory_filter( profile, "watermark", NULL );
				mlt_properties_set_data( producer_properties, "watermark", watermark, 0, (mlt_destructor)mlt_filter_close, NULL );
				mlt_producer_attach( producer, watermark );
			}

			mlt_properties wm_properties = MLT_FILTER_PROPERTIES( watermark );

			char disp[30];
			sprintf(disp, "+%10.2f.txt", actual_position);
			mlt_properties_set( wm_properties, "resource", disp );

		}

	}

	*image = output;
	mlt_frame_set_image( this, output, size, NULL );

	// Make sure that no further scaling is done
	mlt_properties_set( frame_properties, "rescale.interps", "none" );
	mlt_properties_set( frame_properties, "scale", "off" );

	mlt_frame_close( first_frame );
	mlt_frame_close( second_frame );

	return 0;
}

static int slowmotion_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	if ( !frame )
		return 1;
	// Construct a new frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( this ) );

	mlt_properties properties = MLT_PRODUCER_PROPERTIES(this);


	if( frame != NULL )
	{

		mlt_frame first_frame = mlt_properties_get_data( properties, "first_frame", NULL );
		mlt_frame second_frame = mlt_properties_get_data( properties, "second_frame", NULL );

		mlt_position first_position = (first_frame != NULL) ? mlt_frame_get_position( first_frame ) : -1;
		mlt_position second_position = (second_frame != NULL) ? mlt_frame_get_position( second_frame ) : -1;

		// Get the real producer
		mlt_producer real_producer = mlt_properties_get_data( properties, "producer", NULL );

		// Our "in" needs to be the same, keep it so
		mlt_properties_pass_list( MLT_PRODUCER_PROPERTIES( real_producer ), properties, "in" );

		// Calculate our positions
		double actual_position = mlt_producer_get_speed( this ) * (double)mlt_producer_position( this );
		mlt_position need_first = floor( actual_position );
		mlt_position need_second = need_first + 1;

		if( need_first != first_position )
		{
			mlt_frame_close( first_frame );
			first_position = -1;
			first_frame = NULL;
		}

		if( need_second != second_position)
		{
			mlt_frame_close( second_frame );
			second_position = -1;
			second_frame = NULL;
		}

		if( first_frame == NULL )
		{
			// Seek the producer to the correct place
			mlt_producer_seek( real_producer, need_first );

			// Get the frame
			mlt_service_get_frame( MLT_PRODUCER_SERVICE( real_producer ), &first_frame, index );
		}

		if( second_frame == NULL )
		{
			// Seek the producer to the correct place
			mlt_producer_seek( real_producer, need_second );

			// Get the frame
			mlt_service_get_frame( MLT_PRODUCER_SERVICE( real_producer ), &second_frame, index );
		}

		// Make sure things are in their place
		mlt_properties_set_data( properties, "first_frame", first_frame, 0, NULL, NULL );
		mlt_properties_set_data( properties, "second_frame", second_frame, 0, NULL, NULL );

		mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_image", 0 );

		// Stack the producer and producer's get image
		mlt_frame_push_service( *frame, first_frame );
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES( first_frame ) );

		mlt_frame_push_service( *frame, second_frame );
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES( second_frame ) );

		mlt_frame_push_service( *frame, this );
		mlt_frame_push_service( *frame, slowmotion_get_image );

		// Give the returned frame temporal identity
		mlt_frame_set_position( *frame, mlt_producer_position( this ) );
	}

	return 0;
}

mlt_producer producer_slowmotion_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_producer this = mlt_producer_new( profile );

	// Wrap the loader
	mlt_producer real_producer = mlt_factory_producer( profile, NULL, arg );

	// We need to apply the motion estimation filter manually
	mlt_filter filter = mlt_factory_filter( profile, "motion_est", NULL );

	if ( this != NULL && real_producer != NULL && filter != NULL)
	{
		// attach the motion_est filter to the real producer
		mlt_producer_attach( real_producer, filter );

		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

		// The loader normalised it for us already
		mlt_properties_set_int( properties, "loader_normalised", 1);

		// Store the producer and fitler
		mlt_properties_set_data( properties, "producer", real_producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
		mlt_properties_set_data( properties, "motion_est", filter, 0, ( mlt_destructor )mlt_filter_close, NULL );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "macroblock_width", 16 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "macroblock_height", 16 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "denoise", 0 );

		// Grap some stuff from the real_producer
		mlt_properties_pass_list( properties, MLT_PRODUCER_PROPERTIES( real_producer ),
				"in, out, length, resource" );

		// Since we control the seeking, prevent it from seeking on its own
		mlt_producer_set_speed( real_producer, 0 );

		//mlt_properties_set( properties, "method", "onefield" );

		// Override the get_frame method
		this->get_frame = slowmotion_get_frame;

	}
	else
	{
		if ( this )
			mlt_producer_close( this );
		if ( real_producer )
			mlt_producer_close( real_producer );
		if ( filter )
			mlt_filter_close( filter );

		this = NULL;
	}
	return this;
}
