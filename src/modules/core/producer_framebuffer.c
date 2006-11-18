/*
 * producer_framebuffer.c -- create subspeed frames
 * Author: Jean-Baptiste Mardelle, based on the code of motion_est by Zachary Drew
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

#include "producer_framebuffer.h"
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

// Image stack(able) method
static int framebuffer_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	// Get the filter object and properties
	mlt_producer producer = mlt_frame_pop_service( this );
	mlt_frame first_frame = mlt_frame_pop_service( this );

	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );


	// Frame properties objects
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( this );
	mlt_properties first_frame_properties = MLT_FRAME_PROPERTIES( first_frame );

	// image stride
	int size, xstride, ystride;
	switch( *format ){
		case mlt_image_yuv422:
			size = *width * *height * 2;
			xstride = 2;
			ystride = 2 * *width;
			break;
		default:
			fprintf(stderr, "Unsupported image format\n");
			return -1;
	}

	uint8_t *output = mlt_properties_get_data( producer_properties, "output_buffer", 0 );
	if( output == NULL )
	{
		output = mlt_pool_alloc( size );

		// Let someone else clean up
		mlt_properties_set_data( producer_properties, "output_buffer", output, size, mlt_pool_release, NULL ); 
	}

	uint8_t *first_image = mlt_properties_get_data( first_frame_properties, "image", NULL );

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

	// Start with a base image
	memcpy( output, first_image, size );

	*image = output;
	mlt_properties_set_data( frame_properties, "image", output, size, NULL, NULL );

	// Make sure that no further scaling is done
	mlt_properties_set( frame_properties, "rescale.interps", "none" );
	mlt_properties_set( frame_properties, "scale", "off" );

	mlt_frame_close( first_frame );

	return 0;
}

static int framebuffer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Construct a new frame
	*frame = mlt_frame_init( );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES(this);

	if( frame != NULL )
	{
		mlt_frame first_frame = mlt_properties_get_data( properties, "first_frame", NULL );

		mlt_position first_position = (first_frame != NULL) ? mlt_frame_get_position( first_frame ) : -1;

		// Get the real producer
		mlt_producer real_producer = mlt_properties_get_data( properties, "producer", NULL );

		// Our "in" needs to be the same, keep it so
		mlt_properties_pass_list( MLT_PRODUCER_PROPERTIES( real_producer ), properties, "in" );

		// get properties		
		int strobe = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES (this), "strobe");
		double freeze = mlt_properties_get_double( MLT_PRODUCER_PROPERTIES (this), "freeze");
		int freeze_after = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES (this), "freeze_after");
		int freeze_before = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES (this), "freeze_before");

		mlt_position need_first;

		if (!freeze || freeze_after || freeze_before) {
			double prod_speed = mlt_properties_get_double( properties, "_speed");
			double prod_end_speed = mlt_properties_get_double( properties, "end_speed");

			// calculate actual speed and position
			double actual_speed = prod_speed + ((double)mlt_producer_position( this ) / (double)mlt_producer_get_length(this)) * (prod_end_speed - prod_speed);
			double actual_position = actual_speed * (double)mlt_producer_position( this );
			if (mlt_properties_get_int( properties, "reverse")) actual_position = mlt_producer_get_length(this) - actual_position;

			if (strobe < 2)
			{ 
				need_first = floor( actual_position );
			}
			else 
			{
				// Strobe effect wanted, calculate frame position
				need_first = floor( actual_position );
				need_first -= need_first%strobe;
			}
			if (freeze)
			{
				if (freeze_after && need_first > freeze) need_first = freeze;
				else if (freeze_before && need_first < freeze) need_first = freeze;
			}
		}
		else need_first = freeze;

		if( need_first != first_position )
		{
			mlt_frame_close( first_frame );
			first_position = -1;
			first_frame = NULL;
		}

		if( first_frame == NULL )
		{
			// Seek the producer to the correct place
			mlt_producer_seek( real_producer, need_first );

			// Get the frame
			mlt_service_get_frame( MLT_PRODUCER_SERVICE( real_producer ), &first_frame, index );
		}


		// Make sure things are in their place
		mlt_properties_set_data( properties, "first_frame", first_frame, 0, NULL, NULL );

		// Stack the producer and producer's get image
		mlt_frame_push_service( *frame, first_frame );
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES( first_frame ) );

		mlt_frame_push_service( *frame, this );
		mlt_frame_push_service( *frame, framebuffer_get_image );

		// Give the returned frame temporal identity
		mlt_frame_set_position( *frame, mlt_producer_position( this ) );

	}

	return 0;
}


mlt_producer producer_framebuffer_init( char *arg )
{
	mlt_producer this = mlt_producer_new( );
	fprintf( stderr, " + + ++ USING FRAMEBUFF %s\n", arg);
	// Wrap fezzik
	mlt_producer real_producer;
	
	// Check if a speed was specified.
	/** 

	* Speed must be appended to the filename with ':'. To play your video at 50%:
	 inigo framebuffer:my_video.mpg:0.5

	* You can have a variabl speed by specifying a start and an end speed:
	inigo framebuffer:my_video.mpg:0.5:1.0
	
	* Stroboscope effect can be obtained by adding a stobe=x parameter, where
	 x is the number of frames that will be ignored.

	* You can play the movie backwards by adding reverse=1

	* You can freeze the clip at a determined position by adding freeze=frame_pos
	  add freeze_after=1 to freeze only paste position or freeze_before to freeze before it

	**/

	double speed;
	double end_speed;
	int count;
	char *props = strdup( arg );
	char *ptr = props;
	count = strcspn( ptr, ":" );
	ptr[count] = '\0';
	real_producer = mlt_factory_producer( "fezzik", ptr );

	ptr += count + 1;
	ptr += strspn( ptr, ":" );
	count = strcspn( ptr, ":" );
	ptr[count] = '\0';
	speed = atof(ptr);

	ptr += count + 1;
	ptr += strspn( ptr, ":" );
	count = strcspn( ptr, ":" );
	ptr[count] = '\0';
	end_speed = atof(ptr);
	free( props );

	// If no end speed specified, use constant speed
	if (speed == 0.0) speed = 1.0;
	if (end_speed == 0.0) end_speed = speed;


	if ( this != NULL && real_producer != NULL)
	{
		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

		// Fezzik normalised it for us already
		mlt_properties_set_int( properties, "fezzik_normalised", 1);

		// Store the producer and fitler
		mlt_properties_set_data( properties, "producer", real_producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Grap some stuff from the real_producer
		mlt_properties_pass_list( properties, MLT_PRODUCER_PROPERTIES( real_producer ),
				"in, out, length, resource" );

		if (speed != 1.0 || end_speed !=1.0)
		{
			// Speed is not 1.0, so adjust the clip length
			mlt_position real_out = mlt_properties_get_position(properties, "out");
			mlt_properties_set_position( properties, "out", real_out * 2 / (speed + end_speed)); 
			mlt_properties_set_position( properties, "length", real_out * 2 / (speed + end_speed) + 1);
			mlt_properties_set_double( properties, "_speed", speed);
			mlt_properties_set_double( properties, "end_speed", end_speed);
		}
		else mlt_properties_set_double( properties, "end_speed", 1.0);

		// Since we control the seeking, prevent it from seeking on its own
		mlt_producer_set_speed( real_producer, 0 );
		mlt_producer_set_speed( this, speed );

		// Override the get_frame method
		this->get_frame = framebuffer_get_frame;

	}
	else
	{
		if ( this )
			mlt_producer_close( this );
		if ( real_producer )
			mlt_producer_close( real_producer );

		this = NULL;
	}
	return this;
}
