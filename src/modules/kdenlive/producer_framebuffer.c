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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

// Forward references.
static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );

/** Image stack(able) method
*/

static int framebuffer_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	// Get the filter object and properties
	mlt_producer producer = mlt_frame_pop_service( frame );
	int index = mlt_frame_pop_service_int( frame );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	// Frame properties objects
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_frame first_frame = mlt_properties_get_data( properties, "first_frame", NULL );

	// Get producer parameters
	int strobe = mlt_properties_get_int( properties, "strobe" );
	int freeze = mlt_properties_get_int( properties, "freeze" );
	int freeze_after = mlt_properties_get_int( properties, "freeze_after" );
	int freeze_before = mlt_properties_get_int( properties, "freeze_before" );
	int in = mlt_properties_get_position( properties, "in" );

	// Determine the position
	mlt_position first_position = (first_frame != NULL) ? mlt_frame_get_position( first_frame ) : -1;
	mlt_position need_first = freeze;

	if ( !freeze || freeze_after || freeze_before )
	{
		double prod_speed = mlt_properties_get_double( properties, "_speed" );
                double actual_position = prod_speed * ( in + mlt_producer_position( producer ) );

		if ( mlt_properties_get_int( properties, "reverse" ) )
			actual_position = mlt_producer_get_playtime( producer ) - actual_position;

		if ( strobe < 2 )
		{
			need_first = floor( actual_position );
		}
		else
		{
			// Strobe effect wanted, calculate frame position
			need_first = floor( actual_position );
			need_first -= MLT_POSITION_MOD(need_first, strobe);
		}
		if ( freeze )
		{
			if ( freeze_after && need_first > freeze ) need_first = freeze;
			else if ( freeze_before && need_first < freeze ) need_first = freeze;
		}
	}
	
	if ( *format == mlt_image_none )
	{
		// set format to the original's producer format
		*format = (mlt_image_format) mlt_properties_get_int( properties, "_original_format" );
	}
	// Determine output buffer size
	*width = mlt_properties_get_int( frame_properties, "width" );
	*height = mlt_properties_get_int( frame_properties, "height" );
	int size = mlt_image_format_size( *format, *width, *height, NULL );

	// Get output buffer
	int buffersize = 0;
        int alphasize = *width * *height;
	uint8_t *output = mlt_properties_get_data( properties, "output_buffer", &buffersize );
        uint8_t *output_alpha = mlt_properties_get_data( properties, "output_alpha", NULL );
	if( buffersize == 0 || buffersize != size )
	{
		// invalidate cached frame
		first_position = -1;
	}

	if ( need_first != first_position )
	{
		// invalidate cached frame
		first_position = -1;
		
		// Bust the cached frame
		mlt_properties_set_data( properties, "first_frame", NULL, 0, NULL, NULL );
		first_frame = NULL;
	}

	if ( output && first_position != -1 ) {
		// Using the cached frame
	  	uint8_t *image_copy = mlt_pool_alloc( size );
		memcpy( image_copy, output, size );
                uint8_t *alpha_copy = mlt_pool_alloc( alphasize );
                memcpy( alpha_copy, output_alpha, alphasize );

		// Set the output image
		*image = image_copy;
		mlt_frame_set_image( frame, image_copy, size, mlt_pool_release );
                mlt_frame_set_alpha( frame, alpha_copy, alphasize, mlt_pool_release );

		*width = mlt_properties_get_int( properties, "_output_width" );
		*height = mlt_properties_get_int( properties, "_output_height" );
		*format = mlt_properties_get_int( properties, "_output_format" );

		mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );
		return 0;
	}

	// Get the cached frame
	if ( first_frame == NULL )
	{
		// Get the frame to cache from the real producer
		mlt_producer real_producer = mlt_properties_get_data( properties, "producer", NULL );

		// Seek the producer to the correct place
		mlt_producer_seek( real_producer, need_first );

		// Get the frame
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( real_producer ), &first_frame, index );

		// Cache the frame
		mlt_properties_set_data( properties, "first_frame", first_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
	}
	mlt_properties first_frame_properties = MLT_FRAME_PROPERTIES( first_frame );


	// Which frames are buffered?
	uint8_t *first_image = mlt_properties_get_data( first_frame_properties, "image", NULL );
        uint8_t *first_alpha = mlt_properties_get_data( first_frame_properties, "alpha", NULL );
	if ( !first_image )
	{
		mlt_properties_set( first_frame_properties, "rescale.interp", mlt_properties_get( frame_properties, "rescale.interp" ) );

		int error = mlt_frame_get_image( first_frame, &first_image, format, width, height, writable );

		if ( error != 0 ) {
			mlt_log_warning( MLT_PRODUCER_SERVICE( producer ), "first_image == NULL get image died\n" );
			mlt_properties_set_data( properties, "first_frame", NULL, 0, NULL, NULL );
			mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );
			return error;
		}
		output = mlt_pool_alloc( size );
		memcpy( output, first_image, size );
		// Let someone else clean up
		mlt_properties_set_data( properties, "output_buffer", output, size, mlt_pool_release, NULL ); 
		mlt_properties_set_int( properties, "_output_width", *width );
		mlt_properties_set_int( properties, "_output_height", *height );
		mlt_properties_set_int( properties, "_output_format", *format );
	}

	if ( !first_alpha )
        {
                alphasize = *width * *height;
                first_alpha = mlt_frame_get_alpha_mask( first_frame );
                output_alpha = mlt_pool_alloc( alphasize );
                memcpy( output_alpha, first_alpha, alphasize );
                mlt_properties_set_data( properties, "output_alpha", output_alpha, alphasize, mlt_pool_release, NULL ); 
        }

	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	// Create a copy
	uint8_t *image_copy = mlt_pool_alloc( size );
	memcpy( image_copy, first_image, size );
        uint8_t *alpha_copy = mlt_pool_alloc( alphasize );
        memcpy( alpha_copy, first_alpha, alphasize );

	// Set the output image
	*image = image_copy;
	mlt_frame_set_image( frame, image_copy, size, mlt_pool_release );
	mlt_frame_set_alpha( frame, alpha_copy, alphasize, mlt_pool_release );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	if ( frame )
	{
		// Construct a new frame
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

		// Stack the producer and producer's get image
		mlt_frame_push_service_int( *frame, index );
		mlt_frame_push_service( *frame, producer );
		mlt_frame_push_service( *frame, framebuffer_get_image );

		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES(*frame);

		// Get frame from the real producer
		mlt_frame first_frame = mlt_properties_get_data( properties, "first_frame", NULL );

		if ( first_frame == NULL )
		{
		    // Get the frame to cache from the real producer
		    mlt_producer real_producer = mlt_properties_get_data( properties, "producer", NULL );

                    // Get the producer speed
                    double prod_speed = mlt_properties_get_double( properties, "_speed" );

		    // Seek the producer to the correct place
		    mlt_producer_seek( real_producer, mlt_producer_position( producer ) * prod_speed );

		    // Get the frame
		    mlt_service_get_frame( MLT_PRODUCER_SERVICE( real_producer ), &first_frame, index );

		    // Cache the frame
		    mlt_properties_set_data( properties, "first_frame", first_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );

		    // Find the original producer's format
		    int width = 0;
		    int height = 0;
		    mlt_image_format format = mlt_image_none;
		    uint8_t *image = NULL;
		    int error = mlt_frame_get_image( first_frame, &image, &format, &width, &height, 0 );
		    if ( !error )
		    {
			// cache the original producer's pixel format
			mlt_properties_set_int( properties, "_original_format", (int) format );
		    }
		}

		mlt_properties_inherit( frame_properties, MLT_FRAME_PROPERTIES(first_frame) );

		double force_aspect_ratio = mlt_properties_get_double( properties, "force_aspect_ratio" );
		if ( force_aspect_ratio <= 0.0 ) force_aspect_ratio = mlt_properties_get_double( properties, "aspect_ratio" );
		mlt_properties_set_double( frame_properties, "aspect_ratio", force_aspect_ratio );
                
		// Give the returned frame temporal identity
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		mlt_properties_set_int( frame_properties, "meta.media.width", mlt_properties_get_int( properties, "width" ) );
		mlt_properties_set_int( frame_properties, "meta.media.height", mlt_properties_get_int( properties, "height" ) );
		mlt_properties_pass_list( frame_properties, properties, "width, height" );
	}

	return 0;
}


mlt_producer producer_framebuffer_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	if ( !arg ) return NULL;
	mlt_producer producer = NULL;
	producer = calloc( 1, sizeof( struct mlt_producer_s ) );
	if ( !producer )
		return NULL;

	if ( mlt_producer_init( producer, NULL ) )
	{
		free( producer );
		return NULL;
	}

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
		
	real_producer = mlt_factory_producer( profile, "abnormal", props );
	free( props );

	if (speed == 0.0) speed = 1.0;

	if ( producer != NULL && real_producer != NULL)
	{
		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

		mlt_properties_set( properties, "resource", arg);

		// Store the producer and fitler
		mlt_properties_set_data( properties, "producer", real_producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Grab some stuff from the real_producer
		mlt_properties_pass_list( properties, MLT_PRODUCER_PROPERTIES( real_producer ), "progressive, length, width, height, aspect_ratio" );

		if ( speed < 0 )
		{
			speed = -speed;
			mlt_properties_set_int( properties, "reverse", 1 );
		}

		if ( speed != 1.0 )
		{
			double real_length = ( (double)  mlt_producer_get_length( real_producer ) ) / speed;
			mlt_properties_set_position( properties, "length", real_length );
			mlt_properties real_properties = MLT_PRODUCER_PROPERTIES( real_producer );
			const char* service = mlt_properties_get( real_properties, "mlt_service" );
			if ( service && !strcmp( service, "avformat" ) )
			{
				int n = mlt_properties_count( real_properties );
				int i;
				for ( i = 0; i < n; i++ )
				{
					if ( strstr( mlt_properties_get_name( real_properties, i ), "stream.frame_rate" ) )
					{
						double source_fps = mlt_properties_get_double( real_properties, mlt_properties_get_name( real_properties, i ) );
						if ( source_fps > mlt_profile_fps( profile ) )
						{
							mlt_properties_set_double( real_properties, "force_fps", source_fps * speed );
							mlt_properties_set_position( real_properties, "length", real_length );
							mlt_properties_set_position( real_properties, "out", real_length - 1 );
							speed = 1.0;
						}
						break;
					}
				}
			}
		}
		mlt_properties_set_position( properties, "out", mlt_producer_get_length( producer ) - 1 );

		// Since we control the seeking, prevent it from seeking on its own
		mlt_producer_set_speed( real_producer, 0 );
		mlt_producer_set_speed( producer, speed );

		// Override the get_frame method
		producer->get_frame = producer_get_frame;
	}
	else
	{
		if ( producer )
			mlt_producer_close( producer );
		if ( real_producer )
			mlt_producer_close( real_producer );

		producer = NULL;
	}
	return producer;
}
