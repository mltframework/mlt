/*
 * filter_volume.c -- adjust audio volume
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

#include "filter_volume.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties properties = mlt_frame_properties( frame );
	double volume = mlt_properties_get_double( properties, "volume" );

	// Restore the original get_audio
	frame->get_audio = mlt_properties_get_data( properties, "volume.get_audio", NULL );

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Apply the volume
	int i;
	for ( i = 0; i < ( *channels * *samples ); i++ )
		(*buffer)[i] *= volume;
	
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_properties properties = mlt_frame_properties( frame );

	// Propogate the level property
	if ( mlt_properties_get( mlt_filter_properties( this ), "volume" ) != NULL )
		mlt_properties_set_double( properties, "volume",
			mlt_properties_get_double( mlt_filter_properties( this ), "volume" ) );
	
	// Backup the original get_audio (it's still needed)
	mlt_properties_set_data( properties, "volume.get_audio", frame->get_audio, 0, NULL, NULL );

	// Override the get_audio method
	frame->get_audio = filter_get_audio;

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_volume_init( char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( this != NULL && mlt_filter_init( this, NULL ) == 0 )
	{
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set_double( mlt_filter_properties( this ), "volume", atof( arg ) );
	}
	return this;
}

