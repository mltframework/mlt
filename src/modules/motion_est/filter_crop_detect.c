/**
 *	/brief Crop Detection filter
 *
 *	/author Zachary Drew, Copyright 2005
 *
 *	inspired by mplayer's cropdetect filter
 *
 *	Note: The goemetry generated is zero-indexed and is inclusive of the end values 
 *
 *	Options:
 *	-filter crop_detect debug=1			// Visualize crop
 *	-filter crop_detect frequency=25		// Detect the crop once a second
 *	-filter crop_detect frequency=0			// Never detect unless the producer changes
 *	-filter crop_detect thresh=100			// Changes the threshold (default = 25)
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

#define DEBUG
#define DEFAULT_THRESH 20

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "arrow_code.h"

#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

// Image stack(able) method
static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	// Get the filter object and properties
	mlt_filter filter = mlt_frame_pop_service( this );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	// Get the new image
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );

	if( error != 0 ) {
		mlt_properties_debug( MLT_FRAME_PROPERTIES(this), "error after mlt_frame_get_image()", stderr );
		return error;
	}

	// Parameter that describes how often to check for the crop
	int frequency = mlt_properties_get_int( properties, "frequency");

	// Producers may start with blank footage, by default we will skip, oh, 5 frames unless overridden
	int skip = mlt_properties_get_int( properties, "skip");

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// The result
	mlt_geometry_item bounds = mlt_properties_get_data( properties, "bounds", NULL );

	// Initialize if needed
	if( bounds == NULL ) {
		bounds = calloc( 1, sizeof( struct mlt_geometry_item_s ) );
		bounds->w = *width;
		bounds->h = *height;
		mlt_properties_set_data( properties, "bounds", bounds, sizeof( struct mlt_geometry_item_s ), free, NULL );
	}

	// For periodic detection (with offset of 'skip')
	if( frequency == 0 || (int)(mlt_filter_get_position(filter, this)+skip) % frequency  != 0)
	{
		// Inject in stream 
		mlt_properties_set_data( MLT_FRAME_PROPERTIES(this), "bounds", bounds, sizeof( struct mlt_geometry_item_s ), NULL, NULL );

		return 0;
	}
	

	// There is no way to detect a crop for sure, so make up an arbitrary one
	int thresh = mlt_properties_get_int( properties, "thresh" );

	*format = mlt_image_yuv422;
	int xstride = 2;
	int ystride = 2 * *width;

	int x, y, average_brightness, deviation; // Scratch variables
	uint8_t *q;

	// Top crop
	for( y = 0; y < *height/2; y++ ) {
		bounds->y = y;
		average_brightness = 0;
		deviation = 0;
		q = *image + y*ystride;
		for( x = 0; x < *width; x++ )
			average_brightness += q[x*xstride];

		average_brightness /= *width;

		for( x = 0; x < *width; x++ )
			deviation += abs(average_brightness - q[x*xstride]);

		if( deviation*10 >= thresh * *width )
			break;
	}

	// Bottom crop
	for( y = *height - 1; y >= *height/2; y-- ) {
		bounds->h = y;
		average_brightness = 0;
		deviation = 0;
		q = *image + y*ystride;
		for( x = 0; x < *width; x++ )
			average_brightness += q[x*xstride];

		average_brightness /= *width;

		for( x = 0; x < *width; x++ )
			deviation += abs(average_brightness - q[x*xstride]);

		if( deviation*10 >= thresh * *width)
			break;
	}

	// Left crop	
	for( x = 0; x < *width/2; x++ ) {
		bounds->x = x;
		average_brightness = 0;
		deviation = 0;
		q = *image + x*xstride;
		for( y = 0; y < *height; y++ )
			average_brightness += q[y*ystride];

		average_brightness /= *height;

		for( y = 0; y < *height; y++ )
			deviation += abs(average_brightness - q[y*ystride]);

		if( deviation*10 >= thresh * *width )
			break;
	}

	// Right crop
	for( x = *width - 1; x >= *width/2; x-- ) {
		bounds->w = x;
		average_brightness = 0;
		deviation = 0;
		q = *image + x*xstride;
		for( y = 0; y < *height; y++ )
			average_brightness += q[y*ystride];

		average_brightness /= *height;

		for( y = 0; y < *height; y++ )
			deviation += abs(average_brightness - q[y*ystride]);

		if( deviation*10 >= thresh * *width )
			break;
	}

	/* Debug: Draw arrows to show crop */
	if( mlt_properties_get_int( properties, "debug") == 1 )
	{
		init_arrows( format, *width, *height );

		draw_arrow(*image, bounds->x, *height/2, bounds->x+50, *height/2, 100);
		draw_arrow(*image, *width/2, bounds->y, *width/2, bounds->y+50, 100);
		draw_arrow(*image, bounds->w, *height/2, bounds->w-50, *height/2, 100);
		draw_arrow(*image, *width/2, bounds->h, *width/2, bounds->h-50, 100);
		draw_arrow(*image, bounds->x, bounds->y, bounds->x+40, bounds->y+30, 100);
		draw_arrow(*image, bounds->x, bounds->h, bounds->x+40, bounds->h-30, 100);
		draw_arrow(*image, bounds->w, bounds->y, bounds->w-40, bounds->y+30, 100);
		draw_arrow(*image, bounds->w, bounds->h, bounds->w-40, bounds->h-30, 100);
	}

	// Convert to width and correct indexing
	bounds->w -= bounds->x - 1;
	bounds->h -= bounds->y - 1;

	if( mlt_properties_get_int( properties, "debug") == 1 )
		fprintf(stderr, "Top:%f Left:%f Width:%f Height:%f\n", bounds->y, bounds->x, bounds->w, bounds->h);

	/* inject into frame */
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(this), "bounds", bounds, sizeof( struct mlt_geometry_item_s ), NULL, NULL );

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return error;
}



/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{

	// Put the filter object somewhere we can find it
	mlt_frame_push_service( frame, this);

	// Push the frame filter
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_crop_detect_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;

		/* defaults */
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "frequency", 1);
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "thresh", 5);
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "clip", 5);
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "former_producer_id", -1);

	}

	return this;
}

/** This source code will self destruct in 5...4...3...
*/

