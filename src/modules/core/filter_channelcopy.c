/*
 * filter_channelcopy.c -- copy one audio channel to another
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "filter_channelcopy.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#define __USE_ISOC99 1
#include <math.h>

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int i, j;
	int from = mlt_properties_get_int( properties, "channelcopy.from" );
	int to = mlt_properties_get_int( properties, "channelcopy.to" );

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Duplicate channels as necessary
	{
		int size = *channels * *samples * 2;
		int16_t *new_buffer = mlt_pool_alloc( size );
		
		mlt_properties_set_data( properties, "audio", new_buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
		
		// Duplicate the existing channels
		for ( i = 0; i < *samples; i++ )
		{
			for ( j = 0; j < *channels; j++ )
			{
				new_buffer[ ( i * *channels ) + j ] = (*buffer)[ ( i * *channels ) + ( j == to ? from : j ) ];
			}
		}
		*buffer = new_buffer;
	}
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( this );
	mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );

	// Propogate the parameters
	mlt_properties_set_int( frame_props, "channelcopy.to", mlt_properties_get_int( properties, "to" ) );
	mlt_properties_set_int( frame_props, "channelcopy.from", mlt_properties_get_int( properties, "from" ) );

	// Override the get_audio method
	mlt_frame_push_audio( frame, filter_get_audio );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_channelcopy_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "to", atoi( arg ) );
		else
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "to", 1 );
	}
	return this;
}
