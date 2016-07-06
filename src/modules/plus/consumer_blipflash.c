/*
 * consumer_blipflash.c -- a consumer to measure A/V sync from a blip/flash
 *                         source
 * Copyright (C) 2013 Brian Matherly
 * Author: Brian Matherly <pez4brian@yahoo.com>
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
#include <framework/mlt_deque.h>

// System header files
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Private constants
#define SAMPLE_FREQ 48000
#define FLASH_LUMA_THRESHOLD 150
#define BLIP_THRESHOLD 0.5

// Private types
typedef struct
{
	int64_t flash_history[2];
	int flash_history_count;
	int64_t blip_history[2];
	int blip_history_count;
	int blip_in_progress;
	int samples_since_blip;
	int blip;
	int flash;
	int sample_offset;
	FILE* out_file;
	int report_frames;
} avsync_stats;

// Forward references.
static int consumer_start( mlt_consumer consumer );
static int consumer_stop( mlt_consumer consumer );
static int consumer_is_stopped( mlt_consumer consumer );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer consumer );

/** Initialize the consumer.
*/

mlt_consumer consumer_blipflash_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the consumer
	mlt_consumer consumer = mlt_consumer_new( profile );
	mlt_properties consumer_properties = MLT_CONSUMER_PROPERTIES( consumer );
	avsync_stats* stats = NULL;

	// If memory allocated and initializes without error
	if ( consumer != NULL )
	{
		// Set up start/stop/terminated callbacks
		consumer->close = consumer_close;
		consumer->start = consumer_start;
		consumer->stop = consumer_stop;
		consumer->is_stopped = consumer_is_stopped;

		stats = mlt_pool_alloc( sizeof( avsync_stats ) );
		stats->flash_history_count = 0;
		stats->blip_history_count = 0;
		stats->blip_in_progress = 0;
		stats->samples_since_blip = 0;
		stats->blip = 0;
		stats->flash = 0;
		stats->sample_offset = INT_MAX;
		stats->report_frames = 0;
		stats->out_file = stdout;
		if ( arg != NULL )
		{
			FILE* out_file = fopen( arg, "w" );
			if ( out_file != NULL )
				stats->out_file = out_file;
		}
		mlt_properties_set_data( consumer_properties, "_stats", stats, 0, NULL, NULL );

		mlt_properties_set( consumer_properties, "report", "blip" );
	}

	// Return this
	return consumer;
}

/** Start the consumer.
*/

static int consumer_start( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Check that we're not already running
	if ( !mlt_properties_get_int( properties, "_running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "_thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "_running", 1 );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, consumer );
	}
	return 0;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Check that we're running
	if ( mlt_properties_get_int( properties, "_running" ) )
	{
		// Get the thread
		pthread_t *thread = mlt_properties_get_data( properties, "_thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "_running", 0 );

		// Wait for termination
		if ( thread )
			pthread_join( *thread, NULL );
	}

	return 0;
}

/** Determine if the consumer is stopped.
*/

static int consumer_is_stopped( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	return !mlt_properties_get_int( properties, "_running" );
}

static void detect_flash( mlt_frame frame, mlt_position pos, double fps, avsync_stats* stats )
{
	int width = 0;
	int height = 0;
	mlt_image_format format = mlt_image_yuv422;
	uint8_t* image = NULL;
	int error = mlt_frame_get_image( frame, &image, &format, &width, &height, 0 );

	if ( !error && format == mlt_image_yuv422 && image != NULL )
	{
		int i, j = 0;
		int y_accumulator = 0;

		// Add up the luma values from 4 samples in 4 different quadrants.
		for( i = 1; i < 3; i++ )
		{
			int x = ( width / 3 ) * i;
			x = x - x % 2; // Make sure this is a luma sample
			for( j = 1; j < 3; j++ )
			{
				int y = ( height / 3 ) * j;
				y_accumulator += image[ y * height * 2 + x * 2 ];
			}
		}
		// If the average luma value is > 150, assume it is a flash.
		stats->flash = ( y_accumulator / 4 ) > FLASH_LUMA_THRESHOLD;
	}

	if( stats->flash )
	{
		stats->flash_history[1] = stats->flash_history[0];
		stats->flash_history[0] = mlt_sample_calculator_to_now( fps, SAMPLE_FREQ, pos );
		if( stats->flash_history_count < 2 )
		{
			stats->flash_history_count++;
		}
	}
}

static void detect_blip( mlt_frame frame, mlt_position pos, double fps, avsync_stats* stats )
{
	int frequency = SAMPLE_FREQ;
	int channels = 1;
	int samples = mlt_sample_calculator( fps, frequency, pos );
	mlt_audio_format format = mlt_audio_float;
	float* buffer = NULL;
	int error = mlt_frame_get_audio( frame, (void**) &buffer, &format, &frequency, &channels, &samples );

	if ( !error && format == mlt_audio_float && buffer != NULL )
	{
		int i = 0;

		for( i = 0; i < samples; i++ )
		{
			if( !stats->blip_in_progress )
			{
				if( buffer[i] > BLIP_THRESHOLD || buffer[i] < -BLIP_THRESHOLD )
				{
					// This sample must start a blip
					stats->blip_in_progress = 1;
					stats->samples_since_blip = 0;

					stats->blip_history[1] = stats->blip_history[0];
					stats->blip_history[0] = mlt_sample_calculator_to_now( fps, SAMPLE_FREQ, pos );
					stats->blip_history[0] += i;
					if( stats->blip_history_count < 2 )
					{
						stats->blip_history_count++;
					}
					stats->blip = 1;
				}
			}
			else
			{
				if( buffer[i] > -BLIP_THRESHOLD && buffer[i] < BLIP_THRESHOLD )
				{
					if( ++stats->samples_since_blip > frequency / 1000 )
					{
						// One ms of silence means the blip is over
						stats->blip_in_progress = 0;
						stats->samples_since_blip = 0;
					}
				}
				else
				{
					stats->samples_since_blip = 0;
				}
			}
		}
	}
}

static void calculate_sync( avsync_stats* stats )
{
	if( stats->blip || stats->flash )
	{
		if( stats->flash_history_count > 0 &&
			stats->blip_history_count > 0 &&
			stats->blip_history[0] == stats->flash_history[0] )
		{
			// The flash and blip occurred at the same time.
			stats->sample_offset = 0;
		}
		if( stats->flash_history_count > 1 &&
			stats->blip_history_count > 0 &&
			stats->blip_history[0] <= stats->flash_history[0] &&
			stats->blip_history[0] >= stats->flash_history[1] )
		{
			// The latest blip occurred between two flashes
			if( stats->flash_history[0] - stats->blip_history[0] >
			    stats->blip_history[0] - stats->flash_history[1] )
			{
				// Blip is closer to the previous flash.
				// F1---B0--------F0
				// ^----^
				// Video leads audio (negative number).
				stats->sample_offset = (int)(stats->flash_history[1] - stats->blip_history[0] );
			}
			else
			{
				// Blip is closer to the current flash.
				// F1--------B0---F0
				//           ^----^
				// Audio leads video (positive number).
				stats->sample_offset = (int)(stats->flash_history[0] - stats->blip_history[0]);
			}
		}
		else if( stats->blip_history_count > 1 &&
				 stats->flash_history_count > 0 &&
				 stats->flash_history[0] <= stats->blip_history[0] &&
				 stats->flash_history[0] >= stats->blip_history[1] )
		{
			// The latest flash occurred between two blips
			if( stats->blip_history[0] - stats->flash_history[0] >
			    stats->flash_history[0] - stats->blip_history[1] )
			{
				// Flash is closer to the previous blip.
				// B1---F0--------B0
				// ^----^
				// Audio leads video (positive number).
				stats->sample_offset = (int)(stats->flash_history[0] - stats->blip_history[1]);
			}
			else
			{
				// Flash is closer to the latest blip.
				// B1--------F0---B0
				//           ^----^
				// Video leads audio (negative number).
				stats->sample_offset = (int)(stats->flash_history[0] - stats->blip_history[0] );
			}
		}
	}
}

static void report_results( avsync_stats* stats, mlt_position pos )
{
	if( stats->report_frames || stats->blip )
	{
		if( stats->sample_offset == INT_MAX )
		{
			fprintf( stats->out_file, MLT_POSITION_FMT "\t??\n", pos );
		}
		else
		{
			// Convert to milliseconds.
			double ms_offset = (double)stats->sample_offset * 1000.0 / (double)SAMPLE_FREQ;
			fprintf( stats->out_file, MLT_POSITION_FMT "\t%02.02f\n", pos, ms_offset );
		}
	}
	stats->blip = 0;
	stats->flash = 0;
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread( void *arg )
{
	// Map the argument to the object
	mlt_consumer consumer = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Convenience functionality
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	// Frame and size
	mlt_frame frame = NULL;

	// Loop while running
	while( !terminated && mlt_properties_get_int( properties, "_running" ) )
	{
		// Get the frame
		frame = mlt_consumer_rt_frame( consumer );

		// Check for termination
		if ( terminate_on_pause && frame != NULL )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Check that we have a frame to work with
		if ( frame )
		{
			avsync_stats* stats = mlt_properties_get_data( properties, "_stats", NULL );
			double fps = mlt_properties_get_double( properties, "fps" );
			mlt_position pos = mlt_frame_get_position( frame );

			 if( !strcmp( mlt_properties_get( properties, "report" ), "frame" ) )
			 {
				 stats->report_frames = 1;
			 }
			 else
			 {
				 stats->report_frames = 0;
			 }

			detect_flash( frame, pos, fps, stats );
			detect_blip( frame, pos, fps, stats );
			calculate_sync( stats );
			report_results( stats, pos );

			// Close the frame
			mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			mlt_frame_close( frame );
		}
	}

	// Indicate that the consumer is stopped
	mlt_properties_set_int( properties, "_running", 0 );
	mlt_consumer_stopped( consumer );

	return NULL;
}

/** Close the consumer.
*/

static void consumer_close( mlt_consumer consumer )
{
	mlt_properties consumer_properties = MLT_CONSUMER_PROPERTIES( consumer );
	avsync_stats* stats = mlt_properties_get_data( consumer_properties, "_stats", NULL );

	// Stop the consumer
	mlt_consumer_stop( consumer );

	// Close the file
	if( stats->out_file != stdout )
	{
		fclose( stats->out_file );
	}

	// Clean up memory
	mlt_pool_release( stats );

	// Close the parent
	mlt_consumer_close( consumer );

	// Free the memory
	free( consumer );
}
