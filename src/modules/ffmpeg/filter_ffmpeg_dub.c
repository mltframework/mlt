/*
 * filter_ffmpeg_dub.c -- simple ffmpeg dub test case
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "filter_ffmpeg_dub.h"
#include <stdio.h>
#include <stdlib.h>
#include <framework/mlt.h>

/** Do it.
*/

static int filter_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Obtain the frame properties
	mlt_properties frame_properties = mlt_frame_properties( frame );
	
	// Get the producer used.
	mlt_producer producer = mlt_properties_get_data( frame_properties, "producer", NULL );

	// Get the producer properties
	mlt_properties producer_properties = mlt_producer_properties( producer );

	// Get the original get_audio
	frame->get_audio = mlt_properties_get_data( frame_properties, "get_audio", NULL );

	// Call the original get_audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Now if our producer is still producing, override the audio
	if ( !mlt_properties_get_int( producer_properties, "end_of_clip" ) )
	{
		// Get the position
		mlt_position position = mlt_properties_get_position( producer_properties, "dub_position" );

		// We need a frame from the producer
		mlt_frame producer_frame;

		// Set the FPS
		mlt_properties_set_double( producer_properties, "fps", mlt_properties_get_double( frame_properties, "fps" ) );

		// Seek to the position
		mlt_producer_seek( producer, position );

		// Get the next frame
		producer->get_frame( producer, &producer_frame, 0 );

		if ( !mlt_properties_get_int( producer_properties, "end_of_clip" ) )
		{
			// Now get the audio from this frame
			producer_frame->get_audio( producer_frame, buffer, format, frequency, channels, samples );

			// This producer frame will be destroyed automatically when frame is destroyed
			mlt_properties_set_data( frame_properties, "ffmpeg_dub_frame", producer_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );

			// Incrment the position
			mlt_properties_set_position( producer_properties, "dub_position", position + 1 );
		}
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Obtain the filter properties
	mlt_properties filter_properties = mlt_filter_properties( this );

	// Get the producer used.
	mlt_producer producer = mlt_properties_get_data( filter_properties, "producer", NULL );
	
	// Obtain the frame properties
	mlt_properties frame_properties = mlt_frame_properties( frame );
	
	// Get the producer properties
	mlt_properties producer_properties = mlt_producer_properties( producer );

	// Only do this if we have not reached end of clip and ffmpeg_dub has not already been done
	if ( !mlt_properties_get_int( producer_properties, "end_of_clip" ) &&
		 mlt_properties_get_data( frame_properties, "get_audio", NULL ) == NULL )
	{
		// Backup the original get_audio (it's still needed)
		mlt_properties_set_data( frame_properties, "get_audio", frame->get_audio, 0, NULL, NULL );

		// Pass the producer on the frame
		mlt_properties_set_data( frame_properties, "producer", producer, 0, NULL, NULL );

		// Override the get_audio method
		frame->get_audio = filter_get_audio;
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_ffmpeg_dub_init( char *file )
{
	// Create the filter object
	mlt_filter this = mlt_filter_new( );

	// Initialise it
	if ( this != NULL )
	{
		// Obtain the properties
		mlt_properties properties = mlt_filter_properties( this );

		// Create an ffmpeg producer
		// TODO: THIS SHOULD NOT BE HERE....
		mlt_producer producer = mlt_factory_producer( "ffmpeg", file );

		// Overide the filter process method
		this->process = filter_process;

		// Pass the producer 
		mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Initialise the audio frame position
		mlt_properties_set_position( properties, "dub_position", 0 );
	}

	return this;
}

