/*
 * consumer_libdv.c -- a DV encoder based on libdv
 * Copyright (C) 2003-2014 Meltytech, LLC
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

// mlt Header files
#include <framework/mlt_consumer.h>
#include <framework/mlt_frame.h>

// System header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// libdv header files
#include <libdv/dv.h>

#define FRAME_SIZE_525_60 	10 * 150 * 80
#define FRAME_SIZE_625_50 	12 * 150 * 80

// Forward references.
static int consumer_start( mlt_consumer this );
static int consumer_stop( mlt_consumer this );
static int consumer_is_stopped( mlt_consumer this );
static int consumer_encode_video( mlt_consumer this, uint8_t *dv_frame, mlt_frame frame );
static void consumer_encode_audio( mlt_consumer this, uint8_t *dv_frame, mlt_frame frame );
static void consumer_output( mlt_consumer this, uint8_t *dv_frame, int size, mlt_frame frame );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer this );

/** Initialise the dv consumer.
*/

mlt_consumer consumer_libdv_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the consumer
	mlt_consumer this = calloc( 1, sizeof( struct mlt_consumer_s ) );

	// If memory allocated and initialises without error
	if ( this != NULL && mlt_consumer_init( this, NULL, profile ) == 0 )
	{
		// Get properties from the consumer
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

		// Assign close callback
		this->close = consumer_close;

		// Interpret the argument
		if ( arg != NULL )
			mlt_properties_set( properties, "target", arg );

		// Set the encode and output handling method
		mlt_properties_set_data( properties, "video", consumer_encode_video, 0, NULL, NULL );
		mlt_properties_set_data( properties, "audio", consumer_encode_audio, 0, NULL, NULL );
		mlt_properties_set_data( properties, "output", consumer_output, 0, NULL, NULL );

		// Terminate at end of the stream by default
		mlt_properties_set_int( properties, "terminate_on_pause", 1 );

		// Set up start/stop/terminated callbacks
		this->start = consumer_start;
		this->stop = consumer_stop;
		this->is_stopped = consumer_is_stopped;
	}
	else
	{
		// Clean up in case of init failure
		free( this );
		this = NULL;
	}

	// Return this
	return this;
}

/** Start the consumer.
*/

static int consumer_start( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check that we're not already running
	if ( !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, this );
	}
	return 0;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check that we're running
	if ( mlt_properties_get_int( properties, "running" ) )
	{
		// Get the thread
		pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );

		// Wait for termination
		pthread_join( *thread, NULL );

		// Close the output file :-) - this is obtuse - doesn't matter if output file
		// exists or not - the destructor will kick in if it does
		mlt_properties_set_data( properties, "output_file", NULL, 0, NULL, NULL );
	}

	return 0;
}

/** Determine if the consumer is stopped.
*/

static int consumer_is_stopped( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
	return !mlt_properties_get_int( properties, "running" );
}

/** Get or create a new libdv encoder.
*/

static dv_encoder_t *libdv_get_encoder( mlt_consumer this, mlt_frame frame )
{
	// Get the properties of the consumer
	mlt_properties this_properties = MLT_CONSUMER_PROPERTIES( this );

	// Obtain the dv_encoder
	dv_encoder_t *encoder = mlt_properties_get_data( this_properties, "dv_encoder", NULL );

	// Construct one if we don't have one
	if ( encoder == NULL )
	{
		// Get the fps of the consumer (for now - should be from frame)
		double fps = mlt_properties_get_double( this_properties, "fps" );

		// Create the encoder
		encoder = dv_encoder_new( 0, 0, 0 );

		// Encoder settings
		encoder->isPAL = fps == 25;
		encoder->is16x9 = 0;
		encoder->vlc_encode_passes = 1;
		encoder->static_qno = 0;
		encoder->force_dct = DV_DCT_AUTO;

		// Store the encoder on the properties
		mlt_properties_set_data( this_properties, "dv_encoder", encoder, 0, ( mlt_destructor )dv_encoder_free, NULL );
	}

	// Return the encoder
	return encoder;
}


/** The libdv encode video method.
*/

static int consumer_encode_video( mlt_consumer this, uint8_t *dv_frame, mlt_frame frame )
{
	// Obtain the dv_encoder
	dv_encoder_t *encoder = libdv_get_encoder( this, frame );

	// Get the properties of the consumer
	mlt_properties this_properties = MLT_CONSUMER_PROPERTIES( this );

	// This will hold the size of the dv frame
	int size = 0;

	// Is the image rendered
	int rendered = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "rendered" );

	// Get width and height
	int width = mlt_properties_get_int( this_properties, "width" );
	int height = mlt_properties_get_int( this_properties, "height" );

	// If we get an encoder, then encode the image
	if ( rendered && encoder != NULL )
	{
		// Specify desired image properties
		mlt_image_format fmt = mlt_image_yuv422;
		uint8_t *image = NULL;

		// Get the image
		mlt_frame_get_image( frame, &image, &fmt, &width, &height, 0 );

		// Check that we get what we expected
		if ( fmt != mlt_image_yuv422 || 
			 width != mlt_properties_get_int( this_properties, "width" ) ||
			 height != mlt_properties_get_int( this_properties, "height" ) ||
			 image == NULL )
		{
			// We should try to recover here
			fprintf( stderr, "We have a problem houston...\n" );
		}
		else
		{
			// Calculate the size of the dv frame
			size = height == 576 ? FRAME_SIZE_625_50 : FRAME_SIZE_525_60;
		}

		// Process the frame
		if ( size != 0 )
		{
			// Encode the image
			dv_encode_full_frame( encoder, &image, e_dv_color_yuv, dv_frame );
		}
		mlt_events_fire( this_properties, "consumer-frame-show", frame, NULL );
	}
	else if ( encoder != NULL )
	{
		// Calculate the size of the dv frame (duplicate of previous)
		size = height == 576 ? FRAME_SIZE_625_50 : FRAME_SIZE_525_60;
	}
	
	return size;
}

/** The libdv encode audio method.
*/

static void consumer_encode_audio( mlt_consumer this, uint8_t *dv_frame, mlt_frame frame )
{
	// Get the properties of the consumer
	mlt_properties this_properties = MLT_CONSUMER_PROPERTIES( this );

	// Get the properties of the frame
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the dv_encoder
	dv_encoder_t *encoder = libdv_get_encoder( this, frame );

	// Only continue if we have an encoder
	if ( encoder != NULL )
	{
		// Get the frame count
		int count = mlt_properties_get_int( this_properties, "count" );

		// Default audio args
		mlt_audio_format fmt = mlt_audio_s16;
		int channels = 2;
		int frequency = mlt_properties_get_int( this_properties, "frequency" );
		int samples = mlt_sample_calculator( mlt_properties_get_double( this_properties, "fps" ), frequency, count );
		int16_t *pcm = NULL;

		// Get the frame number
		time_t start = time( NULL );
		int height = mlt_properties_get_int( this_properties, "height" );
		int is_pal = height == 576;
		int is_wide = mlt_properties_get_int( this_properties, "display_aspect_num" ) == 16;

		// Temporary - audio buffer allocation
		int16_t *audio_buffers[ 4 ];
		int i = 0;
		int j = 0;
		for ( i = 0 ; i < 4; i ++ )
			audio_buffers[ i ] = mlt_pool_alloc( 2 * DV_AUDIO_MAX_SAMPLES );

		// Get the audio
		mlt_frame_get_audio( frame, (void**) &pcm, &fmt, &frequency, &channels, &samples );

		// Inform the encoder of the number of audio samples
		encoder->samples_this_frame = samples;

		// Fill the audio buffers correctly
		if ( mlt_properties_get_double( frame_properties, "_speed" ) == 1.0 )
		{
			for ( i = 0; i < samples; i ++ )
				for ( j = 0; j < channels; j++ )
					audio_buffers[ j ][ i ] = *pcm ++;
		}
		else
		{
			for ( j = 0; j < channels; j++ )
				memset( audio_buffers[ j ], 0, 2 * DV_AUDIO_MAX_SAMPLES );
		}

		// Encode audio on frame
		dv_encode_full_audio( encoder, audio_buffers, channels, frequency, dv_frame );

		// Specify meta data on the frame
		dv_encode_metadata( dv_frame, is_pal, is_wide, &start, count );
		dv_encode_timecode( dv_frame, is_pal, count );

		// Update properties
		mlt_properties_set_int( this_properties, "count", ++ count );

		// Temporary - free audio buffers
		for ( i = 0 ; i < 4; i ++ )
			mlt_pool_release( audio_buffers[ i ] );
	}
}

/** The libdv output method.
*/

static void consumer_output( mlt_consumer this, uint8_t *dv_frame, int size, mlt_frame frame )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	FILE *output = stdout;
	char *target = mlt_properties_get( properties, "target" );

	if ( target != NULL )
	{
		output = mlt_properties_get_data( properties, "output_file", NULL );
		if ( output == NULL )
		{
			output = fopen( target, "wb" );
			if ( output != NULL )
				mlt_properties_set_data( properties, "output_file", output, 0, ( mlt_destructor )fclose, 0 );
		}
	}

	if ( output != NULL )
	{
		fwrite( dv_frame, size, 1, output );
		fflush( output );
	}
	else
	{
		fprintf( stderr, "Unable to open %s\n", target );
	}
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread( void *arg )
{
	// Map the argument to the object
	mlt_consumer this = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Get the terminate_on_pause property
	int top = mlt_properties_get_int( properties, "terminate_on_pause" );

	// Get the handling methods
	int ( *video )( mlt_consumer, uint8_t *, mlt_frame ) = mlt_properties_get_data( properties, "video", NULL );
	int ( *audio )( mlt_consumer, uint8_t *, mlt_frame ) = mlt_properties_get_data( properties, "audio", NULL );
	int ( *output )( mlt_consumer, uint8_t *, int, mlt_frame ) = mlt_properties_get_data( properties, "output", NULL );

	// Allocate a single PAL frame for encoding
	uint8_t *dv_frame = mlt_pool_alloc( FRAME_SIZE_625_50 );

	// Frame and size
	mlt_frame frame = NULL;
	int size = 0;

	// Loop while running
	while( mlt_properties_get_int( properties, "running" ) )
	{
		// Get the frame
		frame = mlt_consumer_rt_frame( this );

		// Check that we have a frame to work with
		if ( frame != NULL )
		{
			// Terminate on pause
			if ( top && mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0 )
			{
				mlt_frame_close( frame );
				break;
			}

			// Obtain the dv_encoder
			if ( libdv_get_encoder( this, frame ) != NULL )
			{
				// Encode the image
				size = video( this, dv_frame, frame );

				// Encode the audio
				if ( size > 0 )
					audio( this, dv_frame, frame );

				// Output the frame
				output( this, dv_frame, size, frame );

				// Close the frame
				mlt_frame_close( frame );
			}
			else
			{
				fprintf( stderr, "Unable to obtain dv encoder.\n" );
			}
		}
	}

	// Tidy up
	mlt_pool_release( dv_frame );

	mlt_consumer_stopped( this );

	return NULL;
}

/** Close the consumer.
*/

static void consumer_close( mlt_consumer this )
{
	// Stop the consumer
	mlt_consumer_stop( this );

	// Close the parent
	mlt_consumer_close( this );

	// Free the memory
	free( this );
}
