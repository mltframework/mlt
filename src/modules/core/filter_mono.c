/*
 * filter_mono.c -- mix all channels to a mono signal across n channels
 * Copyright (C) 2003-2006 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int channels_out = mlt_properties_get_int( properties, "mono.channels" );
	int i, j, size;
	int16_t *new_buffer;

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	size = *samples * channels_out * sizeof( int16_t );
	new_buffer = mlt_pool_alloc( size );
	mlt_properties_set_data( properties, "audio", new_buffer, size, ( mlt_destructor )mlt_pool_release, NULL );

	// Mix
	for ( i = 0; i < *samples; i++ )
	{
		int16_t mixdown = 0;
		for ( j = 0; j < *channels; j++ )
			mixdown += (*buffer)[ ( i * *channels ) + j ] / *channels;
		for ( j = 0; j < channels_out; j++ )
			new_buffer[ ( i * channels_out ) + j ] = mixdown;
	}

	// Apply results
	*buffer = new_buffer;
	*channels = channels_out;
	
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( this );
	mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );

	// Propogate the parameters
	mlt_properties_set_int( frame_props, "mono.channels", mlt_properties_get_int( properties, "channels" ) );

	// Override the get_audio method
	mlt_frame_push_audio( frame, filter_get_audio );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mono_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "channels", atoi( arg ) );
		else
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "channels", 2 );
	}
	return this;
}
