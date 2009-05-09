/*
 * producer_framebuffer.c -- create subspeed frames
 * Copyright (C) 2007 Jean-Baptiste Mardelle <jb@ader.ch>
 * Author: Jean-Baptiste Mardelle, based on the code of motion_est by Zachary Drew
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
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

// Forward references.
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

/** Image stack(able) method
*/

static int framebuffer_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	// Get the filter object and properties
	mlt_producer producer = mlt_frame_pop_service( this );
	mlt_frame first_frame = mlt_frame_pop_service( this );

	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Frame properties objects
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( this );
	mlt_properties first_frame_properties = MLT_FRAME_PROPERTIES( first_frame );

	*width = mlt_properties_get_int( frame_properties, "width" );
	*height = mlt_properties_get_int( frame_properties, "height" );

	int size;
	switch ( *format )
	{
		case mlt_image_yuv420p:
			size = *width * 3 * ( *height + 1 ) / 2;
			break;
		case mlt_image_rgb24:
			size = *width * ( *height + 1 ) * 3;
			break;
		default:
			*format = mlt_image_yuv422;
			size = *width * ( *height + 1 ) * 2;
			break;
	}

	uint8_t *output = mlt_properties_get_data( producer_properties, "output_buffer", NULL );

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
		mlt_properties props = MLT_FRAME_PROPERTIES( this );
		mlt_properties test_properties = MLT_FRAME_PROPERTIES( first_frame );
		mlt_properties_set_double( test_properties, "consumer_aspect_ratio", mlt_properties_get_double( props, "consumer_aspect_ratio" ) );
		mlt_properties_set( test_properties, "rescale.interp", mlt_properties_get( props, "rescale.interp" ) );

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

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Construct a new frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( this ) );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	if( frame != NULL )
	{
		mlt_frame first_frame = mlt_properties_get_data( properties, "first_frame", NULL );

		mlt_position first_position = (first_frame != NULL) ? mlt_frame_get_position( first_frame ) : -1;

		// Get the real producer
		mlt_producer real_producer = mlt_properties_get_data( properties, "producer", NULL );

		// get properties		
		int strobe = mlt_properties_get_int( properties, "strobe");
		int freeze = mlt_properties_get_int( properties, "freeze");
		int freeze_after = mlt_properties_get_int( properties, "freeze_after");
		int freeze_before = mlt_properties_get_int( properties, "freeze_before");

		mlt_position need_first;

		if (!freeze || freeze_after || freeze_before) {
			double prod_speed = mlt_properties_get_double( properties, "_speed");
			double actual_position = prod_speed * (double) mlt_producer_position( this );

			if (mlt_properties_get_int( properties, "reverse")) actual_position = mlt_producer_get_playtime(this) - actual_position;

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


mlt_producer producer_framebuffer_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	if ( !arg ) return NULL;
	mlt_producer this = NULL;
	this = calloc( 1, sizeof( struct mlt_producer_s ) );
	mlt_producer_init( this, NULL );

	// Wrap loader
	mlt_producer real_producer;
	
	// Check if a speed was specified.
	/** 

	* Speed must be appended to the filename with '?'. To play your video at 50%:
	 melt framebuffer:my_video.mpg?0.5

	* Stroboscope effect can be obtained by adding a stobe=x parameter, where
	 x is the number of frames that will be ignored.

	* You can play the movie backwards by adding reverse=1

	* You can freeze the clip at a determined position by adding freeze=frame_pos
	  add freeze_after=1 to freeze only paste position or freeze_before to freeze before it

	**/

	double speed = 0.0;
	char *props = strdup( arg );
	char *ptr = strrchr( props, '?' );
	
	if ( ptr )
	{
		speed = atof( ptr + 1 );
		if ( speed != 0.0 )
			// If speed was valid, then strip it and the delimiter.
			// Otherwise, an invalid speed probably means this '?' was not a delimiter.
			*ptr = '\0';
	}
		
	real_producer = mlt_factory_producer( profile, NULL, props );
	free( props );

	if (speed == 0.0) speed = 1.0;

	if ( this != NULL && real_producer != NULL)
	{
		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

		// The loader normalised it for us already
		mlt_properties_set_int( properties, "loader_normalised", 1);
		mlt_properties_set( properties, "resource", arg);

		// Store the producer and fitler
		mlt_properties_set_data( properties, "producer", real_producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Grab some stuff from the real_producer
		mlt_properties_pass_list( properties, MLT_PRODUCER_PROPERTIES( real_producer ), "length, width,height" );

		if ( speed < 0 )
		{
			speed = -speed;
			mlt_properties_set_int( properties, "reverse", 1 );
		}

		if ( speed != 1.0 )
		{
			double real_length = ( (double)  mlt_producer_get_length( real_producer ) ) / speed;
			mlt_properties_set_position( properties, "length", real_length );
		}
		mlt_properties_set_position( properties, "out", mlt_producer_get_length( this ) - 1 );

		// Since we control the seeking, prevent it from seeking on its own
		mlt_producer_set_speed( real_producer, 0 );
		mlt_producer_set_speed( this, speed );

		// Override the get_frame method
		this->get_frame = producer_get_frame;
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
