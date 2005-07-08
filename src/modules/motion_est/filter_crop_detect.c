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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define DEBUG
#define DEFAULT_THRESH 20

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

#ifdef DEBUG
// ffmpeg borrowed
static inline int clip(int a, int amin, int amax)
{
    if (a < amin)
        return amin;
    else if (a > amax)
	return amax;
    else
        return a;
}


/**
 * draws an line from (ex, ey) -> (sx, sy).
 * Credits: modified from ffmpeg project
 * @param ystride stride/linesize of the image
 * @param xstride stride/element size of the image
 * @param color color of the arrow
 */
static void draw_line(uint8_t *buf, int sx, int sy, int ex, int ey, int w, int h, int xstride, int ystride, int color){
    int t, x, y, fr, f;

//    buf[sy*ystride + sx*xstride]= color;
    buf[sy*ystride + sx]+= color;

    sx= clip(sx, 0, w-1);
    sy= clip(sy, 0, h-1);
    ex= clip(ex, 0, w-1);
    ey= clip(ey, 0, h-1);

    if(ABS(ex - sx) > ABS(ey - sy)){
        if(sx > ex){
            t=sx; sx=ex; ex=t;
            t=sy; sy=ey; ey=t;
        }
        buf+= sx*xstride + sy*ystride;
        ex-= sx;
        f= ((ey-sy)<<16)/ex;
        for(x= 0; x <= ex; x++){
            y = (x*f)>>16;
            fr= (x*f)&0xFFFF;
            buf[ y   *ystride + x*xstride]= (color*(0x10000-fr))>>16;
            buf[(y+1)*ystride + x*xstride]= (color*         fr )>>16;
        }
    }else{
        if(sy > ey){
            t=sx; sx=ex; ex=t;
            t=sy; sy=ey; ey=t;
        }
        buf+= sx*xstride + sy*ystride;
        ey-= sy;
        if(ey) f= ((ex-sx)<<16)/ey;
        else   f= 0;
        for(y= 0; y <= ey; y++){
            x = (y*f)>>16;
            fr= (y*f)&0xFFFF;
            buf[y*ystride + x    *xstride]= (color*(0x10000-fr))>>16;;
            buf[y*ystride + (x+1)*xstride]= (color*         fr )>>16;;
        }
    }
}

/**
 * draws an arrow from (ex, ey) -> (sx, sy).
 * Credits: modified from ffmpeg project
 * @param stride stride/linesize of the image
 * @param color color of the arrow
 */
static void draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, int w, int h, int xstride, int ystride, int color){
    int dx,dy;

	dx= ex - sx;
	dy= ey - sy;

	if(dx*dx + dy*dy > 3*3){
		int rx=  dx + dy;
		int ry= -dx + dy;
		int length= sqrt((rx*rx + ry*ry)<<4);

		//FIXME subpixel accuracy
		rx= ROUNDED_DIV(rx*3<<4, length);
		ry= ROUNDED_DIV(ry*3<<4, length);

		draw_line(buf, sx, sy, sx + rx, sy + ry, w, h, xstride, ystride, color);
		draw_line(buf, sx, sy, sx - ry, sy + rx, w, h, xstride, ystride, color);
	}
	draw_line(buf, sx, sy, ex, ey, w, h, xstride, ystride, color);
}
#endif

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

	// The result
	mlt_geometry_item bounds = mlt_properties_get_data( properties, "bounds", NULL );

	// Initialize if needed
	if( bounds == NULL ) {
		bounds = calloc( 1, sizeof( struct mlt_geometry_item_s ) );
		bounds->w = *width;
		bounds->h = *height;
		mlt_properties_set_data( properties, "bounds", bounds, sizeof( struct mlt_geometry_item_s ), free, NULL );
	}

//	mlt_properties first = (mlt_properties)  mlt_deque_peek_front( MLT_FRAME_SERVICE_STACK(this) );
//	int current_producer_id = mlt_properties_get_int( first, "_unique_id");
//	int former_producer_id = mlt_properties_get_int(properties, "_former_unique_id");
//	mlt_properties_set_int(properties, "_former_unique_id", current_producer_id);

	// For periodic detection (with offset of 'skip')
	if( frequency == 0 || (mlt_frame_get_position(this)+skip) % frequency != 0)
	{

		// Inject in stream 
		mlt_properties_set_data( MLT_FRAME_PROPERTIES(this), "bounds", bounds, sizeof( struct mlt_geometry_item_s ), NULL, NULL );

		return 0;
	}
	

	// There is no way to detect a crop for sure, so make up an arbitrary one
	int thresh = mlt_properties_get_int( properties, "thresh" );

	int xstride, ystride;

	switch( *format ) {
		case mlt_image_yuv422:
			xstride = 2;
			ystride = 2 * *width;
			break;
		default:
			fprintf(stderr, "image format not supported by filter_crop_detect\n");
			return -1;
	}

	int x, y, average_brightness, deviation; // Scratch variables

	// Top crop
	for( y = 0; y < *height/2; y++ ) {
		average_brightness = 0;
		deviation = 0;
		bounds->y = y;
		for( x = 0; x < *width; x++ )
			average_brightness += *(*image + y*ystride + x*xstride);

		average_brightness /= *width;

		for( x = 0; x < *width; x++ )
			deviation += abs(average_brightness - *(*image + y*ystride + x*xstride));

		if( deviation >= thresh )
			break;
	}

	// Bottom crop
	for( y = *height - 1; y >= *height/2; y-- ) {
		average_brightness = 0;
		deviation = 0;
		bounds->h = y;
		for( x = 0; x < *width; x++ )
			average_brightness += *(*image + y*ystride + x*xstride);

		average_brightness /= *width;

		for( x = 0; x < *width; x++ )
			deviation += abs(average_brightness - *(*image + y*ystride + x*xstride));

		if( deviation >= thresh )
			break;
	}

	// Left crop	
	for( x = 0; x < *width/2; x++ ) {
		average_brightness = 0;
		deviation = 0;
		bounds->x = x;
		for( y = 0; y < *height; y++ )
			average_brightness += *(*image + y*ystride + x*xstride);

		average_brightness /= *height;

		for( y = 0; y < *height; y++ )
			deviation += abs(average_brightness - *(*image + y*ystride + x*xstride));

		if( deviation >= thresh )
			break;
	}

	// Right crop
	for( x = *width - 1; x >= *width/2; x-- ) {
		average_brightness = 0;
		deviation = 0;
		bounds->w = x;
		for( y = 0; y < *height; y++ )
			average_brightness += *(*image + y*ystride + x*xstride);

		average_brightness /= *height;

		for( y = 0; y < *height; y++ )
			deviation += abs(average_brightness - *(*image + y*ystride + x*xstride));

		if( deviation >= thresh )
			break;
	}

	/* Debug: Draw arrows to show crop */
	if( mlt_properties_get_int( properties, "debug") == 1 )
	{
		draw_arrow(*image, bounds->x, *height/2, bounds->x+40, *height/2, *width, *height, xstride, ystride, 0xff);
		draw_arrow(*image, *width/2, bounds->y, *width/2, bounds->y+40, *width, *height, xstride, ystride, 0xff);
		draw_arrow(*image, bounds->w, *height/2, bounds->w-40, *height/2, *width, *height, xstride, ystride, 0xff);
		draw_arrow(*image, *width/2, bounds->h, *width/2, bounds->h-40, *width, *height, xstride, ystride, 0xff);

		fprintf(stderr, "Top:%f Left:%f Right:%f Bottom:%f\n", bounds->y, bounds->x, bounds->w, bounds->h);
	}

	bounds->w -= bounds->x;
	bounds->h -= bounds->y;

	/* inject into frame */
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(this), "bounds", bounds, sizeof( struct mlt_geometry_item_s ), NULL, NULL );


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
mlt_filter filter_crop_detect_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;

		/* defaults */
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "frequency", 1);
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "thresh", 25);
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "clip", 5);
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(this), "former_producer_id", -1);

	}

	return this;
}

/** This source code will self destruct in 5...4...3...
*/

